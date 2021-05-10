#define _CRT_SECURE_NO_WARNINGS
#include <stdio.h>
#include <string.h>
#include "../types.hpp"

u32 hm_hash(u32 u);
inline bool hm_objects_match(u32 a, u32 b);
struct Key;
u32 hm_hash(Key u);
inline bool hm_objects_match(Key a, Key b);

#define ZoneScoped
#define ZoneScopedN(name)

#include "../print.hpp"
#include "../testing.hpp"
#include "../bucket_allocator.hpp"
#include "../error.hpp"
#include "../hooks.hpp"
#include "../hashmap.hpp"
#include "../stacktrace.hpp"
#include "../scheduler.cpp"

#include "../error.hpp"


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

proc is_sorted = [](Array_List<s32> list) -> bool {
    for (u32 i = 0; i < list.count - 1; ++i) {
        if (list.data[i] > list.data[i+1])
            return false;
    }
    return true;
 };

proc is_sorted_vp = [](Array_List<void*> list) -> bool {
    for (u32 i = 0; i < list.count - 1; ++i) {
        if (list.data[i] > list.data[i+1])
            return false;
    }
    return true;
 };


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

proc test_stack_array_lists() -> testresult {
    Stack_Array_List<int> list(20);

    assert_equal_int(list.count, 0);
    assert_equal_int(list.length, 20);
    assert(list.data != NULL, "list should have some data allocated");

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

proc test_queue() -> testresult {
    Queue<int> q;
    q.alloc(4);
    defer {
        q.dealloc();
    };

    assert(q.is_empty(), "queue should start empty");
    assert_equal_int(q.get_count(), 0);

    q.push_back(1);
    q.push_back(2);
    q.push_back(3);

    assert_equal_int(q.get_count(), 3);

    assert_equal_int(q.get_next(), 1);
    assert_equal_int(q.get_count(), 2);
    assert(!q.is_empty(), "should not be empty");

    assert_equal_int(q.get_next(), 2);
    assert_equal_int(q.get_count(), 1);
    q.push_back(4);
    assert_equal_int(q.get_count(), 2);

    assert_equal_int(q.get_next(), 3);
    assert_equal_int(q.get_count(), 1);
    assert(!q.is_empty(), "should not be empty");

    assert_equal_int(q.get_next(), 4);
    assert(q.is_empty(), "should be empty");
    assert_equal_int(q.get_count(), 0);

    return pass;
}

proc test_array_lists_adding_and_removing() -> testresult {
    // test adding and removing
    Array_List<s32> list;
    list.alloc();

    defer {
        list.dealloc();
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


proc test_array_lists_sorting() -> testresult {
    //
    //
    //  Test simple numbers
    //
    //
    Array_List<s32> list;
    list.alloc();
    defer {
        list.dealloc();
    };

    list.append(1);
    list.append(2);
    list.append(3);
    list.append(4);

    list.sort();
    assert_equal_int(is_sorted(list), true);

    list.append(4);
    list.append(2);
    list.append(1);

    assert_equal_int(is_sorted(list), false);
    list.sort();
    assert_equal_int(is_sorted(list), true);

    list.clear();
    list.extend({
            8023, 7529, 2392, 7110,
            3259, 2484, 9695, 2199,
            6729, 9009, 8429, 7208});
    assert_equal_int(is_sorted(list), false);
    list.sort();
    assert_equal_int(is_sorted(list), true);


    //
    //
    //  Test adding and removing
    //
    //
    Array_List<s32> list1;
    list1.alloc();
    defer {
        list1.dealloc();
    };

    list1.append(1);
    list1.append(2);
    list1.append(3);
    list1.append(4);

    list1.sort();

    assert_equal_int(list1.count, 4);

    assert_equal_int(list1[0], 1);
    assert_equal_int(list1[1], 2);
    assert_equal_int(list1[2], 3);
    assert_equal_int(list1[3], 4);
    assert_equal_int(is_sorted(list1), true);

    list1.append(0);
    list1.append(5);

    assert_equal_int(list1.count, 6);

    list1.sort();

    assert_equal_int(list1[0], 0);
    assert_equal_int(list1[1], 1);
    assert_equal_int(list1[2], 2);
    assert_equal_int(list1[3], 3);
    assert_equal_int(list1[4], 4);
    assert_equal_int(list1[5], 5);
    assert_equal_int(is_sorted(list1), true);

    //
    //
    //
    //
    // pointer list
    Array_List<void*> al;
    al.alloc();
    defer {
        al.dealloc();
    };
    al.append((void*)0x1703102F100);
    al.append((void*)0x1703102F1D8);
    al.append((void*)0x1703102F148);
    al.append((void*)0x1703102F190);
    al.append((void*)0x1703102F190);
    al.append((void*)0x1703102F1D8);

    assert_equal_int(is_sorted_vp(al), false);
    al.sort();
    assert_equal_int(is_sorted_vp(al), true);

    assert_not_equal_int(al.sorted_find((void*)0x1703102F100), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F1D8), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F148), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F190), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F190), -1);
    assert_not_equal_int(al.sorted_find((void*)0x1703102F1D8), -1);

    return pass;
}

proc test_array_lists_searching() -> testresult {
    Array_List<s32> list1;
    list1.alloc();
    defer {
        list1.dealloc();
    };

    list1.append(1);
    list1.append(2);
    list1.append(3);
    list1.append(4);

    s32 index = list1.sorted_find(3);
    assert_equal_int(index, 2);

    index = list1.sorted_find(1);
    assert_equal_int(index, 0);

    index = list1.sorted_find(5);
    assert_equal_int(index, -1);
    return pass;
}

proc test_bucket_allocator() -> testresult {
    Bucket_Allocator<s32> ba;
    ba.alloc();
    defer {
        ba.dealloc();
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



    Bucket_Allocator<s32> ba2;
    ba2.alloc();
    defer {
        ba2.dealloc();
    };

    s1 = ba2.allocate();
    s2 = ba2.allocate();
    s3 = ba2.allocate();


    s32* s3_copy = ba2.allocate();
    s32* s3_copy2 = ba2.allocate();
    s32* s3_copy3 = ba2.allocate();
    *s3_copy = 3;
    ba2.free_object(s3_copy);


    s4 = ba2.allocate();

    ba2.free_object(s3_copy2);

    s5 = ba2.allocate();

    ba2.free_object(s3_copy3);

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
    list.alloc();
    defer {
        list.dealloc();
    };

    for (int i = 0; i < 10000; ++i) {
        list.append(rand());
    }

    assert_equal_int(is_sorted(list), false);
    list.sort();
    assert_equal_int(is_sorted(list), true);

    for (int i = 0; i < 10000; ++i) {
        assert_not_equal_int(list.sorted_find(list.data[i]), -1);
    }

    list.clear();
    for (int i = 0; i < 1111; ++i) {
        list.append(rand());
    }

    assert_equal_int(is_sorted(list), false);
    list.sort();
    assert_equal_int(is_sorted(list), true);

    for (int i = 0; i < 1111; ++i) {
        assert_not_equal_int(list.sorted_find(list.data[i]), -1);
    }


    list.clear();
    for (int i = 0; i < 3331; ++i) {
        list.append(rand());
    }

    assert_equal_int(is_sorted(list), false);
    list.sort();
    assert_equal_int(is_sorted(list), true);

    for (int i = 0; i < 3331; ++i) {
        assert_not_equal_int(list.sorted_find(list.data[i]), -1);
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
    using namespace Scheduler;

    Scheduler::init();
    defer { Scheduler::deinit(); };

    f32 val = 0;

    f32 from = 1;
    f32 to   = 2;
    schedule_animation({
            .seconds_to_start   = 1,
            .seconds_to_end     = 2,
            .interpolant        = &val,
            .interpolant_type   = Interpolant_Type::F32,
            .from               = &from,
            .to                 = &to,
            .interpolation_type = Interpolation_Type::Lerp
        });

    assert_equal_f64((f64)val, 0.0);

    update_all(1);
    assert_equal_f64((f64)val, 1.0);

    update_all(0.1);
    assert_equal_int(abs(val - 1.1) < 0.001, true);

    update_all(0.1);
    assert_equal_int(abs(val - 1.2) < 0.001, true);

    update_all(0.2);
    assert_equal_int(abs(val - 1.4) < 0.001, true);

    update_all(1);
    assert_equal_int(abs(val - 2) < 0.001, true);

    // testing custom type interpolation
    enum My_Interpolant_Type : u8 {
        S32
    };

    register_interpolator([](void* p_from, f32 t, void* p_to, void* p_interpolant) {
        s32 from = *(s32*)p_from;
        s32 to   = *(s32*)p_to;
        s32* target = (s32*)p_interpolant;
        *target = from + (to - from) * t;
    }, (Interpolant_Type)My_Interpolant_Type::S32, sizeof(s32));

    s32 test = 0;

    s32 s_from = 1;
    s32 s_to   = 11;
    schedule_animation({
            .seconds_to_start   = 1,
            .seconds_to_end     = 2,
            .interpolant        = &test,
            .interpolant_type   = (Interpolant_Type)My_Interpolant_Type::S32,
            .from               = &s_from,
            .to                 = &s_to,
            .interpolation_type = Interpolation_Type::Lerp
        });

    assert_equal_int(test, 0);

    update_all(1);
    assert_equal_int(test, 1);

    update_all(0.1);
    assert_equal_int(test, 2);

    update_all(0.1);
    assert_equal_int(test, 3);

    update_all(0.2);
    assert_equal_int(test, 5);

    update_all(1);
    assert_equal_int(test, 11);

    return pass;
}

s32 main(s32, char**) {
    init_printer();
    defer { deinit_printer(); };
    testresult result;

    invoke_test(test_array_lists_adding_and_removing);
    invoke_test(test_array_lists_sorting);
    invoke_test(test_array_lists_searching);
    invoke_test(test_array_list_sort_many);
    invoke_test(test_stack_array_lists);
    invoke_test(test_bucket_allocator);
    invoke_test(test_queue);
    invoke_test(test_hooks);
    invoke_test(test_scheduler_animations);


    return 0;
}
