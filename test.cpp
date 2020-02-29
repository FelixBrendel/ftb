#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "./hooks.hpp"

Hook h;
int main(int argc, char* argv[]) {
    printf("Hello world");
    system_shutdown_hook << [] {
        printf("Goodbye world\n");
    };

    h << []{
        printf("Hallo1");
    };
    h << [] {
       printf("Hallo2");
    };

    h << [] {
        printf("Hallo3");
    };
    h();

    return 0;
}
