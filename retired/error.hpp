#pragma once
#include "stdio.h"

#include "math.h"
#include "stdarg.h"
#include "stdlib.h"
#include "types.hpp"
#include "print.hpp"

struct Error {
    String type;
    String message;
};

Error* error = nullptr;

auto delete_error() -> void {
    free(error);
    error = nullptr;
}

auto create_error(const char* c_func_name, const char* c_file_name,
                  u32 c_file_line, String type, const char* format, ...) -> void {

    error = (Error*) malloc(sizeof(Error));

    va_list args;
    va_start(args, format);
    error->message.length = print_va_args_to_string(&(error->message.data), format, &args);
    va_end(args);

    error->type = type;
    print("\n%{color<}%{->Str} error:%{>color} %{->Str}\n",
          console_red, &(error->type), &(error->message));

    print("in");
    s32 spacing = 30-((s32)strlen(c_file_name) + (s32)log10(c_file_line));
    if (spacing < 1) spacing = 1;
    for (s32 i = 0; i < spacing; ++i)
        print(" ");
    print("%s (%u) ", c_file_name, c_file_line);
    print("-> %s\n", c_func_name);
    fflush(stdout);
}

#define __create_error(keyword, ...)            \
    create_error(                               \
        __FUNCTION__, __FILE__, __LINE__,       \
        make_heap_string(keyword),              \
        __VA_ARGS__)

#define create_assertion_error(...)             \
    __create_error("assert", __VA_ARGS__)

#define create_generic_error(...)               \
    __create_error("generic", __VA_ARGS__)

#ifdef assert
#undef assert
#endif
#define assert_msg(condition, ...)                                      \
    do {                                                                \
        if (!(condition)) {                                             \
            char* msg;                                                  \
            print_to_string(&msg,  __VA_ARGS__);                        \
            create_assertion_error("Assertion-error: %s\n"              \
                                   "      condition: %s\n"              \
                                   "             in: %s:%d",            \
                                   msg, #condition,                     \
                                   __FILE__, __LINE__ );                \
            debug_break();                                              \
        }                                                               \
    } while(0)

#define assert(condition)                                               \
    do {                                                                \
        if (!(condition)) {                                             \
            create_assertion_error("Assertion-error:\n"                 \
                                   "      condition: %s\n"              \
                                   "             in: %s:%d",            \
                                   #condition,                          \
                                   __FILE__, __LINE__ );                \
            debug_break();                                              \
        }                                                               \
    } while(0)


#define log_location()                                                  \
    do {                                                                \
        printf("in");                                                   \
        s32 spacing = 30-((s32)strlen(__FILE__) + (s32)log10(__LINE__)); \
        if (spacing < 1) spacing = 1;                                   \
        for (s32 i = 0; i < spacing;++i)                                \
            printf(" ");                                                \
        printf("%s (%d) ", __FILE__,  __LINE__);                        \
        printf("-> %s\n", __FUNCTION__);                                \
    } while(0)

#define if_error_log_location_and_return(val)   \
    do {                                        \
        if (error) {                            \
            log_location();                     \
            return val;                         \
        }                                       \
    } while(0)

#define try_or_else_return(val)                                            \
    if (1)                                                                 \
        goto label(body,__LINE__);                                         \
    else                                                                   \
        while (1)                                                          \
            if (1) {                                                       \
                if (error) {                                               \
                    log_location();                                        \
                    return val;                                            \
                }                                                          \
                break;                                                     \
            }                                                              \
            else label(body,__LINE__):

#define check_error_struct try_or_else_return({})
#define check_error_void   try_or_else_return(;)
#define check_error        try_or_else_return(0)
