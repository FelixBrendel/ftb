#include "arraylist.hpp"

template <typename type>
class Bucket_Allocator {
    unsigned int next_index_in_latest_bucket;
    unsigned int next_bucket_index;
    unsigned int bucket_count;
    unsigned int bucket_size;

    Array_List<type*> free_list;
    type** buckets;

    void expand() {
        // printf("realloc time\n");
        // realloc time
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

    void increment_pointers(int amount = 1) {
        next_index_in_latest_bucket += amount;
        if (next_index_in_latest_bucket >= bucket_size) {
            jump_to_next_bucket();
        }
    }

public:
    void alloc(unsigned int bucket_size, unsigned int initial_bucket_count) {
        this->free_list.alloc();
        this->bucket_size = bucket_size;
        next_index_in_latest_bucket = 0;
        next_bucket_index = 0;
        bucket_count = initial_bucket_count;

        buckets = (type**)malloc(bucket_count * sizeof(type*));
        buckets[0] = (type*)malloc(bucket_size * sizeof(type));
    }

    void dealloc() {
        for (unsigned int i = 0; i <= next_bucket_index; ++i) {
            free(buckets[i]);
        }
        this->free_list.dealloc();
        free(buckets);
    }

    template <typename proc>
    void for_each(proc p) {
        free_list.sort();
        type* val;
        for (unsigned int i = 0; i < next_bucket_index; ++i) {
            for (unsigned int j = 0; j < bucket_size; ++j) {
                val = buckets[i]+j;
                if (free_list.sorted_find(val) == -1)
                    p(val);
            }
        }
        for (unsigned int j = 0; j < next_index_in_latest_bucket; ++j) {
            val = buckets[next_bucket_index]+j;
            if (free_list.sorted_find(val) == -1)
                p(val);
        }
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

    void free_object(type* obj) {
        free_list.append(obj);
    }

};
