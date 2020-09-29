#pragma once

#include "platform.hpp"
#include <stdint.h>
#include <string.h>

typedef int8_t   s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef uint8_t   u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef float       f32;
typedef long double f64;

#ifdef UNICODE
typedef wchar_t path_char;
#else
typedef char    path_char;
#endif

struct String {
    u64 length;
    char* data;
};

inline auto heap_copy_c_string(const char* str) -> char* {
#ifdef FTB_WINDOWS
    return _strdup(str);
#else
    return strdup(str);
#endif
}

inline auto make_heap_string(const char* str) -> String {
    String ret;
    ret.length = strlen(str);
    ret.data = heap_copy_c_string(str);
    return ret;
}

inline const String make_static_string(char* str) {
    String ret;
    ret.length = strlen(str);
    ret.data = str;
    return ret;
}
