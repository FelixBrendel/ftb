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

#include <random>

#include "types.hpp"
#include "arraylist.hpp"
#include "math.hpp"
#include "io.hpp"

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

    inline u32 &operator[](const int &index) {
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
auto resample_mesh(Mesh_Data m, u32 num_samples, Array_List<V3>* out_points) -> void;
auto generate_fibonacci_sphere(u32 num_points, Array_List<V3>* out_points) -> void;

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

auto load_obj(const char* path) -> Mesh_Data {
    String obj_str = read_entire_file(path);
    defer_free(obj_str.data);

    Mesh_Data result;

    result.vertices.init();
    result.faces.init();

    Auto_Array_List<f32> positions(512);
    Auto_Array_List<f32> normals(512);
    Auto_Array_List<f32> uvs(512);
    Auto_Array_List<Vertex_Fingerprint> fprints(512);

    char* cursor = obj_str.data;

    auto eat_line = [&]() {
        while (!(*cursor == '\n' || *cursor == '\r' || *cursor == '\0'))
            ++cursor;
    };

    auto eat_whitespace = [&]() {
        while (*cursor == ' '  || *cursor == '\n' ||
               *cursor == '\t' || *cursor == '\r') {
            ++cursor;
        }
    };

    auto eat_until_relevant = [&]() {
        char* old_read_pos;
        do {
            old_read_pos = cursor;
            eat_whitespace();
            if (*cursor == '#') eat_line(); // comment
            if (*cursor == 'o') eat_line(); // object name
            if (*cursor == 'l') eat_line(); // poly lines
            if (*cursor == 'u') eat_line(); // ???
            if (*cursor == 's') eat_line(); // smooth shading
            if (*cursor == 'm') eat_line(); // material
            if (*cursor == 'g') eat_line(); // obj group
        } while(cursor != old_read_pos);
    };

    auto read_int = [&]() -> u32 {
        eat_whitespace();
        u64 res = 0;
        s32 negation = 1;
        if (*cursor == '-') {
            negation = -1;
            ++cursor;
        }
        while (*cursor >= '0' && *cursor <= '9') {
            res = res*10 + (*cursor - '0');
            ++cursor;
        }
        return negation*(u32)res;
    };

    auto read_float = [&]() -> f32 {
        eat_whitespace();
        u64 res = 0;
        f32 fres;
        f32 negation = 1.0f;
        if (*cursor == '-') {
            negation = -1.0f;
            ++cursor;
        }
        while (*cursor >= '0' && *cursor <= '9') {
            res = res*10 + (*cursor - '0');
            ++cursor;
        }

        fres = (f32)res;

        if (*cursor == '.') {
            int div = 1;
            ++cursor;
            char* cursor_pos = cursor;
            int decimals = read_int();
            u32 read_chars = (u32)(cursor - cursor_pos);

            fres += ((f32)decimals/powf(10, (f32)read_chars));
        }
        if (*cursor == 'e') {
            ++cursor;
            int exp = read_int();
            fres *= powf(10, (f32)exp);
        }

        return fres * negation;
    };

    {
        char* eof = obj_str.data+obj_str.length;
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
                    x = read_float();
                    y = read_float();
                    z = read_float();
                    positions.extend({x, y, z});
                } else if (*cursor == 'n') {
                    // vertex normal
                    ++cursor;
                    x = read_float();
                    y = read_float();
                    z = read_float();
                    normals.extend({x, y, z});
                } else if (*cursor == 't') {
                    // vertex texture corrds
                    ++cursor;
                    u = read_float();
                    v = read_float();
                    v = 1 - v; // NOTE(Felix): Invert v, because in blender v goes up
                    uvs.extend({u, v});
                } else {
                    panic("unknown marker \"v%c\" at %u", cursor, cursor-obj_str.data);
                    return {};
                }
            } else if (*cursor == 'f') {
                // ZoneScopedN("read f");
                ++cursor;
                Vertex_Fingerprint vfp;
                for (u32 i = 0; i < 3; ++i) {
                    vfp.pos_i  = read_int();
                    ++cursor; // overstep slash
                    vfp.uv_i   = read_int();
                    ++cursor; // overstep slash
                    vfp.norm_i = read_int();

                    // NOTE(Felix): all the indices in the obj file start at 1
                    --vfp.pos_i;
                    --vfp.uv_i;
                    --vfp.norm_i;

                    fprints.append(vfp);
                }
                {
                    // NOTE(Felix): check for quads
                    eat_whitespace();
                    if (*cursor >= '0' &&  *cursor <= '9') {
                        vfp.pos_i  = read_int();
                        ++cursor; // overstep slash
                        vfp.uv_i   = read_int();
                        ++cursor; // overstep slash
                        vfp.norm_i = read_int();

                        // NOTE(Felix): all the indices in the obj file start at
                        // 1
                        --vfp.pos_i;
                        --vfp.uv_i;
                        --vfp.norm_i;

                        fprints.append(vfp);
                        fprints.append(fprints[fprints.count-4]);
                        fprints.append(fprints[fprints.count-3]);
                    }
                }
                {
                    // NOTE(Felix): check for polygon with more verts than 4
                    eat_whitespace();
                    if (*cursor >= '0' &&  *cursor <= '9') {
                        panic("Only tris and quads are supported");
                    }
                }
            } else {
                panic("unknown marker \"%c\" (pos: %ld)", *cursor, cursor-obj_str.data);
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
            s32 index = vertex_fp_to_index.get_index_of_living_cell_if_it_exists(vfp, hm_hash(vfp));
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
                    .normal {
                        normals[3 * (u32)vfp.norm_i + 0],
                        normals[3 * (u32)vfp.norm_i + 1],
                        normals[3 * (u32)vfp.norm_i + 2]},
                    .texture_coordinates {
                        uvs[2 * (u32)vfp.uv_i + 0],
                        uvs[2 * (u32)vfp.uv_i + 1]},
                };
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



std::random_device rd;
std::mt19937 e2(rd());
std::uniform_real_distribution<> dist_0_1(0.0f, 1.0f);

static inline auto rand_0_1() -> f32 {
    // return ((f32)rand())/RAND_MAX;

    return (f32)dist_0_1(e2);
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

auto resample_mesh(Mesh_Data m, u32 num_samples, Array_List<V3>* out_points) -> void {
    out_points->reserve(num_samples);

    // sample probability for each face
    Array_List<f32>     probs_store;
    // accumulated face probabilities (sum of all previous ones);
    // -> first entry is 0
    Array_List<f32> acc_probs_store;

    probs_store.init(m.faces.count);
    acc_probs_store.init(m.faces.count);

    defer {
        probs_store.deinit();
        acc_probs_store.deinit();
    };

    f32* probs     = probs_store.data;
    f32* acc_probs = acc_probs_store.data;


    // calcualte (squared) sizes for all faces
    for (u32 f_idx = 0; f_idx < m.faces.count; ++f_idx) {
        V3 v1 = m.vertices[m.faces[f_idx].v1].position;
        V3 v2 = m.vertices[m.faces[f_idx].v2].position;
        V3 v3 = m.vertices[m.faces[f_idx].v3].position;

        V3 s1 = v2 - v1;
        V3 s2 = v3 - v1;

        V3 crss = cross(s1, s2);
        probs[f_idx] = dot(crss, crss);
    }

    // NOTE(Felix): probs now contain the squared sizes of the faces, we
    //   have to sqrt all of them, and use simd for that for speed
    const u8 simd_size = 8;
    u32 i;
    f32 area_sum = 0.0f;
    { // sqrt block and accumulate the sizes
        __m256 simd_area_sum = _mm256_set1_ps(0.0f);

        for (i = 0; i + simd_size <= m.faces.count; i += simd_size) {
            __m256 squares = _mm256_loadu_ps(probs+i);
            __m256 areas   = _mm256_sqrt_ps(squares);
            simd_area_sum  = _mm256_add_ps(simd_area_sum, areas);
            _mm256_storeu_ps(probs+i, areas);
        }

        // reminder loop
        for (; i < m.faces.count; ++i) {
            probs[i] = sqrtf(probs[i]);
            area_sum += probs[i];
        }

        // NOTE(Felix): calculate total surface area (actually 2* surface area,
        //   since with the cross product we always calculate the double of the
        //   side of the triangle but it is okay since we normalize the faces
        //   such that the sum of all of them is 1; so linear factors don't
        //   matter)
        for (int j = 0; j < simd_size; ++j) {
            area_sum += ((f32*)(&simd_area_sum))[j];
        }
    }


    // NOTE(Felix): normalization block -> divide all areas by the sum of all
    //   sizes such that the sum is 1
    {
        f32 prob_factor = 1.0f / area_sum;
        __m256 simd_prob_factor = _mm256_set1_ps(prob_factor);

        for (i = 0; i + simd_size <= m.faces.count; i += simd_size) {
            __m256 simd_areas = _mm256_loadu_ps(probs+i);
            __m256 simd_probs = _mm256_mul_ps(simd_areas, simd_prob_factor);
            _mm256_storeu_ps(probs+i, simd_probs);
        }

        // remainder loop
        for (; i < m.faces.count; ++i) {
            probs[i] *= prob_factor;
        }
    }

    // Calculate the accumulated probabilities to land in a face
    {
        acc_probs[0] = 0;
        for (u32 i = 1; i < m.faces.count; ++i) {
            acc_probs[i] = acc_probs[i-1] + probs[i-1];
        }
    }

    // Actually sample the thing
    for (u32 s = 0; s < num_samples; ++s) {
        f32 face_idx_rng = rand_0_1();
        f32 r1 = rand_0_1();
        f32 r2 = rand_0_1();

        u32 prob_idx = binary_search_prob(acc_probs, face_idx_rng, m.faces.count);
        u32 face_idx = prob_idx;

        V3 v1 = m.vertices[m.faces[face_idx].v1].position;
        V3 v2 = m.vertices[m.faces[face_idx].v2].position;
        V3 v3 = m.vertices[m.faces[face_idx].v3].position;

        f32 sqrt_r1 = sqrtf(r1);

        f32 u = 1-sqrt_r1;
        f32 v = sqrt_r1*(1-r2);
        f32 w = sqrt_r1*r2;

        V3 new_point = u*v1 + v*v2 + w*v3;

        out_points->append(new_point);
    }
}

auto generate_fibonacci_sphere(u32 num_points, Array_List<V3>* out_points) -> void {
    const f32 golden_ratio_inv = 1.0f / 1.618033988749f;
    const f32 two_pi = 2.0f * 3.14159265358979323846f;
    const f32 num_points_inv = 1.0f / num_points;

    out_points->reserve(num_points);

    for (u32 i = 0; i < num_points; ++i) {
        f32 theta = two_pi * i * golden_ratio_inv;
        f32 phi   = acosf(1 - 2*(i+0.5) * num_points_inv);
        out_points->append({
                cosf(theta) * sinf(phi),
                sinf(theta) * sinf(phi),
                cosf(phi)
        });
    }
}

#endif // FTB_MESH_IMPL
