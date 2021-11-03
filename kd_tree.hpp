#pragma once

#include <type_traits>
#include "arraylist.hpp"
#include "math.hpp"
#include "soa_sort.hpp"
#include <vcruntime_typeinfo.h>


/**
 * An axis aligned (bounding) box data structures used for search queries.
 */
struct AxisAlignedBox {
    /// The minimum and maximum coordinate corners of the 3D cuboid.
    V3 min {};
    V3 max {};

    /**
     * Tests whether the axis aligned box contains a point.
     * @param pt The point.
     * @return True if the box contains the point.
     */
    bool contains(const V3 pt) const {
        if (pt.x >= min.x && pt.y >= min.y && pt.z >= min.z &&
            pt.x <= max.x && pt.y <= max.y && pt.z <= max.z)
            return true;
        return false;
    }
};

struct Empty {};



struct Pure_Kd_Tree {

};

/**
 * The k-d-tree class. Used for searching point sets in space efficiently.
 */
template<typename PayloadT = Empty>
struct Kd_Tree {
    typedef s32 Kd_Node_Idx;
#  define if_uses_payload if constexpr (!std::is_same_v<PayloadT, Empty>)
    /**
     * A node in the k-d-tree. It stores in which axis the space is partitioned (x,y,z)
     * as an index, the position of the node, and its left and right children.
     */
    struct Kd_Node {
        int axis  =  0;
        V3  point = {};
        Kd_Node_Idx left_idx  = -1;
        Kd_Node_Idx right_idx = -1;
    };

    // Root of the tree
    Kd_Node_Idx root = -1;

    u32 node_count           = 0;
    u32 allocated_node_count = 0;

    Kd_Node*  nodes    = nullptr;
    PayloadT* payloads = nullptr;

    /**
     * Builds a k-d-tree from the passed point and data array.
     * @param points The point array.
     * @param dataArray The data array.
     */
    static auto build_from(u32 point_count, V3* points, PayloadT* payloads = nullptr) -> Kd_Tree<PayloadT> {
        if (point_count == 0) {
            return {};
        }

        Kd_Tree<PayloadT> tree;

        tree.nodes = (Kd_Node*)malloc(sizeof(Kd_Node)  * point_count);
        if_uses_payload {
            tree.payloads = (PayloadT*)malloc(sizeof(payloads) * point_count);
        } else {
            payloads = nullptr;
        }

        tree.node_count = 0;
        tree.allocated_node_count = point_count;
        tree.root = tree._build(points, payloads, 0, 0, point_count);

        return tree;
    }


    /**
     * Reserves memory for use with @see add.
     * @param maxNumNodes The maximum number of nodes that can be added using @see addPoint.
     */
    void reserve(size_t maxNumNodes) {
        if (allocated_node_count - node_count < maxNumNodes) {
            // at least allocate 16
            allocated_node_count = 2*allocated_node_count < 16 ? 16 : 2*allocated_node_count;
            nodes = (Kd_Node*)realloc(nodes, allocated_node_count * sizeof(Kd_Node));
            if_uses_payload {
                payloads = (PayloadT*)realloc(payloads, allocated_node_count * sizeof(PayloadT));
            }
        }
    }

    /**
     * Adds the passed point and data to the k-d-tree.
     * WARNING: This function may be less efficient than @see build if the points are added in an order suboptimal
     * for the search structure. Furthermore, @see reserveDynamic must be called before calling this function.
     * @param point The point to add.
     * @param data The corresponding data to add.
     */
    void add(V3 point, PayloadT payload = {}) {
        reserve(1);

        const int k = 3; // Number of dimensions

        int depth = 0;
        int axis = 0;
        Kd_Node_Idx* ptr_to_node_idx = &root;
        while (*ptr_to_node_idx != -1) {
            axis = depth % k;
            Kd_Node* node = &nodes[*ptr_to_node_idx];

            if ((axis == 0 && point.x < node->point.x) ||
                (axis == 1 && point.y < node->point.y) ||
                (axis == 2 && point.z < node->point.z))
            {
                ptr_to_node_idx = &node->left_idx;
            } else {
                ptr_to_node_idx = &node->right_idx;
            }

            depth++;
        }

        *ptr_to_node_idx = node_count;

        Kd_Node* node = &nodes[node_count];
        node->axis  = axis;
        node->point = point;
        node->left_idx  = -1;
        node->right_idx = -1;


        if_uses_payload {
            payloads[node_count] = payload;
        }

        node_count++;
    }

    /**
     * Performs an area search in the k-d-tree and returns all points within a certain bounding box.
     * @param box The bounding box.
     * @return The points stored in the k-d-tree inside of the bounding box.
     */
    void query_in_aabb(AxisAlignedBox box, Array_List<V3>* out_points, Array_List<PayloadT>* out_payloads = nullptr) {
        _query_in_aabb(box, out_points, out_payloads, root);
    }


    /**
     * Performs an area search in the k-d-tree and returns all points within a certain distance to some center point.
     * @param centerPoint The center point.
     * @param radius The search radius.
     * @return The points stored in the k-d-tree inside of the search radius.
     */
    void query_in_sphere(V3 center, f32 radius, Array_List<V3>* out_points, Array_List<PayloadT>* out_payloads = nullptr) {
        // First, find all points within the bounding box containing the search circle.
        AxisAlignedBox box;
        box.min = center - V3{ radius, radius, radius };
        box.max = center + V3{ radius, radius, radius };

        u32 out_points_original_count = out_points->count;

        _query_in_aabb(box, out_points, out_payloads, root);

        // Now, filter all points out that are not within the search radius.
        float squaredRadius = radius*radius;
        V3 differenceVector;

        u32 point_idx = out_points_original_count;
        while (point_idx < out_points->count) {
            V3 point = (*out_points)[point_idx];
            differenceVector = point - center;

            if (differenceVector.x * differenceVector.x +
                differenceVector.y * differenceVector.y +
                differenceVector.z * differenceVector.z > squaredRadius)
            {
                out_points->remove_index(point_idx);
                if_uses_payload {
                    if (out_payloads) {
                        out_payloads->remove_index(point_idx);
                    }
                }
                continue;
            }

            ++point_idx;
        }
    }

    /**
     * Performs an area search in the k-d-tree and returns the number of points within a certain distance to some
     * center point.
     * @param centerPoint The center point.
     * @param radius The search radius.
     * @return The number of points stored in the k-d-tree inside of the search radius.
     */
    size_t get_count_in_sphere(V3 center, f32 radius) {
        Array_List<V3> points_in_sphere;
        points_in_sphere.init();
        defer { points_in_sphere.deinit(); };

        query_in_sphere(center, radius, &points_in_sphere);

        return points_in_sphere.count;
    }

    /**
     * Returns the nearest neighbor in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @return The closest neighbor within the maximum distance.
     */
    void findNearestNeighbor(V3 point, f32* out_neighbor_dist, V3* out_neighbor_pos, PayloadT* out_neighbor_payload = nullptr) {
        *out_neighbor_dist = FLT_MAX;
        _findNearestNeighbor(point, out_neighbor_dist, out_neighbor_pos, out_neighbor_payload, root);
    }


    /**
     * Builds a k-d-tree from the passed point array recursively (for internal use only).
     * @param points The point array.
     * @param dataArray The data array.
     * @param depth The current depth in the tree (starting at 0).
     * @return The parent node of the current sub-tree.
     */
    Kd_Node_Idx _build(V3* input_points, PayloadT* input_payloads, int depth, size_t startIdx, size_t endIdx) {
        const int k = 3; // Number of dimensions

        if (endIdx - startIdx == 0) {
            return -1;
        }

        Kd_Node_Idx node_idx = node_count;

        Kd_Node* node     = &nodes[node_idx];
        PayloadT* payload;
        if_uses_payload {
            payload = &payloads[node_idx];
        }

        node_count++;

        int axis = depth % k;

        Array_Description payload_array_desc[] { input_payloads + startIdx, sizeof(input_payloads[0]) };
        int array_descriptors;
        if_uses_payload {
            array_descriptors = 1;
        } else {
            array_descriptors = 0;
        }

        soa_sort_r(
            {input_points  + startIdx, sizeof(input_points[0])},
            payload_array_desc, array_descriptors, endIdx-startIdx,
            [] (const void *v1, const void *v2, void* arg) -> s32 {
                s32 axis = *(s32*)arg;
                V3* a = (V3*)v1;
                V3* b = (V3*)v2;
                return (s32)((*a)[axis] > (*b)[axis]);
            }, &axis);

        size_t medianIndex = startIdx + (endIdx - startIdx) / 2;

        if_uses_payload {
            *payload = input_payloads[medianIndex];
        }
        node->axis = axis;
        node->point = input_points[medianIndex];
        node->left_idx = _build(input_points, input_payloads, depth + 1, startIdx, medianIndex);
        node->right_idx = _build(input_points, input_payloads, depth + 1, medianIndex + 1, endIdx);
        return node_idx;
    }


    /**
     * Performs an area search in the k-d-tree and returns all points within a certain bounding box
     * (for internal use only).
     * @param box The bounding box.
     * @param node The current k-d-tree node that is searched.
     * @param points The points of the k-d-tree inside of the bounding box.
     */
    void _query_in_aabb(AxisAlignedBox& box, Array_List<V3>* out_points,  Array_List<PayloadT>* out_payloads, Kd_Node_Idx node_idx) {
        if (node_idx == -1) {
            return;
        }

        Kd_Node* node = &nodes[node_idx];
        if (box.contains(node->point)) {
            out_points->append(node->point);
            if_uses_payload {
                out_payloads->append(payloads[node_idx]);
            }
        }


        if (box.min[node->axis] <= node->point[node->axis]) {
            _query_in_aabb(box, out_points, out_payloads, node->left_idx);
        }
        if (box.max[node->axis] >= node->point[node->axis]) {
            _query_in_aabb(box, out_points, out_payloads, node->right_idx);
        }
    }

    /**
     * Returns the nearest neighbor in the k-d-tree to the passed point position.
     * @param point The point to which to find the closest neighbor to.
     * @param nearestNeighborDistance The distance to the nearest neighbor found so far.
     * @param nearestNeighbor The nearest neighbor found so far.
     * @param node The current k-d-tree node that is searched.
     */
    void _findNearestNeighbor(V3 point, f32* out_dist_to_neighbor, V3* out_neighbor_pos,
                              PayloadT* out_neighbor_payload, Kd_Node_Idx node_idx)
    {
        if (node_idx == -1) {
            return;
        }

        Kd_Node* node = &nodes[node_idx];

        // Descend on side of split planes where the point lies.
        bool isPointOnLeftSide = point[ node->axis] <= node->point[node->axis];
        if (isPointOnLeftSide) {
            _findNearestNeighbor(point, out_dist_to_neighbor, out_neighbor_pos, node->left_idx);
        } else {
            _findNearestNeighbor(point, out_dist_to_neighbor, out_neighbor_pos, node->right_idx);
        }

        // Compute the distance of this node to the point.
        V3 diff = point - node->point;
        float newDistance = length(diff);
        if (newDistance < *out_dist_to_neighbor) {
            *out_dist_to_neighbor = newDistance;
            *out_neighbor_pos     = node->point;
            if_uses_payload {
                if (out_neighbor_payload) {
                    *out_neighbor_payload = payloads[node_idx];
                }
            }
        }

        // Check whether there could be a closer point on the opposite side.
        if (isPointOnLeftSide && point[node->axis] + *out_dist_to_neighbor >= node->point[node->axis]) {
            _findNearestNeighbor(point, out_dist_to_neighbor, out_neighbor_pos, node->right_idx);
        }
        if (!isPointOnLeftSide && point[node->axis] - *out_dist_to_neighbor <= node->point[node->axis]) {
            _findNearestNeighbor(point, out_dist_to_neighbor, out_neighbor_pos, node->left_idx);
        }
    }
};
