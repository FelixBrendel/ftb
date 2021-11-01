#pragma once
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
        data[index] = data[--count];
    }

    void append(type element) {
        if (count == length) {
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


    void _merge(u32 start, u32 mid, u32 end) {
        u32 start2 = mid + 1;

        /* If the direct merge is already sorted */
        if ((size_t)data[mid] <= (size_t)data[start2]) {
            return;
        }

        /* Two pointers to maintain start of both arrays to merge */
        while (start <= mid && start2 <= end) {
            if ((size_t)data[start] <= (size_t)data[start2]) {
                start++;
            }
            else {
                type value = data[start2];
                u32 index = start2;

                /* Shift all the elements between element 1; element 2, right by 1. */
                while (index != start) {
                    data[index] = data[index - 1];
                    index--;
                }
                data[start] = value;

                /* Update all the pointers */
                start++;
                mid++;
                start2++;
            }
        }
    }

    void sort(s32 left=-1, s32 right=-1) {
        if (left == -1) {
            if (count == 0)
                return;
            sort(0, count - 1);
            return;
        } else if (left == right) {
            return;
        }

        u32 middle = left + (right-left) / 2;

        sort(left, middle);
        sort(middle+1, right);

        _merge(left, middle, right);
    }

    s32 sorted_find(type elem, s32 left=-1, s32 right=-1) {
        if (left == -1) {
            return sorted_find(elem, 0, count - 1);
        } else if (left == right) {
            if ((size_t)data[left] == (size_t)elem)
                return left;
            return -1;
        } else if (right < left)
            return -1;

        u32 middle = left + (right-left) / 2;

        if ((size_t)data[middle] < (size_t)elem)
            return sorted_find(elem, middle+1, right);
        if ((size_t)data[middle] > (size_t)elem)
            return sorted_find(elem, left, middle-1);
        return middle;
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
struct Queue {
    Array_List<type> arr_list;
    u32 next_index;

    void init(u32 initial_capacity = 16) {
        next_index = 0;
        arr_list.init(initial_capacity);
    }

    void deinit() {
        arr_list.deinit();
    }

    void push_back(type e) {
        arr_list.append(e);
    }

    type get_next() {
#ifdef FTB_INTERNAL_DEBUG
        if (next_index >= arr_list.length) {
            fprintf(stderr, "ERROR: Out of bounds access in queue\n");
        }
#endif
        return arr_list.data[next_index++];
    }

    bool is_empty() {
        return next_index == arr_list.count;
    }

    int get_count() {
        return arr_list.count - next_index;
    }

    bool contains(type elem) {
        for (u32 i = next_index; i < arr_list.count; ++i) {
            if (arr_list[i] == elem)
                return true;
        }
        return false;
    }

    void clear() {
        next_index = 0;
        arr_list.clear();
    }
};
