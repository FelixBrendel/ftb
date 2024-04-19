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
#include "core.hpp"

template <typename type>
struct Bucket_List {
    u32 next_index_in_latest_bucket;
    u32 next_bucket_index;
    u32 bucket_count;
    u32 bucket_size;

    Allocator_Base* allocator;
    type** buckets;

    void init(u32 bucket_size = 16, u32 initial_bucket_count = 8, Allocator_Base* back_allocator = nullptr) {
        if (!back_allocator)
            back_allocator = grab_current_allocator();

        allocator = back_allocator;

        this->bucket_size  = bucket_size;
        bucket_count       = initial_bucket_count;
        next_index_in_latest_bucket = 0;
        next_bucket_index           = 0;

        // TODO(Felix): it is important to allocate_0 the buckets itself, since
        //   after adding and deleting this is how we know if we need to
        //   allocate new buckets when we filled the last one, of if we can use
        //   the on still there from before
        buckets    = allocator->allocate_0<type*>(bucket_count);
        buckets[0] = allocator->allocate<type>(bucket_size);
    }

    void deinit() {
        // NOTE(Felix): We have to check all the buckets not only the ones that
        //   are currently in use, because we might have removed enough elements
        //   from the list to be back to older buckets, but still the newer
        //   buckets are held allocated.
        for (u32 i = 0; i < bucket_count; ++i) {
            if (buckets[i])
                allocator->deallocate(buckets[i]);
        }
        allocator->deallocate(buckets);
    }

    void clear() {
        next_index_in_latest_bucket = 0;
        next_bucket_index           = 0;
    }

    void expand() {
        buckets = allocator->resize<type*>(buckets, bucket_count*2);
        memset(buckets+bucket_count, 0, bucket_count*sizeof(buckets[0]));
        bucket_count *= 2;
    }

    void jump_to_next_bucket() {
        next_index_in_latest_bucket = 0;
        ++next_bucket_index;
        if (next_bucket_index >= bucket_count) {
            expand();
        }
        if (!buckets[next_bucket_index])
            buckets[next_bucket_index] = allocator->allocate<type>(bucket_size);
    }

    void increment_pointers(s32 amount = 1) {
        next_index_in_latest_bucket += amount;
        if (next_index_in_latest_bucket >= bucket_size) {
            jump_to_next_bucket();
        }
    }

    u32 count_elements() {
        return
            next_bucket_index * bucket_size // this many full buckets
            + next_index_in_latest_bucket;  // plus the last partially full bucket
    }

    template <typename lambda>
    void for_each(lambda p) {
        // full buckets
        for (u32 i = 0; i < next_bucket_index; ++i) {
            for (u32 j = 0; j < bucket_size; ++j) {
                p(buckets[i]+j);
            }
        }
        // last bucket
        for (u32 j = 0; j < next_index_in_latest_bucket; ++j) {
            p(buckets[next_bucket_index]+j);
        }
    }

    type* allocate_next() {
        type* ret = buckets[next_bucket_index]+next_index_in_latest_bucket;
        increment_pointers(1);
        return ret;
    }


    void append(type element) {
        type* ret = allocate_next();
        *ret = element;
    }

    void extend(std::initializer_list<type> l) {
        for (type e : l) {
            append(e);
        }
    }

    type& operator[] (u32 index) {
#ifdef FTB_INTERNAL_DEBUG
        u32 el_count = count_elements();
        panic_if(index >= el_count,
                 "Bucket list: accessing index (%u) that is not ain use (num elements: %u)\n",
                 index, el_count);
#endif

        u32 bucket_idx    = index / bucket_size;
        u32 idx_in_bucket = index % bucket_size;
        return buckets[bucket_idx][idx_in_bucket];
    }

    void remove_index(u32 index) {
        u32 el_count = count_elements();

#ifdef FTB_INTERNAL_DEBUG
        if (index >= el_count) {
            fprintf(stderr, "ERROR: removing index that is not in use\n");
        }
#endif

        type last = (*this)[el_count-1];

        (*this)[index] = last;

        if (next_index_in_latest_bucket == 0) {
            --next_bucket_index;
            next_index_in_latest_bucket = bucket_size-1;

        } else {
            --next_index_in_latest_bucket;
        }
    }

};

template <typename type>
struct Bucket_Queue {
    Bucket_List<type> bucket_list;
    u32 start_idx;

    void init(u32 bucket_size = 16, u32 initial_bucket_count = 8, Allocator_Base* allocator = nullptr) {
        bucket_list.init(bucket_size, initial_bucket_count, allocator);
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
            bucket_list.next_index_in_latest_bucket == 0 &&
            bucket_list.next_bucket_index == 0)
        {
            panic("Bucket Queue: queue has no next element\n");
        }
#endif

        type result = bucket_list[start_idx++];

        // maybe garbage collect left bucket
        if (start_idx >= bucket_list.bucket_size) {
            start_idx -= bucket_list.bucket_size;

            bucket_list.allocator->deallocate(bucket_list.buckets[0]);

            for (u32 i = 0; i < bucket_list.next_bucket_index; ++i) {
                bucket_list.buckets[i] = bucket_list.buckets[i+1];
            }
            // last bucket will be nullptr because we moved all one to the front
            bucket_list.buckets[bucket_list.next_bucket_index] = nullptr;

            --bucket_list.next_bucket_index;
        }


        // maybe reset if queue now empty
        if (start_idx == bucket_list.next_index_in_latest_bucket &&
            bucket_list.next_bucket_index == 0)
        {
            bucket_list.next_index_in_latest_bucket = 0;
            bucket_list.next_bucket_index = 0;
            start_idx = 0;
        }

        return result;
    }

    u32 count() {
        return bucket_list.count_elements() - start_idx;
    }

    bool is_empty() {
        return count() == 0;
    }
};


template <typename type>
struct Typed_Bucket_Allocator {
    Bucket_List<type> back_list;
    Array_List<type*> free_list;

    void init(u32 bucket_size = 16, u32 initial_bucket_count = 8, Allocator_Base* back_allocator = nullptr) {
        back_list.init(bucket_size, initial_bucket_count, back_allocator);
        free_list.init(32, back_allocator);
    }

    void deinit() {
        back_list.deinit();
        free_list.deinit();
    }

    void clear() {
        back_list.clear();
        free_list.clear();
    }

    u32 count_elements() {
        return back_list.count_elements() - free_list.count;
    }

    template <typename lambda>
    void for_each(lambda p) {

        auto voidp_cmp = [](type* const * a, type* const * b) -> s32 {
            return (s32)((byte*)*a - (byte*)*b);
        };

        free_list.sort(voidp_cmp);
        back_list.for_each([&](type* elem){
            if (free_list.sorted_find(elem, voidp_cmp) == -1) {
                p(elem);
            }
        });
    }

    type* allocate() {
        type* ret;

        if (free_list.count)
            ret = free_list.data[--free_list.count];
        else
            ret = back_list.allocate_next();

        return ret;
    }

    void deallocate(type* obj) {
        free_list.append(obj);
    }
};
