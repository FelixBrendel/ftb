#define _CRT_SECURE_NO_WARNINGS

#include "profiler.hpp"
#include "macros.h"


int var = 100;

void test() {
    profile_this;
    printf("var is %d\n", var);

    fluid_let (var, 200) {
        printf("var is %d\n", var);
    }

    printf("var is %d\n", var);
}

int main(int argc, char* argv[]) {
    profile_this;

    test();

    return 0;
}
