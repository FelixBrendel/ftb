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
    fail    = 0,
    pass    = 1,
    skipped = 2
};

#define test_group(group_name)                                \
    MPP_BEFORE(test_group_1, {                                \
        print("+--- %s ", group_name);                        \
        for(size_t i = strlen(group_name); i < 70 + 4; ++i) { \
             raw_print("-");                                  \
        }                                                     \
        println("");                                          \
        push_print_prefix("|  ");                             \
    })                                                        \
    MPP_DEFER(test_group_2, {                                 \
        pop_print_prefix();                                   \
        raw_print("\n");                                      \
    })

#define print_assert_equal_fail(variable, value, type, format)          \
    one_statement({                                                     \
        raw_print("\n");                                                \
        with_print_prefix(console_red "  > " console_normal) {          \
            println("%s:%d: Assertion failed", __FILE__, __LINE__);     \
            println("  for '" #variable "'");                           \
            println("  expected: " format, (type)(value));              \
            println("  got:      " format, (type)(variable));           \
        }                                                               \
    })

#define print_assert_not_equal_fail(variable, value, type, format)      \
    one_statement({                                                     \
        raw_print("\n");                                                \
        with_print_prefix(console_red "  > " console_normal) {          \
            println("%s:%d: Assertion failed", __FILE__, __LINE__);     \
            println("  for '" #variable "'");                           \
            println("  expected not: " format, (type)(value));          \
            println("  got anyways:  " format, (type)(variable));       \
        }                                                               \
    })

#define assert_equal_string(variable, value)                            \
    do {                                                                \
        auto v1{variable};                                              \
        auto v2{value};                                                 \
        if (!(v1 == v2)) {                                              \
            print_assert_equal_fail(&(v1), &(v2), String*, "%{->Str}"); \
            return fail;                                                \
        }                                                               \
    } while (0)


#define assert_equal_int(variable, value)                               \
    if ((variable) != (value)) {                                        \
        print_assert_equal_fail((variable), (value), size_t, "%{u64}"); \
        return fail;                                                    \
    }

#define assert_not_equal_int(variable, value)                               \
    if ((variable) == (value)) {                                            \
        print_assert_not_equal_fail((variable), (value), size_t, "%{u64}"); \
        return fail;                                                        \
    }


#define assert_equal_f32(variable, value)                               \
    if (fabsl((f32)(variable) - (f32)(value)) > f32_epsilon) {          \
        print_assert_equal_fail((variable), (value), f32, "%{f32}");    \
        return fail;                                                    \
    }

#define assert_not_equal_f32(variable, value)                            \
    if (fabsl((f32)(variable) - (f32)(value)) <= f32_epsilon) {          \
        print_assert_not_equal_fail((variable), (value), f32, "%{f32}"); \
        return fail;                                                     \
    }

#define assert_equal_f64(variable, value)                               \
    if (fabsl((f64)(variable) - (f64)(value)) > f64_epsilon) {          \
        print_assert_equal_fail((variable), (value), f64, "%{f64}");    \
        return fail;                                                    \
    }

#define assert_not_equal_f64(variable, value)                            \
    if (fabsl((f64)(variable) - (f64)(value)) <= f64_epsilon) {          \
        print_assert_not_equal_fail((variable), (value), f64, "%{f64}"); \
        return fail;                                                     \
    }

#define assert_null(variable)                   \
    assert_equal_int((variable), nullptr)

#define assert_not_null(variable)               \
    assert_not_equal_int((variable), nullptr)

#define assert_true(value)                      \
    assert_equal_int((value), true)

#define invoke_test(name, ...)                                              \
    one_statement(                                                          \
        print("" #name ":");                                                \
        auto this_result = name(__VA_ARGS__);                               \
        if (this_result == fail) {                                          \
            ++num_tests_failed;                                             \
            print(console_red);                                             \
            for(s32 i = -1; i < 70; ++i)                                    \
                raw_print((i%3==1)? "." : " ", stdout);                     \
            raw_print(console_red "failed\n" console_normal, stdout);       \
        } else {                                                            \
            for(size_t i = strlen(#name); i < 70; ++i)                      \
                raw_print((i%3==1)? "." : " ", stdout);                     \
            if (this_result == pass) {                                      \
                ++num_tests_passed;                                         \
                raw_print(console_green "passed\n" console_normal, stdout); \
            } else                                                            \
                raw_print(console_blue "skipped\n" console_normal, stdout); \
        })

#define create_test_counters() s32 num_tests_passed = 0, num_tests_failed = 0
#define print_test_summary() \
    one_statement({                                                   \
        if (num_tests_failed == 0) {                                  \
            println(console_green "All %d tests passed!" console_normal, num_tests_passed);\
        } else {                                                      \
            println(console_red "Tests failed: %d/%d" console_normal, num_tests_failed, num_tests_passed+num_tests_failed);\
        }})
