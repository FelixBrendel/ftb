#pragma once
#include "platform.hpp"
#include "types.hpp"
#include "print.hpp"

#ifdef FTB_LINUX
#  include <time.h>
#endif

struct Time_Stamp {
#ifdef FTB_WINDOWS
#else
    struct timespec t;
#endif
};

Time_Stamp start_timer();
u64        stop_timer(Time_Stamp); // returns the elapsed time in µ-seconds

struct Timer {
    Time_Stamp start;
    const char* name;
    const char* file;
    u32   line;
    Timer(const char* p_file, const u32 p_line, const char* p_name = nullptr) {
        file   = p_file;
        line   = p_line;
        name   = p_name;
        start = start_timer();
    }
    ~Timer() {
        u64 nanos = stop_timer(start);
        if (name)
            println("%{color<}[PROFILE] %s (%s:%i) took %.2f ms %{>color}", console_green_bold, name, file, line, nanos / 1.0e6); \
        else
            println("%{color<}[PROFILE] Block at %s:%i took %.2f ms %{>color}", console_green_bold, file, line, nanos / 1.0e6); \
    }
};

#define profile_block             MPP_DECLARE(1, Timer MPI_LABEL(labid, timer)(__FILE__,__LINE__))
#define profile_function          Timer MPI_LABEL(labid, timer)(__FILE__,__LINE__,__FUNCTION__)
#define profile_named_block(name) MPP_DECLARE(1, Timer MPI_LABEL(labid, timer)(__FILE__,__LINE__,name))

#ifdef FTB_DEBUG
#  define debug_profile_block             profile_block
#  define debug_profile_named_block(name) profile_named_block(name)
#  define debug_profile_function          profile_function
#else
#  define debug_profile_block
#  define debug_profile_named_block(name)
#  define debug_profile_function
#endif




#ifdef FTB_PROFILER_IMPL
#  ifdef FTB_WINDOWS

Time_Stamp start_timer() {}
u64   stop_timer(Time_Stamp) {} // returns the elapsed time in nano-seconds

#  else
// LINUX

Time_Stamp start_timer() {
    Time_Stamp t;
    clock_gettime(CLOCK_MONOTONIC, &t.t);
    return t;
}
u64 stop_timer(Time_Stamp then) {
    Time_Stamp now = start_timer();
    if ((now.t.tv_nsec-then.t.tv_nsec)<0) {
        return 1e9 * (now.t.tv_sec-then.t.tv_sec-1) + (1e9+now.t.tv_nsec-then.t.tv_nsec);
    } else {
        return 1e9 * (now.t.tv_sec-then.t.tv_sec) + (now.t.tv_nsec-then.t.tv_nsec);
    }
}

#  endif // platform
#endif // impl
