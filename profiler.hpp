/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2022, Felix Brendel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include "core.hpp"

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

        if (name)
            println("%{color<}[PROFILE] %s%{>color} (%s:%i)", console_green_bold, name, file, line);
        else
            println("%{color<}[PROFILE]%{>color} Block at %s:%i", console_green_bold, file, line);
        push_print_prefix("|   ");


        start = start_timer();
    }
    ~Timer() {
        u64 nanos = stop_timer(start);

        pop_print_prefix();
        println("-> took %{color<}%.2fms%{>color}", console_green_bold, nanos / 1.0e6);
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
