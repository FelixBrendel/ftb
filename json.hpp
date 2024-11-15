#pragma once
#include "core.hpp"
#include "parsing.hpp"
#include "hashmap.hpp"
#include <initializer_list>

// TODO(Felix):
//  - pattern for reading into `maybe` types
//  - pattern for reading into array types, like lists but just offset + array_count
//  - write a test for "multiple object member patterns with different json types"

// DONE(Felix):
//  - allow multiple object member patterns with different json types

namespace json {
    enum struct Pattern_Match_Result {
         OK_CONTINUE,
         OK_DONE,
         MATCHING_ERROR
    };

    enum struct Json_Type {
        Invalid               = 0,

        List                  = 1<<0,
        Object                = 1<<1,
        Object_As_Hash_Map    = 1<<2,
        Object_Member_Name    = 1<<3,

        String                = 1<<4,
        Number                = 1<<5,
        Boolean               = 1<<6,
        Null                  = 1<<7,
    };

    struct Hook_Context {
        Json_Type trigger_type;

        // NOTE(Felix): member_name has to be set if trigger_type ==
        //   Hook_Trigger::Object_Member_Name
        const char* member_name;
    };

    enum struct Parser_Context_Type {
        Root,
        Object_Member,
        List_Entry,
    };
    struct Parser_Context_Stack_Entry {
        Parser_Context_Stack_Entry* previous;
        Parser_Context_Type         parent_type;
        union {
            struct {
                String member_name;
            } object;
            struct {
                s32 index;
            } list_entry;
        } parent;
    };

    struct Parser_Context {
        Parser_Context_Stack_Entry  context_stack;
        const char*                 position_in_string;
    };

    typedef Pattern_Match_Result (*parser_hook)(void* matched_obj,
                                                void* callback_data,
                                                Hook_Context h_context,
                                                Parser_Context p_context);

    typedef u32  (*reader_function)(const char* position, void* out_read_value);

    // NOTE(Felix): In the future when we write into a sting buffer instead of
    //   a file we can merge both functionalities. We first wirte the key and
    //   colon of the objet members and then wirte/check the value. If nothing
    //   is to be written, we rewind the writing pointer a bit to overwrite
    //   the key again.
    typedef u32  (*writer_function)(FILE*           file, void* value_to_write);
    typedef bool (*writer_selection_function)(void* value_to_write);

    struct Hooks {
        // NOTE(Felix): custom_reader might be set or nullptr; if set,
        //   value.destination_offset must also be set, as it will be passed
        //   as the destination to the custom reader
        reader_function           custom_reader;
        writer_function           custom_writer;
        writer_selection_function custom_writer_selection; // NOTE(Felix): see note above at writer_selection_function

        void* callback_data;
        parser_hook enter_hook;
        parser_hook leave_hook;
    };

    struct Key_Mapping {
        const char* key;
        Data_Type   destination_data_type;
        u32         destination_offset;
    };

    struct Object_Member;
    struct Fallback_Pattern;

    struct Pattern {
        Json_Type type;

        union {
            struct {
                Object_Member* members;
                u32 member_count;
                Fallback_Pattern* fallback_pattern; // pattern used for wildcard matches
            } object;
            struct {
                u32 hash_map_offset;
            } object_as_hash_map;
            struct {
                u32 element_size;
                u32 array_list_offset;
                Pattern* child_pattern;
            } list;
            struct {
                Data_Type       destination_type;
                u32             destination_offset;
            } value;
        };

        Hooks hooks;

        void print();
    };

    struct Fallback_Pattern {
        Pattern pattern;
        u32 array_list_offset;
        u32 element_size;
    };

    struct Object_Member {
        const char*  key;
        Pattern      pattern;
    };

    struct List_Info {
        u32 array_list_offset;
        u32 element_size;
    };

    Pattern object(std::initializer_list<Object_Member> members, Fallback_Pattern fallback_pattern={}, Hooks hooks={0});
    Pattern object(u32 hash_map_offset, Hooks hooks={0});

    Pattern list(Pattern&& element_pattern,
                 List_Info list_info={0}, Hooks hooks={0});
    Pattern list(const Pattern& element_pattern,
                 List_Info list_info={0}, Hooks hooks={0});
    Pattern p_s32(u32 offset,  Hooks hooks={0});
    Pattern p_s64(u32 offset,  Hooks hooks={0});
    Pattern p_f32(u32 offset,  Hooks hooks={0});
    Pattern p_bool(u32 offset, Hooks hooks={0});
    Pattern p_str(u32 offset,  Hooks hooks={0});
    Pattern custom(Json_Type source_type, u32 destination_offset,
                   Hooks hooks={0});


    // Pattern member_value(const char* key, Json_Type source_type, Data_Type destination_type, u32 destination_offset);
    Pattern_Match_Result pattern_match(const char* string, Pattern pattern, void* obj_to_match_into, void* callback_data = nullptr, Allocator_Base* allocator = nullptr);
    void write_pattern_to_file(const char* path, Pattern pattern, void* user_data);
    Allocated_String write_pattern_to_string(Pattern pattern, void* user_data, Allocator_Base* allocator = nullptr);

    // helper for custom parsers
    Json_Type identify_thing(const char* string);
    u32 eat_whitespace_and_comments(const char* string);
    u32 read_float_array(const char* point, f32* arr, u32 count);
    u32 write_float_array(FILE* out_file, f32* arr, u32 count);


    // NOTE(Felix): This can go away once we have dedicated array patterns.
    template <u32 ARRAY_SIZE>
    json::Pattern p_f32_arr(u32 offset) {
        return json::custom(
            json::Json_Type::List, offset, {
                .custom_reader = [](const char* point, void* out_vec3) -> u32 {
                    return read_float_array(point, (f32*)out_vec3, ARRAY_SIZE);
                },
                .custom_writer = [](FILE* out_file, void* vec3_to_write) -> u32 {
                    return write_float_array(out_file, (f32*)vec3_to_write, ARRAY_SIZE);
                }
            }
        );
    }

}


#ifdef FTB_JSON_IMPL
namespace json {
    const char true_string[]  = "true";
    const char false_string[] = "false";
    const char null_string[]  = "null";

    u32 read_float_array(const char* point, f32* arr, u32 count) {
        panic_if(point[0] != '[', "Trying to parse float array, but not on list: '%.*s'", 50, point);
        ++point;

        u32 total_length = 1 + eat_construct(point, ']');

        {
            for (u32 i = 0; i < count-1; ++i) {
                point += read_float(point, &arr[i]);
                point += eat_whitespace_and_comments(point);
                panic_if(point[0] != ',', "',' expected after coordinate in float array");
                ++point; // overstep ','
                point += eat_whitespace_and_comments(point);
            }

            point += read_float(point, &arr[count -1]);
            point += eat_whitespace_and_comments(point);
            panic_if(point[0] != ']', "']' expected after coordinate in float array");
        }

        return total_length;
    }

    u32 write_float_array(FILE* out_file, f32* arr, u32 count) {
        u32 written = print_to_file(out_file, "[");

        u32 i = 0;
        for (; i < count-1; ++i) {
            print_to_file(out_file, "%g, ", arr[i]);
        }
        print_to_file(out_file, "%g", arr[i]);

        written += print_to_file(out_file, "]");

        return written;
    }


    Json_Type identify_thing(const char* string) {
        if (is_quotes_char(string[0])) return Json_Type::String;
        if (is_number_char(string[0])
            || string[0] == '+'
            || string[0] == '-') return Json_Type::Number;
        if (string[0] == '{') return Json_Type::Object;
        if (string[0] == '[') return Json_Type::List;

        // NOTE(Felix): -1 after the size because sizeof returns the size
        //   including the null terminator
        if (strncmp(string, true_string,  sizeof(true_string)-1))  return Json_Type::Boolean;
        if (strncmp(string, false_string, sizeof(false_string)-1)) return Json_Type::Boolean;
        if (strncmp(string, null_string,  sizeof(null_string)-1))  return Json_Type::Null;

        return Json_Type::Invalid;
    }

    u32 eat_whitespace_and_comments(const char* string) {
        u32 eaten = 0;
        u32 prev_eaten;
        do {
            prev_eaten = eaten;

            u32 whitespace_len = eat_whitespace(string);
            eaten  += whitespace_len;
            string += whitespace_len;
            // log_error("after eat ws: '%.*s'", 20,string);

            if (string[0] == '/' && string[1] == '/') {
                u32 line_len = eat_line(string);
                eaten  += line_len;
                string += line_len;
                // log_error("after eat line: '%.*s'", 20,string);
            }

        } while (prev_eaten != eaten);
        return eaten;
    }

    u32 eat_thing(const char* string) {
        Json_Type thing = identify_thing(string);

        switch (thing) {
            case Json_Type::String: return eat_string(string);
            case Json_Type::Number: return eat_number(string);
            case Json_Type::List:   return eat_construct(string+1, ']')+1; // +1 to start in the construct
            case Json_Type::Object: return eat_construct(string+1, '}')+1; // +1 to start in the construct
            case Json_Type::Boolean:
            case Json_Type::Null: {
                u32 eaten = 0;
                while (is_alpha_char(string[eaten]))
                    ++eaten;
                return eaten;
            } break;
            default: panic("Dont know how to eat %d", thing);
        }
    }

    Pattern_Match_Result pattern_match_list(Parser_Context ctx,
                                            Pattern list_pattern,
                                            Pattern child,
                                            void* matched_obj, // ptr to arraylist
                                            void* callback_data, u32* out_eaten,
                                            Allocator_Base* allocator);

    Pattern_Match_Result pattern_match_object(Parser_Context ctx,
                                              Pattern p,
                                              void* matched_obj,
                                              void* callback_data, u32* out_eaten,
                                              Allocator_Base* allocator);

    Pattern_Match_Result pattern_match_value(Parser_Context ctx,
                                             Pattern pattern,
                                             void* matched_obj,
                                             void* callback_data, u32* out_eaten,
                                             Allocator_Base* allocator);

    Pattern_Match_Result pattern_match_object_member(Parser_Context ctx,
                                                     Pattern p,
                                                     void* matched_obj,
                                                     void* callback_data, u32* out_eaten,
                                                     Allocator_Base* allocator);

    u32 read_into(void* destination, Data_Type dtype, const char* source) {
        switch (dtype) {
            case Data_Type::Integer: return read_int(source,  (s32*)destination);
            case Data_Type::Long:    return read_long(source, (s64*)destination);
            case Data_Type::Boolean: return read_bool(source, (bool*)destination);
            case Data_Type::String:  return read_string(source, (String*)destination);
            case Data_Type::Float:   return read_float(source, (f32*)destination);
            default: panic("dtype %u not implemented", (u8)dtype - (u8)(1<<7));
        }
    }


    Pattern_Match_Result pattern_match_object(Parser_Context ctx, Pattern p,
                                              void* matched_obj, void* callback_data,
                                              u32* out_eaten, Allocator_Base* allocator)
    {
        // NOTE(Felix): expecting `string` to start on {, this function will eat
        //   just past the closing }
        const char* string = ctx.position_in_string;
        *out_eaten = 0;
        u32 eaten = 1; // overstep {

        eaten += eat_whitespace_and_comments(string+eaten);

        while (string[eaten] != '}') {
            // panic_if(string[eaten] != '\"',
                     // "expecting a string here but got '%s'",
                     // string+eaten);

            u32 sub_eaten = 0;
            Parser_Context call_ctx = ctx;
            call_ctx.position_in_string = string+eaten;
            Pattern_Match_Result sub_result =
                pattern_match_object_member(call_ctx, p,
                                            matched_obj, callback_data,
                                            &sub_eaten, allocator);
            if (sub_result == Pattern_Match_Result::MATCHING_ERROR) {
                *out_eaten = 0;
                return Pattern_Match_Result::MATCHING_ERROR;
            }

            eaten += sub_eaten;
            if (sub_result == Pattern_Match_Result::OK_DONE) {
                eaten += eat_construct(string+eaten, '}');
            }

            eaten += eat_whitespace_and_comments(string+eaten);

            panic_if(string[eaten] != ',' &&
                     string[eaten] != '}',
                     "Expected , or } but got '%s'\n"
                     " - already eaten %u\n"
                     " - base string was %s",
                     string+eaten, eaten, string);

            if (string[eaten] == ',') {
                ++eaten;

                eaten += eat_whitespace_and_comments(string+eaten);
            }
        }

        ++eaten; // overstep }
        *out_eaten = eaten;
        return Pattern_Match_Result::OK_CONTINUE;
    }

    bool pattern_types_match(Json_Type type_in_pattern, Json_Type type_in_string) {
        return (((u64)type_in_pattern & (u64)type_in_string) != 0);
    }

    bool pattern_types_compatible(Json_Type type_in_pattern, Json_Type type_in_string) {
        return pattern_types_match(type_in_pattern, type_in_string)
            || (type_in_string == Json_Type::Object && type_in_pattern == Json_Type::Object_As_Hash_Map)
            || (type_in_string == Json_Type::Number && type_in_pattern == Json_Type::String);
    }

    Pattern_Match_Result pattern_match_value(Parser_Context ctx, Pattern pattern,
                                             void* matched_obj, void* callback_data,
                                             u32* out_eaten, Allocator_Base* allocator)
    {
        const char* string = ctx.position_in_string;

        *out_eaten = 0;
        u32 eaten = 0;
        eaten += eat_whitespace_and_comments(string);

        Json_Type thing_at_point = identify_thing(string+eaten);

        Pattern_Match_Result enter_message = Pattern_Match_Result::OK_CONTINUE;
        Pattern_Match_Result leave_message = Pattern_Match_Result::OK_CONTINUE;
        Parser_Context call_ctx = ctx;
        call_ctx.position_in_string = {string+eaten};

        // maybe run enter hook
        if (pattern.hooks.enter_hook) {
            enter_message =
                pattern.hooks.enter_hook(matched_obj, callback_data,
                                         {thing_at_point}, call_ctx);
        }

        // if children were registered for this pattern
        u32 eaten_sub_object = 0;
        // if (pattern_todo.children.size() > 0) {
            // The only types that actually can contain children
            Pattern_Match_Result sub_result;

            // NOTE(Felix): make sure the pattern type matches the thing_at_point type
            panic_if(!pattern_types_compatible(pattern.type, thing_at_point), "Attempting to match objects of incompatible types.\n"
                     "Pattern type: %d\n"
                     "Actual  type: %d\n"
                     "At: %s", pattern.type, thing_at_point, string+eaten);

            if (pattern.hooks.custom_reader) {
                if (!pattern_types_match(pattern.type, thing_at_point)) {
                    panic("Trying to parse a custom type expecting type %d but type was actually %d", pattern.type, thing_at_point);
                }
                eaten += pattern.hooks.custom_reader(string+eaten, (void*)(((u8*)matched_obj)+pattern.value.destination_offset));
                sub_result = Pattern_Match_Result::OK_CONTINUE;

            } else {

                if (thing_at_point == Json_Type::Object) {
                    sub_result = pattern_match_object(call_ctx, pattern,
                                                      matched_obj, callback_data,
                                                      &eaten_sub_object, allocator);
                } else if (thing_at_point == Json_Type::List) {
                    sub_result = pattern_match_list(call_ctx, pattern,
                                                    *pattern.list.child_pattern,
                                                    ((u8*)matched_obj) + pattern.list.array_list_offset,
                                                    callback_data,
                                                    &eaten_sub_object, allocator);
                } else {
                    // match simple values

                    panic_if(pattern.type == Json_Type::Object_Member_Name,
                             "object member not valid here");
                    if (thing_at_point == pattern.type) {
                        eaten += read_into((void*)(((u8*)matched_obj)+pattern.value.destination_offset),
                                           pattern.value.destination_type,
                                           string+eaten);
                    } else if (thing_at_point == Json_Type::String) {
                        // NOTE(Felix): if types don't match, but in the supplied
                        //   json we are looking at a string, then try to read the
                        //   thing in the string
                        if (identify_thing(string+eaten+1) == pattern.type) {
                            ++eaten; // overstep quotation marks
                            eaten += read_into((void*)(((u8*)matched_obj)+pattern.value.destination_offset),
                                               pattern.value.destination_type,
                                               string+eaten);
                            ++eaten; // overstep quotation marks
                        }
                    } else if (pattern.value.destination_type == Data_Type::String &&
                               thing_at_point == Json_Type::Number)
                    {
                        // NOTE(Felix): if we're on a number but should read into a string
                        u32 str_len = eat_number(string+eaten);
                        String* str = (String*)(((u8*)matched_obj)+pattern.value.destination_offset);
                        str->data   = heap_copy_limited_c_string(string+eaten, str_len);
                        str->length = str_len;
                        eaten += str_len;
                    }

                    sub_result = Pattern_Match_Result::OK_CONTINUE;

                }
            }

            if (sub_result == Pattern_Match_Result::MATCHING_ERROR)
                return Pattern_Match_Result::MATCHING_ERROR;

            eaten += eaten_sub_object;
        // }

        // NOTE(Felix): if pattern is on something that can't have children we
        //   have to overstep it here
        if (!eaten_sub_object) {
            eaten += eat_thing(string+eaten);
        }

        call_ctx.position_in_string = string+eaten;

        // maybe run leave hook
        if (pattern.hooks.leave_hook) {
            leave_message =
                pattern.hooks.leave_hook(matched_obj, callback_data, {thing_at_point}, call_ctx);
        }

        if (enter_message == Pattern_Match_Result::MATCHING_ERROR ||
            leave_message == Pattern_Match_Result::MATCHING_ERROR)
            return Pattern_Match_Result::MATCHING_ERROR;

        *out_eaten = eaten;
        if (enter_message == Pattern_Match_Result::OK_DONE ||
            leave_message == Pattern_Match_Result::OK_DONE)
            return Pattern_Match_Result::OK_DONE;

        return Pattern_Match_Result::OK_CONTINUE;
    }

    void* realloc_zero(void* pBuffer, size_t oldSize, size_t newSize, Allocator_Base* allocator) {
        // source: https://stackoverflow.com/questions/2141277/how-to-zero-out-new-memory-after-realloc
        void* pNew = allocator->resize(pBuffer, (u32)newSize, 8);
        if (newSize > oldSize && pNew) {
            size_t diff = newSize - oldSize;
            void* pStart = ((char*)pNew) + oldSize;
            memset(pStart, 0, diff);
        }
        return pNew;
    }


    void* list_assure_free_slot(void* list_ptr, u32 elem_size, Allocator_Base* allocator) {
        if (elem_size == 0)
            return list_ptr;

        // if we should actually manage a list
        // make sure the list is long enough

        Array_List<byte>* list = (Array_List<byte>*)((u8*)list_ptr);

        u32 old_allocated = list->length;
        if (list->count == list->length) {
            if (list->length == 0)
                list->length = 4;
            else
                list->length *= 2;
            // NOTE(Felix): we need to zero out the newly allocated
            //   part, since it could contain lists itself to which we
            //   can only write sensibly if the memory is 0 as otherwise
            //   it will look as there are already lists present with
            //   garbage length and data pointers
            list->data = (byte*)realloc_zero(list->data, old_allocated * elem_size, list->length  * elem_size, allocator);
            list->allocator = allocator;
        }

        void* result = ((u8*)list->data)+(list->count*elem_size);
        ++(list->count);

        return result;
    }

    Pattern_Match_Result pattern_match_list(Parser_Context ctx, Pattern list_pattern,
                                            Pattern child,
                                            void* matched_obj, void* callback_data,
                                            u32* out_eaten, Allocator_Base* allocator)
    {
        // NOTE(Felix): expecting `string` to start on [, this function will eat
        //   just past the closing ]
        const char* string = ctx.position_in_string;

        // void* base_user_data = user_data;
        *out_eaten = 0;
        u32 eaten = 1; // overstep [
        eaten += eat_whitespace_and_comments(string+eaten);
        void* target_element_ptr = matched_obj;
        s32 matching_list_index = 0;
        while (string[eaten] != ']') {
            u32 sub_eaten = 0;

            target_element_ptr = list_assure_free_slot(matched_obj,
                                                       list_pattern.list.element_size,
                                                       allocator);

            Parser_Context call_ctx {
                .context_stack = {
                    .previous    = &ctx.context_stack,
                    .parent_type = Parser_Context_Type::List_Entry,
                    .parent {
                        .list_entry  = {
                            .index  = matching_list_index,
                        }
                    }
                },
                .position_in_string = string+eaten,
            };

            Pattern_Match_Result sub_result
                = pattern_match_value(call_ctx, child, target_element_ptr,
                                      callback_data, &sub_eaten, allocator);

            if (sub_result == Pattern_Match_Result::MATCHING_ERROR)
                return Pattern_Match_Result::MATCHING_ERROR;

            if (sub_result == Pattern_Match_Result::OK_DONE) {
                // skip rest of the list
                eaten += eat_construct(string+eaten, ']');
                *out_eaten = eaten;
                return Pattern_Match_Result::OK_DONE;
            }

            eaten += sub_eaten;
            eaten += eat_whitespace_and_comments(string+eaten);

            panic_if(string[eaten] != ',' &&
                     string[eaten] != ']',
                     "expected comma or end of list here, but got %s",
                     string+eaten);

            if (string[eaten] == ',') {
                ++eaten; // overstep ,
                eaten += eat_whitespace_and_comments(string+eaten);
            }

            matching_list_index++;
        }

        ++eaten; // overstep ]
        *out_eaten = eaten;
        return Pattern_Match_Result::OK_CONTINUE;
    }

    Pattern_Match_Result pattern_match_object_member(Parser_Context ctx, Pattern p,
                                                     void* matched_obj, void* callback_data,
                                                     u32* out_eaten, Allocator_Base* allocator)
    {
        const char* string = ctx.position_in_string;

        u32 eaten = 0;
        *out_eaten = 0;
        u32 value_lengh = 0;

        u32         member_name_len;
        const char* member_name;

        if (string[eaten] == '"') {
            // NOTE(Felix): expecting `string` to start on ", this function will eat
            //   just past the last char of the value associated to the member. All
            //   patterns should be Object_Member_Name, since this is the only
            //   pattern here

            member_name_len = eat_string(string+eaten)-2; // subtract the "
            member_name = string+eaten+1;
            value_lengh = 0;

            eaten += member_name_len+2; // overstep string and both "
        } else {
            // NOTE(Felix): accept unquoted symbols as keys (just pretend
            //   they are strings (json5))
            member_name_len = eat_identifier(string+eaten);
            member_name     = string+eaten;

            eaten += member_name_len;
        }
        eaten += eat_whitespace_and_comments(string+eaten);

        panic_if(string[eaten] != ':',
                 "expected a : here, but got %s",
                 string+eaten);
        ++eaten; // overstep :

        eaten += eat_whitespace_and_comments(string+eaten);

        Pattern_Match_Result sub_result = Pattern_Match_Result::OK_CONTINUE;

        if (p.type == Json_Type::Object_As_Hash_Map) {
            panic("NYI/To Be Deleted");
            Json_Type thing = identify_thing(string);
            panic_if(thing != Json_Type::String,
                     "When reading into a hashmap, "
                     "the values must be strings for now");

            const char* value = string+eaten+1;
            u32 value_len = eat_string(string+eaten)-2; // subtract the "
            eaten += value_len+2; // overstep both "

            auto hm =
                (Hash_Map<char*, char*>*)
                (((u8*)matched_obj)+p.object_as_hash_map.hash_map_offset);

            if (!hm->data)
                hm->init(8, allocator);

            char* allocated_member = heap_copy_limited_c_string(member_name, member_name_len, allocator);
            u64 hash_val = hm_hash(allocated_member);
            s64 existing_cell =
                hm->get_index_of_living_cell_if_it_exists(allocated_member, hash_val);
            if (existing_cell != -1) {
                // we already have the member in there
                allocator->deallocate(allocated_member);

                // free the old content
                allocator->deallocate(hm->data[existing_cell].object);

                //write new content
                hm->data[existing_cell].object =
                        heap_copy_limited_c_string(value, value_len, allocator);
            } else {
                hm->set_object(
                    allocated_member,
                    heap_copy_limited_c_string(value,       value_len,       allocator),
                    hash_val);
            }

        } else {
            Object_Member pattern_todo;
            bool found_pattern_todo = false;

            Json_Type thing_at_point = identify_thing(string+eaten);

            // check for children patterns with that member name
            for (u32 i = 0; i < p.object.member_count; ++i) {
                const Object_Member om = p.object.members[i];
                bool names_match =
                    strlen(om.key) == member_name_len &&
                    strncmp(om.key, member_name, member_name_len) == 0;
                bool types_compatible = pattern_types_compatible(om.pattern.type, thing_at_point);

                if (names_match && types_compatible)
                {
                    found_pattern_todo = true;
                    pattern_todo = om;
                    break;
                }
            }

            String member_name_str = String {
                // HACK(Felix): casting const away
                (char*)member_name,
                member_name_len
            };
            Parser_Context call_ctx {
                .context_stack = {
                    .previous    = &ctx.context_stack,
                    .parent_type = Parser_Context_Type::Object_Member,
                    .parent {
                        .object  = {
                            .member_name = member_name_str,
                        }
                    }
                },
                .position_in_string = string+eaten,
            };

            value_lengh = 0;
            if (found_pattern_todo) {

                sub_result = pattern_match_value(call_ctx, pattern_todo.pattern,
                                                 matched_obj, callback_data,
                                                 &value_lengh, allocator);


                if (sub_result == Pattern_Match_Result::MATCHING_ERROR) {
                    log_info("When matching %{->Str}", member_name_str);
                    return Pattern_Match_Result::MATCHING_ERROR;
                }

            } else if (p.object.fallback_pattern) {
                void* target = list_assure_free_slot(
                    ((u8*)matched_obj)+(p.object.fallback_pattern->array_list_offset),
                    p.object.fallback_pattern->element_size,
                    allocator);

                sub_result = pattern_match_value(call_ctx, p.object.fallback_pattern->pattern,
                                                 target, callback_data,
                                                 &value_lengh, allocator);

                if (sub_result == Pattern_Match_Result::MATCHING_ERROR)
                    return Pattern_Match_Result::MATCHING_ERROR;

            }


            if (value_lengh)
                eaten += value_lengh;
            else {
                // NOTE(Felix): neither pattern nor mapping was done, so we need to
                //   get the value length ourselved
                eaten += eat_thing(string+eaten);
            }

        }

        *out_eaten = eaten;
        return sub_result;
    }


    Pattern_Match_Result pattern_match(const char* string, Pattern pattern,
                                       void* matched_obj, void* callback_data,
                                       Allocator_Base* allocator) {
        if (!string) {
            return Pattern_Match_Result::MATCHING_ERROR;
        }

        if (!allocator)
            allocator = grab_current_allocator();

        u32 eaten = 0;

        Parser_Context call_ctx {
            .position_in_string = string,
        };
        return pattern_match_value(call_ctx, pattern,
                                   matched_obj, callback_data,
                                   &eaten, allocator);
    }

    // Pattern member_value(const char* key, Json_Type source_type, Data_Type destination_type, u32 destination_offset) {
    //     Pattern p = Pattern {
    //         .type     = Json_Type::Object_Member_Name,
    //         .member   = { .name = key },
    //         .children = Array_List<Pattern>::create_from({
    //             {
    //                 .type = source_type,
    //                 .value = {
    //                     .destination_type   = destination_type,
    //                     .destination_offset = destination_offset
    //                 }
    //             }
    //             })
    //     };
    //     return p;
    // }


    Pattern p_str(u32 offset, Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::String,
            .value = {
                .destination_type   = Data_Type::String,
                .destination_offset = offset
            },
            .hooks = hooks
        };
        return p;
    }

    Pattern p_s32(u32 offset, Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Number,
            .value = {
                .destination_type   = Data_Type::Integer,
                .destination_offset = offset
            },
            .hooks = hooks
        };

        return p;
    }

    Pattern p_s64(u32 offset, Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Number,
            .value = {
                .destination_type   = Data_Type::Long,
                .destination_offset = offset
            },
            .hooks = hooks
        };

        return p;
    }

    Pattern p_f32(u32 offset, Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Number,
            .value = {
                .destination_type   = Data_Type::Float,
                .destination_offset = offset
            },
            .hooks = hooks
        };

        return p;
    }

    Pattern p_bool(u32 offset, Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Boolean,
            .value = {
                .destination_type   = Data_Type::Boolean,
                .destination_offset = offset
            },
            .hooks = hooks
        };
        return p;
    }

    Pattern object(std::initializer_list<Object_Member> members, Fallback_Pattern fallback_pattern, Hooks hooks) {
        Pattern p = Pattern {
            .type = Json_Type::Object,
            .hooks = hooks
        };

        Allocator_Base* temp = grab_temp_allocator();

        p.object.member_count = (u32)members.size();

        if (p.object.member_count != 0) {
            p.object.members      = temp->allocate<Object_Member>(p.object.member_count);

            const Object_Member* it = members.begin();
            for (u64 i = 0; i < p.object.member_count; ++i) {
                p.object.members[i] = *it;
                ++it;
            }
        }

        if (fallback_pattern.pattern.type != Json_Type::Invalid) {
            p.object.fallback_pattern  = temp->allocate<Fallback_Pattern>(1);
            *p.object.fallback_pattern = fallback_pattern;
        }

        return p;
    }

    Pattern object(u32 hash_map_offset, Hooks hooks) {
        Pattern p = Pattern {
            .type = Json_Type::Object_As_Hash_Map,
            .hooks = hooks
        };
        p.object_as_hash_map.hash_map_offset = hash_map_offset;
        return p;
    }

    Pattern list(Pattern&& element_pattern,
                 List_Info list_info,
                 Hooks hooks)
    {
        return list((const Pattern&)element_pattern, list_info, hooks);
    }

    Pattern list(const Pattern& element_pattern,
                 List_Info list_info, Hooks hooks)
    {
        Allocator_Base* temp = grab_temp_allocator();
        Pattern* child = temp->allocate<Pattern>(1);
        *child = element_pattern;

        Pattern p = Pattern {
            .type = Json_Type::List,
            .list = {
                .element_size      = list_info.element_size,
                .array_list_offset = list_info.array_list_offset,
                .child_pattern     = child
            },
            .hooks = hooks
        };


        return p;
    }

    Pattern custom(Json_Type source_type, u32 destination_offset, Hooks hooks) {
        Pattern p = Pattern {
            .type         = source_type,
            .value = {
                .destination_offset = destination_offset
            },
            .hooks = hooks
        };

        return p;
    }


    void write_pattern_to_file(FILE* path, Pattern pattern, void* user_data);
    void write_bool_to_file(FILE* out, u32 offset, void* data) {
        bool* b = (bool*)(((byte*)data)+offset);
        fprintf(out, *b ? "true" : "false");
    }

    void write_int_to_file(FILE* out, u32 offset, void* data) {
        int* i = (int*)(((byte*)data)+offset);
        fprintf(out, "%d", *i);
    }

    void write_long_to_file(FILE* out, u32 offset, void* data) {
        s64* i = (s64*)(((byte*)data)+offset);
        fprintf(out, "%Id", *i);
    }

    void write_float_to_file(FILE* out, u32 offset, void* data) {
        float* f = (float*)(((byte*)data)+offset);
        fprintf(out, "%g", *f);
    }

    void write_string_to_file(FILE* out, u32 offset, void* data) {
        String* s = (String*)(((byte*)data)+offset);
        fprintf(out, "\"%*s\"", (int)s->length, s->data);
    }

    void write_list_to_file(FILE* out, u32 array_list_offset,
                            u32 element_size, Pattern child_pattern, void* data)
    {
        Array_List<void*>* list_p = (Array_List<void*>*)(((byte*)data)+array_list_offset);
        fprintf(out, "[");

        void* element_pointer = list_p->data;
        if (list_p->count != 0) {
            write_pattern_to_file(out, child_pattern, element_pointer);
        }

        for (u32 i = 1; i < list_p->count; ++i) {
            element_pointer = ((u8*)element_pointer)+element_size;
            fprintf(out, ", ");
            write_pattern_to_file(out, child_pattern, element_pointer);
        }

        fprintf(out, "]");
    }


    void write_object_to_file(FILE* out, const Object_Member* members, u32 member_count, void* data)
    {
        auto print_one_key_value_pair = [&](Object_Member om) {
            fprintf(out, "\"%s\" : ", om.key);
            write_pattern_to_file(out, om.pattern, data);
        };

        fprintf(out, "{");

        for (u32 i = 0; i < member_count; ++i) {
            const Object_Member om = members[i];

            if (i != 0) {
                fprintf(out, ", ");
            }

            bool member_should_be_written = true;
            if (om.pattern.hooks.custom_writer_selection) {
                u8* offset_data = ((u8*)data) + om.pattern.value.destination_offset;
                member_should_be_written &= om.pattern.hooks.custom_writer_selection(offset_data);
            }

            if (member_should_be_written)
                print_one_key_value_pair(om);
        }

        fprintf(out, "}");
    }


    Allocated_String write_pattern_to_string(Pattern pattern, void* user_data, Allocator_Base* allocator) {
        if (!allocator)
            allocator = grab_current_allocator();

        FILE* t_file = tmpfile();
        if (!t_file) {
            return {};
        }

        write_pattern_to_file(t_file, pattern, user_data);

        Allocated_String ret {};
        ret.string.length    = ftell(t_file) + 1;

        rewind(t_file);

        ret.allocator = allocator;
        ret.string.data      = allocator->allocate<char>((u32)ret.string.length);
        ret.string.length    = fread(ret.string.data, sizeof(char), ret.string.length, t_file);
        ret.string.data[ret.string.length] = '\0';

        return ret;
    }

    void write_pattern_to_file(FILE* out, Pattern pattern, void* user_data) {
        if (pattern.hooks.custom_reader || pattern.hooks.custom_writer) {
            void* offset_user_data = ((u8*)user_data) + pattern.value.destination_offset;
            if (!pattern.hooks.custom_writer) {
                panic("No custom writer set");
            } else {
                pattern.hooks.custom_writer(out, offset_user_data);
                return;
            }
        }

        switch (pattern.type) {
            case Json_Type::Null: fprintf(out, "null"); break;
            case Json_Type::Boolean: {
                write_bool_to_file(out, pattern.value.destination_offset, user_data);
            } break;
            case Json_Type::String: {
                write_string_to_file(out, pattern.value.destination_offset, user_data);
            } break;
            case Json_Type::Number: {
                if (pattern.value.destination_type == Data_Type::Integer) {
                    write_int_to_file(out, pattern.value.destination_offset, user_data);
                } else if (pattern.value.destination_type == Data_Type::Long) {
                    write_long_to_file(out, pattern.value.destination_offset, user_data);
                } else {
                    write_float_to_file(out, pattern.value.destination_offset, user_data);
                }
            } break;
            case Json_Type::List: {
                write_list_to_file(out, pattern.list.array_list_offset,
                                   pattern.list.element_size,
                                   *pattern.list.child_pattern, user_data);
            } break;
            case Json_Type::Object: {
                write_object_to_file(out, pattern.object.members,
                                     pattern.object.member_count, user_data);
            } break;
            default: panic("Don't know how to print json object with type %d",
                           pattern.type);
        }
    }

    void write_pattern_to_file(const char* path, Pattern pattern, void* user_data) {
        FILE* out = fopen(path, "w");
        if (!out) {
            log_error("could not open %s for writing.", path);
            return;
        }
        defer { fclose(out); };
        write_pattern_to_file(out, pattern, user_data);
    }

    void Pattern::print() {
        switch (type) {
            case Json_Type::Boolean:            raw_print("bool");          break;
            case Json_Type::Invalid:            raw_print("invalid");       break;
            case Json_Type::Null:               raw_print("null");          break;
            case Json_Type::List:               raw_print("list");          break;
            case Json_Type::Number:             raw_print("number");        break;
            case Json_Type::Object:             raw_print("object");        break;
            case Json_Type::Object_Member_Name: raw_print("object member"); break;
            case Json_Type::String:             raw_print("string");        break;
            default: raw_println("???");
        }
        if (type == Json_Type::Object) {
            if (object.member_count == 0) {
                println("{}");
                return;
            }

            raw_println(" {");
            with_print_prefix("|  ") {
                for (u32 i = 0; i < object.member_count; ++i) {
                    Object_Member om = object.members[i];
                    ::print("");
                    raw_print("\"%s\" : ", om.key);
                    om.pattern.print();
                    raw_print("\n");
                }
            }
            ::print("}");
        } else if (type == Json_Type::List) {
            raw_println(" [");
            with_print_prefix("|  ") {
                ::print("");
                Pattern c = *list.child_pattern;
                c.print();
                raw_print("\n");
            }
            ::print("]");
            raw_print(" (e_size : %u, al_off : %u)", list.element_size, list.array_list_offset);
        } else {
            raw_print(" (off : %u)", value.destination_offset);
        }
    }
}
#endif
