#pragma once
#include <stdio.h>
#include "types.hpp"

#define EXPAND( x ) x
#define CAT( A, B ) A ## B
#define SELECT( NAME, NUM ) CAT( NAME ## _, NUM )

#define GET_COUNT( _1, _2, _3, _4, _5, _6, _7, _8, _9, _10, _11, _12 /* ad nauseam */, COUNT, ... ) COUNT
#define VA_SIZE( ... ) EXPAND(GET_COUNT( __VA_ARGS__, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1 ))
#define VA_SELECT( NAME, ... ) SELECT( NAME, VA_SIZE(__VA_ARGS__) )(__VA_ARGS__)


#define mprint_1(_) \
    constexpr int format_count = count_formats(_);                      \
    static_assert(!(format_count > 11),   "mprint only supports 11 args now"); \
    static_assert(!(format_count < argc), "Too many args supplied to mprint"); \
    static_assert(!(format_count > argc), "Too few args supplied to mprint"); \
    return actual_print(_, values, printers)

#define define_stuff(arg, num)                                          \
    auto val_##num = arg;                                               \
    values[num] = &val_##num;                                           \
    printers[num] = (printer_function)(int(*)(decltype(val_##num)*))(print)

#define mprint_2(_, v0)                                                     define_stuff((v0), 0);   mprint_1(_)
#define mprint_3(_, v0, v1)                                                 define_stuff((v1), 1);   mprint_2(_, v0)
#define mprint_4(_, v0, v1, v2)                                             define_stuff((v2), 2);   mprint_3(_, v0, v1)
#define mprint_5(_, v0, v1, v2, v3)                                         define_stuff((v3), 3);   mprint_4(_, v0, v1, v2)
#define mprint_6(_, v0, v1, v2, v3, v4)                                     define_stuff((v4), 4);   mprint_5(_, v0, v1, v2, v3)
#define mprint_7(_, v0, v1, v2, v3, v4, v5)                                 define_stuff((v5), 5);   mprint_6(_, v0, v1, v2, v3, v4)
#define mprint_8(_, v0, v1, v2, v3, v4, v5, v6)                             define_stuff((v6), 6);   mprint_7(_, v0, v1, v2, v3, v4, v5)
#define mprint_9(_, v0, v1, v2, v3, v4, v5, v6, v7)                         define_stuff((v7), 7);   mprint_8(_, v0, v1, v2, v3, v4, v5, v6)
#define mprint_10(_, v0, v1, v2, v3, v4, v5, v6, v7, v8)                    define_stuff((v8), 8);   mprint_9(_, v0, v1, v2, v3, v4, v5, v6, v7)
#define mprint_11(_, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9)                define_stuff((v9), 9);   mprint_10(_, v0, v1, v2, v3, v4, v5, v6, v7, v8)
#define mprint_12(_, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10)           define_stuff((v10), 10); mprint_11(_, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9)
#define mprint_13(_, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)      define_stuff((v11), 11); mprint_12(_, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10)
#define mprint_14(_, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11, v12) define_stuff((v12), 12); mprint_13(_, v0, v1, v2, v3, v4, v5, v6, v7, v8, v9, v10, v11)

#define mprint(...) [&]() -> int {                                      \
        constexpr int argc = VA_SIZE(__VA_ARGS__)-1;                    \
        void*              values[argc == 0  ? 1 : argc];               \
        printer_function printers[argc == 0  ? 1 : argc];               \
        VA_SELECT(mprint, __VA_ARGS__);                                 \
    }()

#define mprintln(...) [&]() -> int {                   \
        int sum = mprint(__VA_ARGS__) + putc('\n', stdout);   \
        fflush(stdout);                                \
        return sum;                                    \
    }()

typedef int (*printer_function)(void*);

constexpr int count_formats(const char* str) {
    int result = 0;
    while (*str) {
        if (*str == '%') ++result;
        ++str;
    }
    return result;
}

int actual_print(const char* format, void** argv, printer_function* printers) {
    int idx = 0;
    int sum = 0;
    while (*format != 0) {
        if (*format == '%') {
            sum += (printers[idx])(argv[idx]);
            idx++;
        } else {
            putc(*format, stdout);
            sum += 1;
        }
        format++;
    }
    return sum;
}

// colors
enum struct Console_Color {
    pop_color,
    push_red,
    push_green,
    push_yellow,
    push_blue,
    push_magenta,
    push_cyan,
    push_white,
    push_normal,
};


int print(Console_Color* arg) {
    static const char* color_stack[6];
    static int color_stack_idx = 0;

    const char* console_red     = "\x1B[31m";
    const char* console_green   = "\x1B[32m";
    const char* console_yellow  = "\x1B[33m";
    const char* console_blue    = "\x1B[34m";
    const char* console_magenta = "\x1B[35m";
    const char* console_cyan    = "\x1B[36m";
    const char* console_white   = "\x1B[37m";
    const char* console_normal  = "\x1B[0m";



    if (*arg == Console_Color::pop_color) {
        if (color_stack_idx <= 1) {
            color_stack_idx = 0;
            return printf("%s", console_normal);
        } else {
            --color_stack_idx;
            return printf("%s", color_stack[color_stack_idx-1]);
        }
    }

    switch (*arg) {
        case Console_Color::push_red:     color_stack[color_stack_idx] = console_red;     break;
        case Console_Color::push_green:   color_stack[color_stack_idx] = console_green;   break;
        case Console_Color::push_yellow:  color_stack[color_stack_idx] = console_yellow;  break;
        case Console_Color::push_blue:    color_stack[color_stack_idx] = console_blue;    break;
        case Console_Color::push_magenta: color_stack[color_stack_idx] = console_magenta; break;
        case Console_Color::push_cyan:    color_stack[color_stack_idx] = console_cyan;    break;
        case Console_Color::push_white:   color_stack[color_stack_idx] = console_white;   break;
        case Console_Color::push_normal:  color_stack[color_stack_idx] = console_normal;  break;
        default: break;
    }

    int ret = printf("%s", color_stack[color_stack_idx]);

    if (color_stack_idx < 5)
        color_stack_idx++;

    return ret;
}

// start of default printers
int print(int* arg) {
    return printf("%d", *arg);
}

int print(unsigned int* arg) {
    return printf("%u", *arg);
}

int print(short* arg) {
    return printf("%d", *arg);
}

int print(unsigned short* arg) {
    return printf("%u", *arg);
}

int print(long* arg) {
    return printf("%ld", *arg);
}

int print(unsigned long* arg) {
    return printf("%lu", *arg);
}

int print(long long* arg) {
    return printf("%lld", *arg);
}

int print(unsigned long long* arg) {
    return printf("%llu", *arg);
}

int print(float* arg) {
    return printf("%f", *arg);
}

int print(double* arg) {
    return printf("%f", *arg);
}

int print(long double* arg) {
    return printf("%Lf", *arg);
}

int print(bool* arg) {
    return printf("%s", *arg ? "true" : "false");
}

int print(unsigned char* arg) {
    return printf("%u", (unsigned)*arg);
}


int print(char* arg) {
    return printf("%c", *arg);
}

int print(const char** arg) {
    return printf("%s", *arg);
}

int print(char** arg) {
    return printf("%s", *arg);
}

int print(void** arg) {
    return printf("%p", *arg);
}

int print(Ftb_String* arg) {
    return printf("%s", arg->data);
}

template <typename type>
int print(Array_Pullover<type>* arg) {
    int result = 0;
    for (auto e : *arg) {
        result += print(&e);
        result += printf(", ");
    }
    return result;
}


template <typename missing_printer_for>
int print(missing_printer_for) {
    static_assert(false, "No overload exists for this type.");
    return 0;
}
