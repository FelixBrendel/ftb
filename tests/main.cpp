// TODO:
// - scheduler scheduling its own reset


#define _CRT_SECURE_NO_WARNINGS
#include <cstddef>
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
#define FTB_MESH_IMPL
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
#include "../ringbuffer.hpp"
#include "../hashmap.hpp"
#include "../scheduler.hpp"
#include "../soa_sort.hpp"
#include "../kd_tree.hpp"
#include "../mesh.hpp"


auto my_int_cmp = [](const int* a, const int* b) -> s32 {
    return (s32)(*a - *b);
};

auto my_voidp_cmp = [](void* const * a, void* const * b) -> s32 {
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

auto test_obj_to_json_str() -> testresult {
    using namespace json;

    enum struct Color {
        Red, Green, Blue
    };

    struct Sub_Data {
        s32   sub_int;
        bool  sub_bool;
        Color sub_color;

        bool operator==(Sub_Data& other) {
            return
                sub_int   == other.sub_int  &&
                sub_bool  == other.sub_bool &&
                sub_color == other.sub_color;
        }
    };
    struct Data {
        s32      integer;
        f32      floating;
        String   string;
        Array_List<Sub_Data> sub_data;

        void free() {
            string.free();
            sub_data.deinit();
        }

        bool operator==(Data& other) {
            bool lists_same = sub_data.count == other.sub_data.count;
            if (lists_same) {
                for (u32 i = 0; i < sub_data.count; ++i) {
                    lists_same &= sub_data[i] == other.sub_data[i];
                }
            }

            return
                integer  == other.integer  &&
                floating == other.floating &&
                string   == other.string   &&
                lists_same;
        }
    };

    auto p_color = [](u32 offset) -> Pattern {
        return json::custom(
            Json_Type::String, offset,
            {
                .custom_reader = [](const char* point, void* out_color_enum) -> u32 {
                    String str;
                    u32 length = read_string(point, &str);
                    defer { str.free(); };
                    Color* out_color = (Color*)out_color_enum;

                    if     (str == string_from_literal("red"))   *out_color = Color::Red;
                    else if(str == string_from_literal("green")) *out_color = Color::Green;
                    else                                             *out_color = Color::Blue;

                    return length;
                },
                .custom_writer = [](FILE* out_file, void* color_to_write) -> u32 {
                    Color* out_color = (Color*)color_to_write;
                    switch (*out_color) {
                        case Color::Red:   return print_to_file(out_file, "\"red\"");
                        case Color::Green: return print_to_file(out_file, "\"green\"");
                        case Color::Blue:  return print_to_file(out_file, "\"blue\"");
                        default: panic("Unknown color %d", *out_color);
                    }
                }
            });
    };

    Pattern p_data = object({
            {"integer",  p_s32(offsetof(Data, integer))},
            {"floating", p_f32(offsetof(Data, floating))},
            {"string",   p_str(offsetof(Data, string))},
            {"sub_data", list(object({
                        {"sub_int",   p_s32(offsetof(Sub_Data,  sub_int))},
                        {"sub_bool",  p_bool(offsetof(Sub_Data, sub_bool))},
                        {"sub_color", p_color(offsetof(Sub_Data, sub_color))}
                    }),
                    {
                        .array_list_offset = offsetof(Data, sub_data),
                        .element_size      = sizeof(Data::sub_data[0]),
                    })},
    });

    const char* json_str = R"JSON(
    {
        "integer"  : 123,
        "floating" : 3.1415,
        "string"   : "Hello There",
        "sub_data" : [
             {
                 "sub_int"  : 333,
                 "sub_bool" : false,
                 "sub_color": "red",
             },
             {
                 "sub_int"  : 555,
                 "sub_bool" : true,
                 "sub_color": "blue",
             },
        ]
    })JSON";

    Data data {};
    defer { data.free(); };
    Pattern_Match_Result result = pattern_match(json_str, p_data, &data);

    assert_true(result == Pattern_Match_Result::OK_DONE ||
                result == Pattern_Match_Result::OK_CONTINUE);

    assert_equal_int(data.integer, 123);
    assert_equal_f32(data.floating, 3.1415);
    assert_equal_string(data.string, string_from_literal("Hello There"));
    assert_equal_int(data.sub_data.count, 2);
    assert_equal_int(data.sub_data[0].sub_int, 333);
    assert_equal_int(data.sub_data[0].sub_bool, false);
    assert_equal_int(data.sub_data[0].sub_color, Color::Red);
    assert_equal_int(data.sub_data[1].sub_int, 555);
    assert_equal_int(data.sub_data[1].sub_bool, true);
    assert_equal_int(data.sub_data[1].sub_color, Color::Blue);

    Allocated_String exported_json_str = write_pattern_to_string(p_data, &data);
    defer { exported_json_str.free(); };

    Data data_reparsed {};
    defer { data_reparsed.free(); };
    result = pattern_match(exported_json_str.string.data, p_data, &data_reparsed);

    assert_true(result == Pattern_Match_Result::OK_DONE ||
                result == Pattern_Match_Result::OK_CONTINUE);

    assert_true(data == data_reparsed);

    return pass;
}

auto print_dots(FILE* f) -> u32 {
    return print_to_file(f, "...");
}

auto test_scratch_arena_can_realloc_last_alloc() -> testresult {

    Linear_Allocator* tmp_alloc = nullptr;
    u64 ptr_before;

    {
        Scratch_Arena scratch = scratch_arena_start();
        defer { scratch_arena_end(scratch); };

        tmp_alloc = (Linear_Allocator*)scratch.arena;
        ptr_before = tmp_alloc->last_segment->count;

        Array_List<s32> al;
        al.init(8, scratch.arena);

        u64 ptr_after_first_alloc = tmp_alloc->last_segment->count;
        s32* data_ptr_after_first_alloc = al.data;

        assert_not_equal_int(ptr_after_first_alloc, ptr_before);

        // NOTE(Felix): force expand
        for (u32 i = 0; i < 10; ++i) {
            al.append(i);
        }

        u64 ptr_after_second_alloc = tmp_alloc->last_segment->count;
        assert_not_equal_int(ptr_after_second_alloc, ptr_before);
        assert_not_equal_int(ptr_after_second_alloc, ptr_after_first_alloc);

        // NOTE(Felix): Because it was the last allocation, the base pointer
        //   should not have changed even though we reallocated
        assert_equal_int(data_ptr_after_first_alloc, al.data);

        // NOTE(Felix): Alloc something new to disrupt the temp memory, and then
        // force a reallocate
        int* i = scratch.arena->allocate<int>(1);
        assert_not_equal_int(tmp_alloc->last_segment->count, ptr_after_second_alloc);

        u32 length = al.length;
        for (u32 i = al.count; i <= length; ++i) {
            al.append(i);
        }

        u64 ptr_after_third_alloc = tmp_alloc->last_segment->count;
        assert_not_equal_int(ptr_after_second_alloc, ptr_after_third_alloc);
    }

    // NOTE(Felix): Everything should be off the stack by now!
    assert_equal_int(tmp_alloc->last_segment->count, ptr_before);

    return pass;
}

auto test_printer() -> testresult {
    u32 arr[]   = {1,2,3,4,1,1,3};
    f32 f_arr[] = {1.1,2.1,3.2};


    register_printer("dots", print_dots, Printer_Function_Type::_void);

    u32 u1 = -1;
    u64 u2 = -1;

    char* str;
    auto alloc = grab_current_allocator();
    print_to_string(&str, alloc, " - %{dots[5]} %{->} <> %{->,2}\n", &u1, &arr, nullptr);
    defer { alloc->deallocate(str); };

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

    println(" - %{->char}%{->char}%{->char}",
            true   ? "general "     : "",
            false  ? "validation "  : "",
            false  ? "performance " : "");

    with_print_prefix("|  ") {
        print_str_lines("Hello\nline2\n3\n4", 2);
    }

    // print("%{->char}%{->char}\n\n", "hallo","");

    return pass;
}

auto test_math_matrix_compose() -> testresult {
    M4x4 model = m4x4_model({1, 2, 3}, quat_from_XYZ(12, 34, 59), {1, 2, 3});
    M3x3 R {};
    V3   t {};

    m4x4_decompose(model, &R, &t);
    assert_equal_f32(t.x, 1);
    assert_equal_f32(t.y, 2);
    assert_equal_f32(t.z, 3);

    M4x4 rebuild = m4x4_compose(R, t);

    for (int i = 0; i < array_length(M4x4::elements); ++i) {
        assert_equal_f32(model[i], rebuild[i]);
    }

    return pass;
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

    scheduler.register_interpolator([](void* p_from, f32 t, void* p_to, void* p_interpolant) -> void {
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
        V3 l = linerp(a, 0, b);

        assert_equal_f32(l.x, a.x);
        assert_equal_f32(l.y, a.y);
        assert_equal_f32(l.z, a.z);

        l = linerp(a, 1, b);

        assert_equal_f32(l.x, b.x);
        assert_equal_f32(l.y, b.y);
        assert_equal_f32(l.z, b.z);

        l = linerp(a, 0.5, b);

        assert_equal_f32(l.x, 0.5);
        assert_equal_f32(l.y, 11.0);
        assert_equal_f32(l.z, 0.5);
    }
    {
        V2 a = { 1,  1 };
        V2 b = { 0, 21 };
        V2 l = linerp(a, 0, b);

        assert_equal_f32(l.x, a.x);
        assert_equal_f32(l.y, a.y);

        l = linerp(a, 1, b);

        assert_equal_f32(l.x, b.x);
        assert_equal_f32(l.y, b.y);

        l = linerp(a, 0.5, b);

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
#if defined(FTB_NO_SIMD_TESTS)
    return skipped;
#else
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
#endif
}

auto test_bucket_list_in_linear_allocator() -> testresult {
    // NOTE(Felix): This function tests:
    // ** 1) Bucket allocators can use a linear allocator to back the memory
    // ** 2) It first fills up the linear segment the linear allocator has and
    //    then requires the linear allocator to get a new segment and then it
    //    is correclty used by the bucket list

    Bookkeeping_Allocator bk_outside; // sees all allocs made from the linear allocator
    bk_outside.init();

    Linear_Allocator la;
    u32 bucket_size  = 4;
    u32 bucket_count = 3;

    // allocating one bucket too few so we gotta get a new segment for the third bucket
    la.init(sizeof(s32)*bucket_size*(bucket_count-1) + bucket_count*sizeof(s32*) + 6*sizeof(u64), &bk_outside.base);
    defer { la.deinit(); };

    Bookkeeping_Allocator bk_inside; // sees all allocs made from the bucket_list
    bk_inside.init(&la.base);

    Bucket_List<s32> bl;
    bl.init(bucket_size, bucket_count, &bk_inside.base);

    auto total_alloc_calls = [](Bookkeeping_Allocator ba) -> u32 {
        return ba.num_allocate_0_calls + ba.num_allocate_calls;
    };

    assert_equal_int(total_alloc_calls(bk_inside), 2);  // two inside allocations (bucket array and 1st data array)

    assert_equal_int(total_alloc_calls(bk_outside), 1); // Lin allocator only saw one allocation so far
    bl.append(1);
    bl.append(2);
    assert_equal_int(bl.count_elements(), 2);            // make sure it has the correct number of elements
    bl.append(3);
    assert_equal_int(total_alloc_calls(bk_outside), 1);  // stil only 1 outside allocation so far

    f32* f1 = la.base.allocate<f32>(1); // allocate something unrelated from the linear allocator
    assert_true((((u8*)f1) > ((u8*)bl.buckets[bl.next_bucket_index]))); // f1 is after the first bucket

    bl.append(4);                                        // fill the current bucket allocate new one
    assert_equal_int(total_alloc_calls(bk_inside),  3);  // the new data array

    assert_true((((u8*)f1) < ((u8*)bl.buckets[bl.next_bucket_index]))); // f1 is before the second bucket
    assert_equal_int(total_alloc_calls(bk_outside), 1);  // no new outside allocation, still fits in the same linear segment
    bl.append(5);
    bl.append(6);
    bl.append(7);
    assert_equal_int(bl.count_elements(), 7);            // make sure it has the correct number of elements
    assert_equal_int(total_alloc_calls(bk_inside),  3);  // still same allocations
    assert_equal_int(total_alloc_calls(bk_outside), 1);  // still same allocations

    bl.append(8);
    assert_equal_int(total_alloc_calls(bk_inside),  4);  // Now next data array necessary
    assert_equal_int(total_alloc_calls(bk_outside), 2);  // Now new linear segment necessary

    // make sure the new data array is allocated in the new segment
    u8* bucket_ptr = (u8*)bl.buckets[bl.next_bucket_index];
    assert_true(bucket_ptr >= (u8*)la.last_segment->data);
    assert_true(bucket_ptr < (u8*)la.last_segment->data + la.last_segment->length);

    assert_equal_int(bl.count_elements(), 8);            // make sure it has the correct number of elements


    bl.clear();
    assert_equal_int(bl.count_elements(), 0);            // make sure it is empty
    assert_equal_int(bl.next_bucket_index, 0);           // make sure it is empty
    assert_equal_int(bl.next_index_in_latest_bucket, 0); // make sure it is empty

    assert_equal_int(bk_inside.num_deallocate_calls,  0);
    bl.deinit();
    assert_equal_int(bk_inside.num_deallocate_calls,  4);

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

auto test_linear_allocator_growing_and_resetting() -> testresult {
    // - lin allocator + grow
    // - lin allocator + grow + reset should not leak


    Bookkeeping_Allocator bk;
    bk.init();
    Allocator_Base* bk_alloc = &bk.base;

    Linear_Allocator la;
    u64 desired_seg_size = sizeof(u64)*4;
    la.init(desired_seg_size, bk_alloc);
    Allocator_Base* lin_alloc = &la.base;

    assert_equal_int(la.standard_segment_size, desired_seg_size); // initted correcty
    assert_equal_int(bk.num_allocate_calls, 1);                // only 1 external allocation
    {
        defer { la.deinit(); };

        auto count_segment_chain_length = [](Linear_Allocator lin_alloc) -> u32 {
            u32 length = 1;
            Linear_Segment* cursor = lin_alloc.last_segment;
            while (cursor->prev_segment != nullptr) {
                cursor = cursor->prev_segment;
                ++length;
            }
            return length;
        };

        assert_equal_int(la.last_segment->length, desired_seg_size); // correct segment size
        assert_equal_int(la.last_segment->prev_segment, nullptr);    // not grown yet
        u64* num1 = lin_alloc->allocate<u64>(1);
        assert_not_equal_int(num1, nullptr);
        assert_equal_int(count_segment_chain_length(la), 1); // not grown yet
        u64* num2 = lin_alloc->allocate<u64>(1);
        assert_not_equal_int(num2, nullptr);
        assert_equal_int(count_segment_chain_length(la), 1); // not grown yet

        // NOTE(Felix): Not grown yet, but we allocate the last allocation
        // **** size before the actual data, so the allocator should be full
        //      now, and next allocation should allocate the next block;
        assert_equal_int(la.last_segment->count, la.last_segment->length);

        u64* num3 = lin_alloc->allocate<u64>(1);
        assert_not_equal_int(num3, nullptr);                    // got a pointer
        assert_equal_int(bk.num_allocate_calls, 2);             // only 2 external allocations now
        assert_equal_int(bk.num_deallocate_calls, 0);           // nothing freed yet
        assert_equal_int(count_segment_chain_length(la), 2);    // did grow once
        assert_equal_int(la.last_segment->length, desired_seg_size); // correct segment size

        u64* num4 = lin_alloc->allocate<u64>(1);
        assert_not_equal_int(num4, nullptr);                                 // got a pointer
        assert_equal_int(count_segment_chain_length(la), 2);                 // did grow once
        assert_equal_int(la.last_segment->count, la.last_segment->length);   // should be full again

        u64* num5 = lin_alloc->allocate<u64>(1);
        assert_not_equal_int(num5, nullptr);                   // got a pointer
        assert_equal_int(bk.num_allocate_calls, 3);            // only 3 external allocations now
        assert_equal_int(bk.num_deallocate_calls, 0);          // nothing freed yet
        assert_equal_int(count_segment_chain_length(la), 3);   // did grow twice
        assert_equal_int(la.last_segment->length, desired_seg_size); // correct segment size

        u64* num6 = lin_alloc->allocate<u64>(1);
        assert_not_equal_int(num6, nullptr);                   // got a pointer
        assert_equal_int(count_segment_chain_length(la), 3);   // did grow twice

        u64* num7 = lin_alloc->allocate<u64>(1);
        assert_not_equal_int(num7, nullptr);                   // got a pointer
        assert_equal_int(bk.num_allocate_calls, 4);            // 4 external allocations now                                          // only 3 external allocations now
        assert_equal_int(count_segment_chain_length(la), 4);   // did grow 3 times
        assert_equal_int(la.last_segment->length, desired_seg_size); // correct segment size

        // last segment only half full now but allocate oversized so we need a
        // full new segment
        struct Big {
            u64 big_numbers[16];
        };

        Big* big = lin_alloc->allocate<Big>(1);
        assert_not_equal_int(big, nullptr);                    // got a pointer
        assert_equal_int(bk.num_allocate_calls, 5);            // 5 external allocations now
        assert_equal_int(la.last_segment->length, sizeof(Big)+sizeof(u64)); // correct segment size
        assert_equal_int(la.last_segment->count, la.last_segment->length);  // directly full


        la.reset();
        assert_equal_int(bk.num_deallocate_calls, 4);             // freed both additional segments
        assert_equal_int(la.last_segment->prev_segment, nullptr); // only one segment now
        assert_equal_int(la.last_segment->count, 0);              // only one segment now

    }

    // everything allocated by the lin alloc is freed again
    assert_equal_int(bk.num_allocate_calls, bk.num_deallocate_calls);

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


auto test_json_simple_object_json5() -> testresult {
    using namespace json;
    const char* json_object = R"JSON(
        {
          int:    123,
          float:  123.123,
          bool:   true,
          string: "Hello",
          object: {
             member : 1
          }
        }
)JSON";

    struct Test {
        s32    i;
        f32    f;
        bool   b;
        String s;
        s32    m;
    };

    Pattern p = json::object({
        {"int",    json::p_s32 (offsetof(Test, i))},
        {"float",  json::p_f32 (offsetof(Test, f))},
        {"bool",   json::p_bool(offsetof(Test, b))},
        {"string", json::p_str (offsetof(Test, s))},
        {"object", json::object({
                    {"member", p_s32(offsetof(Test, m))}
                })},
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
    assert_equal_int(t.m, 1);
    assert_equal_int(strncmp("Hello", t.s.data, 5), 0);

    return pass;
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
        {"int",    json::p_s32 (offsetof(Test, i))},
        {"float",  json::p_f32 (offsetof(Test, f))},
        {"bool",   json::p_bool(offsetof(Test, b))},
        {"string", json::p_str (offsetof(Test, s))},
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


auto test_json_wildcard_match_and_parser_context() -> testresult {
    using namespace json;
    const char* json_str = R"JSON({
    "list"         : ["my", "list", "contents", "is", "just", "strings"],
    "123"          : {"value" : 5},
    "wildcard"     : {"value" : 4},
    "other things" : {"value" : 3},
    "list_pefix"   : {"value" : 2},
    "postfix_list" : {"value" : 1},
})JSON";

    struct Wildcard_Obj {
        String key;
        s32    value;
    };

    struct List_Entry {
        s32    idx;
        String value;
    };

    struct Match_Target {
        Array_List<List_Entry>   list;
        Array_List<Wildcard_Obj> objs;
    };

    Hooks string_list_hooks = {
        .leave_hook = [](void* matched_obj, void* callback_data,
                         Hook_Context h_context, Parser_Context p_context)
        -> Pattern_Match_Result
        {
            List_Entry* entry = (List_Entry*)matched_obj;
            panic_if(p_context.context_stack.parent_type != Parser_Context_Type::List_Entry,
                     "p_context.context_stack.parent_type != Parser_Context_Type::List_Entry");
            entry->idx = p_context.context_stack.parent.list_entry.index;
            return Pattern_Match_Result::OK_CONTINUE;
        }
    };

    Hooks wildcard_list_hooks = {
        .leave_hook = [](void* matched_obj, void* callback_data,
                         Hook_Context h_context, Parser_Context p_context)
        -> Pattern_Match_Result
        {
            Wildcard_Obj* entry = (Wildcard_Obj*)matched_obj;
            panic_if(p_context.context_stack.parent_type != Parser_Context_Type::Object_Member,
                     "p_context.context_stack.parent_type != Parser_Context_Type::Object_Member");

            panic_if(p_context.context_stack.previous->parent_type != Parser_Context_Type::Object_Member,
                     "p_context.context_stack.previous->parent_type != Parser_Context_Type::Object_Member");
            entry->key = p_context.context_stack.previous->parent.object.member_name;
            return Pattern_Match_Result::OK_CONTINUE;
        }
    };

    Pattern json_pattern =
        object({{
                    "list", list({
                            p_str(offsetof(List_Entry, value), string_list_hooks)
                        }, {
                            .array_list_offset = offsetof(Match_Target, list),
                            .element_size      = sizeof(Match_Target::list[0])
                        })
                }},
            {(Fallback_Pattern){
                .pattern = object({{
                            "value", p_s32(offsetof(Wildcard_Obj, value), wildcard_list_hooks)
                        }}),
                .array_list_offset = offsetof(Match_Target, objs),
                .element_size      = sizeof(Match_Target::objs[0]),
            }}
            );

    Match_Target target {};

    Pattern_Match_Result match_result = pattern_match(json_str, json_pattern, &target);
    assert_equal_int(target.list.count, 6);
    assert_equal_string(target.list[0].value, string_from_literal("my"));
    assert_equal_string(target.list[1].value, string_from_literal("list"));
    assert_equal_string(target.list[2].value, string_from_literal("contents"));
    assert_equal_string(target.list[3].value, string_from_literal("is"));
    assert_equal_string(target.list[4].value, string_from_literal("just"));
    assert_equal_string(target.list[5].value, string_from_literal("strings"));

    assert_equal_int(target.list[0].idx, 0);
    assert_equal_int(target.list[1].idx, 1);
    assert_equal_int(target.list[2].idx, 2);
    assert_equal_int(target.list[3].idx, 3);
    assert_equal_int(target.list[4].idx, 4);
    assert_equal_int(target.list[5].idx, 5);

    assert_equal_int(target.objs.count, 5);

    assert_equal_int(target.objs[0].value, 5);
    assert_equal_int(target.objs[1].value, 4);
    assert_equal_int(target.objs[2].value, 3);
    assert_equal_int(target.objs[3].value, 2);
    assert_equal_int(target.objs[4].value, 1);

    assert_equal_string(target.objs[0].key, string_from_literal("123"));
    assert_equal_string(target.objs[1].key, string_from_literal("wildcard"));
    assert_equal_string(target.objs[2].key, string_from_literal("other things"));
    assert_equal_string(target.objs[3].key, string_from_literal("list_pefix"));
    assert_equal_string(target.objs[4].key, string_from_literal("postfix_list"));

    for (auto elem : target.list) {
        elem.value.free();
    }
    target.list.deinit();
    target.objs.deinit();


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
        {"config_version",     p_s32(offsetof(Tafel_Config, config_version))},
        {"db_client_id",       p_str (offsetof(Tafel_Config, db_client_id))},
        {"db_client_secret",   p_str (offsetof(Tafel_Config, db_client_secret))},
        {"mvg_location_names", list({p_str(0)}, {
                .array_list_offset = offsetof(Tafel_Config, mvg_locations),
                .element_size      = sizeof(Tafel_Config::mvg_locations[0]),
                })},
        {"db_station_names",  list({p_str(0)}, {
                .array_list_offset = offsetof(Tafel_Config, db_stations),
                .element_size      = sizeof(Tafel_Config::db_stations[0]),
                })},
        {"destination_blacklist", list({p_str(0)}, {
                .array_list_offset = offsetof(Tafel_Config, destination_blacklist),
                .element_size      = sizeof(Tafel_Config::destination_blacklist[0]),
                })},
        // {nullptr, list({string(0)}, {
        //             .array_list_offset = offsetof(Tafel_Config, destination_blacklist),
        //             .element_size      = sizeof(Tafel_Config::destination_blacklist[0]),
        //         }

        //         )},
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
        object({{"products", list({p_str(0)}, {
                            .array_list_offset = offsetof(Location, products),
                            .element_size = sizeof(String)
                        })},
                {"type",          p_str (offsetof(Location, type))},
                {"latitude",      p_f32 (offsetof(Location, latitude))},
                {"longitude",     p_f32 (offsetof(Location, longitude))},
                {"id",            p_str (offsetof(Location, id))},
                {"divaId",        p_s32 (offsetof(Location, divaId))},
                {"place",         p_str (offsetof(Location, place))},
                {"name",          p_str (offsetof(Location, name))},
                {"has_live_data", p_bool(offsetof(Location, has_live_data))},
                {"has_zoom_data", p_bool(offsetof(Location, has_zoom_data))},
                {"aliases",       p_str (offsetof(Location, aliases))},
                {"tariffZones",   p_str (offsetof(Location, tariffZones))},
            }, {}, {
                // NOTE(Felix): stop after first location
                .leave_hook = [](void* match, void* callback_data, Hook_Context, Parser_Context) -> Pattern_Match_Result {
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

testresult test_json_object_as_hash_map() {
    using namespace json;
    const char* str = "{\"current_condition\":\"temp_C\", \"second\": \"zwei\"}";
    Hash_Map<char*, char*> hm {};

    Pattern p = object(0);

    auto alloc = grab_current_allocator();
    Pattern_Match_Result r = pattern_match(str, p, &hm, alloc);
    assert_equal_int(r, Pattern_Match_Result::OK_CONTINUE);

    u64 hash_1 = hm_hash("current_condition");
    u64 hash_2 = hm_hash("second");
    s32 idx_1 = hm.get_index_of_living_cell_if_it_exists((char*)"current_condition", hash_1);
    s32 idx_2 = hm.get_index_of_living_cell_if_it_exists((char*)"second", hash_2);

    assert_not_equal_int(idx_1, -1);
    assert_not_equal_int(idx_2, -1);

    assert_equal_string(string_from_literal("temp_C"),
                        string_from_literal(hm.get_object((char*)"current_condition")));

    assert_equal_string(string_from_literal("zwei"),
                        string_from_literal(hm.get_object((char*)"second")));

    assert_equal_int(2, hm.cell_count);

    hm.for_each([&](char* key, char* value, u32 idx){
        alloc->deallocate(key);
        alloc->deallocate(value);
    });

    hm.deinit();

    return pass;
}

testresult test_json_extract_value_from_list() {
    using namespace json;
    const char* str = "{\"current_condition\":[{\"temp_C\":3}]}";
    f32 result = 0;
    Pattern p = object({{"current_condition", list({object({{"temp_C", p_f32(0)}})})}});

    Pattern_Match_Result r = pattern_match(str, p, &result);
    assert_equal_int(r, Pattern_Match_Result::OK_CONTINUE);
    assert_equal_f32(result, 3.0);

    return pass;
}

testresult test_json_parse_from_quoted_value() {
    using namespace json;
    const char* str = "{\"value\":\"3\"}";
    f32 result = 0;
    Pattern p = object({{"value",  p_f32(0)}});

    Pattern_Match_Result r = pattern_match(str, p, &result);
    assert_equal_int(r, Pattern_Match_Result::OK_CONTINUE);
    assert_equal_f32(result, 3.0);

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
                {"product",             p_str  (offsetof(Departure, product))},
                {"label",               p_str  (offsetof(Departure, label))},
                {"destination",         p_str  (offsetof(Departure, destination))},
                {"live",                p_bool (offsetof(Departure, live))},
                {"delay",               p_f32  (offsetof(Departure, delay))},
                {"cancelled",           p_bool (offsetof(Departure, cancelled))},
                {"lineBackgroundColor", p_str  (offsetof(Departure, line_background_color))},
                {"departureId",         p_str  (offsetof(Departure, departure_id))},
                {"sev",                 p_bool (offsetof(Departure, sev))},
                {"platform",            p_str  (offsetof(Departure, platform))},
                {"stopPositionNumber",  p_s32  (offsetof(Departure, stop_position_number))},
        });
        Pattern serving_line = object({
                {"destination", p_str (offsetof(Serving_Line, destination))},
                {"sev",         p_bool(offsetof(Serving_Line, sev))},
                {"network",     p_str (offsetof(Serving_Line, network))},
                {"product",     p_str (offsetof(Serving_Line, product))},
                {"lineNumber",  p_str (offsetof(Serving_Line, line_number))},
                {"divaId",      p_str (offsetof(Serving_Line, diva_id))},
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

auto test_half_edge() -> testresult {
	Scratch_Arena scratch = scratch_arena_start();
    defer { scratch_arena_end(scratch); };

    String obj_str = string_from_literal(R"OBJ(
v 1.0 4.0 0.0
v 3.0 4.0 0.0
v 0.0 2.0 0.0
v 2.0 2.0 0.0
v 4.0 2.0 0.0
v 1.0 0.0 0.0
v 3.0 0.0 0.0
f 1 3 4
f 1 4 2
f 2 4 5
f 3 6 4
f 4 6 7
f 4 7 5
)OBJ");

    Mesh_Data mesh = load_obj_from_in_memory_string(obj_str.data, obj_str.data+obj_str.length);

	/*
	 *
	 *                                   e19→
	 *                   v0────────────────────────────────v3
	 *                   ╱╲              ←e5               ╱╲
	 *                  ╱  ╲                              ╱  ╲
	 *                 ╱    ╲                            ╱    ╲
	 *                ╱      ╲                          ╱      ╲
	 *               ╱        ╲           f1           ╱        ╲
	 *              ╱          ╲                      ╱          ╲
	 *             ╱            ╲                    ╱            ╲
	 *            ╱              ╲e3              ↗ ╱              ╲
	 *         ↗ ╱e0             ↖╲↘             e4╱                ╲
	 *       e18╱ ↙              e2╲              ╱e6               ↖╲e20
	 *         ╱                    ╲            ╱ ↙                e8╲↘
	 *        ╱         f0           ╲          ╱          f2          ╲
	 *       ╱                        ╲        ╱                        ╲
	 *      ╱                          ╲      ╱                          ╲
	 *     ╱                            ╲    ╱                            ╲
	 *    ╱                              ╲  ╱                              ╲
	 *   ╱              e1→               ╲╱               e7→              ╲
	 *  v1────────────────────────────────v2────────────────────────────────v4
	 *   ╲             ←e11               ╱╲              ←e17              ╱
	 *    ╲                              ╱  ╲                              ╱
	 *     ╲                            ╱    ╲                            ╱
	 *      ╲                          ╱      ╲                          ╱
	 *       ╲                        ╱        ╲                        ╱
	 *        ╲         f3           ╱          ╲          f5          ╱
	 *         ╲                    ╱            ╲                    ╱
	 *         ↖╲e9              ↗ ╱              ╲e15             ↗ ╱e23
	 *        e21╲↘            e10╱               ↖╲↘            e14╱↙
	 *            ╲              ╱e12            e14╲              ╱
	 *             ╲            ╱ ↙                  ╲            ╱
	 *              ╲          ╱          f4          ╲          ╱
	 *               ╲        ╱                        ╲        ╱
	 *                ╲      ╱                          ╲      ╱
	 *                 ╲    ╱                            ╲    ╱
	 *                  ╲  ╱                              ╲  ╱
	 *                   ╲╱               e13→             ╲╱
	 *                   v5────────────────────────────────v6
	 *                                   ←e22
	 *
	 */

    // Mesh_Data mesh = load_obj("./obj/test.obj");
	Half_Edge_Mesh he_mesh = Half_Edge_Mesh::from(mesh);

    defer {
        mesh.deinit();
		he_mesh.deinit();
    };

	assert_equal_int(he_mesh.vertices.count, 7);
	assert_equal_int(he_mesh.faces.count,    6);
	assert_equal_int(he_mesh.edges.count,   24);

	u32 edges_without_face = 0;
	for (auto& e : he_mesh.edges) {
		if (e.face == Half_Edge_Mesh::ABSENT_FACE)
			++edges_without_face;

		assert_not_equal_int(e.twin, Half_Edge_Mesh::ABSENT_EDGE);
	}

	assert_equal_int(edges_without_face, 6);


	// each edges twin's twin should be the starting edge again
	for (u32 e_idx = 0; e_idx < he_mesh.edges.count; ++e_idx) {
		Half_Edge_Mesh::Edge edge = he_mesh.edges[e_idx];

		u32 twin_idx = edge.twin;
		Half_Edge_Mesh::Edge twin = he_mesh.edges[twin_idx];

		assert_equal_int(e_idx, twin.twin);
	}

	Array_List<Half_Edge_Mesh::Edge_Idx> edges = {};
	edges.init(128, scratch.arena);

	Array_List<Half_Edge_Mesh::Face_Idx> faces = {};
	faces.init(128, scratch.arena);

	Integer_Pair vert_to_expected_vert_neighbor_count[] = {
		{0, 3}, {1, 3}, {2, 6},
		{3, 3}, {4, 3}, {5, 3},
		{6, 3},
	};
	for (auto p : vert_to_expected_vert_neighbor_count) {
		he_mesh.get_all_vertex_neighbors(p.x, &edges);
		assert_equal_int(edges.count, p.y);
		edges.clear();
	}

	Integer_Pair vert_to_expected_face_neighbory_count[] = {
		{0, 2}, {1, 2}, {2, 6},
		{3, 2}, {4, 2}, {5, 2},
		{6, 2},
	};
	for (auto p : vert_to_expected_face_neighbory_count) {
		he_mesh.get_all_face_neighbors_of_vertex(p.x, &faces);
		assert_equal_int(faces.count, p.y);
		faces.clear();
	}

	Integer_Pair face_to_expected_face_neighbory_count[] = {
		{0, 2}, {1, 2}, {2, 2},
		{3, 2}, {4, 2}, {5, 2},
	};
	for (auto p : face_to_expected_face_neighbory_count) {
		he_mesh.get_all_face_neighbors_of_face(p.x, &faces);
		assert_equal_int(faces.count, p.y);
		faces.clear();
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
            {"product",             p_str  (offsetof(Departure, product))},
            {"label",               p_str  (offsetof(Departure, label))},
            {"destination",         p_str  (offsetof(Departure, destination))},
            {"live",                p_bool (offsetof(Departure, live))},
            {"delay",               p_f32  (offsetof(Departure, delay))},
            {"cancelled",           p_bool (offsetof(Departure, cancelled))},
            {"lineBackgroundColor", p_str  (offsetof(Departure, line_background_color))},
            {"departureId",         p_str  (offsetof(Departure, departure_id))},
            {"sev",                 p_bool (offsetof(Departure, sev))},
            {"platform",            p_str  (offsetof(Departure, platform))},
            {"stopPositionNumber",  p_s32 (offsetof(Departure, stop_position_number))},
            {"infoMessages",        list    ({p_str(0)}, {
                        .array_list_offset = offsetof(Departure, info_messages),
                        .element_size      = sizeof(Departure::info_messages[0]),
                    })}
        });
    Pattern serving_line = object({
            {"destination", p_str(offsetof(Serving_Line, destination))},
            {"sev",         p_bool(offsetof(Serving_Line, sev))},
            {"network",     p_str(offsetof(Serving_Line, network))},
            {"product",     p_str(offsetof(Serving_Line, product))},
            {"lineNumber",  p_str(offsetof(Serving_Line, line_number))},
            {"divaId",      p_str(offsetof(Serving_Line, diva_id))},
        });
    Pattern p = object({
            {"departures", list({departure}, {
                        .array_list_offset    = offsetof(Departure_Infos, departures),
                        .element_size         = sizeof(Departure_Infos::departures[0]),
                    })},
            {"servingLines", list({serving_line}, {
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

auto test_ringbuffer() -> testresult {
    Ringbuffer<s32> rb;
    rb.init(3);
    defer { rb.deinit(); };


    assert(rb.is_empty);

    bool did_overflow;
    did_overflow = rb.push(99);
    assert_equal_int(rb.count(), 1);
    assert_equal_int(did_overflow, false);

    did_overflow = rb.push(100);
    assert_equal_int(rb.count(), 2);
    assert_equal_int(did_overflow, false);

    did_overflow = rb.push(101);
    assert_equal_int(rb.count(), 3);
    assert_equal_int(did_overflow, false);

    did_overflow = rb.push(102);
    assert_equal_int(rb.count(), 3);
    assert_equal_int(did_overflow, true);

    did_overflow = rb.push(103);
    assert_equal_int(rb.count(), 3);
    assert_equal_int(did_overflow, true);


    u32 i = 0;
    s32 num = rb.data[(i + rb.start_idx) % rb.length];
    assert_equal_int(num, 101);

    i++;
    num = rb.data[(i + rb.start_idx) % rb.length];
    assert_equal_int(num, 102);

    i++;
    num = rb.data[(i + rb.start_idx) % rb.length];
    assert_equal_int(num, 103);


    i = 0;
    rb.for_each([&](s32& val) -> testresult {
        if (i == 0) assert_equal_int(val, 101);
        if (i == 1) assert_equal_int(val, 102);
        if (i == 2) assert_equal_int(val, 103);
        ++i;
        return pass;
    });

    return pass;
}

testresult test_join_paths() {
    Scratch_Arena scratch = scratch_arena_start();
    defer { scratch_arena_end(scratch); };

    assert_equal_string(join_paths(string_from_literal("folder_structure_test"),
                                   string_from_literal("*"), false, scratch.arena).string,
                        string_from_literal("folder_structure_test/*"));

    assert_equal_string(join_paths(string_from_literal("jsons/"),
                                   string_from_literal("*"), false, scratch.arena).string,
                        string_from_literal("jsons/*"));

    return pass;
}

testresult test_walk_files() {
    Scratch_Arena scratch = scratch_arena_start();
    defer { scratch_arena_end(scratch); };

    struct Params {
        Array_List<Path_Info> pis;
        Scratch_Arena scratch;
    } params {
        .scratch = scratch,
    };
    params.pis.init(3, scratch.arena);

    walk_files("folder_structure_test", {
            .recursive        = true,
            .with_files       = true,
            .with_directories = true
        }, [](Path_Info pi, Walk_Status* status, void* user_data) -> void {
            Params* params = (Params*) user_data;
            pi.full_path = Allocated_String::from(pi.full_path.string, params->scratch.arena);
            params->pis.append(pi);
        }, &params);

    assert_equal_int(params.pis.count, 3);

    String expected_paths[] = {
        string_from_literal("folder_structure_test/file.ini"),
        string_from_literal("folder_structure_test/inside"),
        string_from_literal("folder_structure_test/inside/test.txt"),
    };

    for (String s : expected_paths) {
        bool found = false;
        for (Path_Info pi : params.pis) {
            if (s == pi.full_path.string) {
                found = true;
                break;
            }
        }

        if (!found) {
            log_error("%{->Str} not found", &s);
        }
        assert_equal_int(found, true);
    }
    return pass;
}

testresult test_path_components() {
    Scratch_Arena scratch = scratch_arena_start();
    defer { scratch_arena_end(scratch); };

    Array_List<Path_Info> path_infos;
    path_infos.init(16, scratch.arena);

    get_path_components("/home/felix/test/dir/file.txt", &path_infos, scratch.arena);
    assert_equal_int(path_infos.count, 6);
    assert_equal_string(path_infos[0].get_local_name(), string_from_literal("/"));
    assert_equal_string(path_infos[0].get_base_path(),  string_from_literal(""));

    assert_equal_string(path_infos[1].get_local_name(), string_from_literal("home"));
    assert_equal_string(path_infos[1].get_base_path(),  string_from_literal("/"));

    assert_equal_string(path_infos[2].get_local_name(), string_from_literal("felix"));
    assert_equal_string(path_infos[2].get_base_path(),  string_from_literal("/home/"));

    assert_equal_string(path_infos[3].get_local_name(), string_from_literal("test"));
    assert_equal_string(path_infos[3].get_base_path(),  string_from_literal("/home/felix/"));

    assert_equal_string(path_infos[4].get_local_name(), string_from_literal("dir"));
    assert_equal_string(path_infos[4].get_base_path(),  string_from_literal("/home/felix/test/"));

    assert_equal_string(path_infos[5].get_local_name(), string_from_literal("file.txt"));
    assert_equal_string(path_infos[5].get_base_path(),  string_from_literal("/home/felix/test/dir/"));


    path_infos.clear();
    get_path_components("C:\\Users\\Felix\\test\\file.txt", &path_infos, scratch.arena);
    assert_equal_int(path_infos.count, 5);
    assert_equal_string(path_infos[0].get_local_name(), string_from_literal("C:"));
    assert_equal_string(path_infos[0].get_base_path(),  string_from_literal(""));

    assert_equal_string(path_infos[1].get_local_name(), string_from_literal("Users"));
    assert_equal_string(path_infos[1].get_base_path(),  string_from_literal("C:/"));

    assert_equal_string(path_infos[2].get_local_name(), string_from_literal("Felix"));
    assert_equal_string(path_infos[2].get_base_path(),  string_from_literal("C:/Users/"));

    assert_equal_string(path_infos[3].get_local_name(), string_from_literal("test"));
    assert_equal_string(path_infos[3].get_base_path(),  string_from_literal("C:/Users/Felix/"));

    assert_equal_string(path_infos[4].get_local_name(), string_from_literal("file.txt"));
    assert_equal_string(path_infos[4].get_base_path(),  string_from_literal("C:/Users/Felix/test/"));


    path_infos.clear();
    get_path_components("~/test/dir/file.txt", &path_infos, scratch.arena);
    assert_equal_int(path_infos.count, 4);
    assert_equal_string(path_infos[0].get_local_name(), string_from_literal("~"));
    assert_equal_string(path_infos[0].get_base_path(),  string_from_literal(""));

    assert_equal_string(path_infos[1].get_local_name(), string_from_literal("test"));
    assert_equal_string(path_infos[1].get_base_path(),  string_from_literal("~/"));

    assert_equal_string(path_infos[2].get_local_name(), string_from_literal("dir"));
    assert_equal_string(path_infos[2].get_base_path(),  string_from_literal("~/test/"));

    assert_equal_string(path_infos[3].get_local_name(), string_from_literal("file.txt"));
    assert_equal_string(path_infos[3].get_base_path(),  string_from_literal("~/test/dir/"));


    // log_info("");
    // for(auto& p : path_infos) {
    //     log_info("base:%{->Str}   local:%{->Str}", &p.base_path, &p.local_name);
    // }

    return pass;
}



s32 main(s32, char**) {
    Leak_Detecting_Allocator ld;
    ld.init(true);

    Bookkeeping_Allocator bk;
    bk.init();
    defer { ld.deinit(); };

    with_allocator(bk) {
        defer { bk.print_statistics(); };

        with_allocator(ld) {
            defer { ld.print_leak_statistics(); };

            create_test_counters();
            defer { print_test_summary(); };

			// invoke_test(test_half_edge);
            test_group("Path and Files") {
                invoke_test(test_join_paths);
                invoke_test(test_walk_files);
                invoke_test(test_path_components);
            }

            test_group("Json") {
                invoke_test(test_obj_to_json_str);
                invoke_test(test_json_simple_object_json5);
                invoke_test(test_json_simple_object_new_syntax);
                invoke_test(test_json_mvg);
                invoke_test(test_json_bug);
                invoke_test(test_json_extract_value_from_list);
                // invoke_test(test_json_parse_from_quoted_value);
                // invoke_test(test_json_object_as_hash_map);
                invoke_test(test_json_wildcard_match_and_parser_context);
                invoke_test(test_json_config);
                invoke_test(test_json_bug_again);
            }

            test_group("Array Lists") {
                invoke_test(test_array_lists_adding_and_removing);
                invoke_test(test_array_lists_sorting);
                invoke_test(test_array_lists_sorted_insert_and_remove);
                invoke_test(test_array_lists_searching);
                invoke_test(test_array_list_sort_many);
            }

            test_group("Bucketed Data Structures") {
                invoke_test(test_bucket_list);
                invoke_test(test_bucket_list_leak);
                invoke_test(test_bucket_list_in_linear_allocator);
                invoke_test(test_bucket_queue);
            }

            test_group("Allocators") {
                invoke_test(test_typed_bucket_allocator);
                invoke_test(test_pool_allocator);
                invoke_test(test_growable_pool_allocator);
                invoke_test(test_scratch_arena_can_realloc_last_alloc);
                invoke_test(test_linear_allocator_growing_and_resetting);
            }

            invoke_test(test_defer_runs_after_return);

            invoke_test(test_math);
            invoke_test(test_math_matrix_compose);
            invoke_test(test_hashmap);
            invoke_test(test_sort);
            invoke_test(test_kd_tree);
            invoke_test(test_string_split);
            // invoke_test(test_stack_array_lists);
            invoke_test(test_hooks);
            invoke_test(test_scheduler_animations);

            invoke_test(test_ringbuffer);
            // invoke_test(test_printer);
        }

    }

    return 0;
}
