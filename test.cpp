#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>

#include "profiler.hpp"
#include "macros.hpp"
#include "hashmap.hpp"
#include "arraylist.hpp"

int main(int argc, char* argv[]) {
    Array_List<int> al;
    al.append(10);
    al.append(3);
    al.append(1);

    printf("numbers: %d %d %d %d\n", al[0], al[1], al[2], al[3]);
    al[2] = 1099;

    printf("numbers: %d %d %d %d\n", al[0], al[1], al[2], al[3]);

    al.sort();

    printf("numbers: %d %d %d %d\n", al[0], al[1], al[2], al[3]);

    printf("sortedfind (10): %d\n", al.sorted_find(10));

    for (auto num : al) {
        printf("- %d\n", num);
    }
    return 0;
}
