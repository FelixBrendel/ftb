/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Christoph Neuhauser, Felix Brendel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once

#include "arraylist.hpp"
#include "math.hpp"
#include "soa_sort.hpp"

struct Axis_Aligned_Box {
    /// The minimum and maximum coordinate corners of the 3D cuboid.
    V3 min {};
    V3 max {};

    bool contains(const V3 pt) const {
        if (pt.x >= min.x && pt.y >= min.y && pt.z >= min.z &&
            pt.x <= max.x && pt.y <= max.y && pt.z <= max.z)
            return true;
        return false;
    }
};

struct Empty {};

template<typename PayloadT = Empty>
struct Kd_Tree {
    typedef s32 Kd_Node_Idx;

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

    static auto build_from(u32 point_count, V3* points, PayloadT* payloads = nullptr) -> Kd_Tree<PayloadT> {
        if (point_count == 0) {
            return {};
        }

        Kd_Tree<PayloadT> tree;

        tree.nodes = (Kd_Node*)malloc(sizeof(Kd_Node) * point_count);
        if (payloads) {
            tree.payloads = (PayloadT*)malloc(sizeof(*payloads) * point_count);
        } else {
            payloads = nullptr;
        }

        tree.node_count = 0;
        tree.allocated_node_count = point_count;
        tree.root = tree.build_rec(points, payloads, 0, 0, point_count);

        return tree;
    }

    auto reserve(u32 to_reserve) -> void {
        if (allocated_node_count - node_count < to_reserve) {
            // at least allocate 16
            allocated_node_count = 2*allocated_node_count < 16 ? 16 : 2*allocated_node_count;
            nodes = (Kd_Node*)realloc(nodes, allocated_node_count * sizeof(Kd_Node));
            if (payloads) {
                payloads = (PayloadT*)realloc(payloads, allocated_node_count * sizeof(PayloadT));
            }
        }
    }

    /**
     * Leads to not optimal partitioning
     */
    auto add(V3 point, PayloadT payload = {}) -> void {
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


        if (payloads) {
            payloads[node_count] = payload;
        }

        node_count++;
    }

    auto query_in_aabb(Axis_Aligned_Box box, Array_List<V3>* out_points,
                       Array_List<PayloadT>* out_payloads = nullptr)
        -> void
    {
        query_in_aabb_rec(box, out_points, out_payloads, root);
    }

    auto query_in_sphere(V3 center, f32 radius, Array_List<V3>* out_points,
                         Array_List<PayloadT>* out_payloads = nullptr)
        -> void
    {
        // First, find all points within the bounding box containing the search circle.
        Axis_Aligned_Box box;
        box.min = center - V3{ radius, radius, radius };
        box.max = center + V3{ radius, radius, radius };

        u32 out_points_original_count = out_points->count;

        query_in_aabb_rec(box, out_points, out_payloads, root);

        // Now, filter all points out that are not within the search radius.
        float squared_radius = radius*radius;
        V3 diff_vec;

        u32 point_idx = out_points_original_count;
        while (point_idx < out_points->count) {
            V3 point = (*out_points)[point_idx];
            diff_vec = point - center;

            if (diff_vec.x * diff_vec.x +
                diff_vec.y * diff_vec.y +
                diff_vec.z * diff_vec.z > squared_radius)
            {
                out_points->remove_index(point_idx);
                if (payloads && out_payloads) {
                    out_payloads->remove_index(point_idx);
                }
                continue;
            }

            ++point_idx;
        }
    }

    auto get_count_in_sphere(V3 center, f32 radius) -> u32 {
        Array_List<V3> points_in_sphere;
        points_in_sphere.init();
        defer { points_in_sphere.deinit(); };

        query_in_sphere(center, radius, &points_in_sphere);

        return points_in_sphere.count;
    }

    auto find_nearest_neighbor(V3 point, f32* out_neighbor_dist, V3* out_neighbor_pos,
                               PayloadT* out_neighbor_payload = nullptr)
        -> void
    {
        *out_neighbor_dist = FLT_MAX;
        find_nearest_neighbor_rec(point, out_neighbor_dist, out_neighbor_pos, out_neighbor_payload, root);
    }

    auto build_rec(V3* input_points, PayloadT* input_payloads, int depth,
                   u32 start_idx, u32 edx_idx)
        -> Kd_Node_Idx
    {
        const int k = 3; // Number of dimensions

        if (edx_idx - start_idx == 0) {
            return -1;
        }

        Kd_Node_Idx node_idx = node_count;

        Kd_Node* node     = &nodes[node_idx];
        PayloadT* payload;
        if (payloads) {
            payload = &payloads[node_idx];
        }

        node_count++;

        int axis = depth % k;

        Array_Description payload_array_desc[] { input_payloads + start_idx, sizeof(input_payloads[0]) };
        int array_descriptors;
        if (payloads) {
            array_descriptors = 1;
        } else {
            array_descriptors = 0;
        }

        soa_sort_r(
            {input_points  + start_idx, sizeof(input_points[0])},
            payload_array_desc, array_descriptors, edx_idx-start_idx,
            [] (const void *v1, const void *v2, void* arg) -> s32 {
                s32 axis = *(s32*)arg;
                V3* a = (V3*)v1;
                V3* b = (V3*)v2;
                return (s32)((*a)[axis] > (*b)[axis]);
            }, &axis);

        u32 median_idx = start_idx + (edx_idx - start_idx) / 2;

        if (payloads) {
            *payload = input_payloads[median_idx];
        }

        node->axis      = axis;
        node->point     = input_points[median_idx];
        node->left_idx  = build_rec(input_points, input_payloads, depth + 1, start_idx, median_idx);
        node->right_idx = build_rec(input_points, input_payloads, depth + 1, median_idx + 1, edx_idx);
        return node_idx;
    }


    auto query_in_aabb_rec(Axis_Aligned_Box& box, Array_List<V3>* out_points,
                           Array_List<PayloadT>* out_payloads, Kd_Node_Idx node_idx)
        -> void
    {
        if (node_idx == -1) {
            return;
        }

        Kd_Node* node = &nodes[node_idx];
        if (box.contains(node->point)) {
            out_points->append(node->point);
            if (payloads && out_payloads) {
                out_payloads->append(payloads[node_idx]);
            }
        }


        if (box.min[node->axis] <= node->point[node->axis]) {
            query_in_aabb_rec(box, out_points, out_payloads, node->left_idx);
        }
        if (box.max[node->axis] >= node->point[node->axis]) {
            query_in_aabb_rec(box, out_points, out_payloads, node->right_idx);
        }
    }

    auto find_nearest_neighbor_rec(V3 point, f32* out_dist_to_neighbor, V3* out_neighbor_pos,
                                   PayloadT* out_neighbor_payload, Kd_Node_Idx node_idx)
        -> void
    {
        if (node_idx == -1) {
            return;
        }

        Kd_Node* node = &nodes[node_idx];

        // Descend on side of split planes where the point lies.
        bool point_is_on_left = point[ node->axis] <= node->point[node->axis];
        if (point_is_on_left) {
            find_nearest_neighbor_rec(point, out_dist_to_neighbor, out_neighbor_pos, node->left_idx);
        } else {
            find_nearest_neighbor_rec(point, out_dist_to_neighbor, out_neighbor_pos, node->right_idx);
        }

        // Compute the distance of this node to the point.
        V3 diff = point - node->point;
        float new_dist = length(diff);
        if (new_dist < *out_dist_to_neighbor) {
            *out_dist_to_neighbor = new_dist;
            *out_neighbor_pos     = node->point;
            if (payloads && out_neighbor_payload) {
                *out_neighbor_payload = payloads[node_idx];
            }
        }

        // Check whether there could be a closer point on the opposite side.
        if (point_is_on_left && point[node->axis] + *out_dist_to_neighbor >= node->point[node->axis]) {
            find_nearest_neighbor_rec(point, out_dist_to_neighbor, out_neighbor_pos, node->right_idx);
        }
        if (!point_is_on_left && point[node->axis] - *out_dist_to_neighbor <= node->point[node->axis]) {
            find_nearest_neighbor_rec(point, out_dist_to_neighbor, out_neighbor_pos, node->left_idx);
        }
    }
};
