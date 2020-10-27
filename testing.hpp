#include "./types.hpp"

typedef s32 testresult;

#define epsilon 2.2204460492503131E-16
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

#define assert_no_error()                                               \
    if (error) {                                                        \
        print_assert_equal_fail(error, 0, size_t, "%{u64}");            \
        printf("\nExpected no error to occur,"                          \
               " but an error occured anyways:\n");                     \
        return fail;                                                    \
    }                                                                   \

#define assert_error()                                                  \
    if (!error) {                                                       \
        print_assert_not_equal_fail(error, 0, void*, "%{->}");          \
        printf("\nExpected an error to occur,"                          \
               " but no error occured:\n");                             \
        return fail;                                                    \
    }                                                                   \

#define assert_equal_f64(variable, value)                           \
    if (fabsl((f64)variable - (f64)value) > epsilon) {              \
        print_assert_equal_fail(variable, value, f64, "%{f64}");    \
        return fail;                                                \
    }

#define assert_not_equal_f64(variable, value)                           \
    if (fabsl((f64)variable - (f64)value) <= epsilon) {                 \
        print_assert_not_equal_fail(variable, value, f64, "%{f64}");    \
        return fail;                                                    \
    }

#define assert_null(variable)                   \
    assert_equal_int(variable, nullptr)

#define assert_not_null(variable)               \
    assert_not_equal_int(variable, nullptr)

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
        if(error) {                                             \
            free(error);                                        \
            error = nullptr;                                    \
        }                                                       \
    }
