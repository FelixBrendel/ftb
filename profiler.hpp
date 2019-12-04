#pragma once
#ifdef _PROFILING
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <time.h>
// # include <bits/stdc++.h>
// # include <iostream>
// # include <sys/stat.h>
// # include <sys/types.h>
#ifdef _MSC_VER
// if windows
# include <Windows.h>
#else
# include <sys/time.h>
#endif


struct Profiler {
    LARGE_INTEGER tmp_time;

    // same for all threads
    inline static char file_template[40] = "\0";

    // thread local
    inline thread_local static bool   is_initialized = false;
    inline thread_local static size_t thread_id = -1;
    inline thread_local static int    call_depth = 0;
    inline thread_local static FILE*  out_file = nullptr;


    Profiler(const char* file, const char* name, const int line, const char* comment1, const char* comment2) {
        call_depth += 1;

        // if we never used this thread before
        if (!is_initialized) {
            thread_id = (size_t)&thread_id;
            time_t t = time(NULL);
            tm* tm_i = localtime(&t);

            // create folder
#ifdef _MSC_VER
            _mkdir("./profiler_reports/");
#else
            mkdir("./profiler_reports/", S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif

            // if we never even created the shared file name template
            if (!file_template[0]) {
                // make sure folder exists
                sprintf(file_template, "./profiler_reports/%02d.%02d.%04d-%02d.%02d.%02d-%%zd-profiler.report",
                        tm_i->tm_mday, tm_i->tm_mon+1, tm_i->tm_year+1900,
                        tm_i->tm_hour, tm_i->tm_min, tm_i->tm_sec);
            }

            char file_name[100];
            sprintf(file_name, file_template, thread_id);
            // printf("Hello I am %zd\n", thread_id);
            out_file = fopen(file_name, "w");
            if (!out_file) {
                printf("could not open %s\n", file_name);
            }

            // initially write the performance frequency
            LARGE_INTEGER pf;
            QueryPerformanceFrequency(&pf);
            fprintf(out_file, "%lld,,,,\n", pf.QuadPart);

            is_initialized = true;
        }
        QueryPerformanceCounter(&tmp_time);
        fprintf(out_file, "->,%lld,%s,%s,%d,%s,%s\n",
                tmp_time.QuadPart, name, file,
                line, comment1, comment2);
    };

    ~Profiler() {
        call_depth -= 1;
        QueryPerformanceCounter(&tmp_time);
        fprintf(out_file, "<-,%lld,,,,,\n", tmp_time.QuadPart);
        if (call_depth == 0)
            fflush(out_file);
    };
};

# define profile_this()                             Profiler profiler(__FILE__, __FUNCTION__, __LINE__, "", "")
# define profile_with_name(name)                    Profiler profiler(__FILE__,         name, __LINE__, "", "")
# define profile_with_comment(c1)                   Profiler profiler(__FILE__, __FUNCTION__, __LINE__, c1, "")
# define profile_with_comments(c1,c2)               Profiler profiler(__FILE__, __FUNCTION__, __LINE__, c1, c2)
# define profile_with_name_and_comment(name,c1)     Profiler profiler(__FILE__,         name, __LINE__, c1, "")
# define profile_with_name_and_comments(name,c1,c2) Profiler profiler(__FILE__,         name, __LINE__, c1, c2)
#else
# define profile_this()                             do {} while(0)
# define profile_with_name(name)                    do {} while(0)
# define profile_with_comment(c1)                   do {} while(0)
# define profile_with_comments(c1,c2)               do {} while(0)
# define profile_with_name_and_comment(name,c1)     do {} while(0)
# define profile_with_name_and_comments(name,c1,c2) do {} while(0)
#endif
