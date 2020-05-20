#pragma once
#define _CRT_SECURE_NO_WARNINGS

#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "hashmap.hpp"

#define str_eq(s1, s2) (strcmp(s1, s2) == 0)

typedef const char* static_string;
typedef int (*printer_function_32b)(FILE*, u32);
typedef int (*printer_function_64b)(FILE*, u64);
typedef int (*printer_function_flt)(FILE*, double);
typedef int (*printer_function_ptr)(FILE*, void*);
typedef int (*printer_function_void)(FILE*);

enum struct Printer_Function_Type {
    unknown,
    _32b,
    _64b,
    _flt,
    _ptr,
    _void
};

Hash_Map<char*, printer_function_32b> printer_map  = {0};
Hash_Map<char*, int>                  type_map     = {0};

#define register_printer(spec, fun, type)       \
    register_printer_32(spec, (printer_function_32b)fun, type)

void register_printer_32(const char* spec, printer_function_32b fun, Printer_Function_Type type) {
    printer_map.set_object((char*)spec, fun);
    type_map.set_object((char*)spec, (int)type);
}

int maybe_special_print(FILE* file, static_string format, int* pos, va_list* arg_list) {
    if(format[*pos] != '{')
        return 0;

    int end_pos = (*pos)+1;
    while (format[end_pos] != '}' &&
           format[end_pos] != ',' &&
           format[end_pos] != '[' &&
           format[end_pos] != 0)
        ++end_pos;

    if (format[end_pos] == 0)
        return 0;

    char* spec = (char*)malloc(end_pos - (*pos));
    strncpy(spec, format+(*pos)+1, end_pos - (*pos));
    spec[end_pos - (*pos)-1] = '\0';

    u64 spec_hash = hm_hash(spec);
    Printer_Function_Type type = (Printer_Function_Type)type_map.get_object(spec, spec_hash);


    union {
        printer_function_32b printer_32b;
        printer_function_64b printer_64b;
        printer_function_ptr printer_ptr;
        printer_function_flt printer_flt;
        printer_function_void printer_void;
    } printer;

    // just grab it, it will have the correct type
    printer.printer_32b = printer_map.get_object(spec, spec_hash);

    free(spec);

    if (type == Printer_Function_Type::unknown) {
        return 0;
    }

    if (format[end_pos] == ',') {
        int element_count;

        ++end_pos;
        sscanf(format+end_pos, "%d", &element_count);

        while (format[end_pos] != '}' &&
               format[end_pos] != 0)
            ++end_pos;
        if (format[end_pos] == 0)
            return 0;

        // both brackets already included:
        int written_length = 2;

        fputs("[", file);
        for (int i = 0; i < element_count - 1; ++i) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, va_arg(*arg_list, u32));
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, va_arg(*arg_list, u64));
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, va_arg(*arg_list, double));
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, va_arg(*arg_list, void*));
            else                                          written_length += printer.printer_void(file);
            written_length += 2;
            fputs(", ", file);
        }
        if (element_count > 0) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, va_arg(*arg_list, u32));
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, va_arg(*arg_list, u64));
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, va_arg(*arg_list, double));
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, va_arg(*arg_list, void*));
            else                                          written_length += printer.printer_void(file);
        }
        fputs("]", file);

        *pos = end_pos;
        return written_length;
    } else if (format[end_pos] == '[') {
        end_pos++;
        u32 element_count;
        union {
            u32*    arr_32b;
            u64*    arr_64b;
            f32*    arr_flt;
            void**  arr_ptr;
        } arr;

        if      (type == Printer_Function_Type::_32b) arr.arr_32b = va_arg(*arg_list, u32*);
        else if (type == Printer_Function_Type::_64b) arr.arr_64b = va_arg(*arg_list, u64*);
        else if (type == Printer_Function_Type::_flt) arr.arr_flt = va_arg(*arg_list, f32*);
        else                                          arr.arr_ptr = va_arg(*arg_list, void**);

        if (format[end_pos] == '*') {
            element_count =  va_arg(*arg_list, u32);
        } else {
            sscanf(format+end_pos, "%d", &element_count);
        }

        // skip to next ']'
        while (format[end_pos] != ']' &&
               format[end_pos] != 0)
            ++end_pos;
        if (format[end_pos] == 0)
            return 0;

        // skip to next '}'
        while (format[end_pos] != '}' &&
               format[end_pos] != 0)
            ++end_pos;
        if (format[end_pos] == 0)
            return 0;

        // both brackets already included:
        int written_length = 2;

        fputs("[", file);
        for (u32 i = 0; i < element_count - 1; ++i) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, arr.arr_32b[i]);
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, arr.arr_64b[i]);
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, arr.arr_flt[i]);
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, arr.arr_ptr[i]);
            else                                          written_length += printer.printer_void(file);
            written_length += 2;
            fputs(", ", file);
        }
        if (element_count > 0) {
            if      (type == Printer_Function_Type::_32b) written_length += printer.printer_32b(file, arr.arr_32b[element_count - 1]);
            else if (type == Printer_Function_Type::_64b) written_length += printer.printer_64b(file, arr.arr_64b[element_count - 1]);
            else if (type == Printer_Function_Type::_flt) written_length += printer.printer_flt(file, arr.arr_flt[element_count - 1]);
            else if (type == Printer_Function_Type::_ptr) written_length += printer.printer_ptr(file, arr.arr_ptr[element_count - 1]);
            else                                          written_length += printer.printer_void(file);
        }
        fputs("]", file);

        *pos = end_pos;
        return written_length;
    } else {
        *pos = end_pos;
        if      (type == Printer_Function_Type::_32b) return printer.printer_32b(file, va_arg(*arg_list, u32));
        else if (type == Printer_Function_Type::_64b) return printer.printer_64b(file, va_arg(*arg_list, u64));
        else if (type == Printer_Function_Type::_flt) return printer.printer_flt(file, va_arg(*arg_list, double));
        else if (type == Printer_Function_Type::_ptr) return printer.printer_ptr(file, va_arg(*arg_list, void*));
        else                                          return printer.printer_void(file);
    }
    return 0;

}

int maybe_fprintf(FILE* file, static_string format, int* pos, va_list* arg_list) {
    // %[flags][width][.precision][length]specifier
    // flags     ::= [+- #0]
    // width     ::= [<number>+ \*]
    // precision ::= \.[<number>+ \*]
    // length    ::= [h l L]
    // specifier ::= [c d i e E f g G o s u x X p n %]
    int end_pos = *pos;
    int writen_len = 0;
    int used_arg_values = 1;

    // overstep flags:
    while(format[end_pos] == '+' ||
          format[end_pos] == '-' ||
          format[end_pos] == ' ' ||
          format[end_pos] == '#' ||
          format[end_pos] == '0')
        ++end_pos;

    // overstep width
    if (format[end_pos] == '*') {
        ++used_arg_values;
        ++end_pos;
    }
    else {
        while(format[end_pos] >= '0' && format[end_pos] <= '9')
            ++end_pos;
    }

    // overstep precision
    if (format[end_pos] == '.') {
        ++end_pos;
        if (format[end_pos] == '*') {
            ++used_arg_values;
            ++end_pos;
        }
        else {
            while(format[end_pos] >= '0' && format[end_pos] <= '9')
                ++end_pos;
        }
    }

    // overstep length:
    while(format[end_pos] == 'h' ||
          format[end_pos] == 'l' ||
          format[end_pos] == 'L')
        ++end_pos;

    //  check for actual built_in specifier
    if(format[end_pos] == 'c' ||
       format[end_pos] == 'd' ||
       format[end_pos] == 'i' ||
       format[end_pos] == 'e' ||
       format[end_pos] == 'E' ||
       format[end_pos] == 'f' ||
       format[end_pos] == 'g' ||
       format[end_pos] == 'G' ||
       format[end_pos] == 'o' ||
       format[end_pos] == 's' ||
       format[end_pos] == 'u' ||
       format[end_pos] == 'x' ||
       format[end_pos] == 'X' ||
       format[end_pos] == 'p' ||
       format[end_pos] == 'n' ||
       format[end_pos] == '%')
    {
        writen_len = end_pos - *pos + 2;
        char* temp = (char*)malloc((writen_len+1)* sizeof(char));
        temp[0] = '%';
        temp[1] = 0;
        strncpy(temp+1, format+*pos, writen_len);
        temp[writen_len] = 0;

        // printf("\ntest:: len(%s) = %d\n", temp, writen_len+1);

        writen_len = vfprintf(file, temp, *arg_list);

        // NOTE(Felix): For WSL Linux we have to manually overstep the
        // used args
        for (int i = 0; i < used_arg_values;  ++i) {
            va_arg(*arg_list, void*);
        }

        free(temp);
        *pos = end_pos;
    }

    return writen_len;
}


int print_va_args_to_file(FILE* file, static_string format, va_list* arg_list) {
    int printed_chars = 0;

    char c;
    int pos = -1;
    while ((c = format[++pos])) {
        if (c != '%') {
            putc(c, file);
            ++printed_chars;
        } else {
            c = format[++pos];
            int move = maybe_special_print(file, format, &pos, arg_list);
            if (move == 0) {
                move = maybe_fprintf(file, format, &pos, arg_list);
                if (move == 0) {
                    putchar('%');
                    putchar(c);
                    move = 1;
                }
            }
            printed_chars += move;
        }
    }

    return printed_chars;
}

int print_va_args_to_string(char** out, static_string format, va_list* arg_list) {
    FILE* t_file = tmpfile();
    if (!t_file) {
        return 0;
    }

    int num_printed_chars = print_va_args_to_file(t_file, format, arg_list);

    *out = (char*)malloc(sizeof(char) * (num_printed_chars+1));

    rewind(t_file);
    fread(*out, sizeof(char), num_printed_chars, t_file);
    (*out)[num_printed_chars] = '\0';

    fclose(t_file);

    return num_printed_chars;
}

int print_va_args(static_string format, va_list* arg_list) {
    return print_va_args_to_file(stdout, format, arg_list);
}

int print_to_string(char** out, static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    FILE* t_file = tmpfile();
    if (!t_file) {
        return 0;
    }

    int num_printed_chars = print_va_args_to_file(t_file, format, &arg_list);
    va_end(arg_list);


    *out = (char*)malloc(sizeof(char) * (num_printed_chars+1));

    rewind(t_file);
    fread(*out, sizeof(char), num_printed_chars, t_file);
    (*out)[num_printed_chars] = '\0';

    fclose(t_file);

    return num_printed_chars;
}

int print_to_file(FILE* file, static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = print_va_args_to_file(file, format, &arg_list);

    va_end(arg_list);

    return num_printed_chars;
}

int print(static_string format, ...) {
    va_list arg_list;
    va_start(arg_list, format);

    int num_printed_chars = print_va_args_to_file(stdout, format, &arg_list);

    va_end(arg_list);

    return num_printed_chars;
}

int print_bool(FILE* f, u32 val) {
    return print_to_file(f, val ? "true" : "false");
}

int print_u32(FILE* f, u32 num) {
    return print_to_file(f, "%u", num);
}

int print_u64(FILE* f, u64 num) {
    return print_to_file(f, "%llu", num);
}

int print_s32(FILE* f, s32 num) {
    return print_to_file(f, "%d", num);
}

int print_s64(FILE* f, s64 num) {
    return print_to_file(f, "%lld", num);
}

int print_flt(FILE* f, double arg) {
    return print_to_file(f, "%f", arg);
}

int print_str(FILE* f, char* str) {
    return print_to_file(f, "%s", str);
}

int print_ptr(FILE* f, void* ptr) {
    if (ptr)
        return print_to_file(f, "%#0*X", sizeof(void*)*2+2, ptr);
    return print_to_file(f, "nullptr");
}

void deinit_printer() {
    printer_map.dealloc();
    type_map.dealloc();
}

void init_printer() {
    printer_map.alloc();
    type_map.alloc();

    register_printer("u32",  print_u32,  Printer_Function_Type::_32b);
    register_printer("u64",  print_u64,  Printer_Function_Type::_64b);
    register_printer("bool", print_bool, Printer_Function_Type::_32b);
    register_printer("s64",  print_s64,  Printer_Function_Type::_64b);
    register_printer("s32",  print_s32,  Printer_Function_Type::_32b);
    register_printer("f32",  print_flt,  Printer_Function_Type::_flt);
    register_printer("f64",  print_flt,  Printer_Function_Type::_flt);
    register_printer("str",  print_str,  Printer_Function_Type::_ptr);
    register_printer("ptr",  print_ptr,  Printer_Function_Type::_ptr);
}
