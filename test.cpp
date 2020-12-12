#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "./types.hpp"

u32 hm_hash(u32 u);
inline bool hm_objects_match(u32 a, u32 b);
struct Key;
u32 hm_hash(Key u);
inline bool hm_objects_match(Key a, Key b);

#define ZoneScoped
#define ZoneScopedN(name)

#include "./error.hpp"
#include "./hooks.hpp"
#include "./hashmap.hpp"

#include "./error.hpp"
#include "./print.hpp"


u32 hm_hash(u32 u) {
    return ((u64)u * 2654435761) % 4294967296;
}

inline bool hm_objects_match(u32 a, u32 b) {
    return a == b;
}

struct Key {
    int x;
    int y;
    int z;
};


u32 hm_hash(Key u) {
    return ((u.y ^ (u.x << 1)) >> 1) ^ u.z;
}

inline bool hm_objects_match(Key a, Key b) {
    return a.x == b.x
        && a.y == b.y
        && a.z == b.z;
}


auto print_dots(FILE* f) -> u32 {
    return print_to_file(f, "...");
}

auto test_printer() -> void {
    u32 arr[]   = {1,2,3,4,1,1,3};
    f32 f_arr[] = {1.1,2.1,3.2};


    register_printer("dots", print_dots, Printer_Function_Type::_void);

    u32 u1 = -1;
    u64 u2 = -1;

    char* str;
    print_to_string(&str, " - %{dots[5]} %{->} <> %{->,2}\n", &u1, &arr, nullptr);
    print("---> %{->char}", str);

    print(" - %{dots[3]}\n");
    print(" - %{u32} %{u64}\n", u1, u2);
    print(" - %{u32} %{u32} %{u32}\n", 2, 5, 7);
    print(" - %{f32} %{f32} %{f32}\n", 2.0, 5.0, 7.0);
    print(" - %{u32} %{bool} %{->char}\n", 2, true, "hello");
    print(" - %{f32[3]}\n", f_arr);
    print(" - %{f32,3}\n", 44.9, 55.1, 66.2);
    print(" - %{u32[5]}\n", arr);
    print(" - %{u32[*]}\n", arr, 4);
    print(" - %{u32,5}\n", 1,2,3,4,1,2);
    print(" - %{unknown%d}\n", 1);
    print(" - %{s32,3}\n", -1,200,-300);
    print(" - %{->} <> %{->,2}\n", &u1, &arr, nullptr);

    print("%{->char}%{->char}%{->char}",
          true   ? "general "     : "",
          false  ? "validation "  : "",
          false  ? "performance " : "");

    // print("%{->char}%{->char}\n\n", "hallo","");

}

auto test_hm() -> void {
    Hash_Map<u32, u32> h1;
    h1.alloc();
    defer { h1.dealloc(); };

    h1.set_object(1, 2);
    h1.set_object(2, 4);
    h1.set_object(3, 6);
    h1.set_object(4, 8);

    assert(h1.key_exists(1), "key shoud exist");
    assert(h1.key_exists(2), "key shoud exist");
    assert(h1.key_exists(3), "key shoud exist");
    assert(h1.key_exists(4), "key shoud exist");
    assert(!h1.key_exists(5), "key shoud not exist");

    assert(h1.get_object(1) == 2, "value should be correct");
    assert(h1.get_object(2) == 4, "value should be correct");
    assert(h1.get_object(3) == 6, "value should be correct");
    assert(h1.get_object(4) == 8, "value should be correct");


    Hash_Map<Key, u32> h2;
    h2.alloc();
    defer { h2.dealloc(); };

    h2.set_object({.x = 1, .y = 2, .z = 3}, 1);
    h2.set_object({.x = 3, .y = 3, .z = 3}, 3);

    assert(h2.key_exists({.x = 1, .y = 2, .z = 3}), "key shoud exist");
    assert(h2.key_exists({.x = 3, .y = 3, .z = 3}), "key shoud exist");

    assert(h2.get_object({.x = 1, .y = 2, .z = 3}) == 1, "value should be correct");
    assert(h2.get_object({.x = 3, .y = 3, .z = 3}) == 3, "value should be correct");

    h2.for_each([] (Key k, u32 v, u32 i) {
        print("%{s32} %{u32} %{u32}\n", k.x, v, i);
    });
}

s32 main(s32 argc, char* argv[]) {
    init_printer();

    test_hm();

    print("done.");

    return 0;
}
