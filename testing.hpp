#include <math.h>
#include "./types.hpp"
#include "./allocation_stats.hpp"

typedef s32 testresult;

#define f64_epsilon DBL_EPSILON
#define f32_epsilon FLT_EPSILON
#define pass 1
#define fail 0

#define print_assert_equal_fail(variable, value, type, format)  \
    print("\n%s:%d: Assertion failed\n\tfor '" #variable "'"    \
          "\n\texpected: " format                               \
          "\n\tgot:      " format "\n",                         \
          __FILE__, __LINE__, (type)value, (type)variable)

#define print_assert_not_equal_fail(variable, value, type, format)      \
    print("\n%s:%d: Assertion failed\n\tfor '" #variable "'"            \
          "\n\texpected not: " format                                   \
          "\n\tgot anyways:  " format "\n",                             \
          __FILE__, __LINE__, (type)(value), (type)(variable))

#define assert_equal_string(variable, value)                              \
    do {                                                                  \
        auto v1{variable};                                                \
        auto v2{value};                                                   \
        if (!string_equal(v1, v2)) {                                      \
            print_assert_equal_fail(&(v1), &(v2), String*, "%{->Str}");   \
            return fail;                                                  \
        }                                                                 \
    } while (0)


#define assert_equal_int(variable, value)                               \
    if (variable != value) {                                            \
        print_assert_equal_fail(variable, value, size_t, "%{u64}");     \
        return fail;                                                    \
    }

#define assert_not_equal_int(variable, value)                           \
    if (variable == value) {                                            \
        print_assert_not_equal_fail(variable, value, size_t, "%{u64}"); \
        return fail;                                                    \
    }


#define assert_equal_f32(variable, value)                           \
    if (fabsl((f32)variable - (f32)value) > f32_epsilon) {          \
        print_assert_equal_fail(variable, value, f32, "%{f32}");    \
        return fail;                                                \
    }

#define assert_not_equal_f32(variable, value)                           \
    if (fabsl((f32)variable - (f32)value) <= f32_epsilon) {             \
        print_assert_not_equal_fail(variable, value, f32, "%{f32}");    \
        return fail;                                                    \
    }

#define assert_equal_f64(variable, value)                           \
    if (fabsl((f64)variable - (f64)value) > f64_epsilon) {          \
        print_assert_equal_fail(variable, value, f64, "%{f64}");    \
        return fail;                                                \
    }

#define assert_not_equal_f64(variable, value)                           \
    if (fabsl((f64)variable - (f64)value) <= f64_epsilon) {             \
        print_assert_not_equal_fail(variable, value, f64, "%{f64}");    \
        return fail;                                                    \
    }

#define assert_null(variable)                   \
    assert_equal_int(variable, nullptr)

#define assert_not_null(variable)               \
    assert_not_equal_int(variable, nullptr)

#define assert_true(value)                      \
    assert_equal_int(value, true)

#define invoke_test(name)                                       \
    fputs("" #name ":", stdout);                                \
    if (name() == pass) {                                       \
        for(size_t i = strlen(#name); i < 70; ++i)              \
            fputs((i%3==1)? "." : " ", stdout);                 \
        fputs(console_green "passed\n" console_normal, stdout); \
    }                                                           \
    else {                                                      \
        result = false;                                         \
        for(s32 i = -1; i < 70; ++i)                            \
            fputs((i%3==1)? "." : " ", stdout);                 \
        fputs(console_red "failed\n" console_normal, stdout);   \
    }
