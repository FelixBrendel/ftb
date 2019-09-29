#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "profiler.hpp"
#include "macros.hpp"
#include "hashmap.hpp"

int main(int argc, char* argv[]) {

    StringHashMap* hm = create_hashmap();
    hm_set(hm, (char*)"=", (void*)1);
    hm_set(hm, (char*)"<", (void*)2);
    hm_set(hm, (char*)"<=", (void*)2);

    return 0;
}
