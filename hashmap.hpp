#pragma once
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "types.hpp"

#define for_str_hash_map(hm)                                    \
    if (char* key   = nullptr); else                            \
    if (void* value = nullptr); else                            \
        for(int i = 0; i < hm->current_capacity; ++i)           \
            if (!((key   = hm->data[i].original_string) &&      \
                  (value = hm->data[i].object))); else



struct StringHashMapCell {
    char* original_string;
    void* object;
    u64 hash;
    bool deleted;
};

struct StringHashMap {
    int current_capacity;
    int cell_count;
    StringHashMapCell* data;
};

inline void print_hm(StringHashMap* hm) {
    for_str_hash_map(hm) {
        printf("index: %d key: %s value: %llu\n", i, key, (unsigned long long)value);
    }
}

inline bool strings_match(char* a, char* b) {
    return strcmp(a, b) == 0;
}

u32 hash(char* str) {
    u32 value = str[0] << 7;
    int i = 0;
    while (str[i]) {
        value = (10000003 * value) ^ str[i++];
    }
    return value ^ i;

    // unsigned long hash = 5381;
    // int c;

    // while (c = *str++)
    //     hash = ((hash << 5) + hash) + c; /* hash * 33 + c */

    // return hash;

    // int result = 0x55555555;

    // while (*str) {
    //     result ^= *str++;
    //     // result = (result << 5) | (result >> (32 - 5)) & ~((-1 >> 5) << 5);
    //     // result = rol(result, 5);
    // }

    // return result;

    // uint32_t hash, i;
    // for(hash = i = 0; str[i]; ++i)
    // {
    //     hash += str[i];
    //     hash += (hash << 10);
    //     hash ^= (hash >> 6);
    // }
    // hash += (hash << 3);
    // hash ^= (hash >> 11);
    // hash += (hash << 15);
    // return hash;
}

StringHashMap* create_hashmap(int initial_capacity = 8) {
    printf("1aeaeae\n");
    StringHashMap* hm = new StringHashMap;
    hm->current_capacity = initial_capacity;
    hm->cell_count = 0;
    // set all data to nullptr
    hm->data = (StringHashMapCell *)calloc(initial_capacity, sizeof(StringHashMapCell*));
    // printf("check: %s\n", hm->data[6].original_string);
    printf("after creating...\n");
    print_hm(hm);
    return hm;
}

int hm_get_index_of_living_cell_if_it_exists(StringHashMap* hm, char* key, u64 hash_val) {
    printf("2aeaeae\n");
    int index = hash_val % hm->current_capacity;
    StringHashMapCell cell = hm->data[index];
    // test if cell exists at that index
    if (cell.original_string) {
        // check if strings match
        if (strings_match(key, cell.original_string)) {
            // we found it, now check it it is deleted:
            if (cell.deleted) {
                // we found it but it was deleted, we dont have to
                // check for collisions then
                return -1;
            } else {
                // we found it and it is not deleted
                return index;
            }
        } else {
            // strings dont match, this means we have a collision.
            // We just search front to back, lol
            for (int i = 0; i < hm->current_capacity; ++i) {
                cell = hm->data[i];
                if (!cell.original_string)
                    continue;
                if (!strings_match(key, cell.original_string))
                    continue;
                if (cell.deleted)
                    continue;
                return i;
            }
            // not or only deleted cells found
            return -1;
        }
    } else {
        // no cell exists at this index so the item was never in the
        // hashmap. Either it would be there or be ther and 'deleted'
        // or another item would be there and therefore a collistion
        // would exist
        return -1;
    }
}

inline bool hm_key_exists(StringHashMap* hm, char* key) {
    return hm_get_index_of_living_cell_if_it_exists(hm, key, hash(key)) != -1;
}

inline void* hm_get_object(StringHashMap* hm, char* key) {
    int index = hm_get_index_of_living_cell_if_it_exists(hm, key, hash(key));
    if (index != -1) {
        return hm->data[index].object;
    }
    return nullptr;
}

inline void hm_delete_object(StringHashMap* hm, char* key) {
    int index = hm_get_index_of_living_cell_if_it_exists(hm, key, hash(key));
    if (index != -1) {
        hm->data[index].deleted = true;
    }
}

void hm_set(StringHashMap* hm, char* key, void* obj) {
    printf(" -------------\n");
    printf("before...\n");
    print_hm(hm);

    u64 hash_val = hash(key);
    int index = index = hash_val % hm->current_capacity;

    // if we the desired cell is just empty, write to it and done :)
    printf("  setting:  %s\n", key);
    printf("  capacity: %d\n", hm->current_capacity);
    printf("  index: %d\n", index);
    if (!hm->data[index].original_string) {
        // insert new cell into desired slot
        printf("alles paletti\n");
        ++hm->cell_count;
    } else {
        if (strings_match(key, hm->data[index].original_string)) {
            // overwrite object with same key, dont increment cell
            // count
        } else {
            // collision, check resize
            ++hm->cell_count;
            if ((hm->cell_count*1.0f / hm->current_capacity) > 0.666f) {
                printf("!!!reallocation\n");
                StringHashMapCell* old_data = hm->data;
                hm->data = (StringHashMapCell*)calloc(hm->current_capacity*4, sizeof(StringHashMapCell*));
                hm->cell_count = 0;
                hm->current_capacity *= 4;

                // insert all old items again
                for (int i = 0; i < hm->current_capacity/4; ++i) {
                    StringHashMapCell cell = old_data[i];
                    if (cell.original_string) {
                        hm_set(hm, cell.original_string, cell.object);
                    }
                }
                free(old_data);
            }
            // search for empty slot for new cell starting at desired index;
            // preventing gotos using lambdas!
            [&](){
                for (int i = index; i < hm->current_capacity; ++i) {
                    printf("  checking free index: %d\n", i);
                    if (!hm->data[i].original_string ||
                        strings_match(hm->data[i].original_string, key))
                    {
                        index = i;
                        return;
                    } else {
                        printf("-> not free, found %s\n", hm->data[i].original_string);
                    }
                }
                for (int i = 0; i < index; ++i) {
                    printf("  checking free index: %d\n", i);
                    if (!hm->data[i].original_string ||
                        strings_match(hm->data[i].original_string, key))
                    {
                        index = i;
                        return;
                    } else {
                        printf("-> not free, found %s", hm->data[i].original_string);
                    }
                }
            }();
        }
    }

    printf ("  writing at index: %d\n", index);
    hm->data[index].deleted = false;
    hm->data[index].original_string = key;
    hm->data[index].hash = hash_val;
    hm->data[index].object = obj;
    printf("after...\n");
    print_hm(hm);
}
