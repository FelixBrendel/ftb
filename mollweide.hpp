/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Felix Brendel
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

#include "core.hpp"
#include "math.hpp"
#include "bucket_allocator.hpp"


void lat_lng_to_mollweide(f32 phi, f32 lambda, f32* out_x, f32* out_y);
void mollweide_to_lat_lng(f32 x, f32 y, f32* out_phi, f32* out_lambda);

template <typename Type>
struct Mollweide_Grid {
    struct Entry {
        float u; // [  -1;   1]
        float v; // [-0.5; 0.5]
        Type  payload;
    };

    u32 subdivision_level;
    Bucket_List<Entry>* buckets;
    Allocator_Base* allocator;

    void init(u32 subdivisions, Allocator_Base* back_allocator = nullptr) {
        if (!back_allocator)
            back_allocator = grab_current_allocator();

        allocator = back_allocator;

        buckets = nullptr;
        subdivision_level = 0;
        change_subdivision_level(subdivisions);
    }

    void deinit() {
        u32 cell_count = get_num_cells();
        for (u32 i = 0; i < cell_count; ++i) {
            if (buckets[i].allocator.buckets)
                buckets[i].deinit();
        }
        allocator->deallocate(buckets);
    }

    void clear() {
        u32 cell_count = get_num_cells();
        for (u32 i = 0; i < cell_count; ++i) {
            if (buckets[i].allocator.buckets)
                buckets[i].clear();
        }
    }


    void get_dimensions(u32* out_width, u32* out_height) {
        *out_width  = 1 << (subdivision_level + 1); // width  = 2 * 2 ** s_l
        *out_height = 1 << (subdivision_level);     // height = 2 ** s_l
    }

    u32 get_num_cells() {
        return 1 << (2 * subdivision_level + 1);
    }

    f32 find_theta(f32 lat) {
        f32 theta = lat;
        f32 old_t;
        do {
            old_t = theta;
            f32 denom = cos(theta);

            theta =  theta - (2*theta + sin(2*theta) - pi*sin(lat)) /
                             (4 * denom * denom);

        } while(fabs(old_t - theta) > 0.001f);

        return theta;
    }

    // NOTE(Felix): point should be on the unit sphere (length == 1).
    void insert(V3 point, Type payload) {
        // calculate latitude and longiture from point
        f32 phi     = acos(clamp01(dot(noz(V3{point.x, point.y, 0}), point)));      // [0; pi/2]
        f32 lambda  = acos(dot(noz(V3{point.x, point.y, 0.0f}), {0.0f,1.0f,0.0f})); // [0; pi]

        if (point.z > 0)
            phi = -phi; // [-pi/2; pi/2]

        if (point.x < 0)
            lambda = -lambda; // [-pi; pi]

        f32 x, y;
        lat_lng_to_mollweide(phi, lambda, &x, &y);

        Entry new_entry {
            .u = x,
            .v = y,
            .payload = payload
        };

        insert(&new_entry);
    }

    void insert(Entry* e) {
        u32 grid_width, grid_height;
        get_dimensions(&grid_width, &grid_height);

        f32 f_x_idx = remap(-1,   e->u,   1, 0, grid_width-0.001f);   // [0; grid_width-1]
        f32 f_y_idx = remap(-0.5, e->v, 0.5, 0, grid_height-0.001f);  // [0; grid_height-1]

        u32 x_idx = (u32)f_x_idx;
        u32 y_idx = (u32)f_y_idx;

        u32 idx = grid_width * y_idx + x_idx;

        if (!buckets[idx].allocator.buckets) {
            buckets[idx].init(1024, 4, allocator);
        }

        buckets[idx].append(*e);
    }

    void change_subdivision_level(u32 new_subdivisions) {
        if (new_subdivisions == subdivision_level)
            return;

        auto old_buckets = buckets;
        u32 old_cell_count = get_num_cells();

        subdivision_level = new_subdivisions;
        u32 cell_count = get_num_cells();

        buckets = allocator->allocate_0<Bucket_List<Entry>>(cell_count);

        if (old_buckets) {
            for (u32 i = 0; i < old_cell_count; ++i) {
                old_buckets[i].for_each([&] (Entry* e) {
                    insert(e);
                });

                old_buckets[i].deinit();
            }

            allocator->deallocate(old_buckets);
        }
    }

    void range_query(V2 top_left, V2 bot_right, Array_List<Type>* list) {
        u32 grid_width, grid_height;
        get_dimensions(&grid_width, &grid_height);

        u32 box_left_index  = remap(-1,  top_left.x, 1, 0, grid_width-0.001); // [0; grid_width-1]
        u32 box_right_index = remap(-1, bot_right.x, 1, 0, grid_width-0.001); // [0; grid_width-1]

        u32 box_top_index = remap(-0.5,  top_left.y, 0.5, 0, grid_height-0.001); // [0; grid_height-1]
        u32 box_bot_index = remap(-0.5, bot_right.y, 0.5, 0, grid_height-0.001); // [0; grid_height-1]

        for (int y = box_top_index; y <= box_bot_index; ++y) {
            for (int x = box_left_index; x <= box_right_index; ++x) {
                u32 idx = grid_width * y + x;
                // continue if empty cell
                if (!buckets[idx].allocator.buckets)
                    continue;

                if (x == box_left_index || x == box_right_index ||
                    y == box_top_index  || y == box_bot_index)
                {
                    // have to check all
                    buckets[idx].for_each([=](Entry* elem) {
                        if (elem->u >= top_left.x && elem->u <= bot_right.x &&
                            elem->v >= top_left.y && elem->v <= bot_right.y)
                        {
                            list->append(elem->payload);
                        }
                    });
                } else {
                    // just include all
                    buckets[idx].for_each([=](Entry* elem) {
                        list->append(elem->payload);
                    });
                }
            }
        }
    }

    // NOTE(Felix): Retruned image uses 32 bit per pixel rgba. It is heap
    //   allocated and has to be freed by the user. The width will be 2*height.
    //
    //   Protip: The rendered image does not suffer from resampling issues when
    //   'height' is a multiple of the grid height (2 ** subdiv_level)
    auto render_heatmap(u32 height, Allocator_Base* pixel_allocator = nullptr) -> u8* {
        if (!pixel_allocator)
            pixel_allocator = grab_current_allocator();

        struct Pixel {
            u8 r;
            u8 g;
            u8 b;
            u8 a;
        };

        V3 black = {0,0,0};
        V3 blue  = {0,0,1};
        V3 green = {0,1,0};
        V3 red   = {1,0,0};
        V3 white = {1,1,1};

        u32 width = 2 * height;

        u32 max_occupancy = 0;
        u32 cell_count = get_num_cells();
        for (u32 i = 0; i< cell_count; ++i) {
            if (buckets[i].allocator.buckets) {
                u32 count = buckets[i].count();
                if (count > max_occupancy)
                    max_occupancy = count;
            }
        }

        Pixel* pixels = pixel_allocator->allocate<Pixel>(width * height);

        u32 grid_width, grid_height;
        get_dimensions(&grid_width, &grid_height);

        for (u32 y = 0; y < height; ++y) {
            for (u32 x = 0; x < width; ++x) {

                u32 b_x_idx = remap(0, x, width,  0, grid_width);
                u32 b_y_idx = remap(0, y, height, 0, grid_height);

                if (b_x_idx >= grid_width)
                    b_x_idx = grid_width-1;

                if (b_y_idx >= grid_height)
                    b_y_idx = grid_height-1;

                u32 b_idx = b_y_idx * grid_width + b_x_idx;

                u32 occu = 0;
                if (buckets[b_idx].allocator.buckets)
                    occu = buckets[b_idx].count();

                f32 norm_occ = (f32)occu / (f32)max_occupancy;

                V3 color;
                if (norm_occ < 0.25) {
                    color = lerp(black, norm_occ*4, blue);
                } else if (norm_occ < 0.5) {
                    color = noz(lerp(blue, (norm_occ-0.25)*4, green));
                } else if (norm_occ < 0.75) {
                    color = noz(lerp(green, (norm_occ-0.5)*4, red));
                } else {
                    color = lerp(red, (norm_occ-0.75)*4, white);
                }

                pixels[width * y + x] = {
                    .r = (u8)(color.r*255),
                    .g = (u8)(color.g*255),
                    .b = (u8)(color.b*255),
                    .a = 255,
                };
            }
        }

        return (u8*)pixels;
    }
};

#ifdef FTB_MOLLWEIDE_IMPL

void lat_lng_to_mollweide(f32 phi, f32 lambda, f32* out_x, f32* out_y) {
    f32 theta = phi;
    f32 old_t;
    do {
        old_t = theta;
        f32 denom = cosf(theta);

        theta = theta - (2*theta + sin(2*theta) - pi*sin(phi)) /
                        (4 * denom * denom);

    } while(fabs(old_t - theta) > 0.001f);

    *out_x = lambda * cosf(theta) / pi; // [-1;     1]
    *out_y = sinf(theta) / 2;           // [-0.5; 0.5]
}

void mollweide_to_lat_lng(f32 x, f32 y, f32* out_phi, f32* out_lambda) {
    f32 sqrt_2   = sqrtf(2.0f);
    f32 theta    = asinf(y/sqrt_2);
    *out_phi     = asinf((2.0f*theta + sin(2.0f*theta))/pi);
    *out_lambda  = (x*pi)/(2.0f*sqrt_2*cos(theta));
}

#endif
