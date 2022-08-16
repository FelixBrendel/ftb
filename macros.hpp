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

// The MPI_ and MPP_ macros are taken from Simon Tatham
// https://www.chiark.greenend.org.uk/~sgtatham/mp/mp.h
/*
 * mp.h is copyright 2012 Simon Tatham.
 *
 * Permission is hereby granted, free of charge, to any person
 * obtaining a copy of this software and associated documentation
 * files (the "Software"), to deal in the Software without
 * restriction, including without limitation the rights to use,
 * copy, modify, merge, publish, distribute, sublicense, and/or
 * sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following
 * conditions:
 *
 * The above copyright notice and this permission notice shall be
 * included in all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
 * OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT.  IN NO EVENT SHALL SIMON TATHAM BE LIABLE FOR
 * ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF
 * CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 *
 * $Id$
 */

#pragma once
#include "platform.hpp"
#include "stacktrace.hpp"

#define array_length(arr) (sizeof(arr) / sizeof(arr[0]))
#define zero_out(thing) memset(&(thing), 0, sizeof((thing)));

#ifdef FTB_WINDOWS
#else
#  include <signal.h> // for sigtrap
#endif

#define CONCAT(x, y) x ## y
#define LABEL(x, y) CONCAT(x, y)

#define MPI_LABEL(id1,id2)                              \
    LABEL(MPI_LABEL_ ## id1 ## _ ## id2 ## _, __LINE__)

#define MPP_DECLARE(labid, declaration)                 \
    if (0)                                              \
        ;                                               \
    else                                                \
        for (declaration;;)                             \
            if (1) {                                    \
                goto MPI_LABEL(labid, body);            \
              MPI_LABEL(labid, done): break;            \
            } else                                      \
                while (1)                               \
                    if (1)                              \
                        goto MPI_LABEL(labid, done);    \
                    else                                \
                        MPI_LABEL(labid, body):

#define MPP_BEFORE(labid,before)                \
    if (1) {                                    \
        before;                                 \
        goto MPI_LABEL(labid, body);            \
    } else                                      \
    MPI_LABEL(labid, body):

#define MPP_AFTER(labid,after)                  \
    if (1)                                      \
        goto MPI_LABEL(labid, body);            \
    else                                        \
        while (1)                               \
            if (1) {                            \
                after;                          \
                break;                          \
            } else                              \
                MPI_LABEL(labid, body):

#define MPP_DEFER(labid, defer_code)            \
    MPP_DECLARE(labid, defer { defer_code })

/**
 *   Defer   *
 */
#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } operator bool() const { return false; } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#  define DEFER_(LINE) zz_defer##LINE
#  define DEFER(LINE) DEFER_(LINE)
#  define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer


#define defer_free(var) defer { free(var); }

#define create_guarded_block(before_code, after_code)   \
    if ([&](){before_code; return false;}());           \
    else if (defer {after_code}); else


#if defined(unix) || defined(__unix__) || defined(__unix)
#  define NULL_HANDLE "/dev/null"
#else
#  define NULL_HANDLE "nul"
#endif

#define ignore_stdout                                               \
    MPP_DECLARE(1, FILE* LABEL(ignore_stdout_old_handle, __LINE__) = ftb_stdout) \
    MPP_BEFORE(2, ftb_stdout = fopen(NULL_HANDLE, "w");)            \
    MPP_DEFER(3, {                                                  \
            fclose(ftb_stdout);                                     \
            ftb_stdout=LABEL(ignore_stdout_old_handle, __LINE__);   \
        })

#define fluid_let(var, val)                                             \
    MPP_DECLARE(1, auto LABEL(fluid_let_, __LINE__) = var)              \
    MPP_BEFORE(2, var = val;)                                           \
    MPP_DEFER(3, var = LABEL(fluid_let_, __LINE__);)


/*
 * NOTE(Felix): About logs, asserts and panics
 *
 * 1. All macros that take a message, use the ftb printer with all its printing
 *    functionality.
 *
 * 2. All macros which contain the word 'debug' are only active in debug builds
 *    (FTB_DEBUG defined). They compile away in other builds.
 *
 * 3. debug_break() triggers a debug breakpoint.
 *
 * 4. log macros just print things out.
 *   - log_debug [only active in debug builds]
 *   - log_info
 *   - log_warning
 *   - log_error
 *   - log_trace: prints the current function with file and line
 *
 * 5. panic macros let the program crash with a given error message and print
 *    the stack trace (if available).
 *    - panic(format, ...)
 *    - debug_panic(format, ...) [only active in debug builds]
 *
 * 6. 'panic_if' macros are a shorthand to panic if a condition is met. An error
 *    description has to be supplied. It will call panic internally.
 *    - panic_if(cond, format, ...)
 *    - debug_panic_if(cond, format, ...) [only active in debug builds]
 *
 * 7. Similar to panic_if there are more traditional 'assert' macros. The also
 *    call panic internally, but no error message has to be provided.
 *    - assert(cond)
 *    - debug_assert(cond)
 */

#define print_source_code_location(file)                                \
    ::print_to_file(file,                                               \
                    "  in func: %{color<}%{->char}%{>color}\n"          \
                    "  in file: %{color<}%{->char}%{>color}\n"          \
                    "  on line: %{color<}%{u32}%{>color}\n",            \
                    console_cyan, __func__,                             \
                    console_cyan, __FILE__,                             \
                    console_cyan, __LINE__);

#define panic(...)                                              \
    do {                                                        \
        ::print_to_file(stderr, "%{color<}[ PANIC ]%{>color}",  \
                        console_red);                           \
        print_source_code_location(stderr);                     \
        ::print_to_file(stderr, "%{color<}", console_red);      \
        ::print_to_file(stderr, __VA_ARGS__);                   \
        ::print_to_file(stderr, "%{>color}\n");                 \
        print_stacktrace(stderr);                               \
        debug_break();                                          \
        exit(1);                                                \
    } while(0)


#define panic_if(cond, ...)                     \
    if(!(cond));                                \
    else panic(__VA_ARGS__)

#ifdef FTB_DEBUG
#  define debug_panic(...)    panic(__VA_ARGS__)
#  define debug_panic_if(cond, ...) panic_if(cond, __VA_ARGS__)
#else
#  define debug_panic(...)
#  define debug_panic_if(...)
#endif

#define one_statement(statements) do {statements} while(0)

#ifdef FTB_DEBUG
#  define log_debug(...)   one_statement(print("%{color<}[ DEBUG ] ", console_cyan_dim); print(__VA_ARGS__); println("%{>color}");)
#else
#  define log_debug(...)
#endif
#define log_info(...)    one_statement(::print("%{color<}[  INFO ] ", console_green);    ::print(__VA_ARGS__); ::println("%{>color}");)
#define log_warning(...) one_statement(::print("%{color<}[WARNING] ", console_yellow);   ::print(__VA_ARGS__); ::println("%{>color}");)
#define log_error(...)   one_statement(::print("%{color<}[ ERROR ] ", console_red_bold); ::print(__VA_ARGS__); ::println("%{>color}");)
#define log_trace()      ::println("%{color<}[ TRACE ] %s (%s:%d)%{>color}", console_cyan_dim ,__func__, __FILE__, __LINE__)

#define log_error_and_stacktrace(...)                                   \
    one_statement(::print_to_file(stderr, "%{color<}[ ERROR ] ", console_red_bold); \
                  ::print_to_file(stderr,__VA_ARGS__);                  \
                  ::print_to_file(stderr, "\n");                         \
                  print_source_code_location(stderr)                    \
                  ::print_to_file(stderr, "%{>color}\n");               \
                  print_stacktrace(stderr);)

#ifdef assert
#  undef assert
#endif
#define assert(cond)         if(cond);else panic("Assertion error: (%s)", #cond)

#ifdef FTB_DEBUG
#  define debug_assert(cond) if(cond);else panic("Debug assertion error")
#else
#  define debug_assert(...)
#endif

#ifdef FTB_DEBUG
#  ifdef FTB_WINDOWS
#    pragma message("defining debug_break")
#    define debug_break() __debugbreak()
#  else
#    define debug_break() raise(SIGTRAP)
#  endif
#else
#  define debug_break()
#endif
