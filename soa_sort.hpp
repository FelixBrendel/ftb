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

// NOTE(Felix): Above you can see the original copyright. I adjusted the source
//   a bit to be able to do the soa sorting. The result might not be the best an
//   I am also not sure if the O(1) memory usage still holds, as we use alloca
//   now in `cycle'. The resulting code is also not quite as optimized as it
//   could be, again in `cycle' for every additional array that is processed we
//   compute the same original indices of the main array over and over. So there
//   is potential to speed it up, but for caching the indices, we would need
//   another alloca?  --  02/Nov/2021


#include <vcruntime.h>
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

struct Array_Description {
    void* base;
    u64   width;
};

#ifndef FTB_SOA_SORT_IMPL

void soa_sort_r(Array_Description main, Array_Description* others, size_t other_count, size_t nel, cmpfun_r cmp, void *arg);
void soa_sort(Array_Description main, Array_Description* others, size_t other_count, size_t nel, cmpfun cmp);
void sort(Array_Description main, size_t nel, cmpfun cmp);

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

static void cycle(byte* ar[], int n, Array_Description main, Array_Description* others, size_t other_count) {
    size_t max_width = main.width;
    for (u32 i = 0; i < other_count; ++i) {
        max_width = max_width > others[i].width ? max_width: others[i].width;
    }


    int i;

    if(n < 2) {
        return;
    }

    byte* tmp = (byte*)alloca(max_width); // max of all widths
    ar[n] = tmp;

    // other arrays
    {
        for (int arr_idx = 0; arr_idx < other_count; ++arr_idx) {
            memcpy(ar[n],
                   ((byte*)others[arr_idx].base)+(get_element_idx(main.base, ar[0], main.width) * others[arr_idx].width), others[arr_idx].width);
            for(i = 0; i < n-1; i++) {
                memcpy(((byte*)others[arr_idx].base)+(get_element_idx(main.base, ar[i], main.width) * others[arr_idx].width),
                       ((byte*)others[arr_idx].base)+(get_element_idx(main.base, ar[i+1], main.width) * others[arr_idx].width), others[arr_idx].width);
            }
            memcpy(((byte*)others[arr_idx].base)+(get_element_idx(main.base, ar[n-1], main.width) * others[arr_idx].width),
                   ar[n], others[arr_idx].width);

        }

    }

    // original array
    {
        memcpy(ar[n], ar[0], main.width);
        for(i = 0; i < n; i++) {
            memcpy(ar[i], ar[i + 1], main.width);
            ar[i] += main.width;
        }
    }
}

/* shl() and shr() need n > 0 */
static inline void shl(size_t p[2], int n) {
    if(n >= 8 * sizeof(size_t)) {
        n -= 8 * sizeof(size_t);
        p[1] = p[0];
        p[0] = 0;
    }
    p[1] <<= n;
    p[1] |= p[0] >> (sizeof(size_t) * 8 - n);
    p[0] <<= n;
}

static inline void shr(size_t p[2], int n) {
    if(n >= 8 * sizeof(size_t)) {
        n -= 8 * sizeof(size_t);
        p[0] = p[1];
        p[1] = 0;
    }
    p[0] >>= n;
    p[0] |= p[1] << (sizeof(size_t) * 8 - n);
    p[1] >>= n;
}

static void sift(byte *head, cmpfun_r cmp, void *arg, int pshift, size_t lp[], Array_Description main, Array_Description* others, size_t other_count) {
    byte *rt, *lf;
    byte *ar[14 * sizeof(size_t) + 1];
    int i = 1;

    ar[0] = head;
    while(pshift > 1) {
        rt = head - main.width;
        lf = head - main.width - lp[pshift - 2];

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
    cycle(ar, i, main, others, other_count);
}

static void trinkle(byte *head, cmpfun_r cmp, void *arg, size_t pp[2], int pshift, int trusty, size_t lp[], Array_Description main, Array_Description* others, size_t other_count) {
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
            rt = head - main.width;
            lf = head - main.width - lp[pshift - 2];
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
        cycle(ar, i, main, others, other_count);
        sift(head, cmp, arg, pshift, lp, main, others, other_count);
    }
}

void soa_sort_r(Array_Description main, Array_Description* others, size_t other_count, size_t nel, cmpfun_r cmp, void *arg) {
    size_t lp[12*sizeof(size_t)];
    size_t i, size = main.width * nel;
    byte *head, *high;
    size_t p[2] = {1, 0};
    int pshift = 1;
    int trail;

    if (!size) return;

    head = (byte*)main.base;
    high = head + size - main.width;

    /* Precompute Leonardo numbers, scaled by element width */
    for(lp[0]=lp[1]=main.width, i=2; (lp[i]=lp[i-2]+lp[i-1]+main.width) < size; i++);

    while(head < high) {
        if((p[0] & 3) == 3) {
            sift(head, cmp, arg, pshift, lp, main, others, other_count);
            shr(p, 2);
            pshift += 2;
        } else {
            if(lp[pshift - 1] >= high - head) {
                trinkle(head, cmp, arg, p, pshift, 0, lp, main, others, other_count);
            } else {
                sift(head, cmp, arg, pshift, lp, main, others, other_count);
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
        head += main.width;
    }

    trinkle(head, cmp, arg, p, pshift, 0, lp, main, others, other_count);

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
            trinkle(head - lp[pshift] - main.width, cmp, arg, p, pshift + 1, 1, lp, main, others, other_count);
            shl(p, 1);
            p[0] |= 1;
            trinkle(head - main.width, cmp, arg, p, pshift, 1, lp, main, others, other_count);
        }
        head -= main.width;
    }
}

static int wrapper_cmp(const void *v1, const void *v2, void *cmp) {
    return ((cmpfun)cmp)(v1, v2);
}

void soa_sort(Array_Description main, Array_Description* others, size_t other_count, size_t nel, cmpfun cmp) {
    soa_sort_r(main, others, other_count, nel, wrapper_cmp, (void*)cmp);
}

void sort(Array_Description main, size_t nel, cmpfun cmp) {
    soa_sort(main, nullptr, 0, nel, cmp);
}


#endif // FTB_SOA_SORT_IMPL
