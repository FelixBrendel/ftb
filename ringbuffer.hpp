#pragma once
#include "core.hpp"

template <typename type>
struct Ringbuffer {
    u32             start_idx; // inclusive
    u32             end_idx;   // exclusive
    bool            did_overfow;
    bool            is_empty;

    type*           data;
    u64             length;
    Allocator_Base* allocator;

    void init(u64 length, Allocator_Base* allocator = nullptr) {
        if (!allocator)
            allocator = grab_current_allocator();
        this->allocator = allocator;
        this->length    = length;

        data = allocator->allocate<type>(this->length);
        reset();
    }

    void deinit() {
        if (allocator)
            allocator->deallocate(data);
    }

    void reset() {
        start_idx = 0;
        end_idx   = 0;
        did_overfow = false;
        is_empty    = true;
    }

    bool push(type elem, bool overwrite_if_full=true) {
        bool full = !is_empty && end_idx == start_idx;
        if (full && !overwrite_if_full)
            return true;

        data[end_idx] = elem;

        if (full) {
            // if full, move both pointers
            start_idx   = (start_idx + 1) % length;
            did_overfow = true;
            end_idx     = start_idx;
        } else {
            // otherwise only move end_pointer
            end_idx = (end_idx + 1) % length;
        }

        is_empty = false;
        return did_overfow;
    }

    u64 count() {
        if (is_empty)
            return 0;

        if (end_idx > start_idx) {
            return end_idx - start_idx;
        }

        return length-end_idx + start_idx;
    }

    template <typename lambda>
    void for_each(lambda p) {
        u64 num_elements = count();
        u32 idx = start_idx;
        for (u64 i = 0; i < num_elements; ++i) {
            p(data[idx]);
            ++idx;
            idx = (idx == length) ? 0 : idx;
        }
    }
};
