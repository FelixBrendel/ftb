#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.hpp"
#include "arraylist.hpp"

#define for_hash_map(hm)                                           \
    if (decltype((hm).data[0].original) key   = 0); else           \
    if (decltype((hm).data[0].object)   value = 0); else           \
    for(int index = 0; index < (hm).current_capacity; ++index)     \
        if (!((!(hm).data[index].deleted) &&                       \
              (key   = (hm).data[index].original) &&               \
              (value = (hm).data[index].object))); else

template <typename key_type, typename value_type>
struct Hash_Map {
    int current_capacity;
    int cell_count;
    struct HM_Cell {
        key_type original;
        u64  hash;
        bool deleted;
        value_type object;
    }* data;

    void alloc(int initial_capacity = 8) {
        current_capacity = initial_capacity;
        cell_count = 0;
        data = (HM_Cell*)calloc(initial_capacity, sizeof(HM_Cell));
    }

    void dealloc() {
            free(data);
            data = nullptr;
        }

    int get_index_of_living_cell_if_it_exists(key_type key, u64 hash_val) {
        // int index = hash_val & (current_capacity - 1);
        int index = hash_val % current_capacity;
        HM_Cell cell = data[index];
        /* test if cell exists at that index */
        if (cell.original) {
            /* check if strings match */
            if (hm_objects_match(key, cell.original)) {
                /* we found it, now check it it is deleted: */
                if (cell.deleted) {
                    /* we found it but it was deleted, we     */
                    /* dont have to check for collisions then */
                    return -1;
                } else {
                    /* we found it and it is not deleted */
                    return index;
                }
            } else {
                /* strings dont match, this means we have */
                /* a collision. We just search forward */
                for (int i = 0; i < current_capacity; ++i) {
                    int new_idx = (i + index) % current_capacity;
                    cell = data[new_idx];
                    if (!cell.original)
                        return -1;
                    if (!hm_objects_match(key, cell.original))
                        continue;
                    if (cell.deleted)
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
        for (int i = 0; i < current_capacity; ++i) {
            if (data[i].object == v && !data[i].deleted)
                return data[i].original;
        }
        return nullptr;
    }

    Array_List<key_type> get_all_keys() {
        Array_List<key_type> ret;
        ret.alloc();
        for (int i = 0; i < current_capacity; ++i) {
            if (data[i].original && !data[i].deleted)
                ret.append(data[i].original);
        }
        return ret;
    }

    value_type get_object(key_type key) {
        int index = get_index_of_living_cell_if_it_exists(key, hm_hash((key_type)key));
        if (index != -1) {
            return data[index].object;
        }
        return 0;
    }

    void delete_object(key_type key) {
        int index = get_index_of_living_cell_if_it_exists(key, hm_hash((key_type)key));
        if (index != -1) {
            data[index].deleted = true;
        }
    }

    void set_object(key_type key, value_type obj, u64 hash_val) {
        int index = hash_val % current_capacity;

        /* if we the desired cell is just empty, write to it and done :) */
        if (!data[index].original) {
            /* insert new cell into desired slot */
            ++cell_count;
        } else {
            if (hm_objects_match(key, data[index].original)) {
                /* overwrite object with same key, dont increment cell */
                /* count                                               */
            } else {
                /* collision, check resize */
                ++cell_count;
                if ((cell_count*1.0f / current_capacity) > 0.666f) {
                    auto old_data = data;
                    data = (HM_Cell*)calloc(current_capacity*4, sizeof(HM_Cell));
                    cell_count = 0;
                    current_capacity *= 4;

                    /* insert all old items again */
                    for (int i = 0; i < current_capacity/4; ++i) {
                        auto cell = old_data[i];
                        if (cell.original) {
                            set_object(cell.original, cell.object, cell.hash);
                        }
                    }
                    free(old_data);
                    index = hash_val % current_capacity;
                }
                /* search for empty slot for new cell starting at desired index; */
                /* preventing gotos using lambdas!                               */
                [&]{
                    for (int i = index; i < current_capacity; ++i) {
                        if (!data[i].original ||
                            hm_objects_match(data[i].original, key))
                        {
                            index = i;
                            return;
                        }
                    }
                    for (int i = 0; i < index; ++i) {
                        if (!data[i].original ||
                            hm_objects_match(data[i].original, key))
                        {
                            index = i;
                            return;
                        }
                    }
                }();
            }
        }

        data[index].deleted = false;
        data[index].original = key;
        data[index].hash = hash_val;
        data[index].object = obj;
    }

    void set_object(key_type key, value_type obj) {
        u64 hash_val = hm_hash((key_type)key);
        set_object(key, obj, hash_val);
    }
};
