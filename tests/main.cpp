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

#define FTB_TRACK_MALLOCS
#define FTB_PRINT_IMPL
#define FTB_ALLOCATION_STATS_IMPL
#define FTB_HASHMAP_IMPL
#define FTB_SCHEDULER_IMPL
#define FTB_STACKTRACE_IMPL
#define FTB_MATH_IMPL
#define FTB_SOA_SORT_IMPL

#include "../print.hpp"
#include "../testing.hpp"
#include "../bucket_allocator.hpp"

#include "../hooks.hpp"
#include "../hashmap.hpp"
#include "../stacktrace.hpp"
#include "../scheduler.cpp"
#include "../math.hpp"
#include "../soa_sort.hpp"


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

auto is_sorted = [](Array_List<s32> list) -> bool {
    for (u32 i = 0; i < list.count - 1; ++i) {
        if (list.data[i] > list.data[i+1])
            return false;
    }
    return true;
 };

auto is_sorted_vp = [](Array_List<void*> list) -> bool {
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

auto test_hm() -> testresult {
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

    h2.for_each([] (Key k, u32 v, u32 i) {
        print("%{s32} %{u32} %{u32}\n", k.x, v, i);
    });

    return pass;
}

auto test_stack_array_lists() -> testresult {
    Stack_Array_List<int> list = create_stack_array_list(int, 20);

    assert_equal_int(list.count, 0);
    assert_equal_int(list.length, 20);
    assert_true(list.data != NULL);

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

auto test_queue() -> testresult {
    Queue<int> q;
    q.init(4);
    defer {
        q.deinit();
    };

    assert_true(q.is_empty());
    assert_equal_int(q.get_count(), 0);

    q.push_back(1);
    q.push_back(2);
    q.push_back(3);

    assert_equal_int(q.get_count(), 3);

    assert_equal_int(q.get_next(), 1);
    assert_equal_int(q.get_count(), 2);
    assert_true(!q.is_empty());

    assert_equal_int(q.get_next(), 2);
    assert_equal_int(q.get_count(), 1);
    q.push_back(4);
    assert_equal_int(q.get_count(), 2);

    assert_equal_int(q.get_next(), 3);
    assert_equal_int(q.get_count(), 1);
    assert_true(!q.is_empty());

    assert_equal_int(q.get_next(), 4);
    assert_true(q.is_empty());
    assert_equal_int(q.get_count(), 0);

    return pass;
}

auto test_array_lists_adding_and_removing() -> testresult {
    // test adding and removing
    Array_List<s32> list;
    list.init();

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
    list1.init();
    defer {
        list1.deinit();
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

    s32 index = list1.sorted_find(3);
    assert_equal_int(index, 2);

    index = list1.sorted_find(1);
    assert_equal_int(index, 0);

    index = list1.sorted_find(5);
    assert_equal_int(index, -1);
    return pass;
}

auto test_bucket_allocator() -> testresult {
    Bucket_Allocator<s32> ba;
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



    Bucket_Allocator<s32> ba2;
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
    list.init();
    defer {
        list.deinit();
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
        M3x3 identity {
            {1, 0, 0,
             0, 1, 0,
             0, 0, 1},
        };

        V3 r = identity * v1;
        assert_equal_f32(r.x, 1);
        assert_equal_f32(r.y, 2);
        assert_equal_f32(r.z, 3);

        M3x3 permute_scale {
            {0, 2, 0,
             0, 0, 2,
             2, 0, 0},
        };

        r = permute_scale * v1;
        assert_equal_f32(r.x, 4);
        assert_equal_f32(r.y, 6);
        assert_equal_f32(r.z, 2);
    }
    {
        V2   v1 { 1, 2 };
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

    u64 arr1[4] {
        4,1,2,3
    };

    struct LageTest {
        u64 stuff[4];
        const char* s;
        u32 lotta_stuff[12];
    };

    LageTest arr2[4] {
        {.s = "happy"},
        {.s = "Hello"},
        {.s = "World"},
        {.s = "I am"},
    };

    soa_sort(arr1, sizeof(arr1[0]), arr2, sizeof(arr2[0]), array_length(arr1), [] (const void* a, const void* b) -> s32 {
        u32 sa = *(u32*)a;
        u32 sb = *(u32*)b;
        return sa - sb;
    });

    for (u32 i = 0; i < array_length(arr1); ++i) {
        print("\n%d - %s", arr1[i], arr2[i].s);
    }

    return pass;
}

s32 main(s32, char**) {
    defer { print_malloc_stats(); };

    testresult result;

    invoke_test(test_sort);
    // invoke_test(test_math);
    // invoke_test(test_array_lists_adding_and_removing);
    // invoke_test(test_array_lists_sorting);
    // invoke_test(test_array_lists_searching);
    // invoke_test(test_array_list_sort_many);
    // invoke_test(test_stack_array_lists);
    // invoke_test(test_bucket_allocator);
    // invoke_test(test_queue);
    // invoke_test(test_hooks);
    // invoke_test(test_scheduler_animations);

    return 0;
}
