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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "core.hpp"

struct Integer_Pair {
    s32 x;
    s32 y;
};

#ifndef FTB_HASHMAP_IMPL


u64 hm_hash(const char* str);
u64 hm_hash(char* str);
u64 hm_hash(void* ptr);
u64 hm_hash(u64   value);
u64 hm_hash(Integer_Pair ip);

bool hm_objects_match(const char* a, const char* b);
bool hm_objects_match(char* a, char* b);
bool hm_objects_match(void* a, void* b);
bool hm_objects_match(u64   a, u64 b);
bool hm_objects_match(Integer_Pair i1, Integer_Pair i2);

#else // implementations

u64 hm_hash(Integer_Pair ip) {
    // cantor pairing
    return (u64)(0.5f * (ip.x + ip.y) * (ip.x + ip.y + 1) + ip.y);
}

bool hm_objects_match(Integer_Pair i1, Integer_Pair i2) {
    return i1.x == i2.x && i1.y == i2.y;
}

u64 hm_hash(const char* str) {
    u64 value = str[0] << 7;
    s64 i = 0;
    while (str[i]) {
        value = (10000003 * value) ^ str[i++];
    }
    return value ^ i;
}

u64 hm_hash(char* str) {
    u64 value = str[0] << 7;
    s64 i = 0;
    while (str[i]) {
        value = (10000003 * value) ^ str[i++];
    }
    return value ^ i;
}

u64 hm_hash(u64 value) {
    return (value * 2654435761) % 4294967296;
}

u64 hm_hash(void* ptr) {
    return hm_hash((u64)ptr);
}

bool hm_objects_match(const char* a, const char* b) {
    return strcmp(a, b) == 0;
}

bool hm_objects_match(char* a, char* b) {
    return strcmp(a, b) == 0;
}

bool hm_objects_match(void* a, void* b) {
    return a == b;
}

bool hm_objects_match(u64 a, u64 b) {
    return a == b;
}
#endif //FTB_HASHMAP_IMPL


template <typename key_type, typename value_type>
struct Hash_Map {
    u64 current_capacity;
    u64 cell_count;
    Allocator_Base* allocator;

    struct HM_Cell {
        key_type original;
        u64  hash;
        enum struct Occupancy : u8 {
            Avaliable = 0,
            Occupied,
            Deleted
        } occupancy;
        value_type object;
    }* data;

    void init(u64 initial_capacity = 8, Allocator_Base* back_allocator = nullptr) {
        if (back_allocator)
            allocator = back_allocator;
        else
            allocator = grab_current_allocator();

        // round up to next pow of 2
        --initial_capacity;
        initial_capacity |= initial_capacity >> 1;
        initial_capacity |= initial_capacity >> 2;
        initial_capacity |= initial_capacity >> 4;
        initial_capacity |= initial_capacity >> 8;
        initial_capacity |= initial_capacity >> 16;
        ++initial_capacity;
        // until here
        current_capacity = initial_capacity;
        cell_count = 0;
        data = allocator->allocate_0<HM_Cell>(initial_capacity);
    }

    void deinit() {
        allocator->deallocate(data);
        data = nullptr;
    }


    template <typename lambda>
    void for_each(lambda p) {
        for(u64 index = 0; index < current_capacity; ++index)
            if (data[index].occupancy == HM_Cell::Occupancy::Occupied)
                p(data[index].original, data[index].object, index);
    }

    void clear() {
        cell_count = 0;
        memset(data, 0, current_capacity * sizeof(HM_Cell));
    }


    s64 get_index_of_living_cell_if_it_exists(key_type key, u64 hash_val) {
        s64 index = hash_val & (current_capacity - 1);
        HM_Cell cell = data[index];
        /* test if there is or was something there */
        if (cell.occupancy != HM_Cell::Occupancy::Avaliable) {
            /* check if objects match */
            if (hm_objects_match(key, cell.original)) {
                /* we found it, now check it it is deleted: */
                if (cell.occupancy == HM_Cell::Occupancy::Deleted) {
                    /* we found it but it was deleted, we     */
                    /* dont have to check for collisions then */
                    return -1;
                } else {
                    /* we found it and it is not deleted */
                    return index;
                }
            } else {
                /* objects dont match, this means we have */
                /* a collision. We just search forward    */
                for (u64 i = 0; i < current_capacity; ++i) {
                    u64 new_idx = (i + index) & (current_capacity - 1);
                    cell = data[new_idx];
                    /* If we find a avaliable cell while looking */
                    /* forward, the object is not in the hm      */
                    if (cell.occupancy == HM_Cell::Occupancy::Avaliable)
                        return -1;
                    /* If the objects don't match, keep looking */
                    if (!hm_objects_match(key, cell.original))
                        continue;
                    /* TODO(Felix): If the objects do match,   */
                    /* and it is deleted, we should return -1? */
                    if (cell.occupancy == HM_Cell::Occupancy::Deleted)
                        continue;
                    return new_idx;
                }
                /* not or only deleted cells found */
                return -1;
            }
        } else {
            /* no cell exists at this index so the item was never in the  */
            /* hashmap. Either it would be there or be ther and 'deleted' */
            /* or another item would be there and therefore a collistion  */
            /* would exist */
            return -1;
        }
    }

    bool key_exists(key_type key) {
        return get_index_of_living_cell_if_it_exists(key, hm_hash((key_type)key)) != -1;
    }

    key_type search_key_to_object(value_type v) {
        for (u64 i = 0; i < current_capacity; ++i) {
            if (data[i].object == v &&
                data[i].occupancy == HM_Cell::Occupancy::Occupied)
            {
                return data[i].original;
            }
        }
        return nullptr;
    }

    Array_List<key_type> get_all_keys() {
        Array_List<key_type> ret;
        ret.init();
        // QUESTION(Felix): Does it make sense to
        // ret.reserve(this->cell_count)?
        for (u64 i = 0; i < current_capacity; ++i) {
            if (data[i].occupancy == HM_Cell::Occupancy::Occupied)
                ret.append(data[i].original);
        }
        return ret;
    }

    value_type get_object(key_type key, u64 hash_val) {
        s64 index = get_index_of_living_cell_if_it_exists(key, hash_val);
        if (index != -1) {
            return data[index].object;
        }
        return 0;
    }

    value_type get_object(key_type key) {
        return get_object(key, hm_hash((key_type)key));
    }

    value_type* get_object_ptr(key_type key, u64 hash_val) {
        s64 index = get_index_of_living_cell_if_it_exists(key, hash_val);
        if (index != -1) {
            return &(data[index].object);
        }
        return 0;
    }

    value_type* get_object_ptr(key_type key) {
        return get_object_ptr(key, hm_hash((key_type)key));
    }


    void delete_object(key_type key) {
        s64 index = get_index_of_living_cell_if_it_exists(key, hm_hash((key_type)key));
        if (index != -1) {
            data[index].occupancy = HM_Cell::Occupancy::Deleted;
        }
    }

    void set_object(key_type key, value_type obj, u64 hash_val) {
        u64 index = hash_val & (current_capacity - 1);

        /* if we the desired cell is avaliable, write to it and done :) */
        if (data[index].occupancy == HM_Cell::Occupancy::Avaliable) {
            /* insert new cell into desired slot */
            ++cell_count;
        } else {
            if (hm_objects_match(key, data[index].original)) {
                /* overwrite object with same key, dont increment cell */
                /* count                                               */
            } else {
                /* collision, check resize */
                if ((cell_count*1.0f / current_capacity) > 0.666f) {
                    auto old_data = data;
                    data = allocator->allocate_0<HM_Cell>(current_capacity*4);
                    cell_count = 0;
                    current_capacity *= 4;

                    /* insert all old items again */
                    for (u64 i = 0; i < current_capacity/4; ++i) {
                        auto cell = old_data[i];
                        if (cell.occupancy == HM_Cell::Occupancy::Occupied) {
                            set_object(cell.original, cell.object, cell.hash);
                        }
                    }
                    allocator->deallocate(old_data);
                    index = hash_val & (current_capacity - 1);
                }
                ++cell_count;
                /* search for empty slot for new cell starting at desired index; */
                /* preventing gotos using lambdas!                               */
                [&]{
                    for (u64 i = index; i < current_capacity; ++i) {
                        if (data[i].occupancy == HM_Cell::Occupancy::Avaliable ||
                            hm_objects_match(data[i].original, key))
                        {
                            index = i;
                            return;
                        }
                    }
                    for (u64 i = 0; i < index; ++i) {
                        if (data[i].occupancy == HM_Cell::Occupancy::Avaliable ||
                            hm_objects_match(data[i].original, key))
                        {
                            index = i;
                            return;
                        }
                    }
                }();
            }
        }

        data[index].occupancy = HM_Cell::Occupancy::Occupied;
        data[index].original = key;
        data[index].hash = hash_val;
        data[index].object = obj;
    }

    void set_object(key_type key, value_type obj) {
        u64 hash_val = hm_hash((key_type)key);
        set_object(key, obj, hash_val);
    }

    void print_occupancy(u32 line_width = 80) {
        char dist_chars[] = "0123456789abcdefghijklmniopqrstuvwxyz";
        u32 sum_of_dists = 0;
        for (u64 i = 0; i < current_capacity; ++i) {
            if (i % line_width == 0) {
                println("");
            }
            if (data[i].occupancy == HM_Cell::Occupancy::Avaliable) {
                raw_print(".");
            } else if (data[i].occupancy == HM_Cell::Occupancy::Deleted) {
                raw_print("+");
            } else {
                u64 desired_index = data[i].hash & (current_capacity - 1);
                u32 dist;
                if (desired_index <= i) {
                    dist = i - desired_index;
                } else {
                    dist = (i+current_capacity) - desired_index;
                }

                dist = MIN(dist, sizeof(dist_chars)-1);

                raw_print("%c", dist_chars[dist]);
                sum_of_dists += dist;
            }
        }

        println("");
        println("Avg linear probing dist: %.2f",
                cell_count != 0
                ? 1.0f*sum_of_dists/cell_count
                : 0.0);
    }

    void dump_occupancy(const char* path) {
        FILE* out = fopen(path, "w");
        defer { fclose(out); };
        for (u64 i = 0; i < current_capacity; ++i) {
            if (data[i].occupancy == HM_Cell::Occupancy::Avaliable) {
                fprintf(out, "%04u [AVAILABLE]\n", i);
            } else if (data[i].occupancy == HM_Cell::Occupancy::Deleted) {
                fprintf(out, "%04u [DELETED]  hash: %llu (wants to be %llu)\n", i, data[i].hash, data[i].hash & (current_capacity - 1));
            } else {
                fprintf(out, "%04u [OCCUPIED] hash: %llu (wants to be %llu)\n", i, data[i].hash, data[i].hash & (current_capacity - 1));
            }
        }

    }
};
