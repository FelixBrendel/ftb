#pragma once
#include <functional>

#define proc auto

#ifdef _DEBUG
# define if_debug if constexpr (true)
#else
# define if_debug if constexpr (false)
#endif

#ifdef _MSC_VER
# define debug_break() if_debug __debugbreak()
# define if_windows if constexpr (true)
# define if_linux   if constexpr (false)
#else
# define debug_break() if_debug raise(SIGTRAP)
# define if_windows if constexpr (false)
# define if_linux   if constexpr (true)
#endif

#define TOKENPASTE(x, y) x ## y
#define TOKENPASTE2(x, y) TOKENPASTE(x, y)


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

struct {
    template<typename F>
    defer_finalizer<F> operator<<(F && f) {
        return defer_finalizer<F>(std::forward<F>(f));
    }
} deferrer;

#define defer auto TOKENPASTE2(__deferred_lambda_call, __COUNTER__) = deferrer << [&]


/**
 *   fluid-let   *
 */

#define fluid_let(var, val)                                             \
    if (0)                                                              \
        TOKENPASTE2(finished,__LINE__): ;                               \
    else                                                                \
        for (auto TOKENPASTE2(fluid_let_, __LINE__) = var;;)            \
            for(var = val;;)                                            \
                if (1) {                                                \
                    goto TOKENPASTE2(body,__LINE__);                    \
                }                                                       \
                else                                                    \
                    while (1)                                           \
                        if (1) {                                        \
                            var = TOKENPASTE2(fluid_let_, __LINE__);    \
                            goto TOKENPASTE2(finished, __LINE__);       \
                        }                                               \
                        else TOKENPASTE2(body,__LINE__):