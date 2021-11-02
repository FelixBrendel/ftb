#pragma once

/* Copyright (C) 2011 by Valentin Ochs
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to
 * deal in the Software without restriction, including without limitation the
 * rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
 * IN THE SOFTWARE.
 */

/* Minor changes by Rich Felker for integration in musl, 2011-04-27. */

/* Smoothsort, an adaptive variant of Heapsort.  Memory usage: O(1).
   Run time: Worst case O(n log n), close to O(n) in the mostly-sorted case. */

#define _BSD_SOURCE
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include <intrin.h>
#include <ammintrin.h>
#include <immintrin.h>

#include "types.hpp"

typedef int (*cmpfun_r)(const void *, const void *, void *);
typedef int (*cmpfun)(const void *, const void *);

// struct Array_Description {
//     void* base;
//     u64   element_size;
// };

#define FTB_SOA_SORT_IMPL
#ifndef FTB_SOA_SORT_IMPL

// void soa_sort_r(Array_Description* arrays, u32 array_count, u32 array_element_count, cmpfun_r cmp, void* arg);
// void soa_sort(Array_Description* arrays, u32 array_count, u32 array_element_count, cmpfun cmp);

#else // implementations

auto ntz(u64 num) -> u64 {
    return _tzcnt_u64(num);
}


static inline int pntz(size_t p[2]) {
    int r = ntz(p[0] - 1);
    if(r != 0 || (r = 8*sizeof(size_t) + ntz(p[1])) != 8*sizeof(size_t)) {
        return r;
    }
    return 0;
}

inline auto get_element_idx(void* base, void* element, u64 width) -> u32 {
    return ((byte*)element - (byte*)base) / width;
}

static void cycle(byte* ar[], int n, void *base1, size_t width1, void* base2, size_t width2) {
    byte* tmp = (byte*)alloca(width1 > width2 ? width1 : width2); // max of all widths

    int i;

    if(n < 2) {
        return;
    }

    ar[n] = tmp;

    // second array
    {

        memcpy(ar[n],
               ((byte*)base2)+(get_element_idx(base1, ar[0], width1) * width2), width2);
        for(i = 0; i < n-1; i++) {
            memcpy(((byte*)base2)+(get_element_idx(base1, ar[i], width1) * width2),
                   ((byte*)base2)+(get_element_idx(base1, ar[i+1], width1) * width2), width2);
        }
        memcpy(((byte*)base2)+(get_element_idx(base1, ar[n-1], width1) * width2),
               ar[n], width2);
    }

    // original array
    {
        memcpy(ar[n], ar[0], width1);
        for(i = 0; i < n; i++) {
            memcpy(ar[i], ar[i + 1], width1);
            ar[i] += width1;
        }
    }
}

/* shl() and shr() need n > 0 */
static inline void shl(size_t p[2], int n)
{
    if(n >= 8 * sizeof(size_t)) {
        n -= 8 * sizeof(size_t);
        p[1] = p[0];
        p[0] = 0;
    }
    p[1] <<= n;
    p[1] |= p[0] >> (sizeof(size_t) * 8 - n);
    p[0] <<= n;
}

static inline void shr(size_t p[2], int n)
{
    if(n >= 8 * sizeof(size_t)) {
        n -= 8 * sizeof(size_t);
        p[0] = p[1];
        p[1] = 0;
    }
    p[0] >>= n;
    p[0] |= p[1] << (sizeof(size_t) * 8 - n);
    p[1] >>= n;
}

static void sift(byte *head, size_t width, cmpfun_r cmp, void *arg, int pshift, size_t lp[], void* base1, void *base2, size_t width2) {
    byte *rt, *lf;
    byte *ar[14 * sizeof(size_t) + 1];
    int i = 1;

    ar[0] = head;
    while(pshift > 1) {
        rt = head - width;
        lf = head - width - lp[pshift - 2];

        if(cmp(ar[0], lf, arg) >= 0 && cmp(ar[0], rt, arg) >= 0) {
            break;
        }
        if(cmp(lf, rt, arg) >= 0) {
            ar[i++] = lf;
            head = lf;
            pshift -= 1;
        } else {
            ar[i++] = rt;
            head = rt;
            pshift -= 2;
        }
    }
    cycle(ar, i, base1, width, base2, width2);
}

static void trinkle(byte *head, size_t width, cmpfun_r cmp, void *arg, size_t pp[2], int pshift, int trusty, size_t lp[], void* base1, void *base2, size_t width2) {
    byte *stepson,
        *rt, *lf;
    size_t p[2];
    byte *ar[14 * sizeof(size_t) + 1];
    int i = 1;
    int trail;

    p[0] = pp[0];
    p[1] = pp[1];

    ar[0] = head;
    while(p[0] != 1 || p[1] != 0) {
        stepson = head - lp[pshift];
        if(cmp(stepson, ar[0], arg) <= 0) {
            break;
        }
        if(!trusty && pshift > 1) {
            rt = head - width;
            lf = head - width - lp[pshift - 2];
            if(cmp(rt, stepson, arg) >= 0 || cmp(lf, stepson, arg) >= 0) {
                break;
            }
        }

        ar[i++] = stepson;
        head = stepson;
        trail = pntz(p);
        shr(p, trail);
        pshift += trail;
        trusty = 0;
    }
    if(!trusty) {
        cycle(ar, i, base1, width, base2, width2);
        sift(head, width, cmp, arg, pshift, lp, base1, base2, width2);
    }
}

void soa_sort_r(void *base1, size_t width1, void *base2, size_t width2, size_t nel, cmpfun_r cmp, void *arg) {
    size_t lp[12*sizeof(size_t)];
    size_t i, size = width1 * nel;
    byte *head, *high;
    size_t p[2] = {1, 0};
    int pshift = 1;
    int trail;

    if (!size) return;

    head = (byte*)base1;
    high = head + size - width1;

    /* Precompute Leonardo numbers, scaled by element width */
    for(lp[0]=lp[1]=width1, i=2; (lp[i]=lp[i-2]+lp[i-1]+width1) < size; i++);

    while(head < high) {
        if((p[0] & 3) == 3) {
            sift(head, width1, cmp, arg, pshift, lp, base1, base2, width2);
            shr(p, 2);
            pshift += 2;
        } else {
            if(lp[pshift - 1] >= high - head) {
                trinkle(head, width1, cmp, arg, p, pshift, 0, lp, base1, base2, width2);
            } else {
                sift(head, width1, cmp, arg, pshift, lp, base1, base2, width2);
            }

            if(pshift == 1) {
                shl(p, 1);
                pshift = 0;
            } else {
                shl(p, pshift - 1);
                pshift = 1;
            }
        }

        p[0] |= 1;
        head += width1;
    }

    trinkle(head, width1, cmp, arg, p, pshift, 0, lp, base1, base2, width2);

    while(pshift != 1 || p[0] != 1 || p[1] != 0) {
        if(pshift <= 1) {
            trail = pntz(p);
            shr(p, trail);
            pshift += trail;
        } else {
            shl(p, 2);
            pshift -= 2;
            p[0] ^= 7;
            shr(p, 1);
            trinkle(head - lp[pshift] - width1, width1, cmp, arg, p, pshift + 1, 1, lp, base1, base2, width2);
            shl(p, 1);
            p[0] |= 1;
            trinkle(head - width1, width1, cmp, arg, p, pshift, 1, lp, base1, base2, width2);
        }
        head -= width1;
    }
}

static int wrapper_cmp(const void *v1, const void *v2, void *cmp) {
    return ((cmpfun)cmp)(v1, v2);
}

void soa_sort(void *base1, size_t width1, void *base2, size_t width2, size_t nel, cmpfun cmp) {
    soa_sort_r(base1, width1, base2, width2, nel, wrapper_cmp, cmp);
}

#endif // FTB_SOA_SORT_IMPL
