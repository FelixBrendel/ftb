/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2024, Felix Brendel
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

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>
#include <malloc.h>
#include <initializer_list>

// ----------------------------------------------------------------------------
//                                  platform
// ----------------------------------------------------------------------------
#if defined(_WIN32) || defined(_WIN64)
# define FTB_WINDOWS
#else
#define FTB_LINUX
#endif

#ifdef FTB_WINDOWS
#  include <Windows.h> // for colored prints
#else
#  include <signal.h> // for sigtrap
#endif

// ----------------------------------------------------------------------------
//                                   macros
// ----------------------------------------------------------------------------
#ifndef MIN
#  define MIN(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef MAX
#  define MAX(a, b) ((a)<(b)?(b):(a))
#endif

#define string_from_literal(lit) (String{.data=(char*)lit, .length=strlen(lit)})

#define array_length(arr)   (sizeof(arr) / sizeof(arr[0]))
#define zero_out(thing)     memset(&(thing), 0, sizeof((thing)));

#define FTB_CONCAT(x, y) x ## y
#define LABEL(x, y) FTB_CONCAT(x, y)

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

#ifndef defer
struct defer_dummy {};
template <class F> struct deferrer { F f; ~deferrer() { f(); } operator bool() const { return false; } };
template <class F> deferrer<F> operator*(defer_dummy, F f) { return {f}; }
#  define DEFER_(LINE) zz_defer##LINE
#  define DEFER(LINE) DEFER_(LINE)
#  define defer auto DEFER(__LINE__) = defer_dummy{} *[&]()
#endif // defer

/* TODO(Felix): remove, the before code has weird semantics because it can
     retrun a boolean. Better write cleanup code with straight defers
 */
// #define create_guarded_block(before_code, after_code)   \
    // if ([&](){before_code; return false;}());           \
    // else if (defer {after_code}); else


#if defined(unix) || defined(__unix__) || defined(__unix)
#  define NULL_HANDLE "/dev/null"
#else
#  define NULL_HANDLE "nul"
#endif

#define ignore_stdout                                                   \
    MPP_DECLARE(1, FILE* LABEL(ignore_stdout_old_handle, __LINE__) = ftb_stdout) \
    MPP_BEFORE(2, ftb_stdout = fopen(NULL_HANDLE, "w");)                \
    MPP_DEFER(3, {                                                      \
            fclose(ftb_stdout);                                         \
            ftb_stdout=LABEL(ignore_stdout_old_handle, __LINE__);       \
        })

#define fluid_let(var, val)                                             \
    MPP_DECLARE(1, auto LABEL(fluid_let_, __LINE__) = var)              \
    MPP_BEFORE(2, var = val;)                                           \
    MPP_DEFER(3, var = LABEL(fluid_let_, __LINE__);)


#define EXPECT_OR_RETURN_MSG(expr, ...)         \
    if((expr));                                 \
    else do {                                   \
            log_error(__VA_ARGS__);             \
            log_trace();                        \
            return {0};                         \
        } while(0)

#ifdef FTB_DEBUG
#  define EXPECT_OR_RETURN(expr)                \
    if((expr));                                 \
    else return {0}
#else
#  define EXPECT_OR_RETURN(expr)                        \
    if((expr));                                         \
    else do {                                           \
            log_debug("Failing because '%s' @ %s:%ld"); \
            log_trace();                                \
            return {0};                                 \
        } while(0)
#endif


/*
 * NOTE(Felix): About logs, asserts and panics
 *
 * 1. All macros that take a message, use the ftb printer with all its printing
 *    functionality.
 *
 * 2. All macros which contain the word 'debug' (log_debug or debug_assert)
 *    are only active in debug builds (FTB_DEBUG defined). They compile away
 *    in other builds.
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
 *    - debug_assert(cond) [only active in debug builds]
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
        ::print_to_file(stderr, "%{color<}[ PANIC ]%{>color} ", \
                        console_red);                           \
        ::print_to_file(stderr, "%{color<}", console_red);      \
        ::print_to_file(stderr, __VA_ARGS__);                   \
        ::print_to_file(stderr, "%{>color}\n");                 \
        print_stacktrace(stderr);                               \
        debug_break();                                          \
        exit(1);                                                \
    } while(0)


#define one_statement(statements) do {statements} while(0)

#define panic_if(cond, ...)                                             \
    do {                                                                \
        if(!(cond)){}                                                   \
        else {                                                          \
            ::print_to_file(                                            \
                stderr,                                                 \
                "%{color<}[ PANIC ] Error condition: %s%{>color}\n",    \
                console_red, #cond);                                    \
            panic(__VA_ARGS__);                                         \
        }                                                               \
    } while(0)

#ifdef FTB_DEBUG
#  define debug_panic(...)          panic(__VA_ARGS__)
#  define debug_panic_if(cond, ...) panic_if(cond, __VA_ARGS__)
#else
#  define debug_panic(...)
#  define debug_panic_if(...)
#endif


#ifdef FTB_DEBUG
#  define log_debug(...)   one_statement(::print("%{color<}[ DEBUG ] ", console_cyan_dim); ::raw_print(__VA_ARGS__); ::raw_println("%{>color}");)
#else
#  define log_debug(...)
#endif
#define log_info(...)    one_statement(::print("%{color<}[  INFO ] ", console_green);    ::raw_print(__VA_ARGS__); ::raw_println("%{>color}");)
#define log_warning(...) one_statement(::print("%{color<}[WARNING] ", console_yellow);   ::raw_print(__VA_ARGS__); ::raw_println("%{>color}");)
#define log_error(...)   one_statement(::print("%{color<}[ ERROR ] ", console_red_bold); ::raw_print(__VA_ARGS__); ::raw_println("%{>color}");)
#define log_trace()      ::println("%{color<}[ TRACE ] %s (%s:%d)%{>color}", console_cyan_dim ,__func__, __FILE__, __LINE__)

#define log_error_and_stacktrace(...)                                   \
    one_statement(::print_to_file(stderr, "%{color<}[ ERROR ] ", console_red_bold); \
                  ::print_to_file(stderr,__VA_ARGS__);                  \
                  ::print_to_file(stderr, "\n");                        \
                  print_source_code_location(stderr)                    \
                  ::print_to_file(stderr, "%{>color}\n");               \
                  print_stacktrace(stderr);)

#ifdef assert
#  undef assert
#endif
#define assert(cond)         if(cond);else panic("Assertion error: (%s)", #cond)

#ifdef FTB_DEBUG
#  define debug_assert(cond) if(cond);else panic("Debug assertion error: (%s)", #cond)
#else
#  define debug_assert(...)
#endif

#ifdef FTB_DEBUG
#  ifdef FTB_WINDOWS
#    define debug_break() __debugbreak()
#  else
#    define debug_break() raise(SIGTRAP)
#  endif
#else
#  define debug_break()
#endif


// ----------------------------------------------------------------------------
//                               types
// ----------------------------------------------------------------------------
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

#define f64_epsilon 2.2204460492503131e-16
#define f32_epsilon 1.19209290e-7f

// ----------------------------------------------------------------------------
//                              utf-8 things
// ----------------------------------------------------------------------------

struct Unicode_Code_Point {
    u32 byte_length;  // [1 - 4]
    u32 code_point;   // 21-bit values
};

Unicode_Code_Point get_code_point(u32 cp);
Unicode_Code_Point bytes_to_code_point(const byte*);

// NOTE(Felix): Assumes that the out_str has enough bytes allocated to be able
//   to hold Unicode_Code_Point::byte_length many bytes, returns the number of
//   bytes written.
u32  code_point_to_bytes(Unicode_Code_Point cp, char* out_string);
u32  code_point_to_bytes(u32 cp, char* out_string);
u32  get_byte_length_for_code_point(u32 cp);
u32  count_chars_utf8(const char* str);
bool strncpy_0(char* dest, const char* src, u64 dest_size);
bool strncpy_utf8_0(char* dest, const char* src, u64 dest_size);

// ----------------------------------------------------------------------------
//                              base alloctors
// ----------------------------------------------------------------------------
void* allocate(u64 size_in_bytes, u32 alignment);
void* allocate_0(u64 size_in_bytes, u32 alignment);
void* resize(void* old, u64 size_in_bytes, u32 alignment);
void  deallocate(void* old);

template <typename Type>
inline Type* allocate(u64 amount) {
    return (Type*)allocate(amount * sizeof(Type), alignof(Type));
}
template <typename Type>
inline Type* allocate_0(u64 amount) {
    return (Type*)allocate_0(amount * sizeof(Type), alignof(Type));
}
template <typename Type>
inline Type* resize(void* old, u64 amount) {
    return (Type*)resize(old, amount * sizeof(Type), alignof(Type));
}


#define ALLOCATOR_TYPES_ENUM_START 0x80
#define ALLOCATORS                              \
    /* actual allocators */                     \
    ALLOCATOR(LibC_Allocator)                   \
    ALLOCATOR(Linear_Allocator)                 \
    /* ALLOCATOR(Pool_Allocator)    */          \
    /* ALLOCATOR(Bucket_Allocator)  */          \
                                                \
    /* allocator overlays */                    \
    ALLOCATOR(Printing_Allocator)               \
    ALLOCATOR(Panicking_Allocator)              \
    ALLOCATOR(Bookkeeping_Allocator)            \
    ALLOCATOR(Resettable_Allocator)             \
    ALLOCATOR(Leak_Detecting_Allocator)         \

const int _initial_counter_ = __COUNTER__;
enum struct Allocator_Type : byte {
#define ALLOCATOR(name)  name = ALLOCATOR_TYPES_ENUM_START + __COUNTER__ - _initial_counter_ - 1,
    ALLOCATORS
#undef ALLOCATOR
    Allocator_Type_Max_Enum
};

struct Allocator_Base {
    Allocator_Type type;
    Allocator_Base* next_allocator;

    void* allocate(u64 size_in_bytes, u32 alignment);
    void* allocate_0(u64 size_in_bytes, u32 alignment);
    void* resize(void* old, u64 size_in_bytes, u32 alignment);
    void  deallocate(void* old);

    template <typename Type>
    Type* allocate(u64 amount = 1){
        return (Type*)allocate(amount * sizeof(Type), alignof(Type));
    }

    template <typename Type>
    Type* allocate_0(u64 amount = 1) {
        return (Type*)allocate_0(amount * sizeof(Type), alignof(Type));
    }

    template <typename Type>
    Type* resize(void* old, u64 amount) {
        return (Type*)resize(old, amount * sizeof(Type), alignof(Type));
    }
};

Allocator_Base* grab_current_allocator();
// NOTE(Felix): returns the newly pushed ones, needed for the in_scratch_bffer macro
Allocator_Base* push_allocator(Allocator_Base*);
Allocator_Base* pop_allocator();

void log_allocator(Allocator_Base* allocator); // for debugging

Allocator_Base* grab_temp_allocator(Allocator_Base* previous = nullptr);
u64  get_temp_allocator_depth(Allocator_Base* tmp);
void reset_temp_allocator(Allocator_Base* tmp, u64 depth);

extern Allocator_Base* libc_allocator;

#define with_allocator(alloc_base_ptr)                              \
    MPP_BEFORE(0, push_allocator((Allocator_Base*)&alloc_base_ptr)) \
    MPP_DEFER(1,  pop_allocator();)


inline void* allocate(u64 size_in_bytes, u32 alignment) {
    return grab_current_allocator()->allocate(size_in_bytes, alignment);
}

inline void* allocate_0(u64 size_in_bytes, u32 alignment) {
    return grab_current_allocator()->allocate_0(size_in_bytes, alignment);
}
inline void* resize(void* old, u64 size_in_bytes, u32 alignment) {
    return grab_current_allocator()->resize(old, size_in_bytes, alignment);
}
inline void  deallocate(void* old) {
    grab_current_allocator()->deallocate(old);
}


// ----------------------------------------------------------------------------
//                              Perf_Counter
// ----------------------------------------------------------------------------
struct Perf_Counter {
#ifdef FTB_WINDOWS
    s64 last_counter;
    f32 one_over_perf_frequency;
#else
    struct timespec last_timespec;
#endif
};

void init(Perf_Counter*);
f32  tick(Perf_Counter*);


// ----------------------------------------------------------------------------
//                              Maybe
// ----------------------------------------------------------------------------
struct Empty {};

template <typename type>
struct Maybe : type {
    bool __exists;

    void operator=(type v) {
        memcpy(this, &v, sizeof(type));
        __exists = true;
    }

    operator bool() const {
        return __exists;
    }
};


#define DEFINE_BASIC_TYPE_MAYBE(type)           \
    template <>                                 \
    struct Maybe<type> : Maybe<Empty> {         \
        type value;                             \
        void operator=(type v) {                \
            value = v;                          \
            __exists = true;                    \
        }                                       \
        type operator* () {                     \
            return value;                       \
        }                                       \
    }                                           \

DEFINE_BASIC_TYPE_MAYBE(bool);
DEFINE_BASIC_TYPE_MAYBE(u8);
DEFINE_BASIC_TYPE_MAYBE(u16);
DEFINE_BASIC_TYPE_MAYBE(u32);
DEFINE_BASIC_TYPE_MAYBE(u64);
DEFINE_BASIC_TYPE_MAYBE(s8);
DEFINE_BASIC_TYPE_MAYBE(s16);
DEFINE_BASIC_TYPE_MAYBE(s32);
DEFINE_BASIC_TYPE_MAYBE(s64);
DEFINE_BASIC_TYPE_MAYBE(f32);
DEFINE_BASIC_TYPE_MAYBE(f64);

#undef DEFINE_BASIC_TYPE_MAYBE


// ----------------------------------------------------------------------------
//                              String
// ----------------------------------------------------------------------------
struct String {
    char* data;
    u64 length;

    operator bool() const {
        return data != nullptr;
    }

    bool operator==(const String other) const;
    bool operator!=(const String other) const;
    s32  operator<(const String other) const;

    // NOTE(Felix): does not do any allocation, just puts it a String struct
    //   with length
    static String over(const char* str);

    // TODO(Felix): remove them. The semantics of 'String' compared to
    //   'Allocated_String' is, that the code that handles a 'String' should not
    //   need to care how it was allocated and should not need to free it. If
    //   the code should free/realloc/whatever Allocated_Strings should be used.
    void free(Allocator_Base* allocator = nullptr);
    static String from(const char* str, Allocator_Base* allocator = nullptr);
    bool starts_with(String other);
};

String operator ""_ftb(const char*, size_t);

struct Allocated_String  {
    String          string;
    Allocator_Base* allocator;

    operator bool() const {
        return string.data != nullptr;
    }

    bool operator==(Allocated_String other);
    bool operator!=(Allocated_String other);
    s32 operator<(Allocated_String other);
    static Allocated_String from(String str, Allocator_Base* allocator = nullptr);
    static Allocated_String from(const char* str, Allocator_Base* allocator = nullptr);
    void free();
};

bool print_into_string(String string, const char* format, ...);
char* heap_copy_c_string(const char* str, Allocator_Base* allocator = nullptr);
char* heap_copy_limited_c_string(const char* str, u32 len, Allocator_Base* allocator = nullptr);
const char* string_from_wstring(const wchar_t* w_string, Allocator_Base* allocator);

// ----------------------------------------------------------------------------
//                              print
// ----------------------------------------------------------------------------
extern FILE* ftb_stdout;

// NOTE(Felix): These are defines, so that the preprocessor can concat string
//   literals as in: `console_red "hello" console_normal'
// normal
#define console_black        "\x1B[0;30m"
#define console_red          "\x1B[0;31m"
#define console_green        "\x1B[0;32m"
#define console_yellow       "\x1B[0;33m"
#define console_blue         "\x1B[0;34m"
#define console_magenta      "\x1B[0;35m"
#define console_cyan         "\x1B[0;36m"
#define console_white        "\x1B[0;37m"
// bold
#define console_black_bold   "\x1B[1;30m"
#define console_red_bold     "\x1B[1;31m"
#define console_green_bold   "\x1B[1;32m"
#define console_yellow_bold  "\x1B[1;33m"
#define console_blue_bold    "\x1B[1;34m"
#define console_magenta_bold "\x1B[1;35m"
#define console_cyan_bold    "\x1B[1;36m"
#define console_white_bold   "\x1B[1;37m"
// dim
#define console_black_dim   "\x1B[2;30m"
#define console_red_dim     "\x1B[2;31m"
#define console_green_dim   "\x1B[2;32m"
#define console_yellow_dim  "\x1B[2;33m"
#define console_blue_dim    "\x1B[2;34m"
#define console_magenta_dim "\x1B[2;35m"
#define console_cyan_dim    "\x1B[2;36m"
#define console_white_dim   "\x1B[2;37m"
// blink
#define console_black_blinking   "\x1B[5;30m"
#define console_red_blinking     "\x1B[5;31m"
#define console_green_blinking   "\x1B[5;32m"
#define console_yellow_blinking  "\x1B[5;33m"
#define console_blue_blinking    "\x1B[5;34m"
#define console_magenta_blinking "\x1B[5;35m"
#define console_cyan_blinking    "\x1B[5;36m"
#define console_white_blinking   "\x1B[5;37m"
// underline
#define console_black_underline   "\x1B[4;30m"
#define console_red_underline     "\x1B[4;31m"
#define console_green_underline   "\x1B[4;32m"
#define console_yellow_underline  "\x1B[4;33m"
#define console_blue_underline    "\x1B[4;34m"
#define console_magenta_underline "\x1B[4;35m"
#define console_cyan_underline    "\x1B[4;36m"
#define console_white_underline   "\x1B[4;37m"
// high intensity
#define console_black_high_intensity   "\x1B[0;90m"
#define console_red_high_intensity     "\x1B[0;91m"
#define console_green_high_intensity   "\x1B[0;92m"
#define console_yellow_high_intensity  "\x1B[0;93m"
#define console_blue_high_intensity    "\x1B[0;94m"
#define console_magenta_high_intensity "\x1B[0;95m"
#define console_cyan_high_intensity    "\x1B[0;96m"
#define console_white_high_intensity   "\x1B[0;97m"
// bold+high intensity
#define console_black_bold_high_intensity   "\x1B[1;90m"
#define console_red_bold_high_intensity     "\x1B[1;91m"
#define console_green_bold_high_intensity   "\x1B[1;92m"
#define console_yellow_bold_high_intensity  "\x1B[1;93m"
#define console_blue_bold_high_intensity    "\x1B[1;94m"
#define console_magenta_bold_high_intensity "\x1B[1;95m"
#define console_cyan_bold_high_intensity    "\x1B[1;96m"
#define console_white_bold_high_intensity   "\x1B[1;97m"
// reset
#define console_normal  "\x1B[0m"


typedef const char* static_string;
typedef int (*printer_function_32b)(FILE*, u32);
typedef int (*printer_function_64b)(FILE*, u64);
typedef int (*printer_function_flt)(FILE*, double);
typedef int (*printer_function_ptr)(FILE*, void*);
typedef int (*printer_function_void)(FILE*);

enum struct Printer_Function_Type {
    unknown,
    _32b,
    _64b,
    _flt,
    _ptr,
    _void
};

#define register_printer(spec, fun, type)                       \
    register_printer_ptr(spec, (printer_function_ptr)fun, type)

auto register_printer_ptr(const char* spec, printer_function_ptr fun, Printer_Function_Type type) -> void;
auto print_va_args_to_file(FILE* file, static_string format, va_list* arg_list) -> s32;
auto print_va_args_to_string(char** out, Allocator_Base* allocator, static_string format, va_list* arg_list)  -> s32;
auto print_va_args_to_string(String* out, Allocator_Base* allocator, static_string format, va_list* arg_list)  -> s32;
auto print_va_args(static_string format, va_list* arg_list) -> s32;
auto print_to_string(char** out, Allocator_Base* allocator, static_string format, ...) -> s32;
auto print_to_string(String* out, Allocator_Base* allocator, static_string format, ...) -> s32;
auto print_to_file(FILE* file, static_string format, ...) -> s32;

auto print(static_string format, ...) -> s32;
auto println(static_string format, ...) -> s32;
auto raw_print(static_string format, ...) -> s32;
auto raw_println(static_string format, ...) -> s32;

auto print_str_lines(static_string str, u32 max_lines) -> s32;

auto push_print_prefix(static_string) -> void;
auto pop_print_prefix() -> void;

auto init_printer(Allocator_Base* allocator = nullptr) -> void;
auto deinit_printer() -> void;

auto print_stacktrace(FILE* file = stderr) -> void;
auto hex_dump(void* ptr, s64 count, u32 bytes_per_line = 16) -> void;


#define with_print_prefix(pfx)                  \
    MPP_BEFORE(1, push_print_prefix(pfx);)      \
    MPP_DEFER(2,  pop_print_prefix();)

// ----------------------------------------------------------------------------
//                               Array lists
// ----------------------------------------------------------------------------
// // NOTE(Felix): This is a macro, because we call alloca, which is stack-frame
// //   sensitive. So we really have to avoid calling alloca in another function
// //   (or constructor), with the macro the alloca is called in the callers
// //   stack-frame
// #define create_stack_array_list(type, length)     \
//     Stack_Array_List<type> { (type*)alloca(length * sizeof(type)), length, 0 }


// template <typename type>
// struct Stack_Array_List {
//     type* data;
//     u32 length;
//     u32 count;

//     static Stack_Array_List<type> create_from(std::initializer_list<type> l) {
//         Stack_Array_List<type> ret(l.size());

//         for (type t : l) {
//             ret.data[ret.count++] = t;
//         }
//         return ret;
//     }

//     void extend(std::initializer_list<type> l) {
//         for (type e : l) {
//             append(e);
//         }
//     }

//     void clear() {
//         count = 0;
//     }

//     type* begin() {
//         return data;
//     }

//     type* end() {
//         return data+(count);
//     }

//     void remove_index(u32 index) {
// #ifdef FTB_INTERNAL_DEBUG
//         if (index >= count)
//             fprintf(stderr, "ERROR: removing index that is not in use\n");
// #endif
//         data[index] = data[--count];
//     }

//     void append(type element) {
// #ifdef FTB_INTERNAL_DEBUG
//         if (count == length) {
//             fprintf(stderr, "ERROR: Stack_Array_List is full!\n");
//         }
// #endif
//         data[count] = element;
//         count++;
//     }

//     type& operator[](u32 index) {
// #ifdef FTB_DEBUG
//         if (index >= length) {
//             fprintf(stderr, "ERROR: Stack_Array_List access out of bounds (not even allocated).\n");
//             debug_break();
//         }
// #endif
//         return data[index];
//     }
// };

struct Untyped_Array_List {
    u32   element_size;
    void* data;
    u32   length;
    u32   count;
    Allocator_Base* allocator;

    void init(u32 e_size, u32 initial_capacity = 16, Allocator_Base* base_allocator = nullptr) {
        if (base_allocator)
            allocator = base_allocator;
        else
            allocator = grab_current_allocator();

        element_size = e_size;
        data   = allocator->allocate(initial_capacity * element_size, alignof(void*));
        count  = 0;
        length = initial_capacity;
    }

    void deinit() {
        allocator->deallocate(data);
        data = nullptr;
    }

    void clear() {
        count = 0;
    }

    void append(void* elem) {
        if (count == length) {
            length *= 2;
            data = allocator->resize(data, length * element_size, alignof(void*));
        }

        memcpy(((u8*)(data))+(element_size*count), elem, element_size);
        count++;
    }

    void* reserve_next_slot() {
        if (count == length) {
            length *= 2;
            data = allocator->resize(data, length * element_size, alignof(void*));
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
    Allocator_Base* allocator;
    type* data;
    u32 length;
    u32 count;


    void init(u32 initial_capacity = 16, Allocator_Base* base_allocator = nullptr) {
        if (base_allocator)
            allocator = base_allocator;
        else
            allocator = grab_current_allocator();

        data   = allocator->allocate<type>(initial_capacity);
        count  = 0;
        length = initial_capacity;
    }

    static Array_List<type> create_from(std::initializer_list<type> l, Allocator_Base* base_allocator = nullptr) {
        Array_List<type> ret;
        ret.init_from(l, base_allocator);
        return ret;
    }

    void init_from(std::initializer_list<type> l, Allocator_Base* base_allocator = nullptr) {
        length = (u32)(l.size() > 1 ? l.size() : 1); // alloc at least one
        init(length, base_allocator);

        count = 0;
        // TODO(Felix): Use memcpy here
        for (type t : l) {
            data[count++] = t;
        }
    }

    void extend(std::initializer_list<type> l) {
        // reserve((u32)l.size());
        // TODO(Felix): Use memcpy here
        for (type e : l) {
            append(e);
        }
    }

    void deinit() {
        if (data) {
            allocator->deallocate(data);
            data = nullptr;
        }
    }

    void clear() {
        count = 0;
    }

    void unordered_remove(type elem) {
        s32 idx = find_idx(elem);
        if (idx == -1)
            return;

        data[idx] = data[--count];
    }

    s32 find_idx(type elem) {
        for (u32 i = 0; i < count; ++i) {
            if (data[i] == elem)
                return i;
        }
        return -1;
    }

    bool contains_linear_search(type elem) {
        return find_idx(elem) != -1;
    }

    bool contains_binary_search(type elem) {
        return sorted_find(elem) != -1;
    }

    template <typename other_t>
    static type contains_all_converter_fun(other_t other) {
        return (type)other;
    }

    typedef s32 (*compare_function_t)(const type*, const type*);
    typedef s32 (*void_compare_function_t)(const void*, const void*);
    typedef s32 (*direct_compare_function_t)(const type, const type);

    template <typename other_t>
    bool contains_all(Array_List<other_t>        desired_items,
                      direct_compare_function_t  compare_fun,
                      Array_List<other_t>*       out_missing_items = nullptr,
                      type (*key)(other_t other) = contains_all_converter_fun)
    {
        bool found_all = true;

        for (auto& desired_elem : desired_items) {
            bool found = false;

            for (auto& own_elem : *this) {
                if (compare_fun(own_elem, key(desired_elem)) == 0) {
                    found = true;
                    break;
                }
            }

            if (!found) {
                if (out_missing_items) {
                    found_all = false;
                    out_missing_items->append(desired_elem);
                } else
                    return false;
            }
        }
        return found_all;
    }

    Array_List<type> clone(Allocator_Base* base_allocator = nullptr) {
        Array_List<type> ret;
        ret.init(length, base_allocator);

        ret.count = count;

        memcpy(ret.data, data, length*sizeof(type));

        return ret;
    }

    void copy_values_from(Array_List<type> other) {
        // clear the array
        count = 0;
        // make sure we have allocated enough
        assure_allocated(other.count);
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
            panic("ERROR: removing index that is not in use\n");
        }
#endif
        data[index] = data[--count];
    }

    void sorted_remove_index(u32 index) {
#ifdef FTB_INTERNAL_DEBUG
        if (index >= count) {
            panic("ERROR: removing index that is not in use\n");
        }
#endif
        memmove(data+index, data+index+1, (count-index-1) * sizeof(type));
        --count;
    }


	// NOTE(Felix): Return the index for the thing that was inserted
    u64 append(type element) {
        if (count == length) {
#ifdef FTB_INTERNAL_DEBUG
            if (length == 0) {
                panic("ERROR: Array_List was not initialized.\n");
                length = 8;
            }
#endif
            length *= 2;

            data = allocator->resize<type>(data, length);
        }
        data[count] = element;
        return count++;
    }

    void assure_allocated(u32 allocated_elements) {
        // NOTE(Felix): This method can be used to initialize an Array_List
        if (!allocator) {
            allocator = grab_current_allocator();
        }
        if (length < allocated_elements) {
            if (allocated_elements > 1024) {
                length = allocated_elements;
            } else {
                // NOTE(Felix): find smallest power of two that is larger than
                // 'allocated_elements'
                for (u32 i = 0; i < 32; ++i) {
                    u32 pot = (1u << i);
                    if (pot >= allocated_elements) {
                        length = pot;
                        break;
                    }
                }
            }
            data = allocator->resize<type>(data, length);
        }
    }

    void assure_available(u32 available_elements) {
        assure_allocated(available_elements+count);
    }

    void reserve(u32 additional_elements) {
        assure_allocated(additional_elements+count);
        count += additional_elements;
    }

    bool is_empty() const {
        return count == 0;
    }

    operator bool() const {
        return count != 0;
    }

    type& operator[](u32 index) {
#ifdef FTB_DEBUG
        if (index >= length) {
            panic("ERROR: Array_List access out of bounds (%d/%d) (not even allocated).\n",
                  index, length);
            debug_break();
        }
#endif
        return data[index];
    }


    type& last_element() {
#ifdef FTB_INTERNAL_DEBUG
        if (count == 0) {
            panic("ERROR: Array_List was empty but last_element was called.\n");
        }
#endif
        return data[count-1];
    }

    // typedef s32 (*pointer_compare_function_t)(void**, void**);
    void sort(compare_function_t comparer) {
        qsort(data, count, sizeof(type),
              (void_compare_function_t)comparer);
    }
    // void sort(pointer_compare_function_t comparer) {
        // qsort(data, count, sizeof(type),
              // (void_compare_function_t)comparer);
    // }

    void sorted_insert(type element, compare_function_t compare) {
        assure_available(1);

        u32 insertion_idx = 0;

        // TODO(Felix): implement with binary search insterad of this linear
        //   seach trash:
        for (;insertion_idx < count; ++insertion_idx) {
            if (compare(&element, &data[insertion_idx]) <= 0) {
                break;
            }
        }

        u32 to_move = (count-insertion_idx);
        if (to_move)
            memmove(&data[insertion_idx+1],
                    &data[insertion_idx],
                    to_move * sizeof(type));

        ++count;
        data[insertion_idx] = element;
    }

    // void sorted_insert(type elem, pointer_compare_function_t compare_fun)
    // {
        // return sorted_insert(elem, (compare_function_t)compare_fun);
    // }


    s32 sorted_find(type elem,
                    compare_function_t compare_fun,
                    s32 left=-1, s32 right=-1)
    {
        if (left == -1) {
            return sorted_find(elem, compare_fun,
                               0, count - 1);
        } else if (left == right) {
            if (left < (s32)count) {
                if (compare_fun(&elem, &data[left]) == 0)
                    return left;
            }
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

    // s32 sorted_find(type elem,
    //                 pointer_compare_function_t compare_fun,
    //                 s32 left=-1, s32 right=-1)
    // {
    //     return sorted_find(elem, (compare_function_t)compare_fun,
    //                        left, right);
    // }


    bool is_sorted(compare_function_t compare_fun) {
        for (s32 i = 1; i < count; ++i) {
            if (compare_fun(&data[i-1], &data[i]) > 0)
                return false;
        }
        return true;
    }

    // bool is_sorted(pointer_compare_function_t compare_fun) {
    //     return is_sorted((compare_function_t)compare_fun);
    // }
};

template <typename type>
struct Auto_Array_List : public Array_List<type> {
    Auto_Array_List(u32 length = 16, Allocator_Base* base_allocator = nullptr) {
        this->init(length, base_allocator);
    }

    Auto_Array_List(std::initializer_list<type> l, Allocator_Base* base_allocator = nullptr) {
        this->init((u32)l.size(), base_allocator);
        for (type e : l) {
            this->append(e);
        }
    }

    ~Auto_Array_List() {
        this->deinit();
        this->data = nullptr;
    }
};


template <typename type>
struct Stack {
    Array_List<type> array_list;

    void init(u32 initial_capacity = 16, Allocator_Base* base_allocator = nullptr) {
        array_list.init(initial_capacity, base_allocator);
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

    void init(u32 initial_capacity = 16, Allocator_Base* base_allocator = nullptr) {
        list.init(initial_capacity, base_allocator);
    }

    void init_from(std::initializer_list<const char*> l, Allocator_Base* base_allocator = nullptr) {
        u32 length = l.size() > 1 ? (u32)l.size() : 1; // alloc at least one
        list.init(length, base_allocator);
        // TODO(Felix): Use memcpy here
        for (const char* t : l) {
            list.data[list.count++] = t;
        }
    }

    static String_Builder create_from(std::initializer_list<const char*> l,
                                      Allocator_Base* base_allocator = nullptr)
    {
        String_Builder sb;
        sb.init_from(l, base_allocator);
        return sb;
    }


    static Allocated_String build_from(std::initializer_list<const char*> l,
                                       Allocator_Base* base_allocator = nullptr)
    {
        String_Builder sb;
        sb.init_from(l, base_allocator);
        defer { sb.deinit(); };
        return sb.build(base_allocator);
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

    Allocated_String build(Allocator_Base* base_allocator = nullptr) {
        if (!base_allocator)
            base_allocator = grab_current_allocator();

        Allocated_String ret {};
        ret.allocator = base_allocator;

        u32 total_length = 0;

        u32* lengths = (u32*)alloca(sizeof(*lengths) * list.length);

        for (u32 i = 0; i < list.count; ++i) {
            u32 length = (u32)strlen(list[i]);
            lengths[i] = length;
            total_length += length;
        }

        char* concat = base_allocator->allocate<char>(total_length+1);

        u64 cursor = 0;

        for (u32 i = 0; i < list.count; ++i) {
            strcpy(concat+cursor, list[i]);
            cursor += lengths[i];
        }

#  ifdef FTB_INTERNAL_DEBUG
        panic_if(cursor != total_length, "internal ftb error");
#  endif

        concat[cursor] = 0;

        ret.string.data = concat;
        ret.string.length = cursor+1;

        return ret;
    };
};


struct String_Split {
    String          string;
    Array_List<u32> splits;

    void init(String p_string, char split_char, Allocator_Base* base_allocator = nullptr) {
        string = p_string;
        splits.init(16, base_allocator);

        for (u32 i = 0; i < string.length; ++i) {
            if (string.data[i] == split_char) {
                splits.append(i);
            }
        }
    }

    void deinit() {
        splits.deinit();
    }

    u32 num_splits() {
        return splits.count + 1;
    }

    operator bool() const {
        return string.data != nullptr;
    }

    String operator[](u32 index) {
#ifdef FTB_DEBUG
        // NOTE(Felix): the `splits' store the position of the split, so we have
        //   one more available index than we have splits, hence we only check >
        //   and not >=
        if (index > splits.length) {
            fprintf(stderr,
                    "ERROR: String_Split access out of bounds (not even allocated).\n"
                    "  index:     %u\n"
                    "  allocated: %u\n", index, splits.length);
            debug_break();
        }
#endif
        String result;

        // a|g|d -> splits: 1 3

        // index = 1 -> g
        u32 start_idx;
        if (index == 0)
            start_idx = 0;
        else
            start_idx = splits[index-1] + 1;

        u64 length;
        if (index >= splits.count)
            length = string.length - start_idx;
        else
            length = splits[index] - start_idx;


        result.data = string.data+start_idx;
        result.length = length;

        return result;
    }

    String last() {
        return (*this)[splits.count];
    }

};


// ----------------------------------------------------------------------------
//                              IO
// ----------------------------------------------------------------------------
struct File_Read {
    bool             success;
    Allocated_String contents;
};

struct File_Info {
    bool exists;
    bool is_directory;
    s64  size;
    s64  modification_time;
};

struct Path_Info {
    File_Info        file_info;
    Allocated_String full_path;

    s64 idx_of_local_name_start;
    s64 idx_of_file_extension_start;

    String get_base_path();
    String get_local_name();
    String get_file_extension();
    void recalculate_indices();
};

auto read_entire_file(const char* filename, Allocator_Base* allocator = nullptr) -> File_Read;

auto move_file(const char* old_name, const char* new_name) -> bool;
auto delete_file(const char* path) -> bool;
// auto copy_file(const char* src, const char* dest) -> bool;

struct File_Walk_Info {
    bool recursive;
    bool with_files;
    bool with_directories;
};

enum struct Walk_Status {
    Continue,
    Continue_Recurse,
    Done,
};

auto walk_files(const char* dir_name, File_Walk_Info walk_info, void (*callback)(Path_Info, Walk_Status*, void*), void* user_data) -> void;

auto file_exists(const char* path) -> bool;
auto file_info(const char* path) -> File_Info;
auto is_directory(const char* path) -> bool;
auto delete_directory(const char* dirname) -> bool;
auto create_directory_if_not_exists(const char* path) -> bool;
auto create_directory_if_not_exists(String) -> bool;
auto get_absolute_path(const char* relative_path) -> Allocated_String;
auto get_path_components(const char* dir, Array_List<File_Info>* out_file_infos) -> void;
auto get_path_info(String dir, Allocator_Base* string_allocator) -> Path_Info;
auto get_local_name(String dir, Allocator_Base* string_allocator) -> Allocated_String;
auto get_base_path(String dir, Allocator_Base* string_allocator) -> Allocated_String;
auto join_paths(String base, String extension, bool ensure_tailing_path_separator, Allocator_Base* string_allocator) -> Allocated_String;
auto join_paths(std::initializer_list<String> components, bool ensure_tailing_path_separator, Allocator_Base* string_allocator) -> Allocated_String;

// ----------------------------------------------------------------------------
//                               Stacktraces
// ----------------------------------------------------------------------------
struct Stacktrace_Entry {
    Allocated_String function;
    Allocated_String file;
    s32              line;
};

struct Stacktrace {
    Array_List<Stacktrace_Entry> entries;

    void print(FILE* file = ftb_stdout) {
        for (u32 i = 1; i < entries.count; ++i) {
            auto e = entries[i];
            print_to_file(file, "  %s:%d (%s)\n",
                          e.file.string.data,
                          e.line,
                          e.function.string.data);
            fflush(file);
        }
    }
};

auto get_stacktrace(Allocator_Base* allocator = nullptr, s32 skip_bottom_n = 6) -> Stacktrace;

// ----------------------------------------------------------------------------
//                              specific allocators
// ----------------------------------------------------------------------------
struct LibC_Allocator {
    Allocator_Base base;
    void init();
};

struct Printing_Allocator {
    Allocator_Base base;
    void init(Allocator_Base* next_allocator = nullptr);
};

struct Resettable_Allocator {
    Allocator_Base base;

    Array_List<void*> allocated_prts;

    void init(Allocator_Base* next_allocator = nullptr);
    void deinit();
    void deallocate_everyting_still_allocated();
};

struct Allocation_Info {
    void* prt;
    u64   size_in_bytes;
};

struct Leak_Detecting_Allocator {
    Allocator_Base base;

    bool panic_on_error;
    Array_List<Allocation_Info> allocated_prts;

    void init(bool should_panic_on_error = false, Allocator_Base* next_allocator = nullptr);
    void deinit();

    void print_leak_statistics();
    void deallocate_everyting_still_allocated();
};


struct Bookkeeping_Allocator {
    Allocator_Base base;
    u32 num_allocate_calls;
    u32 num_allocate_0_calls;
    u32 num_resize_calls;
    u32 num_deallocate_calls;

    void init(Allocator_Base* next_allocator = nullptr);
    void print_statistics();
};

// NOTE(Felix): Allocator that checks every allocation and panics if an
//   allocation returned the nullptr;
struct Panicking_Allocator {
    Allocator_Base base;
    // NOTE(Felix): no special fields needed
};

struct Linear_Allocator {
    Allocator_Base base;
    void*          data;
    u64            count;
    u64            length;
    void*          last_alloc;

    operator bool () {return false;} // NOTE(Felix): used for in_scratch_buffer

    void init(u32 initial_size, Allocator_Base* next_allocator = nullptr);

    void reset();
    void deinit();
};

struct Scratch_Arena {
    Allocator_Base* arena; // linear allocator
    u64 size_at_creation;
};

auto scratch_arena_start(Allocator_Base* previous = nullptr) -> Scratch_Arena;
auto scratch_arena_start(Scratch_Arena previous) -> Scratch_Arena;
auto scratch_arena_end(Scratch_Arena) -> void;


// ----------------------------------------------------------------------------
//                              Errors
// ----------------------------------------------------------------------------
enum struct Error_Type : u64 {
    No_Error = 0,
    // NOTE(Felix): leave the bottom 32 bits for custom user errors
    Out_Of_Memory = 1llu << 32,
    File_Not_Found,
    Misc = (u64)-1,
};

struct Result {
    Error_Type error_type; // can be used for whatever

    bool ok() {
        return error_type == Error_Type::No_Error;
    }
};

Result error(Error_Type);

#ifdef FTB_CORE_IMPL

// ----------------------------------------------------------------------------
//                              Perf_Counter
// ----------------------------------------------------------------------------
#ifndef FTB_WINDOWS
# include <time.h>
#endif

void init(Perf_Counter* pc) {
#ifdef FTB_WINDOWS
    QueryPerformanceCounter((LARGE_INTEGER*)&pc->last_counter);
    s64 freq;
    QueryPerformanceFrequency((LARGE_INTEGER*)&freq);
    pc->one_over_perf_frequency = 1.0f / freq;
#else
    clock_gettime(CLOCK_MONOTONIC_RAW, &pc->last_timespec);
#endif
}

f32 tick(Perf_Counter* pc) {
#ifdef FTB_WINDOWS
    s64 old = pc->last_counter;
    QueryPerformanceCounter((LARGE_INTEGER*)&pc->last_counter);

    s64 diff = (pc->last_counter - old);
    return diff * pc->one_over_perf_frequency;
#else
    struct timespec old = pc->last_timespec;
    clock_gettime(CLOCK_MONOTONIC_RAW, &pc->last_timespec);

    f32 diff =
        (pc->last_timespec.tv_sec - old.tv_sec) +
        (pc->last_timespec.tv_nsec - old.tv_nsec) / 1e9;

    return diff;
#endif
}



// ----------------------------------------------------------------------------
//                              IO impl
// ----------------------------------------------------------------------------
auto read_entire_file(const char* filename, Allocator_Base* allocator) -> File_Read  {
    if (!allocator)
        allocator = grab_current_allocator();

    File_Read ret {};
    ret.contents.allocator = allocator;

    FILE *fp = fopen(filename, "rb");
    if (fp) {
        defer { fclose(fp); };
        /* Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            ret.contents.string.length = ftell(fp) + 1;
            if (ret.contents.string.length == 0) {
                ret.success = true;
                return ret;
            }

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                return { false };
            }

            /* Allocate our buffer to that size. */
            ret.contents.string.data = allocator->allocate<char>((u32)ret.contents.string.length+1);
        panic_if(!ret.contents.string.data,
             "Could not allocate space for file contents (%u bytes) ",
             ret.contents.string.length+1);

            /* Read the entire file into memory. */
            ret.contents.string.length = fread(ret.contents.string.data, sizeof(char),
                                               ret.contents.string.length, fp);

            ret.contents.string.data[ret.contents.string.length] = '\0';
            if (ferror(fp) != 0) {
                return { false };
            }
        }
    } else {
        return { false };
    }

    ret.success = true;
    return ret;
}

#ifdef FTB_LINUX
#  include <sys/stat.h>
#  include <errno.h>
#  include <dirent.h>
auto move_file(const char* old_name, const char* new_name) -> bool {
    return rename(old_name, new_name) == 0;
}

auto delete_file(const char* path) -> bool {
    return remove(path) == 0;
}

// auto copy_file(const char* src, const char* dest) -> bool {
//     panic("NYI");
//     return false;
// }


auto walk_files(const char* dir_name, File_Walk_Info walk_info, void (*callback)(Path_Info, Walk_Status*, void*), void* user_data) -> void {
    Scratch_Arena work_list_scratch = scratch_arena_start();
    defer { scratch_arena_end(work_list_scratch); };
    Allocated_String dir_name_alloc_str = Allocated_String::from(dir_name, work_list_scratch.arena);

    Array_List<Allocated_String> dirs_to_do {};
    dirs_to_do.init();
    dirs_to_do.append(dir_name_alloc_str);
    defer { dirs_to_do.deinit(); };
    bool first_dir = true;

    while (dirs_to_do.count != 0) {
        Scratch_Arena scratch = scratch_arena_start(work_list_scratch);
        defer { scratch_arena_end(scratch); };

        Allocated_String dir = dirs_to_do.data[--dirs_to_do.count];
        DIR* dir_handle = opendir(dir.string.data);
        bool success = dir_handle != nullptr;

        if (!success)
            continue;

        defer { closedir(dir_handle); };

        while (true) {
            Scratch_Arena scratch = scratch_arena_start(work_list_scratch);
            defer { scratch_arena_end(scratch); };

            struct dirent* dir_ent = readdir(dir_handle);
            if (dir_ent == nullptr)
                break;

            Path_Info pi {};
            pi.full_path = join_paths(dir.string, String::over(dir_ent->d_name),
                                      false, scratch.arena);
            pi.file_info.exists = true;
            pi.file_info = file_info(pi.full_path.string.data);
            pi.recalculate_indices();

            // NOTE(Felix): Yes, '.' and '..' are returned from readdir
            if (pi.get_local_name() == string_from_literal(".") ||
                pi.get_local_name() == string_from_literal(".."))
            {
                continue;
            }

            bool should_callback =
                (pi.file_info.is_directory && walk_info.with_directories)
                || (!pi.file_info.is_directory && walk_info.with_files);

            Walk_Status status = walk_info.recursive ? Walk_Status::Continue_Recurse : Walk_Status::Continue;

            if (should_callback) {
                callback(pi, &status, user_data);

                if (status == Walk_Status::Done) {
                    return;
                }

            }

            if (pi.file_info.is_directory && status == Walk_Status::Continue_Recurse) {
                Allocated_String inner_path = Allocated_String::from(pi.full_path.string, work_list_scratch.arena);
                dirs_to_do.append(inner_path);
            }
        }
    }
}

auto file_exists(const char* path) -> bool {
    return access(path, 0) == 0;
}

auto file_info(const char* path) -> File_Info {
    File_Info fi = {};

    struct stat stat_info;
    s32 result = stat(path, &stat_info);

    if (result != 0)
        return {};

    fi.exists            = true;
    fi.size              = stat_info.st_size;
    fi.modification_time = stat_info.st_mtime;
    fi.is_directory      = (stat_info.st_mode & S_IFMT) == S_IFDIR;

    return fi;
}


// auto delete_directory(const char* dirname) -> bool {
//
// }

auto create_directory_if_not_exists_1_deep(const char* path) -> bool {
    s32 error = mkdir(path, S_IRWXU);
    return (error == 0) || (errno == EEXIST);
}

auto get_absolute_path(const char* relative_path) -> Allocated_String;

#else // Not on linux

auto fill_file_info_win32(File_Info* file_info, DWORD dwFileAttributes, FILETIME ftLastWriteTime, DWORD nFileSizeHigh, DWORD nFileSizeLow) {
    file_info->is_directory = dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY;

    ULARGE_INTEGER size;
    size.LowPart    = nFileSizeLow;
    size.HighPart   = nFileSizeHigh;
    file_info->size = size.QuadPart;

    ULARGE_INTEGER mod_time;
    mod_time.LowPart  = ftLastWriteTime.dwLowDateTime;
    mod_time.HighPart = ftLastWriteTime.dwHighDateTime;

    const s64 WINDOWS_TICK       = 10000000;
    const s64 SEC_TO_UNIX_EPOCH  = 11644473600LL;
    file_info->modification_time = (mod_time.QuadPart / WINDOWS_TICK - SEC_TO_UNIX_EPOCH);
}

auto file_exists(const char* path) -> bool {
    HANDLE handle = CreateFileA(path, GENERIC_READ,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_READONLY|FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

    return handle != INVALID_HANDLE_VALUE;
}


auto file_info(const char* path) -> File_Info {
    File_Info fi = {};

    HANDLE handle = CreateFileA(path, GENERIC_READ,
                                FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE,
                                NULL, OPEN_EXISTING,
                                FILE_ATTRIBUTE_READONLY|FILE_FLAG_BACKUP_SEMANTICS,
                                NULL);

    if (handle == INVALID_HANDLE_VALUE)
        return {};

    defer { CloseHandle(handle); };

    BY_HANDLE_FILE_INFORMATION handle_info;
    GetFileInformationByHandle(handle, &handle_info);

    fi.exists       = true;
    fill_file_info_win32(&fi, handle_info.dwFileAttributes, handle_info.ftLastWriteTime, handle_info.nFileSizeHigh, handle_info.nFileSizeLow);

    return fi;
}


auto walk_files(const char* dir_name, File_Walk_Info walk_info, void (*callback)(Path_Info, Walk_Status*, void*), void* user_data) -> void {
    Scratch_Arena work_list_scratch = scratch_arena_start();
    defer { scratch_arena_end(work_list_scratch); };

    Allocated_String dir_name_alloc_str = Allocated_String::from(dir_name, work_list_scratch.arena);

    Array_List<Allocated_String> dirs_to_do {};
    dirs_to_do.init();
    dirs_to_do.append(dir_name_alloc_str);
    defer { dirs_to_do.deinit(); };

    while (dirs_to_do.count != 0) {
        Scratch_Arena scratch = scratch_arena_start(work_list_scratch);
        defer { scratch_arena_end(scratch); };

        Allocated_String dir = dirs_to_do.data[--dirs_to_do.count];
        Allocated_String wildcard_search_str = join_paths(dir.string, string_from_literal("*"), false, scratch.arena);


        WIN32_FIND_DATAA find_data;
        HANDLE find_handle = FindFirstFileA(wildcard_search_str.string.data, &find_data);
        bool   success     = find_handle != INVALID_HANDLE_VALUE;

        if (!success) {
            continue;
            log_warning("no success for '%s'", wildcard_search_str.string.data);
        }

        defer { FindClose(find_handle); };

        while (success) {

            Scratch_Arena scratch = scratch_arena_start(work_list_scratch);
            defer {
                scratch_arena_end(scratch);
                success = FindNextFileA(find_handle, &find_data);
            };

            Path_Info pi {};
            pi.full_path = join_paths(dir.string, String::over(find_data.cFileName),
                                      false, scratch.arena);

            pi.file_info.exists = true;
            fill_file_info_win32(&pi.file_info, find_data.dwFileAttributes, find_data.ftLastWriteTime,
                                 find_data.nFileSizeHigh, find_data.nFileSizeLow);

            pi.recalculate_indices();

            // NOTE(Felix): Yes, '.' and '..' are returned from FindNextFile:
            if (pi.get_local_name() == string_from_literal(".") ||
                pi.get_local_name() == string_from_literal(".."))
            {
                continue;
            }

            bool should_callback =
                (pi.file_info.is_directory && walk_info.with_directories)
                || (!pi.file_info.is_directory && walk_info.with_files);

            Walk_Status status = walk_info.recursive ? Walk_Status::Continue_Recurse : Walk_Status::Continue;

            if (should_callback) {
                callback(pi, &status, user_data);

                if (status == Walk_Status::Done) {
                    return;
                }
            }


            if (pi.file_info.is_directory && status == Walk_Status::Continue_Recurse) {
                // NOTE(Felix): It seems necessary to copy here, because pi is
                //   in scratch which will be resettet every walked file, so
                //   to remember it for later, we need to move it to the other
                //   arena
                Allocated_String inner_path = Allocated_String::from(pi.full_path.string, work_list_scratch.arena);
                dirs_to_do.append(inner_path);
            }
        }
    }
}


auto create_directory_if_not_exists_1_deep(const char* path) -> bool {
    BOOL success = CreateDirectory(path, NULL);
    return (success != 0) || (GetLastError() == ERROR_ALREADY_EXISTS);
}

#endif


#ifdef FTB_WINDOWS
#  define IS_PATH_SEPARATOR(c) ((c) == '/' || (c) == '\\')
#else
#  define IS_PATH_SEPARATOR(c) ((c) == '/')
#endif


auto for_path_component(const char* dir, bool (*callback)(String, void*), void* payload = nullptr) -> bool {
    Scratch_Arena scratch = scratch_arena_start();
    defer { scratch_arena_end(scratch); };

    Allocated_String buffer = Allocated_String::from(dir, scratch.arena);

    if (buffer.string.data[buffer.string.length - 1] == '/')
        buffer.string.data[buffer.string.length - 1] = '\0';

#define DO_CALLBACK(cursor)                                 \
    do {                                                    \
        buffer.string.length = cursor - buffer.string.data; \
        if (!callback(buffer.string, payload))              \
            return false;                                   \
    } while (0)                                             \


    // NOTE(Felix): special case for linux paths starting with /,
    //   treat it as a separate path component
    char* cursor = buffer.string.data+1;
    if (IS_PATH_SEPARATOR(buffer.string.data[0])) {
        char reset_char = *cursor;
        *cursor = '\0';

        DO_CALLBACK(cursor);

        *cursor = reset_char;
    }

    for (cursor = buffer.string.data + 1; *cursor != '\0'; ++cursor) {
        if (IS_PATH_SEPARATOR(*cursor)) {
            *cursor = '\0';
            DO_CALLBACK(cursor);
            *cursor = '/';
        }
    }

    DO_CALLBACK(cursor);

    return true;

#undef DO_CALLBACK
}

auto create_directory_if_not_exists(const char* dir) -> bool {
    return for_path_component(dir, [](String path, void* user_data) -> bool {
        return create_directory_if_not_exists_1_deep(path.data);
    });
}


String Path_Info::get_base_path() {
    String base_path = {
        .data   = full_path.string.data,
        .length = (u64)idx_of_local_name_start,
    };

    return base_path;
}

String Path_Info::get_local_name() {
    String local_name = {
        .data   = full_path.string.data + idx_of_local_name_start,
        .length = full_path.string.length - idx_of_local_name_start,
    };
    return local_name;
}


String Path_Info::get_file_extension() {
    String local_name = {
        .data   = full_path.string.data + idx_of_file_extension_start,
        .length = full_path.string.length - idx_of_file_extension_start,
    };
    return local_name;
}

/*
 * Find the index in the string where the base_path ends (exclusive) and
 * where the local_name starts (inclusive). This is important since other
 * vital functions build on it. Some examples (because it might be
 * unintuitive to remember the semantics for different path types)
 *
 *   - /home/felix/test/dir/file.txt
 *                          ^
 *                          |> local = file.txt
 *
 *   - /
 *     ^
 *     |> local = / (edge case: slash in resulting local name)
 *
 *   - /test
 *      ^
 *      |> local = test
 *
 *   - C:
 *     ^
 *     |> local = C:
 *
 *   - C:\Users
 *       ^
 *       |> local = Users
 *
 *   - ~
 *     ^
 *     |> local = ~:
 *
 *   - ~/test
 *      ^
 *      |> local = test:
 */
void Path_Info::recalculate_indices() {
    // NOTE(Felix): special case for linux paths only consisting of a / (root)
    if (full_path.string.length == 1 && IS_PATH_SEPARATOR(full_path.string.data[0])) {
        this->idx_of_local_name_start = 0;
        this->idx_of_file_extension_start = 0;
        return;
    }

    bool extension_found = false;
    u64 idx_local_end = full_path.string.length - 1;

    // NOTE(Felix): if path ends with slash, remove for local_name
    if (IS_PATH_SEPARATOR(full_path.string.data[idx_local_end])) {
        // NOTE(Felix): If path ends with slash we don't have a file
        // extension
        this->idx_of_file_extension_start = idx_local_end;
        extension_found = true;

        --idx_local_end;
    }

    bool separator_found = false;
    s64 idx_local_start = idx_local_end;
    while (idx_local_start > 0) {
        if (IS_PATH_SEPARATOR(full_path.string.data[idx_local_start-1])) {
            separator_found = true;
            break;
        } else if (!extension_found && full_path.string.data[idx_local_start-1] == '.') {
            this->idx_of_file_extension_start = idx_local_start;
            extension_found = true;
        }
        --idx_local_start;
    }

    this->idx_of_local_name_start = idx_local_start;
}

auto get_path_info(String dir, Allocator_Base* string_allocator) -> Path_Info {
    Path_Info pi = {
        .file_info  = file_info(dir.data),
        .full_path  = Allocated_String::from(dir, string_allocator),
    };

    pi.recalculate_indices();

    return pi;
}

auto get_path_components(const char* dir, Array_List<Path_Info>* out_path_infos,
                         Allocator_Base* string_allocator = nullptr)
    -> void
{
    if (string_allocator == nullptr)
        string_allocator = grab_current_allocator();

    struct Params {
        Array_List<Path_Info>* out_path_infos;
        Allocator_Base*        string_allocator;
    } params;
    params.out_path_infos   = out_path_infos;
    params.string_allocator = string_allocator;


    for_path_component(dir, [](String path, void* params_vp) -> bool {
        Params* params = (Params*)params_vp;
        Path_Info pi = get_path_info(path, params->string_allocator);

        params->out_path_infos->append(pi);

        return true; // continue iteration
    }, &params);
}

auto join_paths(std::initializer_list<String> components, bool ensure_tailing_path_separator,
                Allocator_Base* string_allocator)
    -> Allocated_String
{
    s32  slashes_to_add    = 0;
    u64  new_string_length = 0;

    {
        u32 iteration_index = 0;
        for (String s : components) {
            new_string_length += s.length;

            bool ends_in_slash = IS_PATH_SEPARATOR(s.data[s.length-1]);
            bool last_iteration = iteration_index == components.size()-1;

            if ((!last_iteration && !ends_in_slash) ||
                (last_iteration && !ends_in_slash && ensure_tailing_path_separator))
            {
                ++slashes_to_add;
            }

            ++iteration_index;
        }
    }

    Allocated_String result {
        .string = {
            .data   = string_allocator->allocate<char>(new_string_length + slashes_to_add + 1),
            .length = new_string_length + slashes_to_add,
        },
        .allocator = string_allocator
    };

    char* write_cursor = result.string.data;

    {
        u32 iteration_index = 0;
        for (String s : components) {
            bool ends_in_slash  = IS_PATH_SEPARATOR(s.data[s.length-1]);
            bool last_iteration = iteration_index == components.size()-1;

            strncpy(write_cursor, s.data, s.length);
            write_cursor += s.length;

            if ((!last_iteration && !ends_in_slash) ||
                (last_iteration && !ends_in_slash && ensure_tailing_path_separator))
            {
                *write_cursor = '/';
                ++write_cursor;
            }

            ++iteration_index;
        }
    }

    *write_cursor = '\0';

    return result;
}

auto join_paths(String base, String extension, bool ensure_tailing_path_separator,
                Allocator_Base* string_allocator)
    -> Allocated_String
{
    return join_paths({base, extension}, ensure_tailing_path_separator, string_allocator);
    // bool base_ends_in_slash = IS_PATH_SEPARATOR(base.data[base.length-1]);
    // bool should_add_tailing_path_separator =
    //     !IS_PATH_SEPARATOR(extension.data[extension.length-1])
    //     && ensure_tailing_path_separator;

    // u64 new_str_length =
    //     base.length      + (base_ends_in_slash ? 0 : 1) +
    //     extension.length + (should_add_tailing_path_separator ? 1 : 0);

    // Allocated_String result {
    //     .string = {
    //         .data   = string_allocator->allocate<char>(new_str_length + 1),
    //         .length = new_str_length,
    //     },
    //     .allocator = string_allocator
    // };

    // char* write_cursor = result.string.data;
    // strncpy(write_cursor, base.data, base.length);
    // write_cursor += base.length;

    // if (!base_ends_in_slash) {
    //     *write_cursor = '/';
    //     ++write_cursor;
    // }

    // strncpy(write_cursor, extension.data, extension.length);
    // write_cursor += extension.length;

    // if (should_add_tailing_path_separator) {
    //     *write_cursor = '/';
    //     ++write_cursor;
    // }

    // *write_cursor = '\0';

    // return result;
}



// ----------------------------------------------------------------------------
//                              allocator implementation
// ----------------------------------------------------------------------------
//
// Allocator Stack
//
LibC_Allocator internal_libc_allocator = LibC_Allocator{
    .base {
        .type = Allocator_Type::LibC_Allocator,
    }
};

unsigned char tempback_buffer_1[1024*1024*128]; // 128MB
Linear_Allocator temp_linear_allocator_1 = {
    .base {
        .type = Allocator_Type::Linear_Allocator,
    },
    .data   = tempback_buffer_1,
    .length = sizeof(tempback_buffer_1)
};

unsigned char tempback_buffer_2[1024*1024*128]; // 128MB
Linear_Allocator temp_linear_allocator_2 = {
    .base {
        .type = Allocator_Type::Linear_Allocator,
    },
    .data   = tempback_buffer_2,
    .length = sizeof(tempback_buffer_2)
};

Allocator_Base* libc_allocator   = (Allocator_Base*)&internal_libc_allocator;
Allocator_Base* temp_allocator_1 = (Allocator_Base*)&temp_linear_allocator_1;
Allocator_Base* temp_allocator_2 = (Allocator_Base*)&temp_linear_allocator_2;
Allocator_Base* global_allocator_stack = (Allocator_Base*)&internal_libc_allocator;

//
// Global Functions
//
Allocator_Base* grab_current_allocator() {
    return global_allocator_stack;
}

void log_allocator(Allocator_Base* allocator) {
    log_info("Allocator at %X", allocator);
    if (allocator) {
        switch (allocator->type) {
#         define ALLOCATOR(name) case Allocator_Type::name: log_info(" - Type: %s", #name); break;
            ALLOCATORS
#         undef ALLOCATOR
            default: log_info(" - Type: INVALID");
        }

        log_info(" - Next:");
        with_print_prefix("   | ") {
            log_allocator(allocator->next_allocator);
        }
    }
}

Allocator_Base* grab_temp_allocator(Allocator_Base* previous) {
    Allocator_Base* current = grab_current_allocator();

    if (temp_allocator_1 != previous && temp_allocator_1 != current) return temp_allocator_1;
    if (temp_allocator_2 != previous && temp_allocator_2 != current) return temp_allocator_2;

    panic("No suitiable temp_allocator found\n t1: %x\n t2: %x\n p:  %x\n c:  %x",
          temp_allocator_1, temp_allocator_2, previous, current);

    return nullptr;
}

u64 get_temp_allocator_depth(Allocator_Base* temp_allocator) {
    return ((Linear_Allocator*)temp_allocator)->count;
}

void reset_temp_allocator(Allocator_Base* temp_allocator, u64 depth) {
    ((Linear_Allocator*)temp_allocator)->count = depth;
}

Allocator_Base* push_allocator(Allocator_Base* new_allocator) {
    new_allocator->next_allocator = global_allocator_stack;
    global_allocator_stack = new_allocator;
    return new_allocator;
}

Allocator_Base* pop_allocator() {
    if (!global_allocator_stack->next_allocator) {
        panic("Attempting to pop the last allocator on the stack");
    }

    Allocator_Base* old_top = global_allocator_stack;
    global_allocator_stack = global_allocator_stack->next_allocator;

    return old_top;
}

//
// LibC Allocator functions
//
void* LibC_Allocator_allocate(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    return malloc(size_in_bytes);
}

void* LibC_Allocator_allocate_0(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    return calloc(1, size_in_bytes);
}

void* LibC_Allocator_resize(Allocator_Base* base, void* old, u64 size_in_bytes, u32 align) {
    return realloc(old, size_in_bytes);
}

void LibC_Allocator_deallocate(Allocator_Base* base, void* data) {
    free(data);
}

void LibC_Allocator::init() {
    base.type = Allocator_Type::LibC_Allocator;
}

//
// Printing Allocator functions
//
void* Printing_Allocator_allocate(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate(size_in_bytes, align);
    println("%{color<}[Printing Allocator]: Allocating %u (align %u) -> %p.%{>color}",
            console_magenta, size_in_bytes, align, res);
    return res;
}

void* Printing_Allocator_allocate_0(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate_0(size_in_bytes, align);
    println("%{color<}[Printing Allocator]: Allocating zeroed %u (align %u) -> %p.%{>color}",
            console_magenta, size_in_bytes, align, res);
    return res;
}

void* Printing_Allocator_resize(Allocator_Base* base, void* old, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->resize(old, size_in_bytes, align);
    println("%{color<}[Printing Allocator]: Resizing %p to %u (align %u) -> %p.%{>color}",
            console_magenta, old, size_in_bytes, align, res);
    return res;
}

void Printing_Allocator_deallocate(Allocator_Base* base, void* data) {
    println("%{color<}[Printing Allocator]: Deallocating %p.%{>color}",
            console_magenta, data);
    base->next_allocator->deallocate(data);
}

void Printing_Allocator::init(Allocator_Base* next_allocator) {
    base.type = Allocator_Type::Printing_Allocator;
    if (next_allocator)
        base.next_allocator = next_allocator;
    else
        base.next_allocator = grab_current_allocator();
}

//
// Leak Detecting Allocator functions
//

auto alloc_info_cmp = [](const Allocation_Info* a, const Allocation_Info* b) -> s32 {
    return (s32)((u8*)a->prt - (u8*)b->prt);
 };

void Leak_Detecting_Allocator_add_ptr(Allocator_Base* base, void* ptr, u64 amount) {
    Leak_Detecting_Allocator* ld = (Leak_Detecting_Allocator*)base;
    Allocation_Info ai = {
        .prt           = ptr,
        .size_in_bytes = amount
    };
    ld->allocated_prts.sorted_insert(ai, alloc_info_cmp);
}

void Leak_Detecting_Allocator_remove_ptr(Allocator_Base* base, void* ptr) {
    Leak_Detecting_Allocator* ld = (Leak_Detecting_Allocator*)base;

    // Freeing nullptrs is a nop
    if (!ptr)
        return;

    s32 idx = ld->allocated_prts.sorted_find(Allocation_Info{.prt = ptr}, alloc_info_cmp);

    if (idx == -1) {
        if (ld->panic_on_error) {
            panic("Attempting to free %p which was not allocated!", ptr);
        } else {
            println("%{color<}[Leak Detecting Allocator WARNING]%{color<} "
                    "Attempting to free %p which was not allocated!%{>color}%{>color}",
                    console_magenta, console_red, ptr);
        }
    } else  {
        ld->allocated_prts.sorted_remove_index(idx);
    }
}

void* Leak_Detecting_Allocator_allocate(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate(size_in_bytes, align);
    if (res)
        Leak_Detecting_Allocator_add_ptr(base, res, size_in_bytes);
    return res;
}

void* Leak_Detecting_Allocator_allocate_0(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate_0(size_in_bytes, align);
    if (res)
        Leak_Detecting_Allocator_add_ptr(base, res, size_in_bytes);
    return res;
}

void* Leak_Detecting_Allocator_resize(Allocator_Base* base, void* old, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->resize(old, size_in_bytes, align);
    if (res) {
        Leak_Detecting_Allocator_remove_ptr(base, old);
        Leak_Detecting_Allocator_add_ptr(base, res, size_in_bytes);
    } else {
        Leak_Detecting_Allocator_remove_ptr(base, old);
    }

    return res;
}

void Leak_Detecting_Allocator_deallocate(Allocator_Base* base, void* data) {
    Leak_Detecting_Allocator_remove_ptr(base, data);
    base->next_allocator->deallocate(data);
}

void Leak_Detecting_Allocator::init(bool should_panic_on_error, Allocator_Base* next_allocator) {
    base.type = Allocator_Type::Leak_Detecting_Allocator;
    if (next_allocator)
        base.next_allocator = next_allocator;
    else
        base.next_allocator = grab_current_allocator();

    panic_on_error = should_panic_on_error;
    allocated_prts.init(128, base.next_allocator);
}

void Leak_Detecting_Allocator::deinit() {
    allocated_prts.deinit();
}

void Leak_Detecting_Allocator::deallocate_everyting_still_allocated() {
    while (allocated_prts.count > 0) {
        base.next_allocator->deallocate(allocated_prts[--allocated_prts.count].prt);
    }
}

void Leak_Detecting_Allocator::print_leak_statistics() {
    u64 total_leaked_bytes = 0;
    println("%{color<}[Leak Detecting Allocator Statistics]", console_magenta);

    with_print_prefix("  | ") {
        for (Allocation_Info& ai : allocated_prts) {
            total_leaked_bytes += ai.size_in_bytes;
            println("Leaked %lu bytes at %p, hex_dump:",
                    ai.size_in_bytes, ai.prt);

            with_print_prefix("  ") {
                hex_dump(ai.prt, ai.size_in_bytes);
            }
        }

        println("");
        if (total_leaked_bytes)
            println("Totally leaked %lu bytes over %u allocations.",
                    total_leaked_bytes, allocated_prts.count);
        else
            println("No memory was leaked.");
    }
    println("%{>color}");
}

//
// Resettable Allocator functions
//

auto voidp_cmp = [](void* const * a, void* const * b){
    return (s32)((unsigned char*)*a - (unsigned char*)*b);
};

void Resettable_Allocator_add_ptr(Allocator_Base* base, void* ptr) {
    Resettable_Allocator* ra = (Resettable_Allocator*)base;
    ra->allocated_prts.sorted_insert(ptr, voidp_cmp);
}

void Resettable_Allocator_remove_ptr(Allocator_Base* base, void* ptr) {
    Resettable_Allocator* ra = (Resettable_Allocator*)base;
    s32 idx = ra->allocated_prts.sorted_find(ptr, voidp_cmp);

    if (idx != -1) {
        // NOTE(Felix) ignore double frees
        ra->allocated_prts.sorted_remove_index(idx);
    }
}

void* Resettable_Allocator_allocate(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate(size_in_bytes, align);
    if (res)
        Resettable_Allocator_add_ptr(base, res);
    return res;
}

void* Resettable_Allocator_allocate_0(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate_0(size_in_bytes, align);
    if (res)
        Resettable_Allocator_add_ptr(base, res);
    return res;
}

void* Resettable_Allocator_resize(Allocator_Base* base, void* old, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->resize(old, size_in_bytes, align);
    if (res) {
        Resettable_Allocator_add_ptr(base, res);
    }
    Resettable_Allocator_remove_ptr(base, old);

    return res;
}

void Resettable_Allocator_deallocate(Allocator_Base* base, void* data) {
    Resettable_Allocator_remove_ptr(base, data);
    base->next_allocator->deallocate(data);
}

void Resettable_Allocator::init(Allocator_Base* next_allocator) {
    base.type = Allocator_Type::Resettable_Allocator;
    if (next_allocator)
        base.next_allocator = next_allocator;
    else
        base.next_allocator = grab_current_allocator();

    allocated_prts.init(128, base.next_allocator);
}

void Resettable_Allocator::deinit() {
    allocated_prts.deinit();
}

void Resettable_Allocator::deallocate_everyting_still_allocated() {
    while (allocated_prts.count > 0) {
        base.next_allocator->deallocate(allocated_prts[--allocated_prts.count]);
    }
}


//
// Panicing Allocator functions
//
void* Panicking_Allocator_allocate(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate(size_in_bytes, align);
    if (!res)
        panic("Unsucessful allocation (size: %llu, align: %u)", size_in_bytes, align);

    return res;
}

void* Panicking_Allocator_allocate_0(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate_0(size_in_bytes, align);
    if (!res)
        panic("Unsucessful allocation (zeroed; size: %llu, align: %u)", size_in_bytes, align);
    return res;
}

void* Panicking_Allocator_resize(Allocator_Base* base, void* old, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->resize(old, size_in_bytes, align);
    if (!res)
        panic("Unsucessful allocation (resize; size: %llu, align: %u)", size_in_bytes, align);
    return res;
}

void Panicking_Allocator_deallocate(Allocator_Base* base, void* data) {
     base->next_allocator->deallocate(data);
}


//
// Bookkeeping Allocator functions
//
void* Bookkeeping_Allocator_allocate(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate(size_in_bytes, align);
    Bookkeeping_Allocator* bk = (Bookkeeping_Allocator*)base;
    ++bk->num_allocate_calls;
    return res;
}

void* Bookkeeping_Allocator_allocate_0(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->allocate_0(size_in_bytes, align);
    Bookkeeping_Allocator* bk = (Bookkeeping_Allocator*)base;
    ++bk->num_allocate_0_calls;
    return res;
}

void* Bookkeeping_Allocator_resize(Allocator_Base* base, void* old, u64 size_in_bytes, u32 align) {
    void* res = base->next_allocator->resize(old, size_in_bytes, align);
    Bookkeeping_Allocator* bk = (Bookkeeping_Allocator*)base;
    ++bk->num_resize_calls;
    return res;
}

void Bookkeeping_Allocator_deallocate(Allocator_Base* base, void* data) {
    Bookkeeping_Allocator* bk = (Bookkeeping_Allocator*)base;
    ++bk->num_deallocate_calls;
    base->next_allocator->deallocate(data);
}

void Bookkeeping_Allocator::init(Allocator_Base* next_allocator) {
    base.type = Allocator_Type::Bookkeeping_Allocator;
    if (next_allocator)
        base.next_allocator = next_allocator;
    else
        base.next_allocator = grab_current_allocator();

    num_allocate_calls   = 0;
    num_allocate_0_calls = 0;
    num_resize_calls     = 0;
    num_deallocate_calls = 0;
}


void Bookkeeping_Allocator::print_statistics() {
    println("%{color<}[Bookkeeping Allocator Statistics]", console_magenta);
    with_print_prefix("  |") {
        println("    allocate:  %u", num_allocate_calls);
        println("  allocate_0:  %u", num_allocate_0_calls);
        println("      resize:  %u", num_resize_calls);
        println("  deallocate:  %u", num_deallocate_calls);
    }
    println("%{>color}");
}

//
// Linear Allocator functions
//
void* Linear_Allocator_allocate(Allocator_Base* base, u64 amount, u32 align) {
    Linear_Allocator* self = (Linear_Allocator*)base;

    u32 overshot_align_to_8  = self->count % 8;
    u32 bytes_missing_to_align_8 = (overshot_align_to_8 != 0) * (8 - overshot_align_to_8);

    // NOTE(Felix): prev_block | maybe padding | 8 bytes size of next block | next block
    if (self->count + bytes_missing_to_align_8 + 8 + amount > self->length)
        return nullptr;

    self->count += bytes_missing_to_align_8;
    *((u64*)(((byte*)self->data)+self->count)) = amount;
    self->count += 8;

    void* ret = ((byte*)self->data)+self->count;
    self->count += amount;
    self->last_alloc = ret;

    return ret;
}

void* Linear_Allocator_allocate_0(Allocator_Base* base, u64 size_in_bytes, u32 align) {
    void* ret = Linear_Allocator_allocate(base, size_in_bytes, align);
    if (ret) memset(ret, 0, size_in_bytes);
    return ret;
}

void* Linear_Allocator_resize(Allocator_Base* base, void* old, u64 amount, u32 align) {
    Linear_Allocator* self = (Linear_Allocator*)base;

    if (!old) {
        // NOTE(Felix): When resizing nullptr, just allocate fresh
        return Linear_Allocator_allocate(base, amount, align);
    }

    // size was written onto the 8 bytes preceding the actual memory
    u64* old_size_ptr = (((u64*)old)-1);

    // check if we can get away with a resize
    if (old == self->last_alloc) {
        u64 new_count = ((byte*)old + amount) - (byte*)(self->data);
        if (new_count < self->length) {
            *old_size_ptr = amount;
            self->count = (u32)new_count;
            return old;
        } else {
            // Can't resize, allocated size is not big enough..
            return nullptr;
        }
    } else {
        // we actually have to alloc again and copy.
        void* new_block = Linear_Allocator_allocate(base, amount, align);
        memcpy(new_block, old, *old_size_ptr);
        return new_block;
    }
}

void Linear_Allocator_deallocate(Allocator_Base* base, void* data) {
    Linear_Allocator* self = (Linear_Allocator*)base;
    if (data == self->last_alloc) {
        // NOTE(Felix): self->last_alloc, even though we freed it.. It shouldn't
        // be freed again, no should it be reallocated. But both of them would
        // be harmless.. This means though that we can't ever deallocate two
        // consecutive things (take 2 things off the stack), because we don't
        // know where the previous allocation was. If we wanted to remember
        // that, additionally to the allocation size, we would have to store the
        // addres of the last allocation before that, basically like stackframes
        // have a pointer to the previous stackframe

        u64* old_size_ptr = (((u64*)data)-1);
        u64 new_count = ((byte*)old_size_ptr) - (byte*)(self->data);
        self->count = (u32)new_count;
    } else {
        // NOTE(Felix): nothing we can do if the to-be-freed block is not at the
        // end of the stack..
    }
}

void Linear_Allocator::init(u32 initial_size, Allocator_Base* next_allocator) {
    base.type = Allocator_Type::Linear_Allocator;
    if (next_allocator)
        base.next_allocator = next_allocator;
    else
        base.next_allocator = grab_current_allocator();

    length      = initial_size;
    count       = 0;
    last_alloc  = 0;
    data = base.next_allocator->allocate((u32)length, alignof(void*));
}

void Linear_Allocator::reset() {
    count = 0;
}

void Linear_Allocator::deinit() {
    base.next_allocator->deallocate(data);
}

auto scratch_arena_start(Allocator_Base* previous) -> Scratch_Arena {
    Linear_Allocator* temp_linear = (Linear_Allocator*)grab_temp_allocator(previous);
    return Scratch_Arena {
        .arena            = &temp_linear->base,
        .size_at_creation = temp_linear->count,
    };
}

auto scratch_arena_start(Scratch_Arena previous) -> Scratch_Arena {
    return scratch_arena_start(previous.arena);
}

auto scratch_arena_end(Scratch_Arena scratch) -> void {
    Linear_Allocator* linear = (Linear_Allocator*)scratch.arena;
    linear->count = scratch.size_at_creation;
}

struct Allocator_Functions {
    void* (*allocate)(Allocator_Base*, u64 amount, u32 align);
    void* (*allocate_0)(Allocator_Base*, u64 amount, u32 align);
    void* (*resize)(Allocator_Base*, void* old, u64 amount, u32 align);
    void  (*deallocate)(Allocator_Base*, void* old);
};

Allocator_Functions allocator_function_table[((int)Allocator_Type::Allocator_Type_Max_Enum -
                                              ALLOCATOR_TYPES_ENUM_START)] =
{
#define ALLOCATOR(name)                         \
    {                                           \
        .allocate   = name ## _allocate,        \
        .allocate_0 = name ## _allocate_0,      \
        .resize     = name ## _resize,          \
        .deallocate = name ## _deallocate,      \
    },

    ALLOCATORS
#undef ALLOCATOR
};

//
// Base Allocator functions
//
void* Allocator_Base::allocate(u64 size_in_bytes, u32 alignment) {
    if ((int)type >= ALLOCATOR_TYPES_ENUM_START) {
        return allocator_function_table[(int)type-ALLOCATOR_TYPES_ENUM_START].allocate(this, size_in_bytes, alignment);
    }
    return nullptr;
}

void* Allocator_Base::allocate_0(u64 size_in_bytes, u32 alignment) {
    if ((int)type >= ALLOCATOR_TYPES_ENUM_START) {
        return allocator_function_table[(int)type-ALLOCATOR_TYPES_ENUM_START].allocate_0(this, size_in_bytes, alignment);
    }
    return nullptr;
}

void* Allocator_Base::resize(void* old, u64 size_in_bytes, u32 alignment) {
    if ((int)type >= ALLOCATOR_TYPES_ENUM_START) {
        return allocator_function_table[(int)type-ALLOCATOR_TYPES_ENUM_START].resize(this, old, size_in_bytes, alignment);
    }
    return nullptr;
}

void Allocator_Base::deallocate(void* old) {
    if ((int)type >= ALLOCATOR_TYPES_ENUM_START) {
        allocator_function_table[(int)type-ALLOCATOR_TYPES_ENUM_START].deallocate(this, old);
    }
}

// ----------------------------------------------------------------------------
//                            errors impl
// ----------------------------------------------------------------------------
Result error(Error_Type type) {
    return Result{
        .error_type = type
    };
}



// ----------------------------------------------------------------------------
//                              print impl
// ----------------------------------------------------------------------------

Allocator_Base* print_allocator;
FILE* ftb_stdout = stdout;

struct Custom_Printer {
    const char*           invocation;
    printer_function_ptr  fun;
    Printer_Function_Type type;
};

const char** prefix_stack;
u32          prefix_stack_count;
u32          prefix_stack_allocated;

const char** color_stack;
u32          color_stack_count;
u32          color_stack_allocated;

Custom_Printer* custom_printers;
u32             custom_printers_count;
u32             custom_printers_allocated;

Custom_Printer* find_custom_printer(const char *spec) {
    for (u32 i = 0; i < custom_printers_count; ++i) {
        if (strcmp(custom_printers[i].invocation, spec) == 0) {
            return &custom_printers[i];
        }
    }
    return nullptr;
}

void register_printer_ptr(const char* spec, printer_function_ptr fun, Printer_Function_Type type) {
    if (custom_printers_count == custom_printers_allocated) {
        custom_printers_allocated *= 2;
        custom_printers = print_allocator->resize<Custom_Printer>(custom_printers, custom_printers_allocated);
    }
    custom_printers[custom_printers_count] = {
        .invocation = spec,
        .fun        = fun,
        .type       = type
    };

    ++custom_printers_count;
}

int maybe_special_print(FILE* file, static_string format, int* pos, va_list* arg_list) {
    if(format[*pos] != '{')
        return 0;

    int end_pos = (*pos)+1;
    while (format[end_pos] != '}' &&
           format[end_pos] != ',' &&
           format[end_pos] != '[' &&
           format[end_pos] != 0)
        ++end_pos;

    if (format[end_pos] == 0)
        return 0;

    char* spec = (char*)alloca(end_pos - (*pos));
    strncpy(spec, format+(*pos)+1, end_pos - (*pos));
    spec[end_pos - (*pos)-1] = '\0';

    // u64 spec_hash = hm_hash(spec);
    // Printer_Function_Type type = (Printer_Function_Type)type_map.get_object(spec, spec_hash);

    Custom_Printer* custom_printer = find_custom_printer(spec);
    if (!custom_printer) {
        fprintf(stderr, "ERROR: %s printer not found\n", spec);
        return 0;
    }

    Printer_Function_Type type = custom_printer->type;

    union {
        printer_function_32b  printer_32b;
        printer_function_64b  printer_64b;
        printer_function_ptr  printer_ptr;
        printer_function_flt  printer_flt;
        printer_function_void printer_void;
    } printer;

    // just grab it, it will have the correct type
    printer.printer_ptr = custom_printer->fun;
    // printer_map.get_object(spec, spec_hash);

    // if (type == Printer_Function_Type::unknown) {
    //     printf("ERROR: %s printer not found\n", spec);
    //     fflush(stdout);
    //     return 0;
    // }

    if (format[end_pos] == ',') {
        int element_count;

        ++end_pos;
        // TODO(Felix): replace this with strtol
        sscanf(format+end_pos, "%d", &element_count);

        while (format[end_pos] != '}' &&
               format[end_pos] != 0)
            ++end_pos;
        if (format[end_pos] == 0)
            return 0;

        // both brackets already included:
        int written_length = 2;

        fputs("[", file);
        for (int i = 0; i < element_count - 1; ++i) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, va_arg(*arg_list, u32));
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, va_arg(*arg_list, u64));
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, va_arg(*arg_list, double));
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, va_arg(*arg_list, void*));
            else                                          written_length += printer.printer_void(file);
            written_length += 2;
            fputs(", ", file);
        }
        if (element_count > 0) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, va_arg(*arg_list, u32));
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, va_arg(*arg_list, u64));
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, va_arg(*arg_list, double));
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, va_arg(*arg_list, void*));
            else                                          written_length += printer.printer_void(file);
        }
        fputs("]", file);

        *pos = end_pos;
        return written_length;
    } else if (format[end_pos] == '[') {
        end_pos++;
        u32 element_count;
        union {
            u32*    arr_32b;
            u64*    arr_64b;
            f32*    arr_flt;
            void**  arr_ptr;
        } arr;

        if      (type == Printer_Function_Type::_32b) arr.arr_32b = va_arg(*arg_list, u32*);
        else if (type == Printer_Function_Type::_64b) arr.arr_64b = va_arg(*arg_list, u64*);
        else if (type == Printer_Function_Type::_flt) arr.arr_flt = va_arg(*arg_list, f32*);
        else                                          arr.arr_ptr = va_arg(*arg_list, void**);

        if (format[end_pos] == '*') {
            element_count =  va_arg(*arg_list, u32);
        } else {
            // TODO(Felix): replace this with strtol
            sscanf(format+end_pos, "%d", &element_count);
        }

        // skip to next ']'
        while (format[end_pos] != ']' &&
               format[end_pos] != 0)
            ++end_pos;
        if (format[end_pos] == 0)
            return 0;

        // skip to next '}'
        while (format[end_pos] != '}' &&
               format[end_pos] != 0)
            ++end_pos;
        if (format[end_pos] == 0)
            return 0;

        // both brackets already included:
        int written_length = 2;

        fputs("[", file);
        for (u32 i = 0; i < element_count - 1; ++i) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, arr.arr_32b[i]);
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, arr.arr_64b[i]);
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, arr.arr_flt[i]);
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, arr.arr_ptr[i]);
            else                                          written_length += printer.printer_void(file);
            written_length += 2;
            fputs(", ", file);
        }
        if (element_count > 0) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, arr.arr_32b[element_count - 1]);
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, arr.arr_64b[element_count - 1]);
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, arr.arr_flt[element_count - 1]);
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, arr.arr_ptr[element_count - 1]);
            else                                          written_length += printer.printer_void(file);
        }
        fputs("]", file);

        *pos = end_pos;
        return written_length;
    } else {
        *pos = end_pos;
        if      (type == Printer_Function_Type::_32b) return printer.printer_32b(file, va_arg(*arg_list, u32));
        else if (type == Printer_Function_Type::_64b) return printer.printer_64b(file, va_arg(*arg_list, u64));
        else if (type == Printer_Function_Type::_flt) return printer.printer_flt(file, va_arg(*arg_list, double));
        else if (type == Printer_Function_Type::_ptr) return printer.printer_ptr(file, va_arg(*arg_list, void*));
        else                                          return printer.printer_void(file);
    }
    return 0;

}

int maybe_fprintf(FILE* file, static_string format, int* pos, va_list* arg_list) {
    // %[flags][width][.precision][length]specifier
    // flags     ::= [+- #0]
    // width     ::= [<number>+ \*]
    // precision ::= \.[<number>+ \*]
    // length    ::= [h l L]
    // specifier ::= [c d i e E f g G o s u x X p n %]
    int end_pos = *pos;
    int written_len = 0;

    // NOTE(Felix): Somehow we have to pass a copy of the list to vfprintf later
    //   because otherwise it destroys it on some platforms :(
    va_list arg_list_copy;
    va_copy(arg_list_copy, *arg_list);
    defer { va_end(arg_list_copy); };

    // overstep flags:
    while(format[end_pos] == '+' ||
          format[end_pos] == '-' ||
          format[end_pos] == ' ' ||
          format[end_pos] == '#' ||
          format[end_pos] == '0')
        ++end_pos;

    // overstep width
    if (format[end_pos] == '*') {
        va_arg(*arg_list, u32);
        ++end_pos;
    }
    else {
        while(format[end_pos] >= '0' && format[end_pos] <= '9')
            ++end_pos;
    }

    // overstep precision
    if (format[end_pos] == '.') {
        ++end_pos;
        if (format[end_pos] == '*') {
            va_arg(*arg_list, u32);
            ++end_pos;
        }
        else {
            while(format[end_pos] >= '0' && format[end_pos] <= '9')
                ++end_pos;
        }
    }

    // overstep length:
    while(format[end_pos] == 'h' ||
          format[end_pos] == 'l' ||
          format[end_pos] == 'L')
        ++end_pos;

    //  check for actual built_in specifier
    if(format[end_pos] == 'c' ||
       format[end_pos] == 'd' ||
       format[end_pos] == 'i' ||
       format[end_pos] == 'e' ||
       format[end_pos] == 'E' ||
       format[end_pos] == 'f' ||
       format[end_pos] == 'g' ||
       format[end_pos] == 'G' ||
       format[end_pos] == 'o' ||
       format[end_pos] == 's' ||
       format[end_pos] == 'u' ||
       format[end_pos] == 'x' ||
       format[end_pos] == 'X' ||
       format[end_pos] == 'p' ||
       format[end_pos] == 'n' ||
       format[end_pos] == '%')
    {
        written_len = end_pos - *pos + 2;
        char* temp = (char*)alloca((written_len+1)* sizeof(char));
        temp[0] = '%';
        temp[1] = 0;
        strncpy(temp+1, format+*pos, written_len);
        temp[written_len] = 0;


        written_len = vfprintf(file, temp, arg_list_copy);

        // TODO(Felix): todo overstep the correct ones by type
        if (format[end_pos] == 'f' || format[end_pos] == 'g' || format[end_pos] == 'G' ||
            format[end_pos] == 'e' || format[end_pos] == 'E')
        {
            va_arg(*arg_list, double);
        } else {
            va_arg(*arg_list, void*);
        }

        *pos = end_pos;
    }

    return written_len;
}


int print_va_args_to_file(FILE* file, static_string format, va_list* arg_list) {
    int printed_chars = 0;

    char c;
    int pos = -1;
    while ((c = format[++pos])) {
        if (c != '%') {
            putc(c, file);
            ++printed_chars;
        } else {
            c = format[++pos];
            int move = maybe_special_print(file, format, &pos, arg_list);
            if (move == 0) {
                move = maybe_fprintf(file, format, &pos, arg_list);
                if (move == -1) {
                    fputc('%', file);
                    fputc(c, file);
                    move = 1;
                }
            }
            printed_chars += move;
        }
    }

    return printed_chars;
}

int print_va_args(static_string format, va_list* arg_list) {
    return print_va_args_to_file(stdout, format, arg_list);
}

int print_va_args_to_string(String* out, Allocator_Base* allocator, static_string format, va_list* arg_list) {
    if (!allocator)
        allocator = grab_current_allocator();

    FILE* t_file = tmpfile();
    if (!t_file) {
        return 0;
    }

    int num_printed_chars = print_va_args_to_file(t_file, format, arg_list);

    out->data = allocator->allocate<char>(num_printed_chars+1);

    rewind(t_file);
    fread(out->data, sizeof(char), num_printed_chars, t_file);
    out->data[num_printed_chars] = '\0';

    fclose(t_file);

    return num_printed_chars;
}

int print_va_args_to_string(char** out, Allocator_Base* allocator, static_string format, va_list* arg_list) {
    if (!allocator)
        allocator = grab_current_allocator();

    String str;
    int num_printed_chars = print_va_args_to_string(&str, allocator, format, arg_list);
    *out = str.data;
    return num_printed_chars;
}

int print_to_string(String* out, Allocator_Base* allocator, static_string format, ...) {
    if (!allocator)
        allocator = grab_current_allocator();

    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = print_va_args_to_string(out, allocator, format, &arg_list);

    va_end(arg_list);

    return num_printed_chars;
}

int print_to_string(char** out, Allocator_Base* allocator, static_string format, ...) {
    String str;
    if (!allocator)
        allocator = grab_current_allocator();

    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = print_to_string(&str, allocator, format, &arg_list);
    *out = str.data;
    return num_printed_chars;
}

int print_to_file(FILE* file, static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = print_va_args_to_file(file, format, &arg_list);

    va_end(arg_list);

    return num_printed_chars;
}


int print_spaces(FILE* f, s32 num) {
    int sum = 0;

    while (num >= 8) {
        // println("%d", 8);
        sum += print_to_file(f, "        ");
        num -= 8;
    }
    while (num >= 4) {
        // println("%d", 4);
        sum += print_to_file(f, "    ");
        num -= 4;
    }
    while (num >= 1) {
        // println("%d", 1);
        sum += print_to_file(f, " ");
        num--;
    }
    return sum;
}


int raw_print(static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = print_va_args_to_file(ftb_stdout, format, &arg_list);

    va_end(arg_list);

    return num_printed_chars;
}

int raw_println(static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = 0;

    num_printed_chars += print_va_args_to_file(ftb_stdout, format, &arg_list);
    num_printed_chars += raw_print("\n");
    fflush(stdout);

    va_end(arg_list);

    return num_printed_chars;
}

int print_prefixes(FILE* out_file) {
    int num_printed_chars = 0;

    for (u32 i = 0; i < prefix_stack_count; ++i) {
        num_printed_chars += print_to_file(out_file, prefix_stack[i]);
    }

    return num_printed_chars;
}

int print(static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = 0;

    num_printed_chars += print_prefixes(ftb_stdout);
    num_printed_chars += print_va_args_to_file(ftb_stdout, format, &arg_list);

    va_end(arg_list);

    return num_printed_chars;
}

int println(static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = 0;

    num_printed_chars += print_prefixes(ftb_stdout);
    num_printed_chars += print_va_args_to_file(ftb_stdout, format, &arg_list);
    num_printed_chars += raw_print("\n");
    fflush(stdout);

    va_end(arg_list);

    return num_printed_chars;
}

auto print_str_lines(static_string s, u32 max_lines) -> s32 {
    s32 cursor = 0;
    s32 lines = 0;
    s32 printed = 0;

    if (s) {
        while (s[cursor]) {
            if (s[cursor] == '\n') {
                ++lines;

                printed += println("%.*s", cursor, s);
                s = s+cursor+1;
                cursor = 0;

                if (lines == max_lines)
                    break;
            }

            ++cursor;
        }
        if (lines < (int)max_lines)
            printed += println("%.*s", cursor, s);

        if (s[cursor]) {
            printed += println("...");
        }
    }

    return printed;
}

void push_print_prefix(static_string pfx) {

    if (prefix_stack_count == color_stack_allocated) {
        prefix_stack_allocated *= 2;
        prefix_stack = print_allocator->resize<const char*>(prefix_stack, prefix_stack_allocated);
    }

    prefix_stack[prefix_stack_count] = pfx;
    ++prefix_stack_count;
}

void pop_print_prefix() {
#ifdef FTB_INTERNAL_DEBUG
    if (prefix_stack_count == 0)
        fprintf(stderr, "ERROR: prefix_stack_count already 0\n");
#endif

    --prefix_stack_count;
}

int print_indented(u32 indentation, static_string format, ...) {
    int s = print_spaces(stdout, (s32)indentation);
    va_list list;
    va_start(list, format);
    s += print_va_args_to_file(stdout, format, &list);
    va_end(list);

    s += print("\n");

    return s;
}

int print_bool(FILE* f, u32 val) {
    return print_to_file(f, val ? "true" : "false");
}

int print_u32(FILE* f, u32 num) {
    return print_to_file(f, "%u", num);
}


int print_u64(FILE* f, u64 num) {
    return print_to_file(f, "%llu", num);
}

int print_s32(FILE* f, s32 num) {
    return print_to_file(f, "%d", num);
}

int print_s64(FILE* f, s64 num) {
    return print_to_file(f, "%lld", num);
}

int print_flt(FILE* f, double arg) {
    return print_to_file(f, "%f", arg);
}

int print_str(FILE* f, char* str) {
    return print_to_file(f, "%s", str);
}

int print_color_start(FILE* f, void* vp_str) {
    char* str = (char*)vp_str;
    if (color_stack_count == color_stack_allocated) {
        color_stack_allocated *= 2;
        color_stack = print_allocator->resize<const char*>(color_stack, color_stack_allocated);
    }
    color_stack[color_stack_count] = str;
    ++color_stack_count;

    return print_to_file(f, "%s", str);
}

int print_color_end(FILE* f) {
#ifdef FTB_INTERNAL_DEBUG
    if (color_stack_count == 0) {
        fprintf(stderr, "ERROR: color_stack_count already 0\n");
    }
#endif

    --color_stack_count;
    if (color_stack_count == 0) {
        return print_to_file(f, "%s", console_normal);
    } else {
        return print_to_file(f, "%s", color_stack[color_stack_count-1]);
    }
}

int print_ptr(FILE* f, void* ptr) {
    if (ptr)
        return print_to_file(f, "%#0*X", sizeof(void*)*2+2, ptr);
    return print_to_file(f, "nullptr");
}

auto print_Str(FILE* f, String* str) -> s32 {
    return print_to_file(f, "%.*s", str->length, str->data);
}

auto print_str_line(FILE* f, char* str) -> s32 {
    u32 length = 0;
    while (str[length] != '\0') {
        if (str[length] == '\n')
            break;
        length++;
    }
    return print_to_file(f, "%.*s", length, str);
}

auto hex_dump(void* ptr, s64 count, u32 bytes_per_line) -> void {
    u8* bytes = (u8*)ptr;

    u8* line_start = bytes;
    // bool start_of_line = true;
    u32 printed_bytes_this_line = 0;
    for (u32 byte_idx = 0; byte_idx < count; ++byte_idx) {
        if (printed_bytes_this_line == 0) {
            print("%02x ", (bytes[byte_idx]) &0xff);
            ++printed_bytes_this_line;

        }
        else {
            raw_print("%02x ", (bytes[byte_idx]) &0xff);
            ++printed_bytes_this_line;
        }

        if ((byte_idx+1) % bytes_per_line == 0 || byte_idx+1==count) {
            for (u32 p = printed_bytes_this_line; p < bytes_per_line; ++p) {
                raw_print("   ");
            }
            raw_print("| ",
                      (bytes+byte_idx)-line_start,
                     line_start);

            for (u32 char_idx = 0; char_idx < printed_bytes_this_line; ++char_idx) {
                char c = line_start[char_idx];
                if (c > 32 && c < 126) {
                    raw_print("%c", c);
                } else {
                    raw_print(".");
                }
            }

            raw_print("\n");

            line_start = bytes+byte_idx+1;
            printed_bytes_this_line = 0;
        }
    }

    println("");
}

#ifdef FTB_USING_MATH
auto print_v2(FILE* f, V2* v2) -> s32 {
    return print_to_file(f, "{ %f %f }",
                         v2->x, v2->y);
}

auto print_v3(FILE* f, V3* v3) -> s32 {
    return print_to_file(f, "{ %f %f %f }",
                         v3->x, v3->y, v3->z);
}

auto print_v4(FILE* f, V4* v4) -> s32 {
    return print_to_file(f, "{ %f %f %f %f }",
                         v4->x, v4->y, v4->z, v4->w);
}

auto print_quat(FILE* f, Quat* quat) -> s32 {
    return print_v4(f, quat);
}

// NOTE(Felix): Matrices are in column major, but we print them in row major to
//   look more normal
auto print_m2x2(FILE* f, M2x2* m2x2) -> s32 {
    return print_to_file(f,
                         "{ %f %f\n"
                         "  %f %f }",
                         m2x2->_00, m2x2->_10,
                         m2x2->_01, m2x2->_11);
}

auto print_m3x3(FILE* f, M3x3* m3x3) -> s32 {
    return print_to_file(f,
                         "{ %f %f %f\n"
                         "  %f %f %f\n"
                         "  %f %f %f }",
                         m3x3->_00, m3x3->_10, m3x3->_20,
                         m3x3->_01, m3x3->_11, m3x3->_21,
                         m3x3->_02, m3x3->_12, m3x3->_22);
}

auto print_m4x4(FILE* f, M4x4* m4x4) -> s32 {
    return print_to_file(f,
                         "{ %f %f %f %f \n"
                         "  %f %f %f %f \n"
                         "  %f %f %f %f \n"
                         "  %f %f %f %f }",
                         m4x4->_00, m4x4->_10, m4x4->_20, m4x4->_30,
                         m4x4->_01, m4x4->_11, m4x4->_21, m4x4->_31,
                         m4x4->_02, m4x4->_12, m4x4->_22, m4x4->_32,
                         m4x4->_03, m4x4->_13, m4x4->_23, m4x4->_33);
}
#endif

void init_printer(Allocator_Base* allocator) {
    if (!allocator)
        allocator = grab_current_allocator();
    print_allocator = allocator;

#ifdef FTB_WINDOWS
    // enable colored terminal output for windows
    HANDLE hOut  = GetStdHandle(STD_OUTPUT_HANDLE);
    DWORD dwMode = 0;
    GetConsoleMode(hOut, &dwMode);
#ifndef ENABLE_VIRTUAL_TERMINAL_PROCESSING // g++ does not seem to define it
#define ENABLE_VIRTUAL_TERMINAL_PROCESSING 0x0004
#endif
    dwMode |= ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    SetConsoleMode(hOut, dwMode);
#endif
    custom_printers_count     = 0;
    custom_printers_allocated = 32;
    custom_printers           = print_allocator->allocate<Custom_Printer>(custom_printers_allocated);

    color_stack_count     = 0;
    color_stack_allocated = 16;
    color_stack           = print_allocator->allocate<const char*>(color_stack_allocated);

    prefix_stack_count     = 0;
    prefix_stack_allocated = 16;
    prefix_stack           = print_allocator->allocate<const char*>(prefix_stack_allocated);

    register_printer("spaces",      print_spaces,      Printer_Function_Type::_32b);
    register_printer("u32",         print_u32,         Printer_Function_Type::_32b);
    register_printer("u64",         print_u64,         Printer_Function_Type::_64b);
    register_printer("bool",        print_bool,        Printer_Function_Type::_32b);
    register_printer("s64",         print_s64,         Printer_Function_Type::_64b);
    register_printer("s32",         print_s32,         Printer_Function_Type::_32b);
    register_printer("f32",         print_flt,         Printer_Function_Type::_flt);
    register_printer("f64",         print_flt,         Printer_Function_Type::_flt);
    register_printer("->char",      print_str,         Printer_Function_Type::_ptr);
    register_printer("->",          print_ptr,         Printer_Function_Type::_ptr);
    register_printer("color<",      print_color_start, Printer_Function_Type::_ptr);
    register_printer(">color",      print_color_end,   Printer_Function_Type::_void);
    register_printer("->Str",       print_Str,         Printer_Function_Type::_ptr);
    register_printer("->char_line", print_str_line,    Printer_Function_Type::_ptr);

#ifdef FTB_USING_MATH
    register_printer("->v2",   print_v2,   Printer_Function_Type::_ptr);
    register_printer("->v3",   print_v3,   Printer_Function_Type::_ptr);
    register_printer("->v4",   print_v4,   Printer_Function_Type::_ptr);
    register_printer("->quat", print_quat, Printer_Function_Type::_ptr);
    register_printer("->m2x2", print_m2x2, Printer_Function_Type::_ptr);
    register_printer("->m3x3", print_m3x3, Printer_Function_Type::_ptr);
    register_printer("->m4x4", print_m4x4, Printer_Function_Type::_ptr);
#endif
}

void deinit_printer() {
    print_allocator->deallocate(color_stack);
    print_allocator->deallocate(prefix_stack);
    print_allocator->deallocate(custom_printers);
}
#ifndef FTB_NO_INIT_PRINTER
namespace {
    struct Printer_Initter {
        Printer_Initter() {
            init_printer();
        }
        ~Printer_Initter() {
            deinit_printer();
        }
    } p_initter;
}
#endif // FTB_NO_INIT_PRINTER


// ----------------------------------------------------------------------------
//                              string implementation
// ----------------------------------------------------------------------------
bool String::operator==(const String other) const {
    return
        length == other.length &&
        strncmp(data, other.data, other.length) == 0;
}

bool String::operator!=(const String other) const {
    return !(*this == other);
}

s32 String::operator<(const String other) const {
    return strncmp(data, other.data, MIN(length, other.length));
}

bool String::starts_with(String other) {
    if (length < other.length)
        return false;

    String shortened = {
        .data   = data,
        .length = other.length
    };

    return shortened == other;
}

String String::over(const char* str) {
    String r;

    r.length = strlen(str);
    r.data   = (char*)str;

    return r;
}

String String::from(const char* str, Allocator_Base* allocator) {
    String r;

    if (!allocator)
        allocator = grab_current_allocator();

    r.length = strlen(str);
    r.data  = allocator->allocate<char>((u32)r.length+1);
    strcpy(r.data, str);

    return r;
}

void String::free(Allocator_Base* allocator) {
    if (!allocator)
        allocator = grab_current_allocator();

    if (data) {
        allocator->deallocate(data);
    }
#ifdef FTB_INTERNAL_DEBUG
    length = 0;
    data = nullptr;
#endif
}

String operator ""_ftb(const char* data, size_t length) {
    return String {
        .data   = (char*)data,
        .length = length,
    };
}

bool Allocated_String::operator==(Allocated_String other) {
    return this->string == other.string;
}

bool Allocated_String::operator!=(Allocated_String other) {
    return !(this->string == other.string);
}

s32 Allocated_String::operator<(Allocated_String other) {
    return strncmp(string.data, other.string.data, MIN(string.length, other.string.length));
}

Allocated_String Allocated_String::from(String str, Allocator_Base* allocator) {
    Allocated_String r;

    if (!allocator)
        allocator = grab_current_allocator();

    r.allocator     = allocator;
    r.string.length = str.length;
    r.string.data   = allocator->allocate<char>((u32)r.string.length+1);

    strncpy(r.string.data, str.data, str.length);
    r.string.data[r.string.length] = '\0';

    return r;
}

Allocated_String Allocated_String::from(const char* str, Allocator_Base* allocator) {
    String s = {
        .data   = (char*)str,
        .length = strlen(str),
    };

    return Allocated_String::from(s, allocator);
}

void Allocated_String::free() {
    if (string.data) {
        allocator->deallocate(string.data);
    }
#ifdef FTB_INTERNAL_DEBUG
    string.length = 0;
    string.data = nullptr;
#endif
}



// ----------------------------------------------------------------------------
//                            strings implementation
// ----------------------------------------------------------------------------
const char* string_from_wstring(const wchar_t* w_string, Allocator_Base* allocator) {
#ifdef FTB_WINDOWS
    char* buffer      = nullptr;
    int   buffer_size = 0;
    int   result = WideCharToMultiByte(CP_UTF8, 0 /* dwFlags */, w_string,
                                       -1, /*-1 length == 0 terminated string*/
                                       (LPSTR)buffer, buffer_size, 0, 0);

    panic_if(result <= 0, "error converting strings (1)");

    buffer_size = result+1;
    buffer = allocator->allocate_0<char>(buffer_size);

    result = WideCharToMultiByte(CP_UTF8, 0 /* dwFlags */, w_string,
                                 -1, /*-1 length == 0 terminated string*/
                                 (LPSTR)buffer, buffer_size, 0, 0);

    panic_if(result <= 0, "error converting strings (2)");

    return buffer;
#else
    panic("");
    return {};
#endif
}

char* heap_copy_limited_c_string(const char* str, u32 len, Allocator_Base* allocator) {
    if (!allocator)
        allocator = grab_current_allocator();

    char* result = allocator->allocate<char>(len+1);
    if (result) {
        memcpy(result, str, len);
        result[len] = '\0';
    }

    return result;
}

char* heap_copy_c_string(const char* str, Allocator_Base* allocator) {
    u32 len = (u32)strlen(str);
    return heap_copy_limited_c_string(str, len, allocator);
}

bool print_into_string(String string, const char* format, ...) {
    va_list args;
    va_start(args, format);

    s32 written = vsnprintf(string.data, string.length, format, args);

    va_end(args);

    return written >= 0 && (u32)written < string.length;
}


// ----------------------------------------------------------------------------
//                              utf8 implementation
// ----------------------------------------------------------------------------
Unicode_Code_Point bytes_to_code_point(const byte* b) {
    if ((*b & 0b10000000) == 0) {
        // ascii
        return {
            .byte_length = 1,
            .code_point  = *b
        };
    } else if ((*b & 0b11100000) == 0b11000000) {
        // 2 byte
        return {
            .byte_length = 2,
            .code_point  = (u32)(b[0] & 0b00011111) << 6 |
                                (b[1] & 0b00111111),
        };
    } else if ((*b & 0b11110000) == 0b11100000) {
        // 3 byte
        return {
            .byte_length = 3,
            .code_point  = (u32)(b[0] & 0b00001111) << 12 |
                                (b[1] & 0b00111111) <<  6 |
                                (b[2] & 0b00111111),
        };
    } else {
        // 4 byte
        return {
            .byte_length = 3,
            .code_point  = (u32)(b[0] & 0b00000111) << 18 |
                                (b[1] & 0b00111111) << 12 |
                                (b[2] & 0b00111111) <<  6 |
                                (b[3] & 0b00111111),
        };
    }
}

u32 code_point_to_bytes(Unicode_Code_Point cp, char* out_string) {
    byte* bytes = (byte*)out_string;
    if (cp.byte_length == 1) {
        bytes[0] = cp.code_point;
    } else if (cp.byte_length == 2) {
        bytes[0] = (((cp.code_point >> 6) & 0b00011111) | 0b11000000); // [6 - 10]
        bytes[1] = (((cp.code_point >> 0) & 0b00111111) | 0b10000000); // [0 -  5]
    } else if (cp.byte_length == 3) {
        bytes[0] = (((cp.code_point >> 12) & 0b00001111) | 0b11100000); // [12 - 15]
        bytes[1] = (((cp.code_point >>  6) & 0b00111111) | 0b10000000); // [ 6 - 11]
        bytes[2] = (((cp.code_point >>  0) & 0b00111111) | 0b10000000); // [ 0 - 5]
    } else {
        bytes[0] = (((cp.code_point >> 18) & 0b00000111) | 0b11110000); // [18 - 20]
        bytes[1] = (((cp.code_point >> 12) & 0b00111111) | 0b10000000); // [12 - 17]
        bytes[2] = (((cp.code_point >>  6) & 0b00111111) | 0b10000000); // [ 6 - 11]
        bytes[3] = (((cp.code_point >>  0) & 0b00111111) | 0b10000000); // [ 0 -  5]
    }
    return cp.byte_length;
}

u32 get_byte_length_for_code_point(u32 cp) {
    if (cp < 0x80)    return 1;
    if (cp < 0x0800)  return 2;
    if (cp < 0x10000) return 3;
    else              return 4;
}

u32 code_point_to_bytes(u32 cp, char* out_string) {
    Unicode_Code_Point u_cp;
    u_cp.code_point = cp;
    u_cp.byte_length = get_byte_length_for_code_point(cp);
    return code_point_to_bytes(u_cp, out_string);
}

u32 count_chars_utf8(const char* str) {
    u32 len = 0;
    while (*str) {
        ++len;
        Unicode_Code_Point cp = bytes_to_code_point((const byte*)str);
        str += cp.byte_length;
    }
    return len;
}

bool strncpy_0(char* dest, const char* src, u64 dest_size) {
    u64 src_len = strlen(src);
    if (src_len >= dest_size) {
        strncpy(dest, src, dest_size-1);
        dest[dest_size-1] = '\0';
        return false;
    } else {
        strcpy(dest, src);
        dest[src_len] = '\0';
        return true;
    }
}

bool strncpy_utf8_0(char* dest, const char* src, u64 dest_size) {
    u64 bytes_to_copy = 0;

    const char* cursor = src;
    bool copied_all = true;

    while (*cursor) {
        Unicode_Code_Point cp = bytes_to_code_point((const byte*)cursor);
        if (bytes_to_copy + cp.byte_length >= dest_size) {
            copied_all = false;
            break;
        }

        bytes_to_copy += cp.byte_length;
        cursor += cp.byte_length;
    }

    strncpy(dest, src, bytes_to_copy);
    dest[bytes_to_copy] = '\0';

    return copied_all;
}



// ----------------------------------------------------------------------------
//                              Stacktrace impl
// ----------------------------------------------------------------------------

#ifndef FTB_STACKTRACE_INFO

auto print_stacktrace(FILE* file) -> void {
    fprintf(file, "No stacktrace info available (recompile with FTB_STACKTRACE_INFO defined)\n");
}

auto get_stacktrace(Allocator_Base* allocator, s32 skip_bottom_n) -> Stacktrace {
    return {};
}


#else // stacktace should be present
#  if defined FTB_WINDOWS
#    define VC_EXTRALEAN
#    define WIN32_LEAN_AND_MEAN
#    include <Windows.h>
#    include <dbghelp.h>

auto get_stacktrace(Allocator_Base* allocator, s32 skip_bottom_n) -> Stacktrace {
    Stacktrace result = {};
    result.entries.init(16, allocator);


    const u32 max_stacktrace_depth = 100;
    const u32 max_symbol_length    = 255;

    // NOTE(Felix): Just always use LibC's allocator here
    SYMBOL_INFO* symbol = (SYMBOL_INFO*)calloc(sizeof(*symbol) + (max_symbol_length+1 * sizeof(char)), 1);
    defer { free(symbol); };

    symbol->MaxNameLen   = max_symbol_length;
    symbol->SizeOfStruct = sizeof( SYMBOL_INFO );

    void* stack[max_stacktrace_depth];
    u16 num_frames = CaptureStackBackTrace(1, max_stacktrace_depth, stack, NULL);

    HANDLE  process = GetCurrentProcess();
    SymInitialize(process, NULL, TRUE);

    for(int i = 0; i < num_frames - skip_bottom_n; i++ ) {
        Stacktrace_Entry entry;

        SymFromAddr(process, (DWORD64)(stack[i]), 0, symbol);
        entry.function = Allocated_String::from(symbol->Name, allocator);

        IMAGEHLP_LINE64 line_info;
        DWORD line_displacement = 0;
        if (SymGetLineFromAddr64(process, (DWORD64)stack[i], &line_displacement, &line_info)) {
            entry.file = Allocated_String::from(line_info.FileName, allocator);
            entry.line = line_info.LineNumber;
        }

        result.entries.append(entry);
    }

    return result;
}

auto print_stacktrace(FILE* file) -> void {
    Scratch_Arena scratch = scratch_arena_start();
    defer { scratch_arena_end(scratch); };

    fprintf(file, "Stacktrace: \n");
    Stacktrace st = get_stacktrace(scratch.arena, 3);
    st.print(file);

}

#  else // not windows
#    include <stdio.h>
#    include <stdlib.h>
#    include <unistd.h>
#    ifdef FTB_STACKTRACE_USE_GDB
#      include <sys/wait.h>
#      include <sys/prctl.h>

auto print_stacktrace(FILE* file) -> void {
    fprintf(file, "Stacktrace: \n");
    char pid_buf[30];
    sprintf(pid_buf, "%d", getpid());
    char name_buf[512];
    name_buf[readlink("/proc/self/exe", name_buf, 511)]=0;
    prctl(PR_SET_PTRACER, PR_SET_PTRACER_ANY, 0, 0, 0);
    int child_pid = fork();
    if (!child_pid) {
        // dup2(2,1); // redirect output to file - edit: unnecessary?

        // NOTE(Felix): the "-n" flag will prevent loading the ~/.gdbinit file
        //   which users might have created to disable some additional
        //   information or warnings on screen. So we probably let them load it,
        //   by not passing "-n" to gdb
        execl("/usr/bin/gdb", "gdb",
              "--batch",
              /*"-n",*/ "-ex", "thread", "-ex", "bt", name_buf, pid_buf, NULL);
        abort(); /* If gdb failed to start */
    } else {
        waitpid(child_pid,NULL,0);
   }
}

auto get_stacktrace(Allocator_Base* allocator, s32 skip_bottom_n) -> Stacktrace {
    return {};
}

#    else // linux, but not using gdb
#      include <execinfo.h>

auto print_stacktrace(FILE* file) -> void {
    fprintf(file,
            "Stacktrace (this is unmagled -- sorry\n"
            "  (you can recompile with FTB_STACKTRACE_USE_GDB defined and -rdynamic to get one) \n");
    char **strings;
    size_t i, size;
    enum Constexpr { MAX_SIZE = 1024 };
    void *array[MAX_SIZE];
    size = backtrace(array, MAX_SIZE);
    strings = backtrace_symbols(array, size);
    for (i = 0; i < size; i++)
        fprintf(file, "  %3lu: %s\n", size - i - 1, strings[i]);
    fputs("", file);
    free(strings);
}

auto get_stacktrace(Allocator_Base* allocator, s32 skip_bottom_n) -> Stacktrace {
    return {};
}

#    endif // FTB_STACKTRACE_USE_GDB
#  endif   // FTB_WINDOWS
#endif     // FTB_STACKTRACE_INFO

#endif // FTB_CORE_IMPL
