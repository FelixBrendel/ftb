#pragma once
#include <stdio.h>
#include "print.hpp"
#include "macros.hpp"
#include "stacktrace.hpp"

#ifndef FTB_IO_IMPL

auto read_entire_file(const char* filename) -> String;

#else // implementations

auto read_entire_file(const char* filename) -> String  {
    String ret;
    ret.data = nullptr;
    FILE *fp = fopen(filename, "rb");
    if (fp) {
        /*k Go to the end of the file. */
        if (fseek(fp, 0L, SEEK_END) == 0) {
            /* Get the size of the file. */
            ret.length = ftell(fp) + 1;
            if (ret.length == 0) {
                fputs("Empty file", stderr);
                goto closeFile;
            }

            /* Go back to the start of the file. */
            if (fseek(fp, 0L, SEEK_SET) != 0) {
                panic("Error reading file");
                goto closeFile;
            }

            /* Allocate our buffer to that size. */
            ret.data = (char*)calloc(ret.length, sizeof(char));

            /* Read the entire file into memory. */
            ret.length = fread(ret.data, sizeof(char), ret.length, fp);

            ret.data[ret.length] = '\0';
            if (ferror(fp) != 0) {
                panic("Error reading file");
            }
        }
    closeFile:
        fclose(fp);
    } else {
        panic("Cannot read file: %s", filename);
    }

    return ret;
    /* Don't forget to call free() later! */
}

#endif // FTB_IO_IMPL
