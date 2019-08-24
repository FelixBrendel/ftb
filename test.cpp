#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>

#include "profiler.h"
#include "macros.h"

void test() {
    profile_this;

    printf("doing more work!\n");
}

int main(int argc, char* argv[]) {
    profile_this;

    printf("doing some work and sleeping!\n");
    Sleep(1000); // Sleep a seconds
    test();

    return 0;
}
