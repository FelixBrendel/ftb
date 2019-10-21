#include "arraylist.hpp"

template <typename type, unsigned int bucket_size>
class Bucket_Allocator {
    int latest_bucket;
    int next_index_in_latest_bucket;
    int bucket_count;
    int next_bucket_index;

    Array_List<type*> free_list;
    type** buckets;

    void expand() {
        // realloc time
        buckets = (type**)realloc(buckets, bucket_count * 2 * sizeof(type*));
        for (int i = bucket_count; i < bucket_count * 2; ++i) {
            buckets[i] = (type*)malloc(bucket_size * sizeof(type));
        }
        bucket_count *= 2;
    }

    void jump_to_next_bucket() {
        next_index_in_latest_bucket = 0;
        ++next_bucket_index;
        if (next_bucket_index >= bucket_count) {
            expand();
        }
    }

    void increment_pointers(int amount = 1) {
        next_index_in_latest_bucket += amount;
        if (next_index_in_latest_bucket >= bucket_size) {
            jump_to_next_bucket();
        }
    }

public:
    Bucket_Allocator(unsigned int initial_bucket_count = 1) {
        latest_bucket = 0;
        next_index_in_latest_bucket = 0;
        next_bucket_index = 0;
        bucket_count = initial_bucket_count;

        buckets = (type**)malloc(bucket_count * sizeof(type*));
        for (int i = 0; i < bucket_count; ++i) {
            buckets[i] = (type*)malloc(bucket_size * sizeof(type));
        }
    }

    ~Bucket_Allocator() {
        for (int i = 0; i < bucket_count; ++i) {
            free(buckets[i]);
        }
        ::free(buckets);
    }

    type* allocate(unsigned int amount = 1) {
        type* ret;
        if (amount == 0) return nullptr;
        if (amount == 1) {
            if (free_list.next_index != 0) {
                return free_list.data[--free_list.next_index];
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

    void free(type* obj) {
        free_list.append(obj);
    }

    void reset() {
        latest_bucket = 0;
        next_index_in_latest_bucket = 0;
        next_bucket_index = 0;
    }
};
