#pragma once
#include "arraylist.hpp"
#include "types.hpp"

template <typename type>
struct Bucket_Allocator {
    u32 next_index_in_latest_bucket;
    u32 next_bucket_index;
    u32 bucket_count;
    u32 bucket_size;

    Array_List<type*> free_list;
    type** buckets;

    void clear() {
        next_index_in_latest_bucket = 0;
        next_bucket_index = 0;
        free_list.clear();
    }

    void expand() {
        buckets = (type**)realloc(buckets, bucket_count * 2 * sizeof(type*));
        bucket_count *= 2;
    }

    void jump_to_next_bucket() {
        next_index_in_latest_bucket = 0;
        ++next_bucket_index;
        if (next_bucket_index >= bucket_count) {
            expand();
        }
        buckets[next_bucket_index] = (type*)malloc(bucket_size * sizeof(type));
    }

    void increment_pointers(s32 amount = 1) {
        next_index_in_latest_bucket += amount;
        if (next_index_in_latest_bucket >= bucket_size) {
            jump_to_next_bucket();
        }
    }

    void init(u32 bucket_size = 16, u32 initial_bucket_count = 8) {
        this->free_list.init();
        this->bucket_size = bucket_size;
        next_index_in_latest_bucket = 0;
        next_bucket_index = 0;
        bucket_count = initial_bucket_count;

        buckets = (type**)malloc(bucket_count * sizeof(type*));
        buckets[0] = (type*)malloc(bucket_size * sizeof(type));
    }

    void deinit() {
        for (u32 i = 0; i <= next_bucket_index; ++i) {
            free(buckets[i]);
        }
        this->free_list.deinit();
        free(buckets);
    }

    u32 count_elements() {
        free_list.sort();
        type* val;
        u32 count = 0;
        for (u32 i = 0; i < next_bucket_index; ++i) {
            for (u32 j = 0; j < bucket_size; ++j) {
                val = buckets[i]+j;
                if (free_list.sorted_find(val) == -1)
                    count++;
            }
        }
        for (u32 j = 0; j < next_index_in_latest_bucket; ++j) {
            val = buckets[next_bucket_index]+j;
            if (free_list.sorted_find(val) == -1)
                count++;
        }
        return count;
    }

    template <typename lambda>
    void for_each(lambda p) {
        free_list.sort();

        type* val;
        for (u32 i = 0; i < next_bucket_index; ++i) {
            for (u32 j = 0; j < bucket_size; ++j) {
                val = buckets[i]+j;
                if (free_list.sorted_find(val) == -1)
                    p(val);
            }
        }
        for (u32 j = 0; j < next_index_in_latest_bucket; ++j) {
            val = buckets[next_bucket_index]+j;
            if (free_list.sorted_find(val) == -1)
                p(val);
        }
    }

    type* allocate(u32 amount = 1) {
        type* ret;
        if (amount == 0) return nullptr;
        if (amount == 1) {
            if (free_list.count != 0) {
                return free_list.data[--free_list.count];
            }
             ret = buckets[next_bucket_index]+next_index_in_latest_bucket;
             increment_pointers(1);
            return ret;
        }
        if (amount > bucket_size)
            return nullptr;
        if ((bucket_size - next_index_in_latest_bucket) >= 4) {
            // if the current bucket is ahs enough free space
            ret = buckets[next_bucket_index]+(next_index_in_latest_bucket);
            increment_pointers(amount);
            return ret;
        } else {
            // the current bucket does not have enough free space
            // add all remainding slots to free list
            while (next_index_in_latest_bucket < bucket_size) {
                free_list.append(buckets[next_bucket_index]+next_index_in_latest_bucket);
                ++next_index_in_latest_bucket;
            }
            jump_to_next_bucket();
            return allocate(amount);
        }
    }

    void free_object(type* obj) {
        free_list.append(obj);
    }
};


template <typename type>
struct Bucket_List {
    Bucket_Allocator<type> allocator;

    void init(u32 bucket_size = 16, u32 initial_bucket_count = 8) {
        allocator.init(bucket_size, initial_bucket_count);
    }

    void deinit() {
        allocator.deinit();
    }

    void clear() {
        allocator.clear();
    }

    u32 count() {
        return
            allocator.next_bucket_index * allocator.bucket_size +
            allocator.next_index_in_latest_bucket;
    }

    void append(type elem) {
        type* mem = allocator.allocate();
        *mem = elem;
    }

    void remove_index(u32 index) {
        u32 el_count = count();

#ifdef FTB_INTERNAL_DEBUG
        if (index >= el_count) {
            fprintf(stderr, "ERROR: removing index that is not in use\n");
        }
#endif

        type last = (*this)[el_count-1];

        (*this)[index] = last;

        if (allocator.next_index_in_latest_bucket == 0) {
            --allocator.next_bucket_index;
            allocator.next_index_in_latest_bucket = allocator.bucket_size-1;
        } else {
            --allocator.next_index_in_latest_bucket;
        }
    }

    type& operator[] (u32 index) {
#ifdef FTB_INTERNAL_DEBUG
        u32 el_count = count();
        if (index >= el_count) {
            fprintf(stderr, "ERROR: accessing index that is not in use\n");
        }
#endif

        u32 bucket_idx    = index / allocator.bucket_size;
        u32 idx_in_bucket = index % allocator.bucket_size;
        return allocator.buckets[bucket_idx][idx_in_bucket];
    }

    template <typename lambda>
    void for_each(lambda p) {
        allocator.for_each(p);
    }
};

template <typename type>
struct Bucket_Queue {
    Bucket_List<type> bucket_list;
    u32 start_idx;

    void init(u32 bucket_size = 16, u32 initial_bucket_count = 8) {
        bucket_list.init(bucket_size, initial_bucket_count);
        start_idx = 0;
    }

    void deinit() {
        bucket_list.deinit();
    }

    void push_back(type e) {
        bucket_list.append(e);
    }

    type get_next() {
#ifdef FTB_INTERNAL_DEBUG
        if (start_idx == 0 &&
            bucket_list.allocator.next_index_in_latest_bucket == 0 &&
            bucket_list.allocator.next_bucket_index == 0)
        {
            fprintf(stderr, "ERROR: queue has no next element\n");
        }
#endif

        type result = bucket_list[start_idx++];

        // maybe garbage collect left bucket
        if (start_idx >= bucket_list.allocator.bucket_size) {
            start_idx -= bucket_list.allocator.bucket_size;

            for (u32 i = 0; i < bucket_list.allocator.next_bucket_index; ++i) {
                bucket_list.allocator.buckets[i] = bucket_list.allocator.buckets[i+1];
            }
            --bucket_list.allocator.next_bucket_index;
        }


        // maybe reset if queue now empty
        if (start_idx == bucket_list.allocator.next_index_in_latest_bucket &&
            bucket_list.allocator.next_bucket_index == 0)
        {
            bucket_list.allocator.next_index_in_latest_bucket = 0;
            bucket_list.allocator.next_bucket_index = 0;
            start_idx = 0;
        }

        return result;


    }

    u32 count() {
        return bucket_list.count() - start_idx;
    }

    bool is_empty() {
        return count() == 0;
    }
};
