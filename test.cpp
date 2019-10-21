#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "bucket_allocator.hpp"

int main(int argc, char* argv[]) {
    Bucket_Allocator<int, 3> ba;

    int* a = ba.allocate();
    *a = 1;
    printf("%d\n", *a);

    ba.free(a);

    int* b = ba.allocate();
    *b = 2;
    printf("%d\n", *b);

    int* c = ba.allocate();
    *c = 3;
    printf("%d\n", *c);

    int* d = ba.allocate();
    *d = 4;
    printf("%d\n", *d);

    int* e = ba.allocate();
    *e = 5;
    printf("%d\n", *e);

    int* f = ba.allocate();
    *f = 6;
    printf("%d\n", *f);

    return 0;
}
