#define _CRT_SECURE_NO_WARNINGS
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#define FTB_CORE_IMPL
#define FTB_HASHMAP_IMPL
#define FTB_SCHEDULER_IMPL
#define FTB_MATH_IMPL
#if not defined(FTB_NO_SIMD_TESTS)
#  define FTB_SOA_SORT_IMPL
#endif
#define FTB_JSON_IMPL
#define FTB_PARSING_IMPL

#include "../math.hpp"
#include "../core.hpp"
#include "../json.hpp"

u32 hm_hash(u32 u);
inline bool hm_objects_match(u32 a, u32 b);
struct Key;
u32 hm_hash(Key u);
inline bool hm_objects_match(Key a, Key b);

#include "../testing.hpp"
#include "../bucket_allocator.hpp"
#include "../pool_allocator.hpp"

#include "../hooks.hpp"
#include "../hashmap.hpp"
#include "../scheduler.hpp"
#include "../soa_sort.hpp"
#include "../kd_tree.hpp"


auto my_int_cmp = [](const int* a, const int* b) -> s32 {
    return (s32)(*a - *b);
};


auto my_voidp_cmp = [](const void** a, const void** b){
    return (s32)((unsigned char*)*a - (unsigned char*)*b);
};


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
    print_to_string(&str, nullptr, " - %{dots[5]} %{->} <> %{->,2}\n", &u1, &arr, nullptr);
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

auto test_hashmap() -> testresult {
    Hash_Map<u32, u32> h1;
    h1.init();
    defer { h1.deinit(); };

    h1.set_object(1, 2);
    h1.set_object(2, 4);
    h1.set_object(3, 6);
    h1.set_object(4, 8);

    assert_true(h1.key_exists(1));
    assert_true(h1.key_exists(2));
    assert_true(h1.key_exists(3));
    assert_true(h1.key_exists(4));
    assert_true(!h1.key_exists(5));

    assert_true(h1.get_object(1) == 2);
    assert_true(h1.get_object(2) == 4);
    assert_true(h1.get_object(3) == 6);
    assert_true(h1.get_object(4) == 8);


    Hash_Map<Key, u32> h2;
    h2.init();
    defer { h2.deinit(); };

    h2.set_object({.x = 1, .y = 2, .z = 3}, 1);
    h2.set_object({.x = 3, .y = 3, .z = 3}, 3);

    assert_true(h2.key_exists({.x = 1, .y = 2, .z = 3}));
    assert_true(h2.key_exists({.x = 3, .y = 3, .z = 3}));

    assert_true(h2.get_object({.x = 1, .y = 2, .z = 3}) == 1);
    assert_true(h2.get_object({.x = 3, .y = 3, .z = 3}) == 3);

    // h2.for_each([] (Key k, u32 v, u32 i) {
        // print("%{s32} %{u32} %{u32}\n", k.x, v, i);
    // });

    return pass;
}

auto test_stack_array_lists() -> testresult {
    Stack_Array_List<int> list = create_stack_array_list(int, 20);

    assert_equal_int(list.count, 0);
    assert_equal_int(list.length, 20);
    assert_true(list.data != nullptr);

    // test sum of empty list
    int sum = 0;
    int iter = 0;
    for (auto e : list) {
        sum += e;
        iter++;
    }
    assert_equal_int(sum, 0);
    assert_equal_int(iter, 0);

    // append some elements
    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);

    assert_equal_int(list.count, 4);
    assert_equal_int(list.length, 20);

    // test sum again
    sum = 0;
    iter = 0;
    for (auto e : list) {
        sum += e;
        iter++;
    }

    assert_equal_int(sum, 10);
    assert_equal_int(iter, 4);

    // bracketed access
    list[0] = 11;
    list[1] = 3;
    list[2] = 2;
    list.append(5);

    // test sum again
    sum = 0;
    iter = 0;
    for (auto e : list) {
        sum += e;
        ++iter;
    }
    assert_equal_int(sum, 25);
    assert_equal_int(iter, 5);

    // assert memory correct
    assert_equal_int(list.data[0], 11);
    assert_equal_int(list.data[1], 3);
    assert_equal_int(list.data[2], 2);
    assert_equal_int(list.data[3], 4);
    assert_equal_int(list.data[4], 5);

    // removing some indices
    list.remove_index(4);

    // test sum again
    sum = 0;
    iter = 0;
    for (auto e : list) {
        sum += e;
        ++iter;
    }
    assert_equal_int(sum, 20);
    assert_equal_int(iter, 4);

    // removing some indices
    list.remove_index(1);
    list.remove_index(0);

    // test sum again
    sum = 0;
    iter = 0;
    for (auto e : list) {
        sum += e;
        ++iter;
    }

    assert_equal_int(sum, 6);
    assert_equal_int(iter, 2);

    return pass;
}

auto test_array_lists_adding_and_removing() -> testresult {
    // test adding and removing
    Array_List<s32> list;
    list.init(16);

    assert_equal_int(list.length, 16);
    list.assure_allocated(20);
    assert_equal_int(list.length, 32);
    list.assure_allocated(2000);
    assert_equal_int(list.length, 2000);

    defer {
        list.deinit();
    };

    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);

    assert_equal_int(list.count, 4);

    list.remove_index(0);

    assert_equal_int(list.count, 3);
    assert_equal_int(list[0], 4);
    assert_equal_int(list[1], 2);
    assert_equal_int(list[2], 3);

    list.remove_index(2);

    assert_equal_int(list.count, 2);
    assert_equal_int(list[0], 4);
    assert_equal_int(list[1], 2);

    return pass;
}

auto test_array_lists_sorted_insert_and_remove() -> testresult {
    Array_List<s32> list;
    list.init();
    defer {
        list.deinit();
    };

    list.sorted_insert(3, my_int_cmp);
    list.sorted_insert(2, my_int_cmp);
    list.sorted_insert(7, my_int_cmp);
    list.sorted_insert(1, my_int_cmp);
    list.sorted_insert(6, my_int_cmp);
    list.sorted_insert(4, my_int_cmp);
    list.sorted_insert(5, my_int_cmp);

    assert_equal_int(true, list.is_sorted(my_int_cmp));

    list.sorted_remove_index(3);
    assert_equal_int(true, list.is_sorted(my_int_cmp));
    list.sorted_remove_index(2);
    assert_equal_int(true, list.is_sorted(my_int_cmp));
    list.sorted_remove_index(0);
    assert_equal_int(true, list.is_sorted(my_int_cmp));

    return pass;
}

auto test_array_lists_sorting() -> testresult {
    //
    //
    //  Test simple numbers
    //
    //
    Array_List<s32> list;
    list.init();
    defer {
        list.deinit();
    };

    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);

    list.sort(my_int_cmp);
    assert_equal_int(list.is_sorted(my_int_cmp), true);

    list.append(4);
    list.append(2);
    list.append(1);

    assert_equal_int(list.is_sorted(my_int_cmp), false);
    list.sort(my_int_cmp);
    assert_equal_int(list.is_sorted(my_int_cmp), true);

    list.clear();
    list.extend({
            8023, 7529, 2392, 7110,
            3259, 2484, 9695, 2199,
            6729, 9009, 8429, 7208});
    assert_equal_int(list.is_sorted(my_int_cmp), false);
    list.sort(my_int_cmp);
    assert_equal_int(list.is_sorted(my_int_cmp), true);


    //
    //
    //  Test adding and removing
    //
    //
    Array_List<s32> list1;
    list1.init();
    defer {
        list1.deinit();
    };

    list1.append(1);
    list1.append(2);
    list1.append(3);
    list1.append(4);

    list1.sort(my_int_cmp);

    assert_equal_int(list1.count, 4);

    assert_equal_int(list1[0], 1);
    assert_equal_int(list1[1], 2);
    assert_equal_int(list1[2], 3);
    assert_equal_int(list1[3], 4);
    assert_equal_int(list1.is_sorted(my_int_cmp), true);

    list1.append(0);
    list1.append(5);

    assert_equal_int(list1.count, 6);

    list1.sort(my_int_cmp);

    assert_equal_int(list1[0], 0);
    assert_equal_int(list1[1], 1);
    assert_equal_int(list1[2], 2);
    assert_equal_int(list1[3], 3);
    assert_equal_int(list1[4], 4);
    assert_equal_int(list1[5], 5);
    assert_equal_int(list1.is_sorted(my_int_cmp), true);

    //
    //
    //
    //
    // pointer list
    Array_List<void*> al;
    al.init();
    defer {
        al.deinit();
    };
    al.append((void*)0x1703102F100);
    al.append((void*)0x1703102F1D8);
    al.append((void*)0x1703102F148);
    al.append((void*)0x1703102F190);
    al.append((void*)0x1703102F190);
    al.append((void*)0x1703102F1D8);

    assert_equal_int(al.is_sorted(my_voidp_cmp), false);
    al.sort(my_voidp_cmp);
    assert_equal_int(al.is_sorted(my_voidp_cmp), true);

    assert_not_equal_int(al.sorted_find((void*)0x1703102F100, my_voidp_cmp), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F1D8, my_voidp_cmp), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F148, my_voidp_cmp), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F190, my_voidp_cmp), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F190, my_voidp_cmp), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F1D8, my_voidp_cmp), -1);

    return pass;
}

auto test_array_lists_searching() -> testresult {
    Array_List<s32> list1;
    list1.init();
    defer {
        list1.deinit();
    };

    list1.append(1);
    list1.append(2);
    list1.append(3);
    list1.append(4);


    s32 index = list1.sorted_find(3, my_int_cmp);
    assert_equal_int(index, 2);

    index = list1.sorted_find(1, my_int_cmp);
    assert_equal_int(index, 0);

    index = list1.sorted_find(5, my_int_cmp);
    assert_equal_int(index, -1);
    return pass;
}

auto test_typed_bucket_allocator() -> testresult {
    Typed_Bucket_Allocator<s32> ba;
    ba.init();
    defer {
        ba.deinit();
    };

    s32* s1 = ba.allocate();
    s32* s2 = ba.allocate();
    s32* s3 = ba.allocate();
    s32* s4 = ba.allocate();
    s32* s5 = ba.allocate();

    *s1 = 1;
    *s2 = 2;
    *s3 = 3;
    *s4 = 4;
    *s5 = 5;

    s32 wrong_answers = 0;
    s32 counter = 1;
    ba.for_each([&](s32* s) -> void {
        if(counter != *s)
            ++wrong_answers;
        ++counter;
    });
    assert_equal_int(wrong_answers, 0);



    Typed_Bucket_Allocator<s32> ba2;
    ba2.init();
    defer {
        ba2.deinit();
    };

    s1 = ba2.allocate();
    s2 = ba2.allocate();
    s3 = ba2.allocate();


    s32* s3_copy = ba2.allocate();
    s32* s3_copy2 = ba2.allocate();
    s32* s3_copy3 = ba2.allocate();
    *s3_copy = 3;
    ba2.deallocate(s3_copy);


    s4 = ba2.allocate();

    ba2.deallocate(s3_copy2);

    s5 = ba2.allocate();

    ba2.deallocate(s3_copy3);

    *s1 = 1;
    *s2 = 2;
    *s3 = 3;
    *s4 = 4;
    *s5 = 5;

    wrong_answers = 0;
    counter = 1;
    ba2.for_each([&](s32* s) -> void {
        if(counter != *s)
            ++wrong_answers;
        ++counter;
    });
    assert_equal_int(wrong_answers, 0);


    return pass;
}

auto test_array_list_sort_many() -> testresult {
    Array_List<s32> list;
    list.init();
    defer {
        list.deinit();
    };

    for (int i = 0; i < 10000; ++i) {
        list.append(rand());
    }

    assert_equal_int(list.is_sorted(my_int_cmp), false);
    list.sort(my_int_cmp);
    assert_equal_int(list.is_sorted(my_int_cmp), true);

    for (int i = 0; i < 10000; ++i) {
        assert_not_equal_int(list.sorted_find(list.data[i], my_int_cmp), -1);
    }

    list.clear();
    for (int i = 0; i < 1111; ++i) {
        list.append(rand());
    }

    assert_equal_int(list.is_sorted(my_int_cmp), false);
    list.sort(my_int_cmp);
    assert_equal_int(list.is_sorted(my_int_cmp), true);

    for (int i = 0; i < 1111; ++i) {
        assert_not_equal_int(list.sorted_find(list.data[i], my_int_cmp), -1);
    }


    list.clear();
    for (int i = 0; i < 3331; ++i) {
        list.append(rand());
    }

    assert_equal_int(list.is_sorted(my_int_cmp), false);
    list.sort(my_int_cmp);
    assert_equal_int(list.is_sorted(my_int_cmp), true);

    for (int i = 0; i < 3331; ++i) {
        assert_not_equal_int(list.sorted_find(list.data[i], my_int_cmp), -1);
    }

    return pass;
}

auto test_string_split() -> testresult {
    {
        String s = {
            .data   = (char*)"aa|bb|cc",
            .length = 8
        };

        String_Split ss;
        ss.init(s, '|');
        defer { ss.deinit(); };

        String expected0 = (String){.data = (char*)"aa", .length=2};
        String expected1 = (String){.data = (char*)"bb", .length=2};
        String expected2 = (String){.data = (char*)"cc", .length=2};

        assert_equal_int(ss.num_splits(), 3);

        assert_equal_string(ss[0], expected0);
        assert_equal_string(ss[1], expected1);
        assert_equal_string(ss[2], expected2);

    }

    {
        String s = {
            .data   = (char*)"|aa||bb|||cc|",
            .length = 13
        };

        String_Split ss;
        ss.init(s, '|');
        defer { ss.deinit(); };

        String expected0 = (String){.data = (char*)"",   .length=0};
        String expected1 = (String){.data = (char*)"aa", .length=2};
        String expected2 = (String){.data = (char*)"",   .length=0};
        String expected3 = (String){.data = (char*)"bb", .length=2};
        String expected4 = (String){.data = (char*)"",   .length=0};
        String expected5 = (String){.data = (char*)"",   .length=0};
        String expected6 = (String){.data = (char*)"cc", .length=2};
        String expected7 = (String){.data = (char*)"",   .length=0};

        assert_equal_int(ss.num_splits(), 8);

        assert_equal_string(ss[0], expected0);
        assert_equal_string(ss[1], expected1);
        assert_equal_string(ss[2], expected2);
        assert_equal_string(ss[3], expected3);
        assert_equal_string(ss[4], expected4);
        assert_equal_string(ss[5], expected5);
        assert_equal_string(ss[6], expected6);
        assert_equal_string(ss[7], expected7);

    }

    return pass;
}

auto test_hooks() -> testresult {
    s32 a = 0;
    s32 b = 0;
    s32 c = 0;

    Hook hook;
    hook << [&]() {
        a = 1;
    };
    hook << [&]() {
        // NOTE(Felix): assert correct execution order
        if (a == 1 && c == 0) {
            b = 2;
        }
    };
    hook << [&]() {
        c = 3;
    };

    assert_equal_int(a, 0);
    assert_equal_int(b, 0);
    assert_equal_int(c, 0);

    hook();

    assert_equal_int(a, 1);
    assert_equal_int(b, 2);
    assert_equal_int(c, 3);

    a = 0;
    b = 0;
    c = 0;

    // NOTE(Felix): hook should be empty now
    hook();

    assert_equal_int(a, 0);
    assert_equal_int(b, 0);
    assert_equal_int(c, 0);

    return pass;
}

auto test_scheduler_animations() -> testresult {
    Scheduler scheduler;
    Scheduler scheduler2;

    scheduler.init();
    scheduler2.init();
    defer {
        scheduler.deinit();
        scheduler2.deinit();
    };

    f32 val = 0;

    f32 from = 1;
    f32 to   = 2;
    scheduler.schedule_animation({
            .seconds_to_start   = 1,
            .seconds_to_end     = 2,
            .interpolant        = &val,
            .interpolant_type   = Interpolant_Type::F32,
            .from               = &from,
            .to                 = &to,
            .interpolation_type = Interpolation_Type::Lerp
        });


    f32 val2 = 0;

    f32 from2 = 1;
    f32 to2   = 2;
    scheduler2.schedule_animation({
            .seconds_to_start   = 1,
            .seconds_to_end     = 2,
            .interpolant        = &val2,
            .interpolant_type   = Interpolant_Type::F32,
            .from               = &from2,
            .to                 = &to2,
            .interpolation_type = Interpolation_Type::Lerp
        });

    assert_equal_f64((f64)val2, 0.0);

    scheduler.update_all(1);
    assert_equal_f64((f64)val, 1.0);

    assert_equal_f64((f64)val2, 0.0);
    scheduler2.update_all(1);
    assert_equal_f64((f64)val2, 1.0);

    scheduler.update_all(0.1);
    assert_equal_int(abs(val - 1.1) < 0.001, true);

    scheduler.update_all(0.1);
    assert_equal_int(abs(val - 1.2) < 0.001, true);

    scheduler.update_all(0.2);
    assert_equal_int(abs(val - 1.4) < 0.001, true);

    scheduler.update_all(1);
    assert_equal_int(abs(val - 2) < 0.001, true);

    // testing custom type interpolation
    enum My_Interpolant_Type : u8 {
        S32
    };

    scheduler.register_interpolator([](void* p_from, f32 t, void* p_to, void* p_interpolant) {
        s32 from = *(s32*)p_from;
        s32 to   = *(s32*)p_to;
        s32* target = (s32*)p_interpolant;
        *target = from + (to - from) * t;
    }, (Interpolant_Type)My_Interpolant_Type::S32, sizeof(s32));

    s32 test = 0;

    s32 s_from = 1;
    s32 s_to   = 11;
    scheduler.schedule_animation({
            .seconds_to_start   = 1,
            .seconds_to_end     = 2,
            .interpolant        = &test,
            .interpolant_type   = (Interpolant_Type)My_Interpolant_Type::S32,
            .from               = &s_from,
            .to                 = &s_to,
            .interpolation_type = Interpolation_Type::Lerp
        });

    assert_equal_int(test, 0);

    scheduler.update_all(1);
    assert_equal_int(test, 1);

    scheduler.update_all(0.1);
    assert_equal_int(test, 2);

    scheduler.update_all(0.1);
    assert_equal_int(test, 3);

    scheduler.update_all(0.2);
    assert_equal_int(test, 5);

    scheduler.update_all(1);
    assert_equal_int(test, 11);

    return pass;
}

auto test_math() -> testresult {
    // unary operator-
    {
        V3 v { 1, 2, 3 };
        v = -v;

        assert_equal_f32(v.x, -1);
        assert_equal_f32(v.y, -2);
        assert_equal_f32(v.z, -3);
    }
    {
        V2 v { -1, -2 };
        v = -v;

        assert_equal_f32(v.x, 1);
        assert_equal_f32(v.y, 2);
    }
    // binary operator+
    {
        V3 v1 { 1, 2, 3 };
        V3 v2 { 3, 3, 3 };

        V3 sum = v1 + v2;

        assert_equal_f32(sum.x, 4);
        assert_equal_f32(sum.y, 5);
        assert_equal_f32(sum.z, 6);
    }
    {
        V2 v1 { 1, -2 };
        V2 v2 { -3, 3 };

        V2 sum = v1 + v2;

        assert_equal_f32(sum.x, -2);
        assert_equal_f32(sum.y,  1);
    }
    // binary operator-
    {
        V3 v1 { 1, 2, 3 };
        V3 v2 { 3, 3, 3 };

        V3 diff = v1 - v2;

        assert_equal_f32(diff.x, -2);
        assert_equal_f32(diff.y, -1);
        assert_equal_f32(diff.z,  0);
    }
    {
        V2 v1 { 1, -2 };
        V2 v2 { -3, 3 };

        V2 diff = v1 - v2;

        assert_equal_f32(diff.x,  4);
        assert_equal_f32(diff.y, -5);
    }
    // binary operator* with scalar
    {
        V3 v1 { 1, -2, 3 };
        f32 s = 2.5;

        V3 r = v1 * s;

        assert_equal_f32(r.x, 2.5);
        assert_equal_f32(r.y, -5);
        assert_equal_f32(r.z, 7.5);

        r = s * v1;

        assert_equal_f32(r.x, 2.5);
        assert_equal_f32(r.y, -5);
        assert_equal_f32(r.z, 7.5);
    }
    {
        V2 v1 { 1, -2 };
        f32 s = -3;

        V2 r = v1 * s;

        assert_equal_f32(r.x, -3);
        assert_equal_f32(r.y,  6);

        r = s * v1;

        assert_equal_f32(r.x, -3);
        assert_equal_f32(r.y,  6);
    }
    // operator* with mat
    {
        V3   v1 { 1, 2, 3 };
        // column major
        M3x3 identity {
            {1, 0, 0,
             0, 1, 0,
             0, 0, 1},
        };

        V3 r = identity * v1;
        assert_equal_f32(r.x, 1);
        assert_equal_f32(r.y, 2);
        assert_equal_f32(r.z, 3);

        // column major
        M3x3 permute_scale {
            {0, 2, 0,
             0, 0, 2,
             2, 0, 0},
        };

        r = permute_scale * v1;
        assert_equal_f32(r.x, 6);
        assert_equal_f32(r.y, 2);
        assert_equal_f32(r.z, 4);
    }
    {
        V2   v1 { 1, 2 };
        // column major
        M2x2 identity {
            {1, 0,
             0, 1},
        };

        V2 r = identity * v1;
        assert_equal_f32(r.x, 1);
        assert_equal_f32(r.y, 2);

        M2x2 permute_scale {
            {0, 2,
             2, 0},
        };

        r = permute_scale * v1;
        assert_equal_f32(r.x, 4);
        assert_equal_f32(r.y, 2);
    }
    // dot
    {
        V3 v1 { 1, 2, 3 };
        V3 v2 { 3, 3, 3 };

        f32 d = dot(v1, v2);

        assert_equal_f32(d, 18.0)
    }
    {
        V2 v1 { 1, 2 };
        V2 v2 { 3, 3 };

        f32 d = dot(v1, v2);

        assert_equal_f32(d, 9.0)
    }
    // hadamard
    {
        V3 v1 { 1, 2, 3 };
        V3 v2 { 3, 3, 3 };

        auto h = hadamard(v1, v2);

        assert_equal_f32(h.x, 3);
        assert_equal_f32(h.y, 6);
        assert_equal_f32(h.z, 9);
    }
    {
        V2 v1 { 6,  2 };
        V2 v2 { -2, 12 };

        V2 h = hadamard(v1, v2);

        assert_equal_f32(h.x, -12);
        assert_equal_f32(h.y, 24);
    }
    // reflect
    {
        V3 dir    { 1, 2, -3 };
        V3 normal { 0, 0,  1 };
        V3 r = reflect(dir, normal);

        assert_equal_f32(r.x, 1);
        assert_equal_f32(r.y, 2);
        assert_equal_f32(r.z, 3);

        normal = {0, -1, 0};
        r = reflect(dir, normal);
        assert_equal_f32(r.x,  1);
        assert_equal_f32(r.y, -2);
        assert_equal_f32(r.z, -3);
    }
    {
        V2 dir    { 1,  2 };
        V2 normal { 0,  1 };
        V2 r = reflect(dir, normal);

        assert_equal_f32(r.x,  1);
        assert_equal_f32(r.y, -2);

        normal = {-1, 0 };
        r = reflect(dir, normal);

        assert_equal_f32(r.x, -1);
        assert_equal_f32(r.y,  2);
    }
    // length
    {
        assert_equal_f32(length(V3{ 2, 0, 0 }), 2);
        assert_equal_f32(length(V3{ 1, 1, 1 }), sqrt(3));

        assert_equal_f32(length(V2{ 0, 2 }), 2);
        assert_equal_f32(length(V2{ 1, 1 }), sqrt(2));
    }
    // noz
    {
        assert_equal_f32(length(noz(V3{ 2,    0, 0 })), 1);
        assert_equal_f32(length(noz(V3{ 2,    2, 2 })), 1);
        assert_equal_f32(length(noz(V3{ 2, -100, 1 })), 1);
        assert_equal_f32(length(noz(V3{ 0,    0, 0 })), 0);

        assert_equal_f32(length(noz(V3{ 2,    0 })), 1);
        assert_equal_f32(length(noz(V3{ 2,    2 })), 1);
        assert_equal_f32(length(noz(V3{ 2, -100 })), 1);
        assert_equal_f32(length(noz(V3{ 0,    0 })), 0);

        V3 v = { 1, 1, 1 };
        v = noz(v);
        assert_equal_f32(v.x, 1/sqrt(3));
        assert_equal_f32(v.y, 1/sqrt(3));
        assert_equal_f32(v.z, 1/sqrt(3));
    }
    // lerp
    {
        V3 a = { 1,  1, 1 };
        V3 b = { 0, 21, 0 };
        V3 l = lerp(a, 0, b);

        assert_equal_f32(l.x, a.x);
        assert_equal_f32(l.y, a.y);
        assert_equal_f32(l.z, a.z);

        l = lerp(a, 1, b);

        assert_equal_f32(l.x, b.x);
        assert_equal_f32(l.y, b.y);
        assert_equal_f32(l.z, b.z);

        l = lerp(a, 0.5, b);

        assert_equal_f32(l.x, 0.5);
        assert_equal_f32(l.y, 11.0);
        assert_equal_f32(l.z, 0.5);
    }
    {
        V2 a = { 1,  1 };
        V2 b = { 0, 21 };
        V2 l = lerp(a, 0, b);

        assert_equal_f32(l.x, a.x);
        assert_equal_f32(l.y, a.y);

        l = lerp(a, 1, b);

        assert_equal_f32(l.x, b.x);
        assert_equal_f32(l.y, b.y);

        l = lerp(a, 0.5, b);

        assert_equal_f32(l.x, 0.5);
        assert_equal_f32(l.y, 11.0);
    }
    {
        M3x3 ident {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };

        M3x3 m {
            1, 0, 0,
            0, 1, 0,
            0, 0, 1,
        };

        auto res = ident * m;

        assert_equal_f32(res._00, m._00);
        assert_equal_f32(res._01, m._01);
        assert_equal_f32(res._02, m._02);

        assert_equal_f32(res._10, m._10);
        assert_equal_f32(res._11, m._11);
        assert_equal_f32(res._12, m._12);

        assert_equal_f32(res._20, m._20);
        assert_equal_f32(res._21, m._21);
        assert_equal_f32(res._22, m._22);

        m = {
            1, 2, 3,
            4, 5, 6,
            7, 8, 9,
        };

        res = ident * m;

        assert_equal_f32(res._00, m._00);
        assert_equal_f32(res._01, m._01);
        assert_equal_f32(res._02, m._02);

        assert_equal_f32(res._10, m._10);
        assert_equal_f32(res._11, m._11);
        assert_equal_f32(res._12, m._12);

        assert_equal_f32(res._20, m._20);
        assert_equal_f32(res._21, m._21);
        assert_equal_f32(res._22, m._22);

        res = m * m;

        assert_equal_f32(res._00, 30);
        assert_equal_f32(res._01, 36);
        assert_equal_f32(res._02, 42);

        assert_equal_f32(res._10, 66);
        assert_equal_f32(res._11, 81);
        assert_equal_f32(res._12, 96);

        assert_equal_f32(res._20, 102);
        assert_equal_f32(res._21, 126);
        assert_equal_f32(res._22, 150);


    }
    return pass;
}


auto test_sort() -> testresult {
#if defined(FTB_NO_SIMD_TESTS)
    return skipped;
#else
    u64 arr1[4] {
        4,1,3,2
    };

    struct LageTest {
        u64 stuff[4];
        const char* s;
        u32 lotta_stuff[12];
    };

    LageTest arr2[4] {
        {.s = "happy"},
        {.s = "Hello"},
        {.s = "I am"},
        {.s = "World"},
    };

    byte arr3[4] {
        1, 2, 3, 4
    };

    Array_Description arr_decs[] {
        {.base = arr2, .width = sizeof(arr2[0])},
        {.base = arr3, .width = sizeof(arr3[0])},
    };

    soa_sort({arr1, sizeof(arr1[0])},
             arr_decs, array_length(arr_decs), array_length(arr1),
             [] (const void* a, const void* b) -> s32 {
                 u32 sa = *(u32*)a;
                 u32 sb = *(u32*)b;
                 return sa - sb;
             });

    assert_equal_int(arr1[0], 1);
    assert_equal_int(arr1[1], 2);
    assert_equal_int(arr1[2], 3);
    assert_equal_int(arr1[3], 4);

    assert_equal_int(arr3[0], 2);
    assert_equal_int(arr3[1], 4);
    assert_equal_int(arr3[2], 3);
    assert_equal_int(arr3[3], 1);

    assert_equal_string(arr2[0].s, "Hello");
    assert_equal_string(arr2[1].s, "World");
    assert_equal_string(arr2[2].s, "I am");
    assert_equal_string(arr2[3].s, "happy");


    {
        // random test
        s32 arr1[1000];
        s32 arr2[1000];
        s32 arr3[1000];
        s32 arr4[1000];
        s32 arr5[1000];

        for (u32 i = 0; i < 1000; ++i) {
            arr1[i] = rand();
            arr2[i] = arr1[i];
            arr3[i] = arr2[i];
            arr4[i] = arr3[i];
            arr5[i] = arr4[i];
        }

        Array_Description arr_decs[] {
            {.base = arr2, .width = sizeof(arr2[0])},
            {.base = arr3, .width = sizeof(arr3[0])},
            {.base = arr4, .width = sizeof(arr4[0])},
            {.base = arr5, .width = sizeof(arr5[0])},
        };

        soa_sort({arr1, sizeof(arr1[0])},
                 arr_decs, array_length(arr_decs), array_length(arr1),
                 [] (const void* a, const void* b) -> s32 {
                     s32 sa = *(s32*)a;
                     s32 sb = *(u32*)b;
                     return sa - sb;
                 });

        for (u32 i = 0; i < 999; ++i) {
            assert_equal_int(arr1[i], arr2[i]);
            assert_equal_int(arr2[i], arr3[i]);
            assert_equal_int(arr3[i], arr4[i]);
            assert_equal_int(arr4[i], arr5[i]);

            assert_true(arr1[i] <= arr1[i+1])
        }

    }

    return pass;
#endif
}

auto test_kd_tree() -> testresult {
    Array_List<V3> points;
    points.init_from({
        {0,0,0}, {1,1,1},
        {2,2,2}, {3,3,3},
    });
    defer { points.deinit(); };

    Array_List<int> payloads1;
    payloads1.init_from({
            0,1,2,3,
    });
    defer { payloads1.deinit(); };

    struct PL {
        int a[1000];
    };
    Array_List<PL> payloads2;
    payloads2.init_from({
        {},{},{},{},
    });
    defer { payloads2.deinit(); };

    auto tree1 = Kd_Tree<int>::build_from(points.count, points.data, payloads1.data);
    auto tree2 = Kd_Tree<PL>::build_from(points.count, points.data, payloads2.data);
    defer {
        tree1.deinit();
        tree2.deinit();
    };

    Array_List<V3> query_points;
    query_points.init();
    defer { query_points.deinit(); };

    Array_List<int> query_payloads1;
    Array_List<PL> query_payloads2;
    query_payloads1.init();
    query_payloads2.init();
    defer { query_payloads1.deinit(); };
    defer { query_payloads2.deinit(); };

    {
        query_points.clear();
        query_payloads1.clear();
        query_payloads2.clear();

        Axis_Aligned_Box aabb {
            .min { 0.5, 0.5, 0.5 },
            .max { 3.5, 3.5, 3.5 }
        };

        tree1.query_in_aabb(aabb, &query_points, &query_payloads1);
        assert_equal_int(query_points.count, 3);
        assert_equal_int(query_payloads1.count, 3);


        query_points.clear();
        tree2.query_in_aabb(aabb, &query_points, &query_payloads2);

        assert_equal_int(query_points.count, 3);
        assert_equal_int(query_payloads2.count, 3);
    }

    {
        query_points.clear();
        query_payloads1.clear();

        tree1.query_in_sphere({0.5, 0.5, 0.5}, 1, &query_points, &query_payloads1);

        assert_equal_int(query_points.count, 2);
        assert_equal_int(query_payloads1.count, 2);

    }
    {
        query_points.clear();
        query_payloads1.clear();

        Kd_Tree<Empty> tree;
        tree.init();
        defer { tree.deinit(); };

        tree.add({0,0,0});
        tree.add({1,0,0});
        tree.add({0,1,0});
        tree.add({0,0,1});
        tree.add({1,1,1});

        tree.query_in_sphere({1, 0, 0}, 1.1, &query_points, nullptr);
        assert_equal_int(query_points.count, 2);
        assert_equal_int(tree.get_count_in_sphere({1,0,0}, 1.1), 2);

        query_points.clear();
        tree.query_in_sphere({0, 0, 0}, 1.1, &query_points, nullptr);
        assert_equal_int(query_points.count, 4);
        assert_equal_int(tree.get_count_in_sphere({0,0,0}, 1.1), 4);

        query_points.clear();
        tree.query_in_sphere({0, 0, 0}, 2, &query_points, nullptr);
        assert_equal_int(query_points.count, 5);
        assert_equal_int(tree.get_count_in_sphere({0,0,0}, 2), 5);
    }
    return pass;
}

auto test_bucket_list() -> testresult {
    Bucket_List<int> list;
    list.init(/*bucket size  = */ 2,
              /*bucket count = */ 1);
    defer { list.deinit(); };

    list.append(1);
    list.append(2);
    list.append(3);

    assert_equal_int(list.count_elements(), 3);

    u32 index = 0;
    list.for_each([&](int* i) -> testresult {
        ++index;
        assert_equal_int(index, *i);
        return pass;
    });

    assert_equal_int(list[0], 1);
    assert_equal_int(list[1], 2);
    assert_equal_int(list[2], 3);

    list.remove_index(1);
    assert_equal_int(list.count_elements(), 2);
    assert_equal_int(list[0], 1);
    assert_equal_int(list[1], 3);

    index = 0;
    list.for_each([&](int* i) -> testresult {
        if (index == 0) {
            assert_equal_int(1, *i);
        } else {
            assert_equal_int(3, *i);
        }
        ++index;
        return pass;
    });

    list[0] = 3;
    assert_equal_int(list.count_elements(), 2);
    assert_equal_int(list[0], 3);
    list.append(3);
    list.append(3);
    list.append(3);
    list.append(10);
    assert_equal_int(list.count_elements(), 6);

    return pass;
}

auto test_bucket_list_leak() -> testresult {
    Bucket_List<int> l;
    l.init(2);
    defer { l.deinit(); };

    l.append(1);
    l.append(1); // next bucket
    int* old_bucket = l.buckets[1];
    l.remove_index(0);

    l.append(1); // next bucket again
    int* new_bucket = l.buckets[1];

    assert_equal_int(old_bucket, new_bucket);

    return pass;
}

auto test_growable_pool_allocator() -> testresult {
    Growable_Pool_Allocator<int> pool;
    pool.init(10);
    defer { pool.deinit(); };

    assert_equal_int(pool.count_allocated_elements(), 0);

    int* ints[20];
    for (u32 i = 0; i < array_length(ints); ++i) {
        if (i == 0 || i == 10) {
            assert_equal_int(pool.free_list, 0);
        } else {
            assert_not_equal_int(pool.free_list, 0);
        }

        ints[i] = pool.allocate();
        *ints[i] = i;

        assert_equal_int(pool.count_allocated_elements(), i+1);
    }

    assert_equal_int(pool.free_list, 0);

    for (u32 i = 0; i < array_length(ints); ++i) {
        assert_equal_int(*ints[i],  i);
    }

    for (u32 i = 0; i < 10; ++i) {
        pool.deallocate(ints[i]);

        assert_equal_int(pool.count_allocated_elements(), 20-i-1);
    }

    assert_not_equal_int(pool.free_list, 0);

    for (u32 i = 0; i < 10; ++i) {
        assert_not_null(pool.free_list);

        ints[i] = pool.allocate();
        *ints[i] = 100+i;

        assert_equal_int(pool.count_allocated_elements(), 10+i+1);
    }


    u32 i = 0;
    for (; i < 10; ++i) {
        assert_equal_int(*ints[i],  i+100);
    }
    for (; i < 20; ++i) {
        assert_equal_int(*ints[i],  i);
    }

    assert_not_null(pool.next_chunk);
    assert_not_null(pool.next_chunk->next_chunk);
    assert_null(pool.next_chunk->next_chunk->next_chunk);
    assert_null(pool.free_list);

    assert_equal_int(pool.next_chunk->data[0].element, 10);
    assert_equal_int(pool.next_chunk->data[1].element, 11);
    assert_equal_int(pool.next_chunk->data[2].element, 12);
    assert_equal_int(pool.next_chunk->data[3].element, 13);
    assert_equal_int(pool.next_chunk->data[4].element, 14);
    assert_equal_int(pool.next_chunk->data[5].element, 15);
    assert_equal_int(pool.next_chunk->data[6].element, 16);
    assert_equal_int(pool.next_chunk->data[7].element, 17);
    assert_equal_int(pool.next_chunk->data[8].element, 18);
    assert_equal_int(pool.next_chunk->data[9].element, 19);

    assert_equal_int(pool.next_chunk->next_chunk->data[0].element, 109);
    assert_equal_int(pool.next_chunk->next_chunk->data[1].element, 108);
    assert_equal_int(pool.next_chunk->next_chunk->data[2].element, 107);
    assert_equal_int(pool.next_chunk->next_chunk->data[3].element, 106);
    assert_equal_int(pool.next_chunk->next_chunk->data[4].element, 105);
    assert_equal_int(pool.next_chunk->next_chunk->data[5].element, 104);
    assert_equal_int(pool.next_chunk->next_chunk->data[6].element, 103);
    assert_equal_int(pool.next_chunk->next_chunk->data[7].element, 102);
    assert_equal_int(pool.next_chunk->next_chunk->data[8].element, 101);
    assert_equal_int(pool.next_chunk->next_chunk->data[9].element, 100);


    assert_equal_int(pool.count_allocated_elements(), 20);
    pool.reset();
    assert_equal_int(pool.count_allocated_elements(), 0);

    return pass;
}

auto test_pool_allocator() -> testresult {
    Pool_Allocator<int> pool;
    pool.init(10);
    defer { pool.deinit(); };

    assert_equal_int(pool.count_allocated_elements(), 0);

    int* ints[10];

    for (u32 repeat = 0; repeat < 3; ++repeat) {
        for (u32 i = 0; i < array_length(ints); ++i) {
            assert_not_equal_int(pool.next_free_idx, -1);

            ints[i] = pool.allocate();
            *ints[i] = i;

            assert_equal_int(pool.count_allocated_elements(), i+1);
        }

        assert_null(pool.allocate());

        for (u32 i = 0; i < array_length(ints); ++i) {
            assert_equal_int(*ints[i], i);
        }

        if (repeat != 2) {
            pool.reset();
        }
    }

    assert_equal_int(pool.count_allocated_elements(), 10);

    for (u32 i = 0; i < 5; ++i) {
        pool.deallocate(ints[i]);
        assert_equal_int(pool.count_allocated_elements(), 10-i-1);
    }

    for (u32 i = 0; i < 5; ++i) {
        ints[i] = pool.allocate();
        *ints[i] = 100+i;
        assert_equal_int(pool.count_allocated_elements(), 5+i+1);
    }

    assert_null(pool.allocate());

    assert_equal_int(pool.data[0].element, 104);
    assert_equal_int(pool.data[1].element, 103);
    assert_equal_int(pool.data[2].element, 102);
    assert_equal_int(pool.data[3].element, 101);
    assert_equal_int(pool.data[4].element, 100);
    assert_equal_int(pool.data[5].element, 5);
    assert_equal_int(pool.data[6].element, 6);
    assert_equal_int(pool.data[7].element, 7);
    assert_equal_int(pool.data[8].element, 8);
    assert_equal_int(pool.data[9].element, 9);


    return pass;
}

auto test_bucket_queue() -> testresult {
    Bucket_Queue<int> q;
    q.init(/*bucket size  = */ 2,
           /*bucket count = */ 1);
    defer { q.deinit(); };

    assert_equal_int(q.count(), 0);

    q.push_back(0);
    q.push_back(1);
    q.push_back(2);
    q.push_back(3);

    assert_equal_int(q.count(), 4);

    assert_equal_int(q.get_next(), 0);
    assert_equal_int(q.get_next(), 1);
    assert_equal_int(q.get_next(), 2);
    assert_equal_int(q.get_next(), 3);

    assert_equal_int(q.count(), 0);

    q.push_back(3);
    q.push_back(2);
    q.push_back(1);
    q.push_back(0);

    assert_equal_int(q.get_next(), 3);
    assert_equal_int(q.get_next(), 2);
    assert_equal_int(q.get_next(), 1);
    assert_equal_int(q.get_next(), 0);

    assert_equal_int(q.count(), 0);

    q.push_back(0);
    q.push_back(1);
    assert_equal_int(q.get_next(), 0);
    q.push_back(2);
    assert_equal_int(q.get_next(), 1);
    q.push_back(3);
    assert_equal_int(q.get_next(), 2);
    q.push_back(4);
    assert_equal_int(q.get_next(), 3);
    q.push_back(5);
    assert_equal_int(q.get_next(), 4);
    q.push_back(6);
    assert_equal_int(q.get_next(), 5);
    q.push_back(7);
    assert_equal_int(q.get_next(), 6);
    assert_equal_int(q.get_next(), 7);

    assert_equal_int(q.bucket_list.next_index_in_latest_bucket, 0);
    assert_equal_int(q.bucket_list.next_bucket_index,0);
    assert_equal_int(q.start_idx,0);

    return pass;
}

auto test_defer_runs_after_return() -> testresult {
    testresult result;

    defer {
        result = fail;
    };

    result = pass;

    return result;
}


auto test_json_simple_object_new_syntax() -> testresult {
    using namespace json;
    const char* json_object = R"JSON(
        {
          "int":    123,
          "float":  123.123,
          "bool":   true,
          "string": "Hello"
        }
)JSON";

    struct Test {
        s32    i;
        f32    f;
        bool   b;
        String s;
    };

    Pattern p = json::object({
        {"int",    json::integer (offsetof(Test, i))},
        {"float",  json::floating(offsetof(Test, f))},
        {"bool",   json::boolean_(offsetof(Test, b))},
        {"string", json::string  (offsetof(Test, s))},
    });

    Test t;
    Pattern_Match_Result result = pattern_match(json_object, p, &t);


    defer {
        t.s.free();
    };


    assert_equal_int(result, Pattern_Match_Result::OK_CONTINUE);

    assert_equal_int(t.i, 123);
    assert_equal_f32(t.f, 123.123);
    assert_equal_int(t.b, true);
    assert_equal_int(strncmp("Hello", t.s.data, 5), 0);

    return pass;
}

auto test_json_config() -> testresult {
    using namespace json;

    const char* json_str = R"JSON({
    "config_version" : 1,
    "db_client_id" : "bb3f296b73ffba5c09138aa612187290",
    "db_client_secret" : "cd92b98cfd0e2ddf2e70f42bbe8cb744",
    "mvg_location_names" : [
        "Petershausen P+R",
        "Petershausen",
    ],
    "db_station_names" : [
        "Petershausen"
    ],
    "vehicle_type_blacklist" : [],
    "destination_blacklist" : [],
})JSON";

    // TODO(Felix): use the custom Vehicle_Type here too
    struct Tafel_Config {
        int config_version;

        String db_client_id;
        String db_client_secret;

        Array_List<String> mvg_locations;
        Array_List<String> db_stations;
        Array_List<String> destination_blacklist;
        // Array_List<Vehicle_Type> vehicle_type_blacklist;

        void free() {
            db_client_id.free();
            db_client_secret.free();

            for (String s : mvg_locations)         s.free();
            for (String s : db_stations)           s.free();
            for (String s : destination_blacklist) s.free();

            mvg_locations.deinit();
            db_stations.deinit();
            destination_blacklist.deinit();
            // vehicle_type_blacklist.deinit();
        }
    };

    Pattern p = object({
        {"config_version",     integer(offsetof(Tafel_Config, config_version))},
        {"db_client_id",       string (offsetof(Tafel_Config, db_client_id))},
        {"db_client_secret",   string (offsetof(Tafel_Config, db_client_secret))},
        {"mvg_location_names", list(string(0), {
                .array_list_offset = offsetof(Tafel_Config, mvg_locations),
                .element_size      = sizeof(Tafel_Config::mvg_locations[0]),
                })},
        {"db_station_names",  list(string(0), {
                .array_list_offset = offsetof(Tafel_Config, db_stations),
                .element_size      = sizeof(Tafel_Config::db_stations[0]),
                })},
        {"destination_blacklist", list(string(0), {
                .array_list_offset = offsetof(Tafel_Config, destination_blacklist),
                .element_size      = sizeof(Tafel_Config::destination_blacklist[0]),
                })},
        // {"vehicle_type_blacklist", list(
                    // custom(Json_Type::String, (Data_Type)Custom_Data_Types::VEHICLE_TYPE, 0),
            // {
                // .array_list_offset = offsetof(Tafel_Config, vehicle_type_blacklist),
                // .element_size      = sizeof(Tafel_Config::vehicle_type_blacklist[0]),
            // })}
    });


    Tafel_Config config_result {};
    Pattern_Match_Result match_result = pattern_match(json_str, p, &config_result);

    defer {
        config_result.free();
    };

    assert_equal_int(config_result.config_version, 1);
    assert_equal_string(config_result.db_client_id,
                        string_from_literal("bb3f296b73ffba5c09138aa612187290"));

    assert_equal_string(config_result.db_client_secret,
                        string_from_literal("cd92b98cfd0e2ddf2e70f42bbe8cb744"));

    assert_equal_int(config_result.mvg_locations.count, 2);
    assert_equal_string(config_result.mvg_locations[0],
                        string_from_literal("Petershausen P+R"));
    assert_equal_string(config_result.mvg_locations[1],
                        string_from_literal("Petershausen"));

    assert_equal_int(config_result.db_stations.count, 1);
    assert_equal_string(config_result.db_stations[0],
                        string_from_literal("Petershausen"));

    return pass;
}

auto test_json_mvg() -> testresult {
    const char* json_str =  R"JSON(
        {
           "locations" : [ {
             "type" :  "station",
             "latitude" : 48.4126,
             "longitude" : 11.46992,
             "id" :  "de:09174:6840",
             "divaId" : 6840,
             "place" :  "Petershausen 2",
             "name" :  "Petershausen 1",
             "hasLiveData" : false,
             "hasZoomData" : false,
             "products" : [  "BAHN",  "SBAHN",  "BUS" ],
             "aliases" :  "Bahnhof Bf. DAH MMPE",
             "tariffZones" :  "4|5",
             "lines" : {
               "tram" : [ ],
               "nachttram" : [ ],
               "sbahn" : [ ],
               "ubahn" : [ ],
               "bus" : [ ],
               "nachtbus" : [ ],
               "otherlines" : [ ]
            }
          }, {
             "type" :  "station",
             "latitude" : 48.41323,
             "longitude" : 11.46913,
             "id" :  "de:09174:7167 ",
             "divaId" : 7167,
             "place" :  "Petershausen",
             "name" :  "Petershausen Bf. P+R-Platz",
             "hasLiveData" : false,
             "hasZoomData" : false,
             "products" : [  "BUS " ],
             "aliases" :  "DAH ",
             "tariffZones" :  "4|5 ",
             "lines" : {
               "tram" : [ ],
               "nachttram" : [ ],
               "sbahn" : [ ],
               "ubahn" : [ ],
               "bus" : [ ],
               "nachtbus" : [ ],
               "otherlines" : [ ]
            }
            }]}
)JSON";

    using namespace json;

    struct Location {
        String             type;
        f32                latitude;
        f32                longitude;
        String             id;
        s32                divaId;
        String             place;
        String             name;
        bool               has_live_data;
        bool               has_zoom_data;
        Array_List<String> products;
        String             aliases;
        String             tariffZones;

        auto free() -> void {
            type.free();
            id.free();
            place.free();
            name.free();
            aliases.free();
            tariffZones.free();

            for (auto p: products) {
                p.free();
            }
            products.deinit();
        }

    };

    using namespace json;
    Pattern product =
        object({{"products", list({string(0)}, {
                            .array_list_offset = offsetof(Location, products),
                            .element_size = sizeof(String)
                        })},
                {"type",          string  (offsetof(Location, type))},
                {"latitude",      floating(offsetof(Location, latitude))},
                {"longitude",     floating(offsetof(Location, longitude))},
                {"id",            string  (offsetof(Location, id))},
                {"divaId",        integer (offsetof(Location, divaId))},
                {"place",         string  (offsetof(Location, place))},
                {"name",          string  (offsetof(Location, name))},
                {"has_live_data", boolean_(offsetof(Location, has_live_data))},
                {"has_zoom_data", boolean_(offsetof(Location, has_zoom_data))},
                {"aliases",       string  (offsetof(Location, aliases))},
                {"tariffZones",   string  (offsetof(Location, tariffZones))},
            }, {
                // NOTE(Felix): stop after first location
                .leave_hook = [](void*, Hook_Context, Parser_Context) -> Pattern_Match_Result {
                    return Pattern_Match_Result::OK_DONE;
                }
            });

    Pattern p = object({{"locations", list({product})},});

    Location mvg_loc {};

    auto result = json::pattern_match((char*)json_str, p, &mvg_loc);

    defer {
        mvg_loc.free();
    };

    assert_equal_int(result, json::Pattern_Match_Result::OK_CONTINUE);

    assert_equal_int(strncmp("station", mvg_loc.type.data, mvg_loc.type.length), 0);
    assert_equal_f32(mvg_loc.latitude, 48.412601f);
    assert_equal_f32(mvg_loc.longitude, 11.469920f);
    assert_equal_int(strncmp("de:09174:6840",    mvg_loc.id.data, mvg_loc.id.length), 0);
    assert_equal_int(mvg_loc.divaId, 6840);
    assert_equal_int(strncmp("Petershausen 2",   mvg_loc.place.data, mvg_loc.place.length), 0);
    assert_equal_int(strncmp("Petershausen 1",   mvg_loc.name.data, mvg_loc.name.length), 0);
    assert_equal_int(mvg_loc.has_live_data, false);
    assert_equal_int(mvg_loc.has_zoom_data, false);

    assert_equal_int(mvg_loc.products.count, 3);
    assert_equal_int(strncmp("BAHN",  mvg_loc.products[0].data, mvg_loc.products[0].length), 0);
    assert_equal_int(strncmp("SBAHN", mvg_loc.products[1].data, mvg_loc.products[1].length), 0);
    assert_equal_int(strncmp("BUS",   mvg_loc.products[2].data, mvg_loc.products[2].length), 0);

    assert_equal_int(strncmp("Bahnhof Bf. DAH MMPE",   mvg_loc.aliases.data, mvg_loc.aliases.length), 0);
    assert_equal_int(strncmp("4|5",   mvg_loc.tariffZones.data, mvg_loc.tariffZones.length), 0);

    return pass;
}

testresult test_json_bug() {
    using namespace json;

    struct Departure {
        String             product;
        String             label;
        String             destination;
        bool               live;
        f32                delay;
        bool               cancelled;
        String             line_background_color;
        String             departure_id;
        bool               sev;
        String             platform;
        s32                stop_position_number;
        Array_List<String> info_messages;

        void free() {
            product.free();
            label.free();
            destination.free();
            line_background_color.free();
            departure_id.free();
            platform.free();

            for (auto& s: info_messages) {
                s.free();
            }

            info_messages.deinit();
        }

    };

    struct Serving_Line {
        String destination;
        bool   sev;
        String network;
        String product;
        String line_number;
        String diva_id;

        void free() {
            destination.free();
            network.free();
            product.free();
            line_number.free();
            diva_id.free();
        }
    };

    struct Departure_Infos {
        Array_List<Serving_Line> serving_lines;
        Array_List<Departure>    departures;

        void free() {
            for (auto& d : departures) {
                d.free();
            }
            departures.deinit();

            for (auto& s : serving_lines) {
                s.free();
            }
            serving_lines.deinit();
        }
    };

    const char* json_str =  R"JSON(
       {
         "servingLines" : [ {
           "destination" : "Erding",
           "sev" : false,
           "network" : "ddb",
           "product" : "SBAHN",
           "lineNumber" : "S2",
           "divaId" : "92M02"
         }, {
           "destination" : "Lohhof (S) Süd",
           "sev" : false,
           "network" : "mvv",
           "product" : "REGIONAL_BUS",
           "lineNumber" : "771",
           "divaId" : "19771"
         }, {
           "destination" : "Petershausen",
           "sev" : false,
           "network" : "mvv",
           "product" : "RUFTAXI",
           "lineNumber" : "7280",
           "divaId" : "18728"
         } ],
         "departures" : [ {
           "departureTime" : 1662174480000,
           "product" : "SBAHN",
           "label" : "S2",
           "destination" : "Erding",
           "live" : false,
           "delay" : 0,
           "cancelled" : false,
           "lineBackgroundColor" : "#9bc04c",
           "departureId" : "c9b4b3875a8d2f53ad018b204899ffc0#1662174480000#de:09174:6840",
           "sev" : false,
           "platform" : "6",
           "stopPositionNumber" : 0,
           "infoMessages" : [ "Linie S2: Maskenpflicht nach gesetzl. Regelung; wir empfehlen eine FFP2-Maske Linie S2: Fahrradmitnahme begrenzt möglich Linie S2: Bei Fahrradmitnahme Sperrzeiten beachten Linie S2: nur 2. Kl." ]
         }, {
           "departureTime" : 1662175920000,
           "product" : "SBAHN",
           "label" : "S2",
           "destination" : "Erding",
           "live" : false,
           "delay" : 0,
           "cancelled" : false,
           "lineBackgroundColor" : "#9bc04c",
           "departureId" : "88cd45142d5ee37ba77295e93bb3b88f#1662175920000#de:09174:6840",
           "sev" : false,
           "platform" : "6",
           "stopPositionNumber" : 0,
           "infoMessages" : [ "Linie S2: Maskenpflicht nach gesetzl. Regelung; wir empfehlen eine FFP2-Maske Linie S2: Fahrradmitnahme begrenzt möglich Linie S2: Bei Fahrradmitnahme Sperrzeiten beachten Linie S2: nur 2. Kl." ]
         } ]
       }
)JSON";
        Pattern departure = object({
                {"product",             string  (offsetof(Departure, product))},
                {"label",               string  (offsetof(Departure, label))},
                {"destination",         string  (offsetof(Departure, destination))},
                {"live",                boolean_(offsetof(Departure, live))},
                {"delay",               floating(offsetof(Departure, delay))},
                {"cancelled",           boolean_(offsetof(Departure, cancelled))},
                {"lineBackgroundColor", string  (offsetof(Departure, line_background_color))},
                {"departureId",         string  (offsetof(Departure, departure_id))},
                {"sev",                 boolean_(offsetof(Departure, sev))},
                {"platform",            string  (offsetof(Departure, platform))},
                {"stopPositionNumber",  integer (offsetof(Departure, stop_position_number))},
        });
        Pattern serving_line = object({
                {"destination", string( offsetof(Serving_Line, destination))},
                {"sev",         boolean_(offsetof(Serving_Line, sev))},
                {"network",     string( offsetof(Serving_Line, network))},
                {"product",     string( offsetof(Serving_Line, product))},
                {"lineNumber",  string( offsetof(Serving_Line, line_number))},
                {"divaId",      string( offsetof(Serving_Line, diva_id))},
        });
        Pattern p =
            object({
                    {"departures",
                     list({departure},{
                             .array_list_offset = offsetof(Departure_Infos, departures),
                             .element_size      = sizeof(Departure)})},
                    {"servingLines",
                     list({serving_line},{
                             .array_list_offset = offsetof(Departure_Infos, serving_lines),
                             .element_size      = sizeof(Serving_Line),
                         })}
            });

    Departure_Infos deps {};
    Pattern_Match_Result res = pattern_match(json_str, p, &deps);

    // write_pattern_to_file("out.json", p, &deps);

    defer {
        deps.free();
    };
    assert_equal_int(res, Pattern_Match_Result::OK_CONTINUE);


    assert_equal_int(deps.serving_lines.count, 3);
    {
        Serving_Line& s = deps.serving_lines[0];
        assert_equal_int(strncmp("Erding", s.destination.data, s.destination.length), 0);
        assert_equal_int(strncmp("ddb",    s.network.data, s.network.length), 0);
        assert_equal_int(strncmp("SBAHN",  s.product.data, s.product.length), 0);
        assert_equal_int(strncmp("S2",     s.line_number.data, s.line_number.length), 0);
        assert_equal_int(strncmp("92M02",  s.diva_id.data, s.diva_id.length), 0);
        assert_equal_int(s.sev, false);
    } {
        Serving_Line& s = deps.serving_lines[1];
        assert_equal_int(strncmp("Lohhof (S) Süd", s.destination.data, s.destination.length), 0);
        assert_equal_int(strncmp("mvv",            s.network.data, s.network.length), 0);
        assert_equal_int(strncmp("REGIONAL_BUS",   s.product.data, s.product.length), 0);
        assert_equal_int(strncmp("771",            s.line_number.data, s.line_number.length), 0);
        assert_equal_int(strncmp("19771",          s.diva_id.data, s.diva_id.length), 0);
        assert_equal_int(s.sev, false);
    } {
        Serving_Line& s = deps.serving_lines[2];
        assert_equal_int(strncmp("Petershausen", s.destination.data, s.destination.length), 0);
        assert_equal_int(strncmp("mvv",          s.network.data, s.network.length), 0);
        assert_equal_int(strncmp("RUFTAXI",      s.product.data, s.product.length), 0);
        assert_equal_int(strncmp("7280",         s.line_number.data, s.line_number.length), 0);
        assert_equal_int(strncmp("18728",        s.diva_id.data, s.diva_id.length), 0);
        assert_equal_int(s.sev, false);
    }

    assert_equal_int(deps.departures.count, 2);
    {
        Departure& d0 = deps.departures[0];
        assert_equal_string(d0.label, string_from_literal("S2"));
        assert_equal_string(d0.product, string_from_literal("SBAHN"));
        assert_equal_string(d0.destination, string_from_literal("Erding"));
        assert_equal_string(d0.line_background_color, string_from_literal("#9bc04c"));
        assert_equal_string(d0.departure_id, string_from_literal("c9b4b3875a8d2f53ad018b204899ffc0#1662174480000#de:09174:6840"));
        assert_equal_string(d0.platform, string_from_literal("6"));

        Departure& d1 = deps.departures[1];
        assert_equal_string(d1.label, string_from_literal("S2"));
        assert_equal_string(d1.product, string_from_literal("SBAHN"));
        assert_equal_string(d1.destination, string_from_literal("Erding"));
        assert_equal_string(d1.line_background_color, string_from_literal("#9bc04c"));
        assert_equal_string(d1.departure_id, string_from_literal("88cd45142d5ee37ba77295e93bb3b88f#1662175920000#de:09174:6840"));
        assert_equal_string(d1.platform, string_from_literal("6"));
    }

    return pass;
}

auto test_json_bug_again() -> testresult {
    const char* json = R"JSON({
  "servingLines" : [ {
    "destination" : "Tandern, Adlerstraße",
    "sev" : false,
    "network" : "mvv",
    "product" : "REGIONAL_BUS",
    "lineNumber" : "707",
    "divaId" : "19707"
  }, {
    "destination" : "Tandern, Adlerstraße",
    "sev" : false,
    "network" : "mvv",
    "product" : "RUFTAXI",
    "lineNumber" : "7070",
    "divaId" : "16707"
  } ],
  "departures" : [ {
    "departureTime" : 1671996780000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "delay" : 0,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "57749c348504ffe0de51b7cc9cb48c66#1671996780000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672000380000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "delay" : 0,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "7c8c797e8982f841c292f45b77df8e2c#1672000380000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672036380000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "15054e32f9768828b7a2ed8ba9d37a0a#1672036380000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672039980000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "e49700d23e4fdf250752e02c81c27507#1672039980000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672043580000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "4462969e1ce4b766d6f4534cc928f4fc#1672043580000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672047180000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "90a092fc041022ee713c851ca8a10245#1672047180000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672050780000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "11c9e9e2ff56f899101d3deedc659b93#1672050780000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672054380000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "607ce44954cccb9bc400b0a079b01f0d#1672054380000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672057980000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "855ed5734137dbc10d96ec9bcbd8c932#1672057980000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672061580000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "389a01d4bba3df19461fe5c0cd54406a#1672061580000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672065180000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "d72f91c1b28214f6906e4d117a1c2e31#1672065180000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672068780000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "e355774d343c72acbec55936a9b0042c#1672068780000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672072380000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "8f2ae7d4ae162e0fb816ed9a7b67de86#1672072380000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672075980000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "6e3948b1b3228643a7849703905b6e2f#1672075980000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  }, {
    "departureTime" : 1672079580000,
    "product" : "REGIONAL_BUS",
    "label" : "707",
    "destination" : "Tandern, Adlerstraße",
    "live" : false,
    "cancelled" : false,
    "lineBackgroundColor" : "#0d5c70",
    "departureId" : "08352cdbae115fd9811e4cd1d073907c#1672079580000#de:09174:7167",
    "sev" : false,
    "platform" : "",
    "stopPositionNumber" : 0,
    "infoMessages" : [ ]
  } ]
})JSON";

    struct Time {
        s32 year;
        u8  month;   // [1-12]
        u8  day;     // [1-31]
        u8  hour;    // [0-23]
        u8  minute;  // [0-59]
        f32 seconds; // [0-59.99…]
    };

    struct Departure {
        Time               departure_time;
        String             product;
        String             label;
        String             destination;
        bool               live;
        f32                delay;
        bool               cancelled;
        String             line_background_color;
        String             departure_id;
        bool               sev;
        String             platform;
        s32                stop_position_number;
        Array_List<String> info_messages;

        void free() {
            product.free();
            label.free();
            destination.free();
            line_background_color.free();
            departure_id.free();
            platform.free();

            for (auto& s: info_messages) {
                s.free();
            }

            info_messages.deinit();
        }
    };

    struct Serving_Line {
        String destination;
        bool   sev;
        String network;
        String product;
        String line_number;
        String diva_id;

        void free() {
            destination.free();
            network.free();
            product.free();
            line_number.free();
            diva_id.free();
        }

        void print() {
            println("Serving Line:");
            with_print_prefix("    ") {
                println("destination : %s", destination.data);
                println("sev         : %s", sev ? "yes" : "no");
                println("network     : %s", network);
                println("product     : %s", product);
                println("line_number : %s", line_number);
                println("diva_id     : %s", diva_id);
            }
        }
    };

    struct Departure_Infos {
        Array_List<Serving_Line> serving_lines;
        Array_List<Departure>    departures;

        void free() {
            for (auto& d : departures) {
                d.free();
            }
            departures.deinit();

            for (auto& s : serving_lines) {
                s.free();
            }
            serving_lines.deinit();
        }

    };

    using namespace json;
    Pattern departure = object(
        {
            // {"departureTime",       custom(Json_Type::Number, (Data_Type)Custom_Data_Types::UNIX_TIME_IN_MS, offsetof(Departure, departure_time))},
            {"product",             string  (offsetof(Departure, product))},
            {"label",               string  (offsetof(Departure, label))},
            {"destination",         string  (offsetof(Departure, destination))},
            {"live",                boolean_ (offsetof(Departure, live))},
            {"delay",               floating(offsetof(Departure, delay))},
            {"cancelled",           boolean_ (offsetof(Departure, cancelled))},
            {"lineBackgroundColor", string  (offsetof(Departure, line_background_color))},
            {"departureId",         string  (offsetof(Departure, departure_id))},
            {"sev",                 boolean_ (offsetof(Departure, sev))},
            {"platform",            string  (offsetof(Departure, platform))},
            {"stopPositionNumber",  integer (offsetof(Departure, stop_position_number))},
            {"infoMessages",        list    ({string(0)}, {
                        .array_list_offset = offsetof(Departure, info_messages),
                        .element_size      = sizeof(Departure::info_messages[0]),
                    })}
        });
    Pattern serving_line = object({
            {"destination", string(offsetof(Serving_Line, destination))},
            {"sev",         boolean_(offsetof(Serving_Line, sev))},
            {"network",     string(offsetof(Serving_Line, network))},
            {"product",     string(offsetof(Serving_Line, product))},
            {"lineNumber",  string(offsetof(Serving_Line, line_number))},
            {"divaId",      string(offsetof(Serving_Line, diva_id))},
        });
    Pattern p = object({
            {"departures", list(departure, {
                        .array_list_offset    = offsetof(Departure_Infos, departures),
                        .element_size         = sizeof(Departure_Infos::departures[0]),
                    })},
            {"servingLines", list(serving_line, {
                        .array_list_offset    = offsetof(Departure_Infos, serving_lines),
                        .element_size         = sizeof(Departure_Infos::serving_lines[0]),
                    })}
        });

    Departure_Infos deps {};
    deps.departures.allocator    = libc_allocator;
    deps.serving_lines.allocator = libc_allocator;
    Pattern_Match_Result res = pattern_match(json, p, &deps);

    defer {deps.free();};

    assert_equal_int(deps.serving_lines.count, 2);

    {
        Serving_Line& s = deps.serving_lines[0];
        assert_equal_string(s.destination, string_from_literal("Tandern, Adlerstraße"));
        assert_equal_int(s.sev, false);
        assert_equal_string(s.network, string_from_literal("mvv"));
        assert_equal_string(s.product, string_from_literal("REGIONAL_BUS"));
        assert_equal_string(s.line_number, string_from_literal("707"));
        assert_equal_string(s.diva_id, string_from_literal("19707"));
    } {
        Serving_Line& s = deps.serving_lines[1];
        assert_equal_string(s.destination, string_from_literal("Tandern, Adlerstraße"));
        assert_equal_int(s.sev, false);
        assert_equal_string(s.network, string_from_literal("mvv"));
        assert_equal_string(s.product, string_from_literal("RUFTAXI"));
        assert_equal_string(s.line_number, string_from_literal("7070"));
        assert_equal_string(s.diva_id, string_from_literal("16707"));
    }

    return pass;
}

s32 main(s32, char**) {
    testresult result;

    Leak_Detecting_Allocator ld;
    ld.init();
    Bookkeeping_Allocator bk;
    bk.init();
    defer {
        ld.deinit();
    };
    with_allocator(bk) {
        defer { bk.print_statistics(); };

        with_allocator(ld) {
            defer { ld.print_leak_statistics(); };


            invoke_test(test_json_simple_object_new_syntax);
            invoke_test(test_json_mvg);
            invoke_test(test_json_bug);
            invoke_test(test_json_config);
            invoke_test(test_json_bug_again);

            invoke_test(test_defer_runs_after_return);
            invoke_test(test_pool_allocator);
            invoke_test(test_growable_pool_allocator);
            invoke_test(test_bucket_list);
            invoke_test(test_bucket_list_leak);
            invoke_test(test_bucket_queue);
            invoke_test(test_kd_tree);
            invoke_test(test_math);
            invoke_test(test_hashmap);
            invoke_test(test_sort);
            invoke_test(test_array_lists_adding_and_removing);
            invoke_test(test_array_lists_sorting);
            invoke_test(test_array_lists_sorted_insert_and_remove);
            invoke_test(test_array_lists_searching);
            invoke_test(test_array_list_sort_many);
            invoke_test(test_string_split);
            invoke_test(test_stack_array_lists);
            invoke_test(test_typed_bucket_allocator);
            invoke_test(test_hooks);
            invoke_test(test_scheduler_animations);
        }
    }
    return 0;
}
