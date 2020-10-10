#pragma once
#include <stdlib.h>
#include <utility>
#include <initializer_list>
#include "types.hpp"


// template<class E> class my_initializer_list {
//     public:
//         using value_type      = E;
//         using reference       = const E&;
//         using const_reference = const E&;
//         using size_type       = size_t;

//         using iterator        = const E*;
//         using const_iterator  = const E*;

//         constexpr my_initializer_list() noexcept;

//         constexpr size_t size() const noexcept;     // number of elements
//         constexpr const E* begin() const noexcept;  // first element
//         constexpr const E* end() const noexcept;    // one past the last element
// };

// // initializer list range access
// template<class E> constexpr const E* begin(my_initializer_list<E> il) noexcept;
// template<class E> constexpr const E* end(my_initializer_list<E> il) noexcept;

template <typename type>
struct Array_List {
    type* data;
    u32 length;
    u32 next_index;

    Array_List(u32 initial_capacity = 16) {
        data = (type*)malloc(initial_capacity * sizeof(type));
        next_index = 0;
        length = initial_capacity;
    }

    Array_List(std::initializer_list<type> list)
        : Array_List(list.size())
    {
        for (type e : list) {
            append(e);
        }
    }

    Array_List(Array_List<type>&& other) {
        data       = other.data;
        length     = other.length;
        next_index = other.next_index;

        other.data = nullptr;
    }

    ~Array_List() {
        free(data);
        data = nullptr;
    }

    void clear() {
        next_index = 0;
    }

    Array_List<type> clone() {
        Array_List<type> ret;
        ret.length = length;
        ret.next_index = next_index;

        ret.data = (type*)malloc(length * sizeof(type));
        for (u32 i = 0; i < next_index; ++i) {
            ret.data[i] = data[i];
        }
        return ret;
    }

    type* begin() {
        return data;
    }

    type* end() {
        return data+(next_index);
    }

    void remove_index(u32 index) {
        data[index] = data[--next_index];
    }

    void append(type element) {
        if (next_index == length) {
            length *= 2;
            data = (type*)realloc(data, length * sizeof(type));
        }
        data[next_index] = std::move(element);
        next_index++;
    }

    void reserve(u32 count) {
        if (next_index+count >= (u32)length) {
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
            if (next_index == 0)
                return;
            sort(0, next_index - 1);
            return;
        } else if (left == right) {
            return;
        }

        u32 middle = left + (right-left) / 2;

        sort(left, middle);
        sort(middle+1, right);

        _merge(left, middle, right);
    }

    u32 sorted_find(type elem, s32 left=-1, s32 right=-1) {
        if (left == -1) {
            return sorted_find(elem, 0, next_index - 1);
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
