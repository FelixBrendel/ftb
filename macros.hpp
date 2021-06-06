#pragma once
#include "platform.hpp"

#ifdef FTB_WINDOWS
#else
#  include <signal.h> // for sigtrap
#endif

#include <functional>

#define proc auto
#define concat(x, y) x ## y
#define label(x, y) concat(x, y)

#define MPI_LABEL(id1,id2)                                      \
    label(MPI_LABEL_ ## id1 ## _ ## id2 ## _, __LINE__)

#define MPP_DECLARE(labid, declaration)                 \
    if (0)                                              \
        ;                                               \
    else                                                \
        for (declaration;;)                             \
            if (1) {                                    \
                goto MPI_LABEL(labid, body);            \
              MPI_LABEL(labid, done): break;            \
            } else                                      \
                while (1)                               \
                    if (1)                              \
                        goto MPI_LABEL(labid, done);    \
                    else                                \
                        MPI_LABEL(labid, body):

#define MPP_BEFORE(labid,before)                \
    if (1) {                                    \
        before;                                 \
        goto MPI_LABEL(labid, body);            \
    } else                                      \
    MPI_LABEL(labid, body):

#define MPP_AFTER(labid,after)                  \
    if (1)                                      \
        goto MPI_LABEL(labid, body);            \
    else                                        \
        while (1)                               \
            if (1) {                            \
                after;                          \
                break;                          \
            } else                              \
                MPI_LABEL(labid, body):


// #ifndef min
// #define min(a, b) (((a) < (b)) ? (a) : (b))
// #endif
// #ifndef max
// #define max(a, b) (((a) > (b)) ? (a) : (b))
// #endif

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
                                     ;


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

#define panic(...)                                                          \
    do {                                                                    \
        print("%{color<}[Panic]%{>color} in "                               \
              "file %{color<}%{->char}%{>color} "                           \
              "line %{color<}%{u32}%{>color}: "                             \
              "(%{color<}%{->char}%{>color})\n",                            \
              console_red, console_cyan, __FILE__, console_cyan, __LINE__,  \
              console_cyan, __func__);                                      \
        print("%{color<}", console_red);                                    \
        print(__VA_ARGS__);                                                 \
        print("%{>color}\n");                                               \
        fflush(stdout);                                                     \
        print_stacktrace();                                                 \
        debug_break();                                                      \
    } while(0)


#define panic_if(cond, ...)                     \
    if(!(cond));                                \
    else panic(__VA_ARGS__)

#ifdef FTB_DEBUG_LOG
#  define debug_log(...)                                        \
    do {                                                        \
        print("%{color<}[INFO " __FILE__ ":%{color<}%{->char}"  \
              "%{>color}]%{>color} ",                           \
              console_green, console_cyan, __func__);           \
        println(__VA_ARGS__);                                   \
    } while (0)
#else
#  define debug_log(...)
#endif

#ifdef FTB_WINDOWS
#  define debug_break() __debugbreak()
#else
#  define debug_break() raise(SIGTRAP);
#endif
