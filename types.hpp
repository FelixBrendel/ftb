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

struct Ftb_StringSlice {
    const char* data;
    u64 length;
};

struct Ftb_String {
    char* data;
    u64 length;
};


inline auto heap_copy_c_string(const char* str) -> char* {
#ifdef FTB_WINDOWS
    return _strdup(str);
#else
    return strdup(str);
#endif
}

inline auto make_heap_string(const char* str) -> Ftb_String {
    Ftb_String ret;
    ret.length = strlen(str);
    ret.data = heap_copy_c_string(str);
    return ret;
}

inline auto make_static_string(const char* str) -> const Ftb_StringSlice {
    Ftb_StringSlice ret;
    ret.length = strlen(str);
    ret.data = str;
    return ret;
}

auto inline string_equal(const char* input, const char* check) -> bool {
    return strcmp(input, check) == 0;
}

auto inline string_equal(Ftb_StringSlice str, const char* check) -> bool {
    if (str.length != strlen(check))
        return false;
    return strncmp(str.data, check, str.length) == 0;
}

auto inline string_equal(const char* check, Ftb_StringSlice str) -> bool {
    if (str.length != strlen(check))
        return false;
    return strncmp(str.data, check, str.length) == 0;
}

auto inline string_equal(Ftb_StringSlice str1, Ftb_StringSlice str2) -> bool {
    if (str1.length != str2.length)
        return false;

    return strncmp(str1.data, str2.data, str2.length) == 0;
}
