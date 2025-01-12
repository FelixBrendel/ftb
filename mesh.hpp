/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Felix Brendel
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

#include <immintrin.h>

#include "hashmap.hpp"

#include "core.hpp"
#include "math.hpp"
#include "parsing.hpp"

struct Vertex {
    V3 position;
    V3 normal;
    V2 texture_coordinates;
};


union Face {
    struct {
        u32 v1;
        u32 v2;
        u32 v3;
    };
    u32 elements[3];

    inline u32 &operator[](const int& index) {
        return elements[index];
    }
};

struct Mesh_Data {
    Array_List<Vertex> vertices;
    Array_List<Face>   faces;

    void init(u32 initial_vertex_count, u32 initial_face_count) {
        vertices.init(initial_vertex_count);
        faces.init(initial_face_count);
    }

	void deinit() {
		vertices.deinit();
        faces.deinit();
	}
};

struct Half_Edge_Mesh {
	typedef s32 Edge_Idx;
	typedef s32 Vert_Idx;
	typedef s32 Face_Idx;

	struct Edge {
		Edge_Idx twin;
		Edge_Idx prev;
		Edge_Idx next;
		Vert_Idx origin;
		Face_Idx face;
	};

	struct Vertex {
		Edge_Idx edge_from_here;

		V3 position;
		V3 normal;
		V2 texture_coords;
	};

	struct Face {
		Edge_Idx edge_inside_here;
	};

	Array_List<Vertex> vertices;
	Array_List<Face>   faces;
    Array_List<Edge>   edges;

	void deinit() {
		vertices.deinit();
		faces.deinit();
		edges.deinit();
	}

	static const Edge_Idx ABSENT_EDGE = -1;
	static const Face_Idx ABSENT_FACE = -1;


	/*
	 * Convenience accessors
	 */

	inline Edge_Idx twin_idx(Edge_Idx edge_index) {
		return edges[edge_index].twin;
	}

	inline Edge& twin(Edge_Idx edge_index) {
		return edges[twin_idx(edge_index)];
	}

	inline Face_Idx face_idx(Edge_Idx edge_index) {
		return edges[edge_index].face;
	}

	inline Face& face(Edge_Idx edge_index) {
		return faces[face_idx(edge_index)];
	}

	inline Edge_Idx prev_idx(Edge_Idx edge_index) {
		return edges[edge_index].prev;
	}

	inline Edge& prev(Edge_Idx edge_index) {
		return edges[prev_idx(edge_index)];
	}

	inline Edge_Idx next_idx(Edge_Idx edge_index) {
		return edges[edge_index].next;
	}

	inline Edge& next(Edge_Idx edge_index) {
		return edges[next_idx(edge_index)];
	}

	inline Vert_Idx origin_idx(Edge_Idx edge_index) {
		return edges[edge_index].origin;
	}

	inline Vertex& origin(Edge_Idx edge_index) {
		return vertices[origin_idx(edge_index)];
	}

	/*
	 * Modifiers
	 */
	void split_edge(Edge_Idx edge_idx) {
		// NOTE(Felix): Identify both vertices that need to be
		//   duplicated

		Vert_Idx v1_idx = origin_idx(edge_idx);
		Vert_Idx v2_idx = origin_idx(next_idx(edge_idx));

		Vertex& v1 = vertices[v1_idx];
		Vertex& v2 = vertices[v2_idx];

		// NOTE(Felix): Duplicate them
		Vert_Idx v1_copy_idx = vertices.count;
		Vert_Idx v2_copy_idx = vertices.count+1;

		Vertex v1_copy = v1;
		Vertex v2_copy = v2;

		// NOTE(Felix): defer adding so the references to v1 and v2
		//   stay alive
		defer {
			vertices.append(v1_copy);
			vertices.append(v2_copy);
		};

		// NOTE(Felix): v1 and v2 will be on the side of edge and
		//   v1_copy and v2_copy will be on the side of the original
		//   twin
		Edge_Idx twin_idx = this->twin_idx(edge_idx);
		v1.edge_from_here = edge_idx;
		v2.edge_from_here = next_idx(edge_idx);

		// oposite order because twin goes the oposite direction
		v2_copy.edge_from_here = twin_idx;
		v1_copy.edge_from_here = next_idx(twin_idx);

		// NOTE(Felix): Create new twin edges, since the origial edge
		//   and its twin are not twins anymore. Instead we will have
		//   2 edges with and absent face on one side
		Edge_Idx new_twin_idx      = edges.count;
		Edge_Idx new_twin_twin_idx = edges.count+1;

		Edge new_twin    = {};
		new_twin.face   = ABSENT_FACE;
		new_twin.twin   = edge_idx;
		new_twin.next   = new_twin_twin_idx;
		new_twin.prev   = new_twin_twin_idx;
		new_twin.origin = origin_idx(next_idx(edge_idx));


	}

	V3 get_flat_surface_normal_at_edge(Edge edge, bool* result_valid) {
		/*
		 * NOTE(Felix): Faces are not necessarily triangles, but we
		 *   still assume planar faces, here to make calculations
		 *   easier.
		 *
		 *
		 *  v6        f0         v3
		 *   \                 ↗ /
		 *    \e1_prev   e1_next/
		 *     \↘              /
		 *      \     e1->    /
		 *      v1-----------v2
		 *      /   <-e2      \
		 *     /               \
		 *    /                 \
		 *   /        f1         \
		 *  v7                   v8
		 *
		 */

		Vert_Idx v1_idx = edge.origin;
		Vert_Idx v2_idx = origin_idx(edge.next);

		Vertex& edge_v_1 = vertices[v1_idx];                // v1 in example
		Vertex& edge_v_2 = vertices[v2_idx];                // v2 in example

		V3  e1         = edge_v_2.position - edge_v_1.position;
		f32 e1_length  = length(e1);

		if (e1_length < 0.0001f) {
			*result_valid = false;
			return {0,0,0};
		}

		// TODO(Felix): unhandled edege cases:
		//   - e1_prev could be {0,0,0} -> should instead loop backwards until non zero
		//   - e1_prev could be parallel to e1 -> don't know how to handle actually


		Vertex& f0_prev    = origin(edge.prev);           // v6 in example
		V3  e1_prev        = f0_prev.position - edge_v_1.position;
		f32 e1_prev_length = length(e1_prev);

		if (e1_prev_length < 0.0001f) {
			*result_valid = false;
			return {0,0,0};
		}

		V3  f0_normal        = cross(e1, e1_prev);
		f32 f0_normal_length = length(f0_normal);

		if (f0_normal_length < 0.0001f) {
			*result_valid = false;
			return {0,0,0};
		}

		*result_valid = true;
		return f0_normal;
	}

	f32 get_face_angle_at_edge_in_deg(Edge edge) {
		bool n1_valid, n2_valid;
		V3 n1 = get_flat_surface_normal_at_edge(edge, &n1_valid);
		V3 n2 = get_flat_surface_normal_at_edge(edges[edge.twin], &n2_valid);
		bool both_valid = n1_valid &&  n2_valid;

		return both_valid ? acosf(clamp01(dot(n1, n2))) : 0.0f;
	}

	void split_sharp_edges(f32 min_split_angle_in_deg) {

		for (s32 edge_idx = 0; edge_idx < edges.count; ++edge_idx) {
			Edge& edge = edges[edge_idx];

			// NOTE(Felix): don't process the same edge multiple times
			if (edge.twin < edge_idx)
				continue;

			// NOTE(Felix): only consider edges where there are faces
			//   on both sides
			if (edge.face == ABSENT_FACE || edges[edge.twin].face == ABSENT_FACE)
				continue;

			f32 angle = get_face_angle_at_edge_in_deg(edge);

			if (angle >= min_split_angle_in_deg)
				split_edge(edge_idx);
		}
	}

	/*
	 * Query functions
	 */
	void get_all_edges_in_face(Face_Idx face_idx, Array_List<Edge_Idx>* out_edges) {
		Face& face = faces[face_idx];

		Edge_Idx start_edge = face.edge_inside_here;
		Edge_Idx walk_edge  = start_edge;
		do {
			out_edges->append(walk_edge);
			walk_edge = edges[walk_edge].next;
		} while (walk_edge != start_edge);
	}

	void get_all_edges_from_vertex(Vert_Idx vert_idx, Array_List<Edge_Idx>* out_edges) {
		Vertex& vert = vertices[vert_idx];

		Edge_Idx start_edge = vert.edge_from_here;
		Edge_Idx walk_edge  = start_edge;
		do {
			out_edges->append(walk_edge);
			walk_edge = prev(walk_edge).twin;
		} while (walk_edge != start_edge);
	}

	void get_all_vertex_neighbors(Vert_Idx vert_idx, Array_List<Vert_Idx>* out_vertices) {
		Vertex& vert = vertices[vert_idx];

		Edge_Idx start_edge = vert.edge_from_here;
		Edge_Idx walk_edge  = start_edge;
		do {
			out_vertices->append(next(walk_edge).origin);
			walk_edge = prev(walk_edge).twin;
		} while (walk_edge != start_edge);
	}

	void get_all_face_neighbors_of_vertex(Vert_Idx vert_idx, Array_List<Face_Idx>* out_faces) {
		Vertex& vert = vertices[vert_idx];

		Edge_Idx start_edge = vert.edge_from_here;
		Edge_Idx walk_edge  = start_edge;
		do {
			Face_Idx neighbor = edges[walk_edge].face;
			if (neighbor != ABSENT_FACE)
				out_faces->append(neighbor);
			walk_edge = prev(walk_edge).twin;
		} while (walk_edge != start_edge);
	}

	void get_all_face_neighbors_of_face(Face_Idx face_idx, Array_List<Face_Idx>* out_faces) {
		Face& face = faces[face_idx];

		Edge_Idx start_edge = face.edge_inside_here;
		Edge_Idx walk_edge  = start_edge;
		do {
			Face_Idx neighbor = twin(walk_edge).face;
			if (neighbor != ABSENT_FACE)
				out_faces->append(neighbor);

			walk_edge = edges[walk_edge].next;
		} while (walk_edge != start_edge);
	}

	static Half_Edge_Mesh from(Mesh_Data mesh, Allocator_Base* allocator = nullptr) {
		if (allocator == nullptr)
			allocator = grab_current_allocator();


		Half_Edge_Mesh he_mesh {};
		he_mesh.vertices.init(mesh.vertices.count);
		he_mesh.faces.init(mesh.faces.count);
		// NOTE(Felix): assume all faces are triangles and the mesh is
		//   closed for a first appoximation of the total half edge
		//   count
		he_mesh.edges.init(mesh.faces.count * 3);
		Hash_Map<Integer_Pair, Edge_Idx> vertices_to_edge_map {};

		const int OVERALLOCATING_FACTOR = 4;
		vertices_to_edge_map.init(mesh.faces.count * 3 * OVERALLOCATING_FACTOR);
		defer {
			// vertices_to_edge_map.print_occupancy();
			vertices_to_edge_map.deinit();
		};


		// NOTE(Felix): copy over vertices
		for (auto& v : mesh.vertices) {
			he_mesh.vertices.append({
			   .position       = v.position,
			   .normal         = v.normal,
			   .texture_coords = v.texture_coordinates
			});
		}

		// NOTE(Felix): Extract half edges from all faces
		for (auto& f : mesh.faces) {
			// NOTE(Felix): The face with this idx will be created at
			//   the end of the loop
			Face_Idx face_idx = he_mesh.faces.count;

			Edge_Idx e1_idx = he_mesh.edges.count;
			Edge_Idx e2_idx = he_mesh.edges.count+1;
			Edge_Idx e3_idx = he_mesh.edges.count+2;

			Edge_Idx* e1_twin = vertices_to_edge_map.get_object_ptr({(Vert_Idx)f.v2, (Vert_Idx)f.v1});
			Edge_Idx* e2_twin = vertices_to_edge_map.get_object_ptr({(Vert_Idx)f.v3, (Vert_Idx)f.v2});
			Edge_Idx* e3_twin = vertices_to_edge_map.get_object_ptr({(Vert_Idx)f.v1, (Vert_Idx)f.v3});

			// e1
			he_mesh.edges.append({
					.twin   = e1_twin ? *e1_twin : ABSENT_EDGE,
					.prev   = e3_idx,
					.next   = e2_idx,
					.origin = (Vert_Idx)f.v1,
					.face   = face_idx,
			});
			he_mesh.vertices[f.v1].edge_from_here = e1_idx;

			// e2
			he_mesh.edges.append({
					.twin   = e2_twin ? *e2_twin : ABSENT_EDGE,
					.prev   = e1_idx,
					.next   = e3_idx,
					.origin = (Vert_Idx)f.v2,
					.face   = face_idx,
			});
			he_mesh.vertices[f.v2].edge_from_here = e2_idx;

			// e3
			he_mesh.edges.append({
					.twin   = e3_twin ? *e3_twin : ABSENT_EDGE,
					.prev   = e2_idx,
					.next   = e1_idx,
					.origin = (Vert_Idx)f.v3,
					.face   = face_idx,
			});
			he_mesh.vertices[f.v3].edge_from_here = e3_idx;

			if (e1_twin) he_mesh.edges[*e1_twin].twin = e1_idx;
			if (e2_twin) he_mesh.edges[*e2_twin].twin = e2_idx;
			if (e3_twin) he_mesh.edges[*e3_twin].twin = e3_idx;

			vertices_to_edge_map.set_object({(Vert_Idx)f.v1, (Vert_Idx)f.v2}, e1_idx);
			vertices_to_edge_map.set_object({(Vert_Idx)f.v2, (Vert_Idx)f.v3}, e2_idx);
			vertices_to_edge_map.set_object({(Vert_Idx)f.v3, (Vert_Idx)f.v1}, e3_idx);

			he_mesh.faces.append({
					.edge_inside_here = e1_idx,
			});
		}

		// NOTE(Felix): fill all the absent edges twins (boundary)
		for (Edge_Idx e_idx = 0; e_idx < he_mesh.edges.count; ++e_idx) {
			Edge& edge = he_mesh.edges[e_idx];

			if (edge.twin != ABSENT_EDGE)
				continue;

			// create a twin for h_edge
			Edge_Idx twin_idx = he_mesh.edges.count;
			edge.twin = twin_idx;

			// NOTE(Felix): we defer appending it, since appending it
			//   might reallocate and invalidate the reference to
			//   `edge`, alternatively we could ask for a new
			//   reference after appending
			Edge twin_edge {
				.twin   = e_idx,
				.prev   = ABSENT_EDGE,
				.next   = ABSENT_EDGE,
				.origin = he_mesh.edges[edge.next].origin,
				.face   = ABSENT_FACE,
			};
			defer {
				he_mesh.edges.append(twin_edge);
			};

			// NOTE(Felix): I am staring at this while implementing
			// this. https://jerryyin.info/geometry-processing-algorithms/half-edge/

			// back twin
			Edge_Idx back_twin_idx = he_mesh.edges[edge.prev].twin;
			while (true) {
				if (back_twin_idx == ABSENT_EDGE)
					// NOTE(Felix): if we reach a dead end, there is
					//   also a missing twin there, so we have nothing
					//   to connect to
					break;

				Edge& back_twin = he_mesh.edges[back_twin_idx];
				if (back_twin.face == ABSENT_FACE) {
					// NOTE(Felix): We reached the end and there is
					//   already a twin edge at the boundary so we can
					//   connect

					back_twin.prev = twin_idx;
					twin_edge.next = back_twin_idx;
					break;
				}

				back_twin_idx = he_mesh.edges[back_twin.prev].twin;
			}

			// next twin
			Edge_Idx next_twin_idx = he_mesh.edges[edge.next].twin;
			while (true) {
				if (next_twin_idx == ABSENT_EDGE)
					// NOTE(Felix): if we reach a dead end, there is
					//   also a missing twin there, so we have nothing
					//   to connect to
					break;

				Edge& next_twin = he_mesh.edges[next_twin_idx];
				if (next_twin.face == ABSENT_FACE) {
					// NOTE(Felix): We reached the end and there is
					//   already a twin edge at the boundary so we can
					//   connect

					next_twin.next = twin_idx;
					twin_edge.prev = next_twin_idx;
					break;
				}

				next_twin_idx = he_mesh.edges[next_twin.next].twin;
			}
		}


		return he_mesh;
	}
};

struct Vertex_Fingerprint {
    // TODO(Felix): why are all these u64?
    u64 pos_i;
    u64 norm_i;
    u64 uv_i;
};

#ifndef FTB_MESH_IMPL

auto hm_hash(Vertex_Fingerprint v) -> u64;
auto hm_objects_match(Vertex_Fingerprint a, Vertex_Fingerprint b) -> bool;

auto load_obj(const char* path) -> Mesh_Data;
auto load_obj_from_in_memory_string(const char* cursor) -> Mesh_Data;

auto resample_mesh(Mesh_Data m, u32 num_samples, Array_List<V3>* out_points) -> void;
auto generate_fibonacci_sphere(u32 num_points, Array_List<V3>* out_points) -> void;
auto get_random_barycentric_coordinates(f32* out_u, f32* out_v, f32* out_w) -> void;

#else // implementations

auto hm_hash(Vertex_Fingerprint v) -> u64 {
    u32 h = 0;
    u32 highorder = h & 0xf8000000;     // extract high-order 5 bits from h
                                        // 0xf8000000 is the hexadecimal representation
                                        //   for the 32-bit number with the first five
                                        //   bits = 1 and the other bits = 0
    h = h << 5;                         // shift h left by 5 bits
    h = h ^ (highorder >> 27);          // move the highorder 5 bits to the low-order
                                        //   end and XOR into h
    h = h ^ (u32)v.pos_i;               // XOR h and ki
                                        //
    highorder = h & 0xf8000000;
    h = h << 5;
    h = h ^ (highorder >> 27);
    h = h ^ (u32)v.norm_i;

    highorder = h & 0xf8000000;
    h = h << 5;
    h = h ^ (highorder >> 27);
    h = h ^ (u32)v.uv_i;

    return h;
}

inline auto hm_objects_match(Vertex_Fingerprint a, Vertex_Fingerprint b) -> bool {
    return a.pos_i  == b.pos_i
        && a.uv_i   == b.uv_i
        && a.norm_i == b.norm_i;
}

auto read_vertex_fingerprint(const char** cursor, Vertex_Fingerprint* out_vfp) -> void {
    // NOTE(Felix): all the indices in the obj file start at 1, so we subtract
    //   1 of all after reading.
    (*cursor) += read_long(*cursor, (s64*)&(out_vfp->pos_i));
    --(out_vfp->pos_i);

    if (*cursor[0] == '/') {
        ++(*cursor); // overstep slash
        (*cursor) += read_long(*cursor, (s64*)&(out_vfp->uv_i));
        --(out_vfp->uv_i);

        if (*cursor[0] == '/') {
            ++(*cursor); // overstep slash
            (*cursor) += read_long(*cursor, (s64*)&(out_vfp->norm_i));
            --(out_vfp->norm_i);
        } else {
            out_vfp->norm_i = -1;
        }
    } else {
        out_vfp->uv_i   = -1;
        out_vfp->norm_i = -1;
    }
}

auto load_obj_from_in_memory_string(const char* cursor, const char* eof) -> Mesh_Data {
    const char* start_cursor = cursor;

    Mesh_Data result;

    result.vertices.init();
    result.faces.init();

    Auto_Array_List<f32> positions(512);
    Auto_Array_List<f32> normals(512);
    Auto_Array_List<f32> uvs(512);
    Auto_Array_List<Vertex_Fingerprint> fprints(512);

    auto eat_until_relevant = [&]() {
        const char* old_read_pos;
        do {
            old_read_pos = cursor;
            cursor += eat_whitespace(cursor);
            if (*cursor == '#') cursor += eat_line(cursor); // comment
            if (*cursor == 'o') cursor += eat_line(cursor); // object name
            if (*cursor == 'l') cursor += eat_line(cursor); // poly lines
            if (*cursor == 'u') cursor += eat_line(cursor); // ???
            if (*cursor == 's') cursor += eat_line(cursor); // smooth shading
            if (*cursor == 'm') cursor += eat_line(cursor); // material
            if (*cursor == 'g') cursor += eat_line(cursor); // obj group
        } while(cursor != old_read_pos);
    };

    {
        while (true) {
            eat_until_relevant();
            f32 x, y, z, u, v;
            if (cursor == eof)
                break;
            if (*cursor == 'v') {
                ++cursor;
                if (*cursor == ' ') {
                    // vertex pos
                    ++cursor;
                    cursor += read_float(cursor, &x);
                    cursor += read_float(cursor, &y);
                    cursor += read_float(cursor, &z);

                    // skip optional 4th component, or vertex colors
                    cursor += eat_line(cursor);

                    positions.extend({x, y, z});
                } else if (*cursor == 'n') {
                    // vertex normal
                    ++cursor;
                    cursor += read_float(cursor, &x);
                    cursor += read_float(cursor, &y);
                    cursor += read_float(cursor, &z);
                    normals.extend({x, y, z});
                } else if (*cursor == 't') {
                    // vertex texture corrds
                    ++cursor;
                    cursor += read_float(cursor, &u);
                    cursor += read_float(cursor, &v);
                    v = 1 - v; // NOTE(Felix): Invert v, because in blender v goes up
                    uvs.extend({u, v});
                } else {
                    panic("unknown marker \"v%c\" at %u", cursor, cursor-start_cursor);
                    return {};
                }
            } else if (*cursor == 'f') {
                // ZoneScopedN("read f");
                ++cursor;
                Vertex_Fingerprint vfp {};
                for (u32 i = 0; i < 3; ++i) {
                    read_vertex_fingerprint(&cursor, &vfp);
                    fprints.append(vfp);
                }
                {
                    // NOTE(Felix): check for n-gons (assume triangle stip)
                    cursor += eat_whitespace(cursor);
                    u32 first = fprints.count-3;
                    while (*cursor >= '0' && *cursor <= '9') {
                        read_vertex_fingerprint(&cursor, &vfp);

                        fprints.append(fprints[first]);
                        fprints.append(fprints[fprints.count-2]);
                        fprints.append(vfp);

                        cursor += eat_whitespace(cursor);
                    }
                }
            } else {
                log_warning("unknown marker \"%c\" (pos: %ld)", *cursor, cursor-start_cursor);
                return {};
            }
        }
    }

    Hash_Map<Vertex_Fingerprint, u32> vertex_fp_to_index;
    vertex_fp_to_index.init(fprints.count);
    defer { vertex_fp_to_index.deinit(); };

    {
        u32 counter{0};
        for (auto vfp : fprints) {
            s64 index = vertex_fp_to_index.get_index_of_living_cell_if_it_exists(vfp, hm_hash(vfp));
            if(counter%3 == 0) {
                Face f {};
                result.faces.append(f);
            }
            if (index != -1) {
                result.faces.data[result.faces.count-1][counter%3] = vertex_fp_to_index.data[index].object;
            } else {
                Vertex v {
                    .position {
                        positions[3 * (u32)vfp.pos_i + 0],
                        positions[3 * (u32)vfp.pos_i + 1],
                        positions[3 * (u32)vfp.pos_i + 2]},
                };
                if (vfp.norm_i != -1) {
                    v.normal = {
                        normals[3 * (u32)vfp.norm_i + 0],
                        normals[3 * (u32)vfp.norm_i + 1],
                        normals[3 * (u32)vfp.norm_i + 2]
                    };
                }
                if (vfp.uv_i != -1) {
                    v.texture_coordinates = {
                        uvs[2 * (u32)vfp.uv_i + 0],
                        uvs[2 * (u32)vfp.uv_i + 1]
                    };
                }

                u32 new_index = result.vertices.count;
                result.vertices.append(v);
                result.faces.data[result.faces.count-1][counter%3] = new_index;
                vertex_fp_to_index.set_object(vfp, new_index);
            }
            counter++;
        }
    }
    return result;

}

auto load_obj(const char* path) -> Mesh_Data {
    File_Read obj_str_read = read_entire_file(path);
    if (!obj_str_read.success)
        return {};

    Allocated_String obj_str = obj_str_read.contents;
    defer{
        obj_str.free();
    };

    char* cursor = obj_str.string.data;
    char* eof    = obj_str.string.data + obj_str.string.length;
    Mesh_Data data =  load_obj_from_in_memory_string(cursor, eof);
    if (data.vertices.count == 0)
        log_error("error while reading '%s'", path);
    return data;
}



std::random_device rd;
std::mt19937 e2(rd());
std::uniform_real_distribution<> dist_0_1(0.0f, 1.0f);

static inline auto rand_0_1() -> f32 {
    // return ((f32)rand())/RAND_MAX;

    return (f32)dist_0_1(e2);
}

auto get_random_barycentric_coordinates(f32* out_u, f32* out_v, f32* out_w) -> void {
    f32 r1 = rand_0_1();
    f32 r2 = rand_0_1();
    f32 sqrt_r1 = sqrtf(r1);

    *out_u = 1-sqrt_r1;
    *out_v = sqrt_r1*(1-r2);
    *out_w = sqrt_r1*r2;
}

static auto binary_search_prob(f32* acc_probs, f32 needle, u32 count) -> u32 {
    f32* base = acc_probs;

    while (count > 1) {
        u32 middle = count / 2;
        base += (needle < base[middle]) ? 0 : middle;
        count -= middle;
    }

    return (u32)(base-acc_probs);
}


// auto resample_mesh(Mesh_Data m, u32 num_samples, Array_List<V3>* out_points) -> void {
//     out_points->assure_available(num_samples);

//     // sample probability for each face
//     Array_List<f32>     probs_store;
//     // accumulated face probabilities (sum of all previous ones);
//     // -> first entry is 0
//     Array_List<f32> acc_probs_store;

//     probs_store.init(m.faces.count);
//     acc_probs_store.init(m.faces.count);

//     defer {
//         probs_store.deinit();
//         acc_probs_store.deinit();
//     };

//     f32* probs     = probs_store.data;
//     f32* acc_probs = acc_probs_store.data;


//     // calcualte (squared) sizes for all faces
//     for (u32 f_idx = 0; f_idx < m.faces.count; ++f_idx) {
//         V3 v1 = m.vertices[m.faces[f_idx].v1].position;
//         V3 v2 = m.vertices[m.faces[f_idx].v2].position;
//         V3 v3 = m.vertices[m.faces[f_idx].v3].position;

//         V3 s1 = v2 - v1;
//         V3 s2 = v3 - v1;

//         V3 crss = cross(s1, s2);
//         probs[f_idx] = dot(crss, crss);
//     }

//     // NOTE(Felix): probs now contain the squared sizes of the faces, we
//     //   have to sqrt all of them, and use simd for that for speed
//     const u8 simd_size = 8;
//     u32 i;
//     f32 area_sum = 0.0f;
//     { // sqrt block and accumulate the sizes
//         __m256 simd_area_sum = _mm256_set1_ps(0.0f);

//         for (i = 0; i + simd_size <= m.faces.count; i += simd_size) {
//             __m256 squares = _mm256_loadu_ps(probs+i);
//             __m256 areas   = _mm256_sqrt_ps(squares);
//             simd_area_sum  = _mm256_add_ps(simd_area_sum, areas);
//             _mm256_storeu_ps(probs+i, areas);
//         }

//         // reminder loop
//         for (; i < m.faces.count; ++i) {
//             probs[i] = sqrtf(probs[i]);
//             area_sum += probs[i];
//         }

//         // NOTE(Felix): calculate total surface area (actually 2* surface area,
//         //   since with the cross product we always calculate the double of the
//         //   side of the triangle but it is okay since we normalize the faces
//         //   such that the sum of all of them is 1; so linear factors don't
//         //   matter)
//         for (int j = 0; j < simd_size; ++j) {
//             area_sum += ((f32*)(&simd_area_sum))[j];
//         }
//     }


//     // NOTE(Felix): normalization block -> divide all areas by the sum of all
//     //   sizes such that the sum is 1
//     {
//         f32 prob_factor = 1.0f / area_sum;
//         __m256 simd_prob_factor = _mm256_set1_ps(prob_factor);

//         for (i = 0; i + simd_size <= m.faces.count; i += simd_size) {
//             __m256 simd_areas = _mm256_loadu_ps(probs+i);
//             __m256 simd_probs = _mm256_mul_ps(simd_areas, simd_prob_factor);
//             _mm256_storeu_ps(probs+i, simd_probs);
//         }

//         // remainder loop
//         for (; i < m.faces.count; ++i) {
//             probs[i] *= prob_factor;
//         }
//     }

//     // Calculate the accumulated probabilities to land in a face
//     {
//         acc_probs[0] = 0;
//         for (u32 i = 1; i < m.faces.count; ++i) {
//             acc_probs[i] = acc_probs[i-1] + probs[i-1];
//         }
//     }

//     // Actually sample the thing
//     for (u32 s = 0; s < num_samples; ++s) {
//         f32 face_idx_rng = rand_0_1();
//         u32 prob_idx = binary_search_prob(acc_probs, face_idx_rng, m.faces.count);
//         u32 face_idx = prob_idx;

//         V3 v1 = m.vertices[m.faces[face_idx].v1].position;
//         V3 v2 = m.vertices[m.faces[face_idx].v2].position;
//         V3 v3 = m.vertices[m.faces[face_idx].v3].position;

//         f32 u, v, w;
//         get_random_barycentric_coordinates(&u, &v, &w);

//         V3 new_point = u*v1 + v*v2 + w*v3;

//         out_points->append(new_point);
//     }
// }

auto generate_fibonacci_sphere(u32 num_points, Array_List<V3>* out_points) -> void {
    const f32 golden_ratio_inv = 1.0f / 1.618033988749f;
    const f32 two_pi = 2.0f * 3.14159265358979323846f;
    const f32 num_points_inv = 1.0f / num_points;

    out_points->assure_available(num_points);

    for (u32 i = 0; i < num_points; ++i) {
        f32 theta = two_pi * i * golden_ratio_inv;
        f32 phi   = acosf(1 - 2*(i+0.5f) * num_points_inv);
        out_points->append({
                cosf(theta) * sinf(phi),
                sinf(theta) * sinf(phi),
                cosf(phi)
        });
    }
}

#endif // FTB_MESH_IMPL
