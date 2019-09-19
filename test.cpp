#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "profiler.hpp"
#include "macros.hpp"

int var = 100;

void test() {
    profile_this;
    printf("var is %d\n", var);

    fluid_let (var, 200) {
        printf("var is %d\n", var);
    }

}

int main(int argc, char* argv[]) {
    profile_this;

    test();
    printf("var is %d\n", var);

    return 0;
}
