#define _CRT_SECURE_NO_WARNINGS
#define FTB_CORE_IMPL

#include <stdio.h>
#include "../core.hpp"

int main() {
    println("Hello World");
    defer {
        println("Program end");
    };

    int* int_arr {};

    Printing_Allocator printing_alloc {};
    printing_alloc.init();

    Bookkeeping_Allocator bookkeeping_alloc {};
    bookkeeping_alloc.init();

    Resettable_Allocator resettable_alloc {};
    resettable_alloc.init();

    Leak_Detecting_Allocator leak_alloc {};
    leak_alloc.init();


    with_allocator(bookkeeping_alloc) {
        with_allocator(printing_alloc) {
            with_allocator(leak_alloc) {
                {
                    Array_List<bool> b_list;
                    b_list.init(2);
                    // defer { b_list.deinit(); };
                    b_list.append(1);
                    b_list.append(1);
                    b_list.append(1);
                    b_list.append(1);
                    b_list.append(1);
                    b_list.append(1);
                    b_list.append(1);
                    b_list.append(1);
                }

                {
                    println("pointer before %{->}", int_arr);
                    int_arr = allocate<int>(10);
                    // defer {
                        // deallocate(int_arr);
                    // };

                    int_arr = resize<int>(int_arr, 20);

                    println("pointer after %{->}", int_arr);
                }

                leak_alloc.print_leak_statistics();
            }
        }

        bookkeeping_alloc.print_statistics();
    }

    return 0;
}
