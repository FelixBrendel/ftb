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

#include "platform.hpp"
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>

#define f64_epsilon 2.2204460492503131e-16
#define f32_epsilon 1.19209290e-7f

typedef int8_t   s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef uint8_t byte;

typedef float       f32;
typedef double      f64;

#ifdef UNICODE
typedef wchar_t path_char;
#else
typedef char    path_char;
#endif

struct String_Slice;
struct String;

inline auto heap_copy_limited_c_string(const char* str, u64 length) -> char* {
    char* result = (char*)malloc(length+1);

    if (!result)
        return nullptr;

    memcpy(result, str, length);
    result[length] = '\0';

    return result;
}

inline auto heap_copy_c_string(const char* str) -> char* {
    u64 length = strlen(str);
    return heap_copy_limited_c_string(str, length);
}


auto inline string_equal(const char* input, const char* check) -> bool {
    return strcmp(input, check) == 0;
}


struct String {
    char* data;
    u64 length;

    operator bool() const {
        return data != nullptr;
    }

    bool operator==(String other) {
        return
            length == other.length &&
            strncmp(data, other.data, other.length) == 0;
    }

    s32 operator<(String other) {
        return strncmp(data,
                       other.data,
                       length < other.length ? length : other.length); // min length
    }

    static String from(const char* str) {
        String r;

        r.length = strlen(str);
        r.data  = (char*)malloc(r.length*sizeof(char)+1);
        strcpy(r.data, str);

        return r;
    }

    void free() {
        if (data) {
            ::free(data);
        }
#ifdef FTB_INTERNAL_DEBUG
        length = 0;
        data = nullptr;
#endif
    }
};

auto inline print_into_string(String string, const char* format, ...) -> bool {
    va_list args;
    va_start(args, format);

    s32 written = vsnprintf(string.data, string.length, format, args);

    va_end(args);

    return written >= 0 && written < string.length;
}
