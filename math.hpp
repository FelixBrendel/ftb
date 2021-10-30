#pragma once
#include "types.hpp"

union V2 {
    struct {
        f32 x, y;
    };
    struct {
        f32 u, v;
    };
    f32 elements[2];

    inline f32 &operator[](const int &index) {
        return elements[index];
    }
};

union V3 {
    struct {
        f32 x, y, z;
    };
    struct {
        f32 r, g, b;
    };
    struct {
        V2 xy;
        f32 _padding_1;
    };
    struct {
        f32 _padding_2;
        V2 yz;
    };
    f32 elements[3];

    inline f32 &operator[](const int &index) {
        return elements[index];
    }
};

