#pragma once

#include <stdio.h>
#include <string.h>
#include <time.h>

struct Profiler {
    inline thread_local static int   thread_id = 0;
    inline thread_local static FILE* out_file = nullptr;
    inline static char file_template[40] = "\0";

    Profiler(const char* file, const char* func, const int line) {
        if (!out_file) {
            time_t t = time(NULL);
            tm tm_i;
            localtime_s(&tm_i, &t);

            if (!file_template[0]) {
                sprintf(file_template, "%02d.%02d.%04d-%02d.%02d.%02d-%%02d-profiler.report",
                        tm_i.tm_mday, tm_i.tm_mon+1, tm_i.tm_year+1900,
                        tm_i.tm_hour, tm_i.tm_min, tm_i.tm_sec);
            }

            char file_name[38];
            sprintf(file_name, file_template, thread_id);
            fopen_s(&out_file, file_name, "w");
            if (!out_file) {
                printf("could not open %s\n", file_name);
            }

        }

        fprintf(out_file, "-> %s %s %d\n", func, file, line);
    };
    ~Profiler() {
        fprintf(out_file, "<-\n");
    };
};

#define profile_this Profiler profiler(__FILE__, __FUNCTION__, __LINE__)
