#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>

#include "./types.hpp"
#include "./hooks.hpp"


#include "./error.hpp"
#include "./print.hpp"


int print_dots(FILE* f) {
    return print_to_file(f, "...");
}

void test_printer() {
    // u32 arr[]   = {1,2,3,4,1,1,3};
    // f32 f_arr[] = {1.1,2.1,3.2};

    // init_printer();
    // register_printer("dots", print_dots, Printer_Function_Type::_void);

    // u32 u1 = -1;
    // u64 u2 = -1;

    // char* str;
    // print_to_string(&str, " - %{dots[5]} %{->} <> %{->,2}\n", &u1, &arr, nullptr);
    // print("---> %{->char}", str);

    // print(" - %{dots[3]}\n");
    // print(" - %{u32} %{u64}\n", u1, u2);
    // print(" - %{u32} %{u32} %{u32}\n", 2, 5, 7);
    // print(" - %{f32} %{f32} %{f32}\n", 2.0, 5.0, 7.0);
    // print(" - %{u32} %{bool} %{->char}\n", 2, true, "hello");
    // print(" - %{f32[3]}\n", f_arr);
    // print(" - %{f32,3}\n", 44.9, 55.1, 66.2);
    // print(" - %{u32[5]}\n", arr);
    // print(" - %{u32[*]}\n", arr, 4);
    // print(" - %{u32,5}\n", 1,2,3,4,1,2);
    // print(" - %{unknown%d}\n", 1);
    // print(" - %{s32,3}\n", -1,200,-300);
    // print(" - %{->} <> %{->,2}\n", &u1, &arr, nullptr);

}

s32 main(s32 argc, char* argv[]) {

    // test_printer();

    init_printer();
    create_generic_error("nothing to lex was found:\n"
                         "  in %{color<}%{->char}%{>color}\n"
                         "  at %{color<}%{->char}%{>color}\n"
                         "bottom text\n",
                         console_green,
                         "some file name",
                         console_cyan,
                         "yesssssss");

    return 0;
}
