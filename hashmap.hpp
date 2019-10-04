#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.hpp"


// inline void print_hm(StringHashMap* hm) {
    // for_str_hash_map(hm) {
        // printf("index: %d desired-index: %llu key: %s hash: %llu value: %llu\n", i, (hm->data[i].hash % hm->current_capacity), (char*)key, hm->data[i].hash, (unsigned long long)value);
    // }
// }

#define __for_hm_generator(key_type, hm)                           \
    if (key_type key   = nullptr); else                            \
    if (void*    value = nullptr); else                            \
    for(int      index = 0; index < hm->current_capacity; ++index) \
    if (!((key   = hm->data[index].original) &&                    \
          (value = hm->data[index].object))); else


#define define_hash_map(type, name)                                     \
    bool hm_objects_match(type,type);                                   \
    u32 hm_hash(type);                                                  \
                                                                        \
    struct name##_Hash_Map_Cell {                                       \
        type original;                                                  \
        u64  hash;                                                      \
        bool deleted;                                                   \
        void* object;                                                   \
    };                                                                  \
                                                                        \
    struct name##_Hash_Map {                                            \
        int current_capacity;                                           \
        int cell_count;                                                 \
        name##_Hash_Map_Cell* data;                                     \
    };                                                                  \
                                                                        \
    name##_Hash_Map* create_##name##_hashmap(int initial_capacity=8)  { \
        name##_Hash_Map* hm = new name##_Hash_Map;                      \
        hm->current_capacity = initial_capacity;                        \
        hm->cell_count = 0;                                             \
        /* set all data to nullptr */                                   \
        hm->data = (name##_Hash_Map_Cell*)calloc(initial_capacity, sizeof(name##_Hash_Map_Cell)); \
        /* printf("check: %s\n", hm->data[6].original_string); */       \
        return hm;                                                      \
    }                                                                   \
                                                                        \
    int hm_get_index_of_living_cell_if_it_exists(name##_Hash_Map* hm, type key, u64 hash_val) { \
        int index = hash_val % hm->current_capacity;                    \
        name##_Hash_Map_Cell cell = hm->data[index];                    \
        /* test if cell exists at that index */                         \
        if (cell.original) {                                            \
            /* check if strings match */                                \
            if (hm_objects_match(key, cell.original)) {                 \
                /* we found it, now check it it is deleted: */          \
                if (cell.deleted) {                                     \
                    /* we found it but it was deleted, we     */        \
                    /* dont have to check for collisions then */        \
                    return -1;                                          \
                } else {                                                \
                    /* we found it and it is not deleted */             \
                    return index;                                       \
                }                                                       \
            } else {                                                    \
                /* strings dont match, this means we have */            \
                /* a collision. We just search forward */               \
                for (int i = 0; i < hm->current_capacity; ++i) {        \
                    int new_idx = (i + index) % hm->current_capacity;   \
                    cell = hm->data[new_idx];                           \
                    if (!cell.original)                                 \
                        return -1;                                      \
                    if (!hm_objects_match(key, cell.original))          \
                        continue;                                       \
                    if (cell.deleted)                                   \
                        continue;                                       \
                    return new_idx;                                     \
                }                                                       \
                /* not or only deleted cells found */                   \
                return -1;                                              \
            }                                                           \
        } else {                                                        \
            /* no cell exists at this index so the item was never in the  */ \
            /* hashmap. Either it would be there or be ther and 'deleted' */ \
            /* or another item would be there and therefore a collistion  */ \
            /* would exist */                                           \
            return -1;                                                  \
        }                                                               \
    }                                                                   \
                                                                        \
    inline bool hm_key_exists(name##_Hash_Map* hm, type key) {          \
        return hm_get_index_of_living_cell_if_it_exists(hm, key, hm_hash(key)) != -1; \
    }                                                                   \
                                                                        \
    inline void* hm_get_object(name##_Hash_Map* hm, type key) {         \
        int index = hm_get_index_of_living_cell_if_it_exists(hm, key, hm_hash(key)); \
        if (index != -1) {                                              \
            return hm->data[index].object;                              \
        }                                                               \
        return nullptr;                                                 \
    }                                                                   \
                                                                        \
    inline void hm_delete_object(name##_Hash_Map* hm, type key) {                    \
        int index = hm_get_index_of_living_cell_if_it_exists(hm, key, hm_hash(key)); \
        if (index != -1) {                                              \
            hm->data[index].deleted = true;                             \
        }                                                               \
    }                                                                   \
                                                                        \
    void hm_set(name##_Hash_Map* hm, type key, void* obj) {             \
        u64 hash_val = hm_hash(key);                                    \
        int index = hash_val % hm->current_capacity;                    \
                                                                        \
        /* if we the desired cell is just empty, write to it and done :) */ \
        if (!hm->data[index].original) {                                \
            /* insert new cell into desired slot */                     \
            ++hm->cell_count;                                           \
        } else {                                                        \
            if (hm_objects_match(key, hm->data[index].original)) {      \
                /* overwrite object with same key, dont increment cell */ \
                /* count                                               */ \
            } else {                                                    \
                /* collision, check resize */                           \
                ++hm->cell_count;                                       \
                if ((hm->cell_count*1.0f / hm->current_capacity) > 0.666f) { \
                    auto old_data = hm->data;                           \
                    hm->data = (name##_Hash_Map_Cell*)calloc(hm->current_capacity*4, sizeof(name##_Hash_Map_Cell)); \
                    hm->cell_count = 0;                                 \
                    hm->current_capacity *= 4;                          \
                                                                        \
                    /* insert all old items again */                    \
                    for (int i = 0; i < hm->current_capacity/4; ++i) {  \
                        auto cell = old_data[i];                        \
                        if (cell.original) {                            \
                            hm_set(hm, cell.original, cell.object);     \
                        }                                               \
                    }                                                   \
                    free(old_data);                                     \
                    index = hash_val % hm->current_capacity;            \
                }                                                       \
                /* search for empty slot for new cell starting at desired index; */ \
                /* preventing gotos using lambdas!                               */ \
                [&](){                                                  \
                    for (int i = index; i < hm->current_capacity; ++i) { \
                        if (!hm->data[i].original ||                    \
                            hm_objects_match(hm->data[i].original, key)) \
                        {                                               \
                            index = i;                                  \
                            return;                                     \
                        }                                               \
                    }                                                   \
                    for (int i = 0; i < index; ++i) {                   \
                        if (!hm->data[i].original ||                    \
                            hm_objects_match(hm->data[i].original, key)) \
                        {                                               \
                            index = i;                                  \
                            return;                                     \
                        }                                               \
                    }                                                   \
                }();                                                    \
            }                                                           \
        }                                                               \
                                                                        \
        hm->data[index].deleted = false;                                \
        hm->data[index].original = key;                                 \
        hm->data[index].hash = hash_val;                                \
        hm->data[index].object = obj;                                   \
    }                                                                   \
