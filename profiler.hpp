#pragma once
#ifdef _PROFILING
# include <stdlib.h>
# include <stdio.h>
# include <string.h>
# include <time.h>
// for syscall:
# include <unistd.h>
# include <sys/syscall.h>

struct Profiler {
    // same for all threads
    inline static char file_template[40] = "\0";

    // thread local
    inline thread_local static bool  is_initialized = false;
    inline thread_local static int   thread_id = -1;
    inline thread_local static int   call_depth = 0;
    inline thread_local static FILE* out_file = nullptr;
    Profiler(const char* file, const char* func, const int line) {
        call_depth += 1;

        // if we never used this thread before
        if (!is_initialized) {
            thread_id = syscall(__NR_gettid);
            printf("Hello I am %d\n", thread_id);
            time_t t = time(NULL);
            tm* tm_i = localtime(&t);

            // if we never even created the shared file name template
            if (!file_template[0]) {
                sprintf(file_template, "%02d.%02d.%04d-%02d.%02d.%02d-%%02d-profiler.report",
                        tm_i->tm_mday, tm_i->tm_mon+1, tm_i->tm_year+1900,
                        tm_i->tm_hour, tm_i->tm_min, tm_i->tm_sec);
            }


            char file_name[38];
            sprintf(file_name, file_template, thread_id);
            out_file = fopen(file_name, "w");
            if (!out_file) {
                printf("could not open %s\n", file_name);
            }

            is_initialized = true;
        }

        fprintf(out_file, "-> %s %s %d\n", func, file, line);
    };
    ~Profiler() {
        call_depth -= 1;
        fprintf(out_file, "<-\n");
        if (call_depth == 0)
            fflush(out_file);
    };
};

# define profile_this Profiler profiler(__FILE__, __FUNCTION__, __LINE__)
#else
# define profile_this enum {}
#endif
