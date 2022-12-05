/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Felix Brendel
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

template <typename Type>
struct Pool_Allocator {
    union Pool_Cell {
        Type element;
        s32  relative_next_free;
        //  relative_next_free+1 is the actual free index (so we can have 0 as
        //  default which is easy to memset)
        //
        //  0 means next index is free,
        //  1 means the one after the next one is free
        // -2 means the previous index is free
        // -1 means there is no next free element (reference to self)

    };

    Pool_Cell*      data;
    Allocator_Base* allocator;
    u32             num_elements;
    s32             next_free_idx;

    void init(u32 max_elements, Allocator_Base* back_allocator = nullptr) {
        if (!back_allocator)
            back_allocator = grab_current_allocator();

        num_elements = max_elements;
        allocator = back_allocator;
        data      = allocator->allocate_0<Pool_Cell>(max_elements);

        if (!data)
            return;

        next_free_idx = 0;
        data[num_elements-1].relative_next_free = -1; // end of free list

    }

    void deinit() {
        if (data)
            allocator->deallocate(data);
    }

    void reset() {
        next_free_idx = 0;
        memset(data, 0, sizeof(Type) * num_elements);
        data[num_elements-1].relative_next_free = -1; // end of free list
    }

    Type* allocate() {
        if (next_free_idx == -1) {
            return nullptr;
        }

        Type* to_return = &data[next_free_idx].element;

        if (data[next_free_idx].relative_next_free == -1) {
            next_free_idx = -1;
        } else {
            next_free_idx =
                next_free_idx +
                data[next_free_idx].relative_next_free + 1;
        }

        return to_return;
    }

    void deallocate(Type* to_free) {
        Pool_Cell* cell = (Pool_Cell*)to_free;
        s32 my_idx      = cell - data;

        cell->relative_next_free = (next_free_idx - my_idx) - 1;
        next_free_idx            = my_idx;
    }

    u32 count_allocated_elements() {
        u32 count = num_elements;
        s32 iter_next_free_idx = next_free_idx;

        while (iter_next_free_idx != -1) {
            --count;
            s32 offset = data[iter_next_free_idx].relative_next_free + 1;

            if (offset)
                iter_next_free_idx += offset;
            else
                break;
        }
        return count;
    }
};

template <typename Type>
struct Growable_Pool_Allocator {
    union Pool_Cell {
        Pool_Cell* next_free;
        Type       element;
    };

    struct Chunk {
        Chunk*    next_chunk;
        Pool_Cell data[1]; // actual length will depend on `chunk_size'
    };

    Chunk*          next_chunk;
    Pool_Cell*      free_list;

    Allocator_Base* allocator;
    u32             chunk_size;

    void init(u32 elements_per_chunk = 16, Allocator_Base* back_allocator = nullptr) {
        *this = {0};

        if (!back_allocator)
            back_allocator = grab_current_allocator();

        allocator  = back_allocator;
        chunk_size = elements_per_chunk;
    }

    void deinit() {
        while (next_chunk) {
            Chunk* to_free = next_chunk;
            next_chunk = next_chunk->next_chunk;
            allocator->deallocate(to_free);
        }
    }

    void init_chunk(Chunk* chunk) {
        u32 i = 0;
        for (; i < chunk_size-1; ++i) {
            chunk->data[i].next_free = &chunk->data[i+1];
        }
        chunk->data[i].next_free = free_list;

        free_list = &chunk->data[0];
    }

    void reset() {
        free_list = nullptr;

        Chunk* iter = next_chunk;
        while (iter) {
            init_chunk(iter);
            iter = iter->next_chunk;
        }
    }

    u32 count_allocated_elements() {
        u32 count = 0;

        Chunk* iter = next_chunk;
        while (iter) {
            count += chunk_size;
            iter = iter->next_chunk;
        }

        Pool_Cell* free_list_iter = free_list;
        while (free_list_iter) {
            --count;
            free_list_iter = free_list_iter->next_free;
        }

        return count;
    }

    void allocate_new_chunk() {
        u64 size_to_alloc = offsetof(Chunk, data) + (sizeof(Pool_Cell) * chunk_size);
        Chunk* new_chunk = (Chunk*)allocator->allocate(size_to_alloc, alignof(Chunk));

        new_chunk->next_chunk = next_chunk;
        next_chunk = new_chunk;

        init_chunk(new_chunk);
    }

    Type* allocate() {
        if (!free_list) {
            allocate_new_chunk();
        }

        Type* to_return = &free_list->element;
        free_list = free_list->next_free;
        return to_return;
    }

    void deallocate(Type* to_free) {
        Pool_Cell* cell = (Pool_Cell*)to_free;
        cell->next_free = free_list;
        free_list = cell;
    }

};
