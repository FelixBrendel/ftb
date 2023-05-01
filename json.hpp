#pragma once
#include "core.hpp"
#include "parsing.hpp"
#include <initializer_list>

namespace json {
    enum struct Pattern_Match_Result {
         OK_CONTINUE,
         OK_DONE,
         MATCHING_ERROR
    };

    enum struct Json_Type {
        Invalid,

        List,
        Object,
        Object_Member_Name,

        String,
        Number,
        Bool,
        Null,
    };

    struct Hook_Context {
        Json_Type trigger_type;

        // NOTE(Felix): member_name has to be set if trigger_type ==
        //   Hook_Trigger::Object_Member_Name
        const char* member_name;
    };

    struct Parser_Context {
        const char* position_in_string;
    };

    typedef Pattern_Match_Result (*parser_hook)(void* user_data,
                                                Hook_Context h_context,
                                                Parser_Context p_context);

    typedef u32 (*reader_function)(const char* position, void* out_read_value);

    struct Key_Mapping {
        const char* key;
        Data_Type   destination_data_type;
        u32         destination_offset;
    };

    struct Object_Member;

    struct Pattern {
        Json_Type type;

        // only active if type == object
        std::initializer_list<Object_Member> object_members;
        // only active if type == list
        std::initializer_list<Pattern>       list_child;
        union {
            struct {
            } object;
            struct {
                u32 element_size;
                u32 array_list_offset;
                // Pattern* child_pattern;
            } list;
            struct {
                Data_Type   destination_type;
                u32         destination_offset;
            } value;
        };

        parser_hook             enter_hook;
        parser_hook             leave_hook;

        void print();

    };

    struct Object_Member {
        const char*   key;
        Pattern pattern;
    };

    struct Parser_Hooks {
        parser_hook enter_hook;
        parser_hook leave_hook;
    };

    struct List_Info {
        u32 array_list_offset;
        u32 element_size;
    };

    Pattern object(std::initializer_list<Object_Member> members, Parser_Hooks hooks={0});
    Pattern list(std::initializer_list<Pattern> element_pattern,
                 List_Info list_info={0}, Parser_Hooks hooks={0});
    Pattern integer(u32 offset,  Parser_Hooks hooks={0});
    Pattern longint(u32 offset,  Parser_Hooks hooks={0});
    Pattern floating(u32 offset, Parser_Hooks hooks={0});
    Pattern boolean_(u32 offset,  Parser_Hooks hooks={0});
    Pattern string(u32 offset,   Parser_Hooks hooks={0});
    Pattern custom(Json_Type source_type, Data_Type destination_type,
                   u32 destination_offset, Parser_Hooks hooks={0});

    // Pattern member_value(const char* key, Json_Type source_type, Data_Type destination_type, u32 destination_offset);
    Pattern_Match_Result pattern_match(const char* string, Pattern pattern, void* user_data, Allocator_Base* allocator = nullptr);
    void write_pattern_to_file(const char* path, Pattern pattern, void* user_data);
    void register_custom_reader_function(Data_Type dt, reader_function fun);
}


#ifdef FTB_JSON_IMPL
namespace json {
    const char true_string[]  = "true";
    const char false_string[] = "false";
    const char null_string[]  = "null";

    reader_function custom_readders[127];

    void register_custom_reader_function(Data_Type dt, reader_function fun) {
        panic_if((u8)dt >= 1 << 7,
                 "custom readers must have an "
                 "enum value of less than 1<<7(=%u)",1<<7);

        custom_readders[(u8)dt] = fun;
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
        if (strncmp(string, true_string,  sizeof(true_string)-1))  return Json_Type::Bool;
        if (strncmp(string, false_string, sizeof(false_string)-1)) return Json_Type::Bool;
        if (strncmp(string, null_string,  sizeof(null_string)-1))  return Json_Type::Null;

        return Json_Type::Invalid;
    }

    u32 eat_construct(const char* string, char delimiter) {
        // NOTE(Felix): Expects to start somewhere inside construct, not on the
        //   start of the onctruct
        u32 eaten = 0;

        while (string[eaten] && string[eaten] != delimiter) {
            if (string[eaten] == '"')
                eaten += eat_string(string+eaten);
            else if (string[eaten] == '{') {
                ++eaten;
                eaten += eat_construct(string+eaten, '}');
            } else if (string[eaten] == '[') {
                ++eaten;
                eaten += eat_construct(string+eaten, ']');
            }
            else
                ++eaten;
        }

        ++eaten; // overstep delimiter

        return eaten;
    }

    u32 eat_thing(const char* string) {
        Json_Type thing = identify_thing(string);

        switch (thing) {
            case Json_Type::String: return eat_string(string);
            case Json_Type::Number: return eat_number(string);
            case Json_Type::List:   return eat_construct(string+1, ']')+1; // +1 to start in the construct
            case Json_Type::Object: return eat_construct(string+1, '}')+1; // +1 to start in the construct
            case Json_Type::Bool:
            case Json_Type::Null: {
                u32 eaten = 0;
                while (is_alpha_char(string[eaten]))
                    ++eaten;
                return eaten;
            } break;
            default: panic("Dont know how to eat %d", thing);
        }
    }

    Pattern_Match_Result pattern_match_list(const char* string,
                                            Pattern list_pattern,
                                            Pattern child,
                                            void* user_data, u32* out_eaten,
                                            Allocator_Base* allocator);
    Pattern_Match_Result pattern_match_object(const char* string,
                                              Array_List<Object_Member> members,
                                              void* user_data, u32* out_eaten,
                                              Allocator_Base* allocator);

    Pattern_Match_Result pattern_match_value(const char* string,
                                             Pattern pattern,
                                             void* user_data, u32* out_eaten,
                                             Allocator_Base* allocator);

    Pattern_Match_Result pattern_match_object_member(const char* string,
                                                     std::initializer_list<Object_Member> object_members,
                                                     void* user_data, u32* out_eaten,
                                                     Allocator_Base* allocator);

    u32 read_into(void* destination, Data_Type dtype, const char* source) {
        // NOTE(Felix): Check for custom type
        if ((u8)dtype < 1<<7) {
            panic_if(!custom_readders[(u8)dtype],
                     "Attempting to read custom data type %u "
                     "but no reader was registered for it", (u8)dtype);
            return custom_readders[(u8)dtype](source, destination);
        }

        switch (dtype) {
            case Data_Type::Integer: return read_int(source,  (s32*)destination);
            case Data_Type::Long:    return read_long(source, (s64*)destination);
            case Data_Type::Boolean: return read_bool(source, (bool*)destination);
            case Data_Type::String:  return read_string(source, (String*)destination);
            case Data_Type::Float:   return read_float(source, (f32*)destination);
            default: panic("dtype %u not implemented", (u8)dtype - (u8)(1<<7));
        }
    }

    Pattern_Match_Result pattern_match_object(const char* string,
                                              std::initializer_list<Object_Member> members,
                                              void* user_data, u32* out_eaten,
                                              Allocator_Base* allocator)
    {
        // NOTE(Felix): expecting `string` to start on {, this function will eat
        //   just past the closing }
        *out_eaten = 0;
        u32 eaten = 1; // overstep {

        eaten += eat_whitespace(string+eaten);

        while (string[eaten] != '}') {
            panic_if(string[eaten] != '\"',
                     "expecting a string here but got %s",
                     string+eaten);

            u32 sub_eaten = 0;
            Pattern_Match_Result sub_result =
                pattern_match_object_member(string+eaten, members, user_data,
                                            &sub_eaten, allocator);
            if (sub_result == Pattern_Match_Result::MATCHING_ERROR) {
                *out_eaten = 0;
                return Pattern_Match_Result::MATCHING_ERROR;
            }

            eaten += sub_eaten;
            if (sub_result == Pattern_Match_Result::OK_DONE) {
                eaten += eat_construct(string+eaten, '}');
            }

            eaten += eat_whitespace(string+eaten);

            panic_if(string[eaten] != ',' &&
                     string[eaten] != '}',
                     "Expected , or } but got '%s'\n"
                     " - already eaten %u\n"
                     " - base string was %s",
                     string+eaten, eaten, string);

            if (string[eaten] == ',') {
                ++eaten;

                eaten += eat_whitespace(string+eaten);
            }
        }

        ++eaten; // overstep }
        *out_eaten = eaten;
        return Pattern_Match_Result::OK_CONTINUE;
    }

    Pattern_Match_Result pattern_match_value(const char* string,
                                             Pattern pattern,
                                             void* user_data, u32* out_eaten,
                                             Allocator_Base* allocator)
    {
        *out_eaten = 0;
        u32 eaten = 0;
        eaten += eat_whitespace(string);

        Json_Type thing_at_point = identify_thing(string+eaten);

        Pattern_Match_Result enter_message = Pattern_Match_Result::OK_CONTINUE;
        Pattern_Match_Result leave_message = Pattern_Match_Result::OK_CONTINUE;
        // maybe run enter hook
        if (pattern.enter_hook) {
            enter_message =
                pattern.enter_hook(user_data, {thing_at_point}, {string+eaten});
        }

        // if children were registered for this pattern
        u32 eaten_sub_object = 0;
        // if (pattern_todo.children.size() > 0) {
            // The only types that actually can contain children
            Pattern_Match_Result sub_result;
            if (thing_at_point == Json_Type::Object) {
                sub_result = pattern_match_object(string+eaten, pattern.object_members,
                                                  user_data, &eaten_sub_object, allocator);
            } else if (thing_at_point == Json_Type::List) {
                sub_result = pattern_match_list(string+eaten, pattern,
                                                *pattern.list_child.begin(),
                                                user_data, &eaten_sub_object, allocator);
            } else {
                // match simple values

                panic_if(pattern.type == Json_Type::Object_Member_Name,
                         "object member not valid here");
                if (thing_at_point == pattern.type) {
                    eaten += read_into((void*)(((u8*)user_data)+pattern.value.destination_offset),
                              pattern.value.destination_type,
                              string+eaten);
                } else if (thing_at_point == Json_Type::String) {
                    // NOTE(Felix): if types don't match, but in the supplied
                    //   json we are looking at a string, then try to read the
                    //   thing in the string
                    if (identify_thing(string+eaten+1) == pattern.type) {
                        ++eaten; // overstep quotation marks
                        eaten += read_into((void*)(((u8*)user_data)+pattern.value.destination_offset),
                                           pattern.value.destination_type,
                                           string+eaten);
                        ++eaten; // overstep quotation marks
                    }

                }

                sub_result = Pattern_Match_Result::OK_CONTINUE;

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

        // maybe run leave hook
        if (pattern.leave_hook) {
            leave_message =
                pattern.leave_hook(user_data, {thing_at_point}, {string+eaten});
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
        void* pNew = allocator->resize(pBuffer, newSize, 8);
        if (newSize > oldSize && pNew) {
            size_t diff = newSize - oldSize;
            void* pStart = ((char*)pNew) + oldSize;
            memset(pStart, 0, diff);
        }
        return pNew;
    }

    Pattern_Match_Result pattern_match_list(const char* string,
                                            Pattern list_pattern,
                                            Pattern child,
                                            void* user_data, u32* out_eaten,
                                            Allocator_Base* allocator)
    {
        // NOTE(Felix): expecting `string` to start on [, this function will eat
        //   just past the closing ]

        void* base_user_data = user_data;
        *out_eaten = 0;
        u32 eaten = 1; // overstep [
        eaten += eat_whitespace(string+eaten);

        while (string[eaten] != ']') {
            u32 sub_eaten = 0;

            // if we should manage a list
            if (list_pattern.list.element_size) {
                // make sure the list is long enough
                u32    elem_size = list_pattern.list.element_size;
                Array_List<byte>* list = (Array_List<byte>*)((u8*)base_user_data+list_pattern.list.array_list_offset);

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

                user_data = ((u8*)list->data)+(list->count*elem_size);
                ++(list->count);
            }

            Pattern_Match_Result sub_result
                = pattern_match_value(string+eaten, child, user_data,
                                      &sub_eaten, allocator);

            if (sub_result == Pattern_Match_Result::MATCHING_ERROR)
                return Pattern_Match_Result::MATCHING_ERROR;

            if (sub_result == Pattern_Match_Result::OK_DONE) {
                // skip rest of the list
                eaten += eat_construct(string+eaten, ']');
                *out_eaten = eaten;
                return Pattern_Match_Result::OK_DONE;
            }

            eaten += sub_eaten;
            eaten += eat_whitespace(string+eaten);

            panic_if(string[eaten] != ',' &&
                     string[eaten] != ']',
                     "expected comma or end of list here, but got %s",
                     string+eaten);

            if (string[eaten] == ',') {
                ++eaten; // overstep ,
                eaten += eat_whitespace(string+eaten);
            }
        }

        ++eaten; // overstep ]
        *out_eaten = eaten;
        return Pattern_Match_Result::OK_CONTINUE;
    }

    Pattern_Match_Result pattern_match_object_member(const char* string,
                                                     std::initializer_list<Object_Member> members,
                                                     void* user_data, u32* out_eaten,
                                                     Allocator_Base* allocator)
    {
        // NOTE(Felix): expecting `string` to start on ", this function will eat
        //   just past the last char of the value associated to the member. All
        //   patterns should be Object_Member_Name, since this is the only
        //   pattern here
        panic_if (*string != '"',
                  "expecting to land on a \" here, but I'm on: %s",
                  string);

        *out_eaten = 0;
        u32 eaten = 0; // eat_string expects to start on the quote

        u32 member_name_len = eat_string(string+eaten)-2; // subtract the "
        const char* member_name     = string+eaten+1;
        u32 value_lengh = 0;

        eaten += member_name_len+2; // overstep string and both "
        eaten += eat_whitespace(string+eaten);

        panic_if(string[eaten] != ':',
                 "expected a : here, but got %s",
                 string+eaten);
        ++eaten; // overstep :

        eaten += eat_whitespace(string+eaten);

        // check for children patterns with that member name
        bool found_pattern_todo = false;
        Pattern pattern_todo;
        for (Object_Member om : members) {
            if (strlen(om.key) == member_name_len &&
                strncmp(om.key, member_name, member_name_len) == 0)
            {
                found_pattern_todo = true;
                pattern_todo = om.pattern;
                break;
            }
        }
        Pattern_Match_Result sub_result = Pattern_Match_Result::OK_CONTINUE;
        if (found_pattern_todo) {
            value_lengh = 0;
            sub_result = pattern_match_value(string+eaten, pattern_todo, user_data,
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

        *out_eaten = eaten;
        return sub_result;
    }


    Pattern_Match_Result pattern_match(const char* string, Pattern pattern, void* user_data, Allocator_Base* allocator) {
        if (!string) {
            return Pattern_Match_Result::MATCHING_ERROR;
        }

        if (!allocator)
            allocator = grab_current_allocator();

        u32 eaten = 0;

        return pattern_match_value(string, pattern, user_data, &eaten, allocator);
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


    Pattern string(u32 offset, Parser_Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::String,
            .value = {
                .destination_type   = Data_Type::String,
                .destination_offset = offset
            },
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
        };
        return p;
    }

    Pattern integer(u32 offset, Parser_Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Number,
            .value = {
                .destination_type   = Data_Type::Integer,
                .destination_offset = offset
            },
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
        };

        return p;
    }

    Pattern longint(u32 offset, Parser_Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Number,
            .value = {
                .destination_type   = Data_Type::Long,
                .destination_offset = offset
            },
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
        };

        return p;
    }

    Pattern floating(u32 offset, Parser_Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Number,
            .value = {
                .destination_type   = Data_Type::Float,
                .destination_offset = offset
            },
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
        };

        return p;
    }

    Pattern boolean_(u32 offset, Parser_Hooks hooks) {
        Pattern p = Pattern {
            .type  = Json_Type::Bool,
            .value = {
                .destination_type   = Data_Type::Boolean,
                .destination_offset = offset
            },
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
        };
        return p;
    }

    Pattern object(std::initializer_list<Object_Member> members, Parser_Hooks hooks) {
        Pattern p = Pattern {
            .type = Json_Type::Object,
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
        };

        if (members.size() == 0)
            return p;

        p.object_members = members;

        return p;
    }

    Pattern list(std::initializer_list<Pattern> element_pattern,
                 List_Info list_info,
                 Parser_Hooks hooks)
    {
        panic_if (element_pattern.size() != 1,
                  "You can only pass one pattern here, "
                  "but we had to make it a std::initializer_list "
                  "so that we can put it as a member of the "
                  "pattern struct. (Because we can't put a Pattern "
                  "in a Pattern unless we make it a pointer, but then, "
                  "where does it have to be allocated? If it is on the "
                  " stack at least we can't make the concise pattern syntax work.)");


        Pattern p = Pattern {
            .type = Json_Type::List,
            .list_child = element_pattern,
            .list = {
                .element_size      = list_info.element_size,
                .array_list_offset = list_info.array_list_offset,
            },
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
        };


        return p;
    }

    Pattern custom(Json_Type source_type, Data_Type destination_type,
                   u32 destination_offset, Parser_Hooks hooks)
    {
        Pattern p = Pattern {
            .type  = source_type,
            .value =  {
                .destination_type = destination_type,
                .destination_offset = destination_offset
            },
            .enter_hook = hooks.enter_hook,
            .leave_hook = hooks.leave_hook
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
        fprintf(out, "%lld", *i);
    }

    void write_float_to_file(FILE* out, u32 offset, void* data) {
        float* f = (float*)(((byte*)data)+offset);
        fprintf(out, "%f", *f);
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
            fprintf(out, ", ");
            write_pattern_to_file(out, child_pattern, element_pointer);
        }

        fprintf(out, "]");
    }


    void write_object_to_file(FILE* out, std::initializer_list<Object_Member> members, void* data)
    {
        auto print_one_key_value_pair = [&](Object_Member om) {
            fprintf(out, "\"%s\" : ", om.key);
            write_pattern_to_file(out, om.pattern, data);
        };

        fprintf(out, "{");

        if (members.size() != 0) {
        }

        u32 i = 0;
        for (Object_Member om : members) {
            if (i++ != 0) {
                fprintf(out, ", ");
            }

            print_one_key_value_pair(om);
        }

        fprintf(out, "}");
    }



    void write_pattern_to_file(FILE* out, Pattern pattern, void* user_data) {

        switch (pattern.type) {
            case Json_Type::Null: fprintf(out, "null"); break;
            case Json_Type::Bool: {
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
                                   *pattern.list_child.begin(), user_data);
            } break;
            case Json_Type::Object: {
                write_object_to_file(out, pattern.object_members, user_data);
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
            case Json_Type::Bool:               raw_print("bool");          break;
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
            if (object_members.size() == 0) {
                println("{}");
                return;
            }

            raw_println(" {");
            with_print_prefix("|  ") {
                for (Object_Member om : object_members){
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
                Pattern c = *list_child.begin();
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
