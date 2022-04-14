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
#include <cstdlib>
#include <stdlib.h>
#include <initializer_list>

#ifdef FTB_INTERNAL_DEBUG
#  include <stdio.h>
#endif

#include "types.hpp"
#include "allocation_stats.hpp"
#include "macros.hpp"

// NOTE(Felix): This is a macro, because we call alloca, which is stack-frame
//   sensitive. So we really have to avoid calling alloca in another function
//   (or constructor), with the macro the alloca is called in the callers
//   stack-frame
#define create_stack_array_list(type, length)     \
    Stack_Array_List<type> { (type*)alloca(length * sizeof(type)), length, 0 }


static s32 int_cmp (const s32* a, const s32* b) {
    return *a - *b;
}

static s32 voidp_cmp (const void** a, const void** b) {
    return (s32)((byte*)*a - (byte*)*b);
}


template <typename type>
struct Stack_Array_List {
    type* data;
    u32 length;
    u32 count;

    static Stack_Array_List<type> create_from(std::initializer_list<type> l) {
        Stack_Array_List<type> ret(l.size());

        for (type t : l) {
            ret.data[ret.count++] = t;
        }
        return ret;
    }

    void extend(std::initializer_list<type> l) {
        for (type e : l) {
            append(e);
        }
    }

    void clear() {
        count = 0;
    }

    type* begin() {
        return data;
    }

    type* end() {
        return data+(count);
    }

    void remove_index(u32 index) {
#ifdef FTB_INTERNAL_DEBUG
        if (index >= count)
            fprintf(stderr, "ERROR: removing index that is not in use\n");
#endif
        data[index] = data[--count];
    }

    void append(type element) {
#ifdef FTB_INTERNAL_DEBUG
        if (count == length) {
            fprintf(stderr, "ERROR: Stack_Array_List is full!\n");
        }
#endif
        data[count] = element;
        count++;
    }

    type& operator[](u32 index) {
        return data[index];
    }

};

struct Untyped_Array_List {
    u32   element_size;
    void* data;
    u32   length;
    u32   count;

    void init(u32 e_size, u32 initial_capacity = 16) {
        element_size = e_size;
        data   = malloc(initial_capacity * element_size);
        count  = 0;
        length = initial_capacity;
    }

    void deinit() {
        free(data);
        data = nullptr;
    }

    void clear() {
        count = 0;
    }

    void append(void* elem) {
        if (count == length) {
            length *= 2;
            data = realloc(data, length * element_size);
        }

        memcpy(((u8*)(data))+(element_size*count), elem, element_size);
        count++;
    }

    void* reserve_next_slot() {
        if (count == length) {
            length *= 2;
            data = realloc(data, length * element_size);
        }
        count++;
        return ((u8*)(data)) + (element_size*(count-1));
    }



    void* get_at(u32 idx) {
        return ((u8*)(data)) + (element_size*idx);
    }

};

template <typename type>
struct Array_List {
    type* data;
    u32 length;
    u32 count;

    void init(u32 initial_capacity = 16) {
        data = (type*)malloc(initial_capacity * sizeof(type));
        count = 0;
        length = initial_capacity;
    }

    static Array_List<type> create_from(std::initializer_list<type> l) {
        Array_List<type> ret;
        ret.init_from(l);
        return ret;
    }

    void init_from(std::initializer_list<type> l) {
        length = l.size() > 1 ? l.size() : 1; // alloc at least one

        data = (type*)malloc(length * sizeof(type));
        count = 0;
        // TODO(Felix): Use memcpy here
        for (type t : l) {
            data[count++] = t;
        }
    }

    void extend(std::initializer_list<type> l) {
        reserve((u32)l.size());
        // TODO(Felix): Use memcpy here
        for (type e : l) {
            append(e);
        }
    }

    void deinit() {
        free(data);
        data = nullptr;
    }

    void clear() {
        count = 0;
    }

    bool contains_linear_search(type elem) {
        for (u32 i = 0; i < count; ++i) {
            if (data[i] == elem)
                return true;
        }
        return false;
    }

    bool contains_binary_search(type elem) {
        return sorted_find(elem) != -1;
    }


    Array_List<type> clone() {
        Array_List<type> ret;
        ret.length = length;
        ret.count = count;

        ret.data = (type*)malloc(length * sizeof(type));
        // TODO(Felix): Maybe use memcpy here
        for (u32 i = 0; i < count; ++i) {
            ret.data[i] = data[i];
        }
        return ret;
    }

    void copy_values_from(Array_List<type> other) {
        // clear the array
        count = 0;
        // make sure we have allocated enough
        reserve(other.count);
        // copy stuff
        count = other.count;
        memcpy(data, other.data, sizeof(type) * other.count);
    }

    type* begin() {
        return data;
    }

    type* end() {
        return data+(count);
    }

    void remove_index(u32 index) {
#ifdef FTB_INTERNAL_DEBUG
        if (index >= count) {
            fprintf(stderr, "ERROR: removing index that is not in use\n");
        }
#endif
        data[index] = data[--count];
    }

    void append(type element) {
        if (count == length) {
#ifdef FTB_INTERNAL_DEBUG
            if (length == 0) {
                fprintf(stderr, "ERROR: Array_List was not initialized.\n");
                length = 8;
            }
#endif
            length *= 2;

            data = (type*)realloc(data, length * sizeof(type));
        }
        data[count] = element;
        count++;
    }

    void reserve(u32 amount) {
        if (count+amount >= (u32)length) {
            length *= 2;
            data = (type*)realloc(data, length * sizeof(type));
        }
    }

    type& operator[](u32 index) {
        return data[index];
    }


    type& last_element() {
#ifdef FTB_INTERNAL_DEBUG
        if (count == 0) {
            fprintf(stderr, "ERROR: Array_List was empty but last_element was called.\n");
        }
#endif
        return data[count-1];
    }

    typedef s32 (*compare_function_t)(const type*, const type*);
    typedef s32 (*void_compare_function_t)(const void*, const void*);
    typedef s32 (*pointer_compare_function_t)(const void**, const void**);
    void sort(compare_function_t comparer) {
        qsort(data, count, sizeof(type),
              (void_compare_function_t)comparer);
    }
    void sort(pointer_compare_function_t comparer) {
        qsort(data, count, sizeof(type),
              (void_compare_function_t)comparer);
    }

    s32 sorted_find(type elem,
                    compare_function_t compare_fun,
                    s32 left=-1, s32 right=-1)
    {
        if (left == -1) {
            return sorted_find(elem, compare_fun,
                               0, count - 1);
        } else if (left == right) {
            if (compare_fun(&elem, &data[left]) == 0)
                return left;
            return -1;
        } else if (right < left)
            return -1;

        u32 middle = left + (right-left) / 2;

        s32 compare = compare_fun(&elem, &data[middle]);

        if (compare > 0)
            return sorted_find(elem, compare_fun, middle+1, right);
        if (compare < 0)
            return sorted_find(elem, compare_fun, left, middle-1);
        return middle;
    }

    s32 sorted_find(type elem,
                    pointer_compare_function_t compare_fun,
                    s32 left=-1, s32 right=-1)
    {
        return sorted_find(elem, (compare_function_t)compare_fun,
                           left, right);
    }

    bool is_sorted(compare_function_t compare_fun) {
        for (s32 i = 1; i < count; ++i) {
            if (compare_fun(&data[i-1], &data[i]) > 0)
                return false;
        }
        return true;
    }

    bool is_sorted(pointer_compare_function_t compare_fun) {
        return is_sorted((compare_function_t)compare_fun);
    }
};


template <typename type>
struct Auto_Array_List : public Array_List<type> {
    Auto_Array_List(u32 length) {
        this->init(length);
    }

    Auto_Array_List() {
        this->init(16);
    }

    Auto_Array_List(std::initializer_list<type> l) {
        this->init((u32)l.size());
        for (type e : l) {
            this->append(e);
        }
    }

    ~Auto_Array_List() {
        free(this->data);
        this->data = nullptr;
    }
};


template <typename type>
struct Stack {
    Array_List<type> array_list;

    void init(u32 initial_capacity = 16) {
        array_list.init(initial_capacity);
    }

    void deinit() {
        array_list.deinit();
    }

    void push(type elem) {
        array_list.append(elem);
    }

    type pop() {
#ifdef FTB_INTERNAL_DEBUG
        if (array_list.count == 0) {
            fprintf(stderr, "ERROR: Stack was empty but pop was called.\n");
        }
#endif
        return array_list[--array_list.count];
    }

    type peek() {
#ifdef FTB_INTERNAL_DEBUG
        if (array_list.count == 0) {
            fprintf(stderr, "ERROR: Stack was empty but peek was called.\n");
        }
#endif
        return array_list[array_list.count-1];
    }

    void clear() {
        array_list.clear();
    }

    bool is_empty() {
        return array_list.count == 0;
    }

};

struct String_Builder {
    Array_List<const char*> list;

    void init(u32 initial_capacity = 16) {
        list.init(initial_capacity);
    }

    void init_from(std::initializer_list<const char*> l) {
        list.length = l.size() > 1 ? l.size() : 1; // alloc at least one

        list.data = (const char**)malloc(list.length * sizeof(char*));
        list.count = 0;
        // TODO(Felix): Use memcpy here
        for (const char* t : l) {
            list.data[list.count++] = t;
        }
    }

    static String_Builder create_from(std::initializer_list<const char*> l) {
        String_Builder sb;
        sb.init_from(l);
        return sb;
    }

    void clear() {
        list.clear();
    }

    void deinit() {
        list.deinit();
    }

    void append(const char* string) {
        list.append(string);
    }

    char* build() {
        u64 total_length = 0;

        u32* lengths = (u32*)alloca(sizeof(*lengths) * list.length);

        for (u32 i = 0; i < list.count; ++i) {
            u64 length = strlen(list[i]);
            lengths[i] = length;
            total_length += length;
        }

        char* concat = (char*)malloc(total_length+1);

        u64 cursor = 0;

        for (u32 i = 0; i < list.count; ++i) {
            strcpy(concat+cursor, list[i]);
            cursor += lengths[i];
        }

#  ifdef FTB_INTERNAL_DEBUG
        panic_if(cursor != total_length);
#  endif

        concat[cursor] = 0;

        return concat;
    };
};
