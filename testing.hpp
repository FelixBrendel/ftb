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

#include <math.h>
#include "./core.hpp"

enum testresult {
    fail = 0,
    pass = 1,
    skipped = 2
};

#define print_assert_equal_fail(variable, value, type, format)  \
    print("\n%s:%d: Assertion failed\n\tfor '" #variable "'"    \
          "\n\texpected: " format                               \
          "\n\tgot:      " format "\n",                         \
          __FILE__, __LINE__, (type)value, (type)variable)

#define print_assert_not_equal_fail(variable, value, type, format)      \
    print("\n%s:%d: Assertion failed\n\tfor '" #variable "'"            \
          "\n\texpected not: " format                                   \
          "\n\tgot anyways:  " format "\n",                             \
          __FILE__, __LINE__, (type)(value), (type)(variable))

#define assert_equal_string(variable, value)                              \
    do {                                                                  \
        auto v1{variable};                                                \
        auto v2{value};                                                   \
        if (!(v1 == v2)) {                                                       \
            print_assert_equal_fail(&(v1), &(v2), String*, "%{->Str}");   \
            return fail;                                                  \
        }                                                                 \
    } while (0)


#define assert_equal_int(variable, value)                               \
    if (variable != value) {                                            \
        print_assert_equal_fail(variable, value, size_t, "%{u64}");     \
        return fail;                                                    \
    }

#define assert_not_equal_int(variable, value)                           \
    if (variable == value) {                                            \
        print_assert_not_equal_fail(variable, value, size_t, "%{u64}"); \
        return fail;                                                    \
    }


#define assert_equal_f32(variable, value)                           \
    if (fabsl((f32)variable - (f32)value) > f32_epsilon) {          \
        print_assert_equal_fail(variable, value, f32, "%{f32}");    \
        return fail;                                                \
    }

#define assert_not_equal_f32(variable, value)                           \
    if (fabsl((f32)variable - (f32)value) <= f32_epsilon) {             \
        print_assert_not_equal_fail(variable, value, f32, "%{f32}");    \
        return fail;                                                    \
    }

#define assert_equal_f64(variable, value)                           \
    if (fabsl((f64)variable - (f64)value) > f64_epsilon) {          \
        print_assert_equal_fail(variable, value, f64, "%{f64}");    \
        return fail;                                                \
    }

#define assert_not_equal_f64(variable, value)                           \
    if (fabsl((f64)variable - (f64)value) <= f64_epsilon) {             \
        print_assert_not_equal_fail(variable, value, f64, "%{f64}");    \
        return fail;                                                    \
    }

#define assert_null(variable)                   \
    assert_equal_int((variable), nullptr)

#define assert_not_null(variable)               \
    assert_not_equal_int((variable), nullptr)

#define assert_true(value)                      \
    assert_equal_int((value), true)

#define invoke_test(name)                                               \
    one_statement(                                                      \
        fputs("" #name ":", stdout);                                    \
        fflush(stdout);                                                 \
        auto this_result = name();                                           \
        if (this_result == fail) {                                           \
            result = fail;                                              \
            for(s32 i = -1; i < 70; ++i)                                \
                fputs((i%3==1)? "." : " ", stdout);                     \
            fputs(console_red "failed\n" console_normal, stdout);       \
        } else {                                                        \
            for(size_t i = strlen(#name); i < 70; ++i)                  \
                fputs((i%3==1)? "." : " ", stdout);                     \
            if (this_result == pass)                                         \
                fputs(console_green "passed\n" console_normal, stdout); \
            else                                                        \
                fputs(console_blue "skipped\n" console_normal, stdout); \
        })
