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
    u64 length;
    char* data;
};

struct StringSlice {
    u64 length;
    const char* data;
};

inline auto make_heap_string(const char* str) -> String {
    String ret;
    ret.length = strlen(str);
    ret.data = _strdup(str);
    return ret;
}

inline const StringSlice make_static_string(const char* str) {
    StringSlice ret;
    ret.length = strlen(str);
    ret.data = str;
    return ret;
}

auto inline string_equal(const char* input, const char* check) -> bool {
    return strcmp(input, check) == 0;
}

template <typename Str>
auto inline string_equal(Str str, const char* check) -> bool {
    if (str.length != strlen(check))
        return false;
    return strncmp(str.data, check, str.length) == 0;
}

template <typename Str>
auto inline string_equal(const char* check, Str str) -> bool {
    if (str.length != strlen(check))
        return false;
    return strncmp(str.data, check, str.length) == 0;
}

template <typename StrA, typename StrB>
auto inline string_equal(StrA str1, StrB str2) -> bool {
    if (str1.length != str2.length)
        return false;

    return strncmp(str1.data, str2.data, str2.length) == 0;
}

template auto string_equal(StringSlice str1, StringSlice str2) -> bool;
