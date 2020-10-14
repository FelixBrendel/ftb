#pragma once
#include <functional>

#define proc auto
#define concat(x, y) x ## y
#define label(x, y) concat(x, y)
#define line_label(x) label(x, __LINE__)


/**
 *   Defer   *
 */
template<typename F>
class defer_finalizer {
    F f;
    bool moved;
public:
    template<typename T>
    defer_finalizer(T && f_) : f(std::forward<T>(f_)), moved(false) { }

    defer_finalizer(const defer_finalizer &) = delete;

    defer_finalizer(defer_finalizer && other) : f(std::move(other.f)), moved(other.moved) {
        other.moved = true;
    }

    ~defer_finalizer() {
        if (!moved) f();
    }
};

static struct {
    template<typename F>
    defer_finalizer<F> operator<<(F && f) {
        return defer_finalizer<F>(std::forward<F>(f));
    }
} deferrer;

#define defer auto label(__deferred_lambda_call, __COUNTER__) = deferrer << [&]

#define defer_free(var) defer { free(var); }


/*
   defer {
       call();
    };

expands to:

    auto __deferred_lambda_call0 = deferrer << [&] {
       call();
    };
*/

#if defined(unix) || defined(__unix__) || defined(__unix)
#define NULL_HANDLE "/dev/null"
#else
#define NULL_HANDLE "nul"
#endif
#define ignore_stdout                                                   \
    if (0)                                                              \
        label(finished,__LINE__): ;                                     \
    else                                                                \
        for (FILE* label(fluid_let_, __LINE__) = ftb_stdout;;)                             \
            for (defer{ fclose(ftb_stdout); ftb_stdout=label(fluid_let_, __LINE__) ; } ;;) \
                if (1) {                                                \
                    ftb_stdout = fopen(NULL_HANDLE, "w");               \
                    goto label(body,__LINE__);                          \
                }                                                       \
                else                                                    \
                    while (1)                                           \
                        if (1) {                                        \
                            goto label(finished, __LINE__);             \
                        }                                               \
                        else label(body,__LINE__):


/*****************
 *   fluid-let   *
 *****************/

#define fluid_let(var, val)                                             \
    if (0)                                                              \
        label(finished,__LINE__): ;                               \
    else                                                                \
        for (auto label(fluid_let_, __LINE__) = var;;)            \
            for (defer{var = label(fluid_let_, __LINE__);};;)     \
                for(var = val;;)                                        \
                    if (1) {                                            \
                        goto label(body,__LINE__);                \
                    }                                                   \
                    else                                                \
                        while (1)                                       \
                            if (1) {                                    \
                                goto TOKENPASTE2(finished, __LINE__);   \
                            }                                           \
                            else TOKENPASTE2(body,__LINE__):


/**
fluid_let(var, val) {
    call1(var);
    call2(var);
}

expands to

if (0)
    finished98:;
else
    for (auto fluid_let_98 = var;;)
        for (auto __deferred_lambda_call0 = deferrer << [&] { var = fluid_let_98; };;)
            for (var = val;;)
                if (1) {
                    goto body98;
                } else
                    while (1)
                        if (1) {
                            goto finished98;
                        } else
                          body98 : {
                              call1(var);
                              call2(var);
                          }
*/
