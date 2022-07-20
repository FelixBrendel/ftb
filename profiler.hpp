#pragma once
#include "platform.hpp"
#include "types.hpp"
#include "print.hpp"

#ifdef FTB_LINUX
#  include <time.h>
#else
// # include <Windows.h>
#endif


struct Time_Stamp {
#ifdef FTB_WINDOWS
    LARGE_INTEGER t;
#else
    struct timespec t;
#endif
};

Time_Stamp start_timer();
u64        stop_timer(Time_Stamp); // returns the elapsed time in nano-seconds

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
            println("%{color<}[PROFILE] %s%{>color} (%s:%i)\n -> took %{color<}%.2fms%{>color}", console_green_bold, name, file, line, console_green_bold, nanos / 1.0e6); \
        else
            println("%{color<}[PROFILE]%{>color} Block at %s:%i\n -> took %{color<}%.2fms%{>color}", console_green_bold, file, line, console_green_bold, nanos / 1.0e6); \
    }
};

#define profile_block             MPP_DECLARE(1, Timer MPI_LABEL(labid, timer)(__FILE__,__LINE__))
#define profile_named_block(name) MPP_DECLARE(1, Timer MPI_LABEL(labid, timer)(__FILE__,__LINE__,name))
#define profile_function          Timer MPI_LABEL(labid, timer)(__FILE__,__LINE__,__FUNCTION__)

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

Time_Stamp start_timer() {
    Time_Stamp t {};
    bool success = QueryPerformanceCounter(&t.t);
    debug_assert(success);

    return t;
}
u64   stop_timer(Time_Stamp stamp) { // returns the elapsed time in nano-seconds
    LARGE_INTEGER freq;

    // NOTE(Felix): Docs say: [QueryPerformanceFrequency] retrieves the
    //   frequency of the performance counter. The frequency of the performance
    //   counter is fixed at system boot and is consistent across all
    //   processors. Therefore, the frequency need only be queried upon
    //   application initialization, and the result can be cached.
    //   (https://docs.microsoft.com/en-us/windows/win32/api/profileapi/nf-profileapi-queryperformancefrequency)

    bool success = QueryPerformanceFrequency(&freq);
    debug_assert(success);

    Time_Stamp now = start_timer();

    LONGLONG diff = now.t.QuadPart - stamp.t.QuadPart;

    return diff * 1'000'000'000 / freq.QuadPart;
}

#  else
// LINUX

Time_Stamp start_timer() {
    Time_Stamp t;
    clock_gettime(CLOCK_MONOTONIC, &t.t);
    return t;
}
u64 stop_timer(Time_Stamp then) { // returns the elapsed time in nano-seconds
    Time_Stamp now = start_timer();
    if ((now.t.tv_nsec-then.t.tv_nsec)<0) {
        return 1e9 * (now.t.tv_sec-then.t.tv_sec-1) + (1e9+now.t.tv_nsec-then.t.tv_nsec);
    } else {
        return 1e9 * (now.t.tv_sec-then.t.tv_sec) + (now.t.tv_nsec-then.t.tv_nsec);
    }
}

#  endif // platform
#endif // impl
