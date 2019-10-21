#pragma once
#include <stdlib.h>

template <typename type>
struct Array_List {
    type* data;
    int length;
    int next_index;

    Array_List(int initial_capacity = 16) {
        data = (type*)malloc(initial_capacity * sizeof(type));
        next_index = 0;
        length = initial_capacity;
    }

    type* begin() {
        return data;
    }

    type* end() {
        return data+(next_index);
    }

    void remove_index(int index) {
        data[index] = data[--next_index];
    }

    void append(type element) {
        if (next_index == length) {
            length *= 2;
            data = (type*)realloc(data, length * sizeof(type));
        }
        data[next_index] = element;
        next_index++;
    }

    type& operator[](int index) {
        return data[index];
    }

    void _merge(int start, int mid, int end) {
        int start2 = mid + 1;

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
                int index = start2;

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

    void sort(int left=-1, int right=-1) {
        if (left == -1) {
            sort(0, next_index - 1);
            return;
        } else if (left == right) {
            return;
        }

        int middle = left + (right-left) / 2;

        sort(left, middle);
        sort(middle+1, right);

        _merge(left, middle, right);
    }

    int sorted_find(type elem, int left=-1, int right=-1) {
        if (left == -1) {
            return sorted_find(elem, 0, next_index - 1);
        } else if (left == right) {
            if ((size_t)data[left] == (size_t)elem)
                return left;
            return -1;
        } else if (right < left)
            return -1;

        int middle = left + (right-left) / 2;

        if ((size_t)data[middle] < (size_t)elem)
            return sorted_find(elem, middle+1, right);
        if ((size_t)data[middle] > (size_t)elem)
            return sorted_find(elem, left, middle-1);
        return middle;
    }
};
