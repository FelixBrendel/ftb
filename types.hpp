#pragma once

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
    u32 length;
    char* data;
};

inline String make_heap_string(const char* str) {
    String ret;
    ret.length = strlen(str);
    ret.data = _strdup(str);
    return ret;
}
