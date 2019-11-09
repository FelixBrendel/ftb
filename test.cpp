#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "bucket_allocator.hpp"

int main(int argc, char* argv[]) {
    Bucket_Allocator<int> ba(2, 1);

    int* a = ba.allocate();
    *a = 1;
    ba.free_object(a);

    int* b = ba.allocate();
    *b = 2;

    int* c = ba.allocate();
    *c = 3;

    int* d = ba.allocate();
    *d = 4;

    int* e = ba.allocate();
    *e = 5;

    int* f = ba.allocate();
    *f = 6;

    ba.for_each([](int* i){
        printf("%d\n", *i);
    });

    return 0;
}
