#define _CRT_SECURE_NO_WARNINGS
#define FTB_CORE_IMPL
#include "../core.hpp"

int main() {
#define test_color(code) \
    println("%{color<}%s%{>color}", code, #code);
    printf("@%4.1f@\n", 2.56f);
    test_color(console_black);
    test_color(console_red);
    test_color(console_green);
    test_color(console_yellow);
    test_color(console_blue);
    test_color(console_magenta);
    test_color(console_cyan);
    test_color(console_white);
    test_color(console_black_bold);
    test_color(console_red_bold);
    test_color(console_green_bold);
    test_color(console_yellow_bold);
    test_color(console_blue_bold);
    test_color(console_magenta_bold);
    test_color(console_cyan_bold);
    test_color(console_white_bold);
    test_color(console_black_dim);
    test_color(console_red_dim);
    test_color(console_green_dim);
    test_color(console_yellow_dim);
    test_color(console_blue_dim);
    test_color(console_magenta_dim);
    test_color(console_cyan_dim);
    test_color(console_white_dim);
    test_color(console_black_blinking);
    test_color(console_red_blinking);
    test_color(console_green_blinking);
    test_color(console_yellow_blinking);
    test_color(console_blue_blinking);
    test_color(console_magenta_blinking);
    test_color(console_cyan_blinking);
    test_color(console_white_blinking);
    test_color(console_black_underline);
    test_color(console_red_underline);
    test_color(console_green_underline);
    test_color(console_yellow_underline);
    test_color(console_blue_underline);
    test_color(console_magenta_underline);
    test_color(console_cyan_underline);
    test_color(console_white_underline);
    test_color(console_black_high_intensity);
    test_color(console_red_high_intensity);
    test_color(console_green_high_intensity);
    test_color(console_yellow_high_intensity);
    test_color(console_blue_high_intensity);
    test_color(console_magenta_high_intensity);
    test_color(console_cyan_high_intensity);
    test_color(console_white_high_intensity);
    test_color(console_black_bold_high_intensity);
    test_color(console_red_bold_high_intensity);
    test_color(console_green_bold_high_intensity);
    test_color(console_yellow_bold_high_intensity);
    test_color(console_blue_bold_high_intensity);
    test_color(console_magenta_bold_high_intensity);
    test_color(console_cyan_bold_high_intensity);
    test_color(console_white_bold_high_intensity);
}
