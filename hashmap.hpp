#pragma once

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.hpp"
#include "arraylist.hpp"


u32 hm_hash(char* str) {
    u32 value = str[0] << 7;
    s32 i = 0;
    while (str[i]) {
        value = (10000003 * value) ^ str[i++];
    }
    return value ^ i;
}

u32 hm_hash(void* ptr) {
    return ((u64)ptr * 2654435761) % 4294967296;
}

inline bool hm_objects_match(char* a, char* b) {
    return strcmp(a, b) == 0;
}

inline bool hm_objects_match(void* a, void* b) {
    return a == b;
}


#define for_hash_map(hm)                                           \
    if (decltype((hm).data[0].original) key   = 0); else           \
    if (decltype((hm).data[0].object)   value = 0); else           \
    for(u32 index = 0; index < (hm).current_capacity; ++index)     \
        if (!((!(hm).data[index].deleted) &&                       \
              (key   = (hm).data[index].original) &&               \
              (value = (hm).data[index].object))); else

template <typename key_type, typename value_type>
struct Hash_Map {
    u32 current_capacity;
    u32 cell_count;
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

    void alloc(u32 initial_capacity = 8) {
        current_capacity = initial_capacity;
        cell_count = 0;
        data = (HM_Cell*)calloc(initial_capacity, sizeof(HM_Cell));
    }

    void dealloc() {
        free(data);
        data = nullptr;
    }

    s32 get_index_of_living_cell_if_it_exists(key_type key, u64 hash_val) {
        // s32 index = hash_val & (current_capacity - 1);
        s32 index = hash_val % current_capacity;
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
                for (u32 i = 0; i < current_capacity; ++i) {
                    u32 new_idx = (i + index) % current_capacity;
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
        for (u32 i = 0; i < current_capacity; ++i) {
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
        ret.alloc();
        // QUESTION(Felix): Does it make sense to
        // ret.reserve(this->cell_count)?
        for (u32 i = 0; i < current_capacity; ++i) {
            if (data[i].occupancy == HM_Cell::Occupancy::Occupied)
                ret.append(data[i].original);
        }
        return ret;
    }

    value_type get_object(key_type key, u64 hash_val) {
        s32 index = get_index_of_living_cell_if_it_exists(key, hash_val);
        if (index != -1) {
            return data[index].object;
        }
        return 0;
    }

    value_type get_object(key_type key) {
        return get_object(key, hm_hash((key_type)key));
    }


    void delete_object(key_type key) {
        s32 index = get_index_of_living_cell_if_it_exists(key, hm_hash((key_type)key));
        if (index != -1) {
            data[index].occupancy = HM_Cell::Occupancy::Deleted;
        }
    }

    void set_object(key_type key, value_type obj, u64 hash_val) {
        u32 index = hash_val % current_capacity;

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
                    data = (HM_Cell*)calloc(current_capacity*4, sizeof(HM_Cell));
                    cell_count = 0;
                    current_capacity *= 4;

                    /* insert all old items again */
                    for (u32 i = 0; i < current_capacity/4; ++i) {
                        auto cell = old_data[i];
                        if (cell.occupancy == HM_Cell::Occupancy::Occupied) {
                            set_object(cell.original, cell.object, cell.hash);
                        }
                    }
                    free(old_data);
                    index = hash_val % current_capacity;
                }
                ++cell_count;
                /* search for empty slot for new cell starting at desired index; */
                /* preventing gotos using lambdas!                               */
                [&]{
                    for (u32 i = index; i < current_capacity; ++i) {
                        if (data[i].occupancy == HM_Cell::Occupancy::Avaliable ||
                            hm_objects_match(data[i].original, key))
                        {
                            index = i;
                            return;
                        }
                    }
                    for (u32 i = 0; i < index; ++i) {
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
};
