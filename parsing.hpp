#pragma once
#include "core.hpp"

enum struct Data_Type : u8 {
    Custom = 0,
    String = 1 << 7,
    Integer,
    Long,
    Float,
    Boolean,
    Maybe_Integer,
    Maybe_Boolean
};


bool is_alpha_char(const char c);
bool is_number_char(const char c);
bool is_quotes_char(const char c);
bool is_whitespace(const char c);


u32 read_identifier(const char* str, String* out_string, Allocator_Base* allocator = nullptr);
u32 read_string(const char* str, String* out_string, Allocator_Base* allocator = nullptr);
u32 read_int(const char* str, s32* out_int);
u32 read_long(const char* str, s64* out_int);
u32 read_bool(const char* str, bool* out_bool);
u32 read_float(const char* str, f32* out_float);

u32 eat_line(const char* str);
u32 eat_construct(const char* string, char delimiter);
u32 eat_whitespace(const char* str);
u32 eat_string(const char* str);
u32 eat_identifier(const char* str);
u32 eat_number(const char* str);

#ifdef FTB_PARSING_IMPL
inline bool is_quotes_char(const char c) {
    return c == '\'' || c == '"';
}

inline bool is_number_char(const char c) {
    return c >= '0' && c <= '9';
}

bool is_alpha_char(const char c) {
    return
        (c >= 'a' && c <= 'z') ||
        (c >= 'A' && c <= 'Z');
}


inline bool is_whitespace(const char c) {
    return c == ' '  || c == '\n' ||
        c == '\r' || c == '\t';
}

u32 eat_line(const char* str) {
    u32 eaten = 0;
    while (*str != '\n' && *str != '\0') {
        ++str;
        ++eaten;
    }
    return eaten;
}

u32 eat_construct(const char* string, char delimiter) {
    // NOTE(Felix): Expects to start somewhere inside construct, not on the
    //   start of the onctruct
    u32 eaten = 0;

    while (string[eaten] && string[eaten] != delimiter) {
        if (string[eaten] == '"')
            eaten += eat_string(string+eaten);
        else if (string[eaten] == '{') {
            ++eaten;
            eaten += eat_construct(string+eaten, '}');
        } else if (string[eaten] == '[') {
            ++eaten;
            eaten += eat_construct(string+eaten, ']');
        }
        else
            ++eaten;
    }

    ++eaten; // overstep delimiter

    return eaten;
}

u32 eat_whitespace(const char* str) {
    u32 eaten = 0;
    while (is_whitespace(*str)) {
        ++str;
        ++eaten;
    }

    return eaten;
}

u32 eat_number(const char* str) {
    u32 eaten = 0;

    if (str[eaten] == '+'
        || str[eaten] == '-')
        ++eaten;

    while (is_number_char(str[eaten]))
        ++eaten;

    if (str[eaten] == '.') {
        ++eaten;
        while (is_number_char(str[eaten]))
            ++eaten;
    } else if (str[eaten] == 'e') {
        if (str[eaten] == '+'
            || str[eaten] == '-')
            ++eaten;

        while (is_number_char(str[eaten]))
            ++eaten;
    }

    return eaten;
}

u32 eat_identifier(const char* str) {
    u32 eaten = 0;

    if (is_alpha_char(str[eaten]) || str[eaten] == '_') {
        ++eaten;
        while (is_alpha_char(str[eaten]) || is_number_char(str[eaten]) || str[eaten] == '_') {
            ++eaten;
        }
        return eaten;
    } else {
        return 0;
    }
}

u32 eat_string(const char* str) {
    // NOTE(Felix): Assumes we are on a " or '
    char to_search = *str;

    u32 eaten = 1;
    bool escaped = false;
    while (str[eaten] && (str[eaten] != to_search || escaped)) {

        if (str[eaten] == '\\') {
            if (escaped)
                escaped = false;
            else
                escaped = true;

        } else {
            if (escaped)
                escaped = false;
        }
        ++eaten;
    }
    if (str[eaten])
        ++eaten; // eat the closing quote as well
    return eaten;
}

u32 read_string(const char* str, String* out_string, Allocator_Base* allocator) {
    if (allocator == nullptr)
        allocator = grab_current_allocator();

    // NOTE(Felix): Assumes we are on a " or '
    u32 length = eat_string(str);

    // TODO(Felix): unescape string
    out_string->data = heap_copy_limited_c_string(str+1, length-2, allocator); // quotes are not part of the string
    out_string->length = length-2;

    return length;
}

u32 read_identifier(const char* str, String* out_string, Allocator_Base* allocator) {
    if (allocator == nullptr)
        allocator = grab_current_allocator();

    // NOTE(Felix): Assumes we are on the first letter
    u32 length = eat_identifier(str);

    // TODO(Felix): unescape string
    out_string->data = heap_copy_limited_c_string(str, length, allocator);
    out_string->length = length;

    return length;
}

u32 read_long(const char* str, s64* out_long) {
    u32 quotes_chars = 0;

    bool in_on_quotes = is_quotes_char(*str);
    if (in_on_quotes)
        quotes_chars = 1;

    char* end;
    s64 result = strtoll(str+quotes_chars, &end, 10);
    *out_long = result;

    if (in_on_quotes) {
        panic_if(!is_quotes_char(end[0]),
                 "Expected a quote here but got |%s|",
                 end);

        quotes_chars = 2;
    }

    return (u32)(end-str + quotes_chars);
}

u32 read_int(const char* str, s32* out_int) {
    s64 l;
    u32 read = read_long(str, &l);
    *out_int = (s32)l;
    return read;
}

u32 read_float(const char* str, f32* out_float) {
    char* end;
    *out_float = strtof(str, &end);
    return (u32)(end-str);
}

u32 read_bool(const char* str, bool* out_bool) {
    char true_str[] = "true";
    s32  true_str_len = sizeof(true_str)-1;

    *out_bool = strncmp(str, true_str, true_str_len) == 0;

    if (*out_bool)
        return true_str_len;

    u32 eaten = 0;
    while (is_alpha_char(str[eaten]))
        ++eaten;

    return eaten;
}
#endif
