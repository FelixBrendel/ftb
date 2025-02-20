/*
 * BSD 2-Clause License
 *
 * Copyright (c) 2021, Felix Brendel
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * * Redistributions of source code must retain the above copyright notice, this
 *   list of conditions and the following disclaimer.
 *
 * * Redistributions in binary form must reproduce the above copyright notice,
 *   this list of conditions and the following disclaimer in the documentation
 *   and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR CONTRIBUTORS BE LIABLE
 * FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR
 * SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 * CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY,
 * OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#pragma once
#include <cmath>
#include <float.h>
#include "core.hpp"

#define FTB_USING_MATH

extern f32 pi;
extern f32 two_pi;

template<typename type>
union Gen_V2 {
    struct {
        type x, y;
    };
    struct {
        type u, v;
    };
    type elements[2];

    inline type &operator[](const int &index) {
        return elements[index];
    }
};

template<typename type>
union Gen_V3 {
    struct {
        type x, y, z;
    };
    struct {
        type r, g, b;
    };
    struct {
        Gen_V2<type> xy;
        type _padding_1;
    };
    struct {
        type _padding_2;
        Gen_V2<type> yz;
    };
    type elements[3];

    inline type &operator[](const int &index) {
        return elements[index];
    }
};

template<typename type>
union Gen_V4 {
    struct {
        type x, y, z, w;
    };
    struct {
        type r, g, b, a;
    };
    struct {
        Gen_V2<type> xy;
        Gen_V2<type> zw;
    };
    struct {
        type _padding_1;
        Gen_V2<type> yz;
        type _padding_2;
    };
    struct {
        Gen_V3<type> xyz;
        type _padding_3;
    };
    struct {
        Gen_V3<type> rgb;
        type _padding_4;
    };
    struct {
        type _padding_5;
        Gen_V3<type> yzw;
    };
    type elements[4];

    inline type &operator[](const int &index) {
        return elements[index];
    }
};

template<typename type>
struct Gen_Ray {
    type offset;
    type direction;
};

typedef Gen_V2<f32> V2;
typedef Gen_V3<f32> V3;
typedef Gen_V4<f32> V4;
typedef V4 Quat;
typedef Gen_V2<s32> IV2;
typedef Gen_V3<s32> IV3;
typedef Gen_V4<s32> IV4;

typedef Gen_Ray<V3> Ray3;
typedef Gen_Ray<V2> Ray2;

// NOTE(Felix): All matrics are column-major, since glsl shaders expect it that
//   way, so we can just memcpy the matrices. This also includes the _00, _01
//   selectors where the first number also represents the column and the second
//   the row.
union M2x2 {
    struct {
        f32 _00; f32 _01;
        f32 _10; f32 _11;
    };
    V2  columns[2];
    f32 elements[4];

    inline f32 &operator[](const int &index) {
        return elements[index];
    }
};

union M3x3 {
    struct {
        f32 _00; f32 _01; f32 _02;
        f32 _10; f32 _11; f32 _12;
        f32 _20; f32 _21; f32 _22;
    };
    V3  columns[3];
    f32 elements[9];

    inline f32 &operator[](const int &index) {
        return elements[index];
    }
};

union M4x4 {
    struct {
        f32 _00; f32 _01; f32 _02; f32 _03;
        f32 _10; f32 _11; f32 _12; f32 _13;
        f32 _20; f32 _21; f32 _22; f32 _23;
        f32 _30; f32 _31; f32 _32; f32 _33;
    };
    V4  columns[4];
    f32 elements[16];

    inline f32 &operator[](const int &index) {
        return elements[index];
    }
};

// ---------------------
//     f32 functions
// ---------------------
auto rad_to_deg(f32 rad) -> f32;
auto deg_to_rad(f32 dec) -> f32;

auto round_to_precision(f32 num, u32 decimals) ->  f32;
auto round_to_int(f32 num) ->  s32;

auto linerp(f32 from, f32 t, f32 to) -> f32;
auto unlinerp(f32 from, f32 val, f32 to) -> f32;
auto remap(f32 from_a, f32 val, f32 to_a, f32 from_b, f32 to_b) -> f32;

auto clamp(u32 from, u32 x, u32 to) -> u32;
auto clamp(f32 from, f32 x, f32 to) -> f32;
auto clamp01(f32 x) -> f32;
auto clamped_linerp(f32 from, f32 t, f32 to) -> f32;

auto rand_0_1() -> f32;
auto rand_int(s32 lower, s32 upper) -> int;

// ---------------------
//   vector functions
// ---------------------

auto remap(V2 from_a, V2 val, V2 to_a, V2 from_b, V2 to_b) -> V2;
auto remap(V3 from_a, V3 val, V3 to_a, V3 from_b, V3 to_b) -> V3;
auto remap(V4 from_a, V4 val, V4 to_a, V4 from_b, V4 to_b) -> V4;

auto v3(V2 xy, f32 z) -> V3;
auto v4(V3 xyz, f32 w) -> V4;

auto cross(V3 a, V3 b) -> V3;

auto dot(V2 a, V2 b) -> f32;
auto dot(V3 a, V3 b) -> f32;
auto dot(V4 a, V4 b) -> f32;

auto hadamard(V2 a, V2 b) -> V2;
auto hadamard(V3 a, V3 b) -> V3;
auto hadamard(V4 a, V4 b) -> V4;

auto length(V2 vector) -> f32;
auto length(V3 vector) -> f32;
auto length(V4 vector) -> f32;

auto reflect(V2 vector, V2 normal) -> V2;
auto reflect(V3 vector, V3 normal) -> V3;
auto reflect(V4 vector, V4 normal) -> V4;

auto noz(V2 vector) -> V2;
auto noz(V3 vector) -> V3;
auto noz(V4 vector) -> V4;

auto linerp(V2 from, f32 t, V2 to) -> V2;
auto linerp(V3 from, f32 t, V3 to) -> V3;
auto linerp(V4 from, f32 t, V4 to) -> V4;

auto operator==(V2 a, V2 b) -> bool;
auto operator==(V3 a, V3 b) -> bool;
auto operator==(V4 a, V4 b) -> bool;

auto operator!=(V2 a, V2 b) -> bool;
auto operator!=(V3 a, V3 b) -> bool;
auto operator!=(V4 a, V4 b) -> bool;

auto operator+(V2 a, V2 b) -> V2;
auto operator+(V3 a, V3 b) -> V3;
auto operator+(V4 a, V4 b) -> V4;

auto operator-(V2 a) -> V2;
auto operator-(V3 a) -> V3;
auto operator-(V4 a) -> V4;

auto operator-(V2 a, V2 b) -> V2;
auto operator-(V3 a, V3 b) -> V3;
auto operator-(V4 a, V4 b) -> V4;

auto operator*(V2 v, f32 s) -> V2;
auto operator*(V3 v, f32 s) -> V3;
auto operator*(V4 v, f32 s) -> V4;

auto operator*(f32 s, V2 v) -> V2;
auto operator*(f32 s, V3 v) -> V3;
auto operator*(f32 s, V4 v) -> V4;

auto operator*(M2x2 m, V2 v) -> V2;
auto operator*(M3x3 m, V3 v) -> V3;
auto operator*(M4x4 m, V4 v) -> V4;

// ---------------------
//    matrix functions
// ---------------------
auto operator==(M2x2 a, M2x2 b) -> bool;
auto operator==(M3x3 a, M3x3 b) -> bool;
auto operator==(M4x4 a, M4x4 b) -> bool;

auto operator*(M2x2 a, M2x2 b) -> M2x2;
auto operator*(M3x3 a, M3x3 b) -> M3x3;
auto operator*(M4x4 a, M4x4 b) -> M4x4;

auto operator+(M2x2 a, M2x2 b) -> M2x2;
auto operator+(M3x3 a, M3x3 b) -> M3x3;
auto operator+(M4x4 a, M4x4 b) -> M4x4;

auto operator-(M2x2 a, M2x2 b) -> M2x2;
auto operator-(M3x3 a, M3x3 b) -> M3x3;
auto operator-(M4x4 a, M4x4 b) -> M4x4;

auto operator*(M2x2 a, f32 s)  -> M2x2;
auto operator*(M3x3 a, f32 s)  -> M3x3;
auto operator*(M4x4 a, f32 s)  -> M4x4;

auto operator*(f32 s, M2x2 a)  -> M2x2;
auto operator*(f32 s, M3x3 a)  -> M3x3;
auto operator*(f32 s, M4x4 a)  -> M4x4;

auto quick_invert_transformation_mat_without_scale(M4x4 mat) -> M4x4;

auto m4x4(M3x3) -> M4x4;
auto m4x4_identity() -> M4x4;
auto m4x4_from_axis_angle(V3 axis, f32 angle_in_rad) -> M4x4;
auto identity_look_at(V3* out_eye, V3* out_target, V3* out_up) -> void;
auto look_at_from_m4x4(M4x4 view_mat, V3* out_eye, V3* out_target, V3* out_up) -> void;
auto m4x4_look_at(V3 eye, V3 target, V3 up) -> M4x4;
auto m4x4_perspective(f32 fov_y, f32 aspect, f32 clip_near, f32 clip_far) -> M4x4;
auto m4x4_ortho(f32 left, f32 right, f32 bottom, f32 top) -> M4x4;
auto m4x4_ortho(f32 x_extend, f32 aspect) -> M4x4;


auto m4x4_decompose(M4x4 input, M3x3* out_rot_sheer, V3* out_translation) -> void;
auto m4x4_compose(M3x3 out_rot_sheer, V3 out_translation) -> M4x4;
auto m4x4_transpose(M4x4 mat) -> M4x4;
auto m4x4_orientation(Quat orientation) -> M4x4;
auto m4x4_translate(V3 tanslation) -> M4x4;
auto m4x4_scale(V3 scale) -> M4x4;
auto m4x4_model(V3 tanslation, Quat orientation, V3 scale) -> M4x4;
auto m4x4_print(M4x4) -> void;
auto m3x3_print(M3x3 m) -> void;
auto v3_print(V3 v) -> void;

auto m3x3_identity() -> M3x3;
auto m3x3_orientation(Quat orientation) -> M3x3;
auto m3x3_transpose(M3x3 mat) -> M3x3;

auto frobenius(M3x3) -> float;
auto frobenius(M4x4) -> float;
// ---------------------
//     quat functions
// ---------------------
auto quat_from_axis_angle(V3 axis, f32 angle) -> Quat;
auto quat_from_XYZ(f32 x, f32 y, f32 z) -> Quat;
auto quat_to_XYZ_Euler(Quat q) -> V3;
auto quat_from_m4x4(M4x4 m) -> Quat;

// ---------------------
//  geometry functions
// ---------------------
auto closest_t_on_ray(Ray3 ray, V3 point) -> f32;
auto closest_point_on_ray(Ray3 ray, V3 point) -> V3;


#ifdef FTB_MATH_IMPL
#include <random>

f32 pi     = 3.1415926535897932384626433832795f;
f32 two_pi = 6.283185307179586476925286766559f;

// ---------------------
//     f32 functions
// ---------------------
auto rad_to_deg(f32 rad) -> f32 {
    return rad / pi * 180;
}

auto deg_to_rad(f32 deg) -> f32 {
    return deg / 180 * pi;
}

auto round_to_precision(f32 num, u32 decimals) ->  f32 {
    f32 factor = powf(10.0f, (f32)decimals);
    return roundf(num * factor) / factor;
}

auto round_to_int(f32 num) -> s32 {
    return (s32)(num < 0 ? (num - 0.5f) : (num + 0.5f));
}

auto linerp(f32 from, f32 t, f32 to) -> f32 {
    return from + (to - from) * t;
}

auto unlinerp(f32 from, f32 val, f32 to) -> f32 {
    return (val - from) / (to - from);
}

auto remap(f32 from_a, f32 val, f32 to_a, f32 from_b, f32 to_b) -> f32 {
    return linerp(from_b, unlinerp(from_a, val, to_a), to_b);
}

auto clamp(u32 from, u32 x, u32 to) -> u32 {
    u32 t = x < from ? from : x;
    return t > to ? to : t;
}

auto clamp(f32 from, f32 x, f32 to) -> f32 {
    f32 t = x < from ? from : x;
    return t > to ? to : t;
}

auto clamp01(f32 x) -> f32 {
    return clamp(0, x, 1);
}

auto clamped_linerp(f32 from, f32 t, f32 to) -> f32 {
    t = clamp01(t);
    return from + (to - from) * t;
}

// ---------------------
//   vector functions
// ---------------------
auto remap(V2 from_a, V2 val, V2 to_a, V2 from_b, V2 to_b) -> V2 {
    return {
        remap(from_a.x, val.x, to_a.x, from_b.x, to_b.x),
        remap(from_a.y, val.y, to_a.y, from_b.y, to_b.y),
    };
}

auto remap(V3 from_a, V3 val, V3 to_a, V3 from_b, V3 to_b) -> V3 {
    return {
        remap(from_a.x, val.x, to_a.x, from_b.x, to_b.x),
        remap(from_a.y, val.y, to_a.y, from_b.y, to_b.y),
        remap(from_a.z, val.z, to_a.z, from_b.z, to_b.z),
    };
}

auto remap(V4 from_a, V4 val, V4 to_a, V4 from_b, V4 to_b) -> V4 {
    return {
        remap(from_a.x, val.x, to_a.x, from_b.x, to_b.x),
        remap(from_a.y, val.y, to_a.y, from_b.y, to_b.y),
        remap(from_a.z, val.z, to_a.z, from_b.z, to_b.z),
        remap(from_a.w, val.w, to_a.w, from_b.w, to_b.w),
    };
}

auto v3(V2 xy, f32 z = 0) -> V3 {
    return {
        xy.x,
        xy.y,
        z
    };
}


auto v4(V3 xyz, f32 w = 0) -> V4 {
    return {
        xyz.x,
        xyz.y,
        xyz.z,
        w
    };
}

auto dot(V2 a, V2 b) -> f32 {
    return a.x * b.x + a.y * b.y;
}

auto dot(V3 a, V3 b) -> f32 {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

auto dot(V4 a, V4 b) -> f32 {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

auto cross(V3 a, V3 b) -> V3 {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x};
}

auto hadamard(V2 a, V2 b) -> V2 {
    return {
        a.x * b.x,
        a.y * b.y,
    };
}

auto hadamard(V3 a, V3 b) -> V3 {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
    };
}

auto hadamard(V4 a, V4 b) -> V4 {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
        a.w * b.w,
    };
}

auto operator+(V2 a, V2 b) -> V2 {
    return {
        a.x + b.x,
        a.y + b.y
    };
}

auto operator+(V3 a, V3 b) -> V3 {
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

auto operator+(V4 a, V4 b) -> V4 {
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w
    };
}

auto operator-(V2 a) -> V2 {
    return {
        -a.x,
        -a.y
    };
}

auto operator-(V3 a) -> V3 {
    return {
        -a.x,
        -a.y,
        -a.z
    };
}

auto operator-(V4 a) -> V4 {
    return {
        -a.x,
        -a.y,
        -a.z,
        -a.w
    };
}

auto operator-(V2 a, V2 b) -> V2 {
    return {
        a.x - b.x,
        a.y - b.y,
    };
}

auto operator-(V3 a, V3 b) -> V3 {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
    };
}

auto operator-(V4 a, V4 b) -> V4 {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w,
    };
}


auto operator*(V2 v, f32 s) -> V2 {
    return {
        v.x * s,
        v.y * s,
    };
}

auto operator*(V3 v, f32 s) -> V3 {
    return {
        v.x * s,
        v.y * s,
        v.z * s,
    };
}

auto operator*(V4 v, f32 s) -> V4 {
    return {
        v.x * s,
        v.y * s,
        v.z * s,
        v.w * s,
    };
}

auto operator*(f32 s, V2 v) -> V2 {
    return v * s;
}

auto operator*(f32 s, V3 v) -> V3 {
    return v * s;
}

auto operator*(f32 s, V4 v) -> V4 {
    return v * s;
}

auto operator*(M2x2 m, V2 v) -> V2 {
    return {
        dot({m.columns[0][0], m.columns[1][0]}, v),
        dot({m.columns[0][1], m.columns[1][1]}, v),
    };
}

auto operator*(M3x3 m, V3 v) -> V3 {
    return {
        dot({m.columns[0][0], m.columns[1][0], m.columns[2][0]}, v),
        dot({m.columns[0][1], m.columns[1][1], m.columns[2][1]}, v),
        dot({m.columns[0][2], m.columns[1][2], m.columns[2][2]}, v),
    };
}

auto operator*(M4x4 m, V4 v) -> V4 {
    return {
        dot({m.columns[0][0], m.columns[1][0], m.columns[2][0], m.columns[3][0]}, v),
        dot({m.columns[0][1], m.columns[1][1], m.columns[2][1], m.columns[3][1]}, v),
        dot({m.columns[0][2], m.columns[1][2], m.columns[2][2], m.columns[3][2]}, v),
        dot({m.columns[0][3], m.columns[1][3], m.columns[2][3], m.columns[3][3]}, v),
    };
}

auto reflect(V2 vector, V2 normal) -> V2 {
    return 2 * dot(-vector, normal) * normal + vector;
}

auto reflect(V3 vector, V3 normal) -> V3 {
    return 2 * dot(-vector, normal) * normal + vector;
}

auto reflect(V4 vector, V4 normal) -> V4 {
    return 2 * dot(-vector, normal) * normal + vector;
}

auto length(V2 vector) -> f32 {
    f32 length =
        vector.x * vector.x +
        vector.y * vector.y;
    return sqrtf(length);
}

auto length(V3 vector) -> f32 {
    f32 length =
        vector.x * vector.x +
        vector.y * vector.y +
        vector.z * vector.z;
    return sqrtf(length);
}

auto length(V4 vector) -> f32 {
    f32 length =
        vector.x * vector.x +
        vector.y * vector.y +
        vector.z * vector.z +
        vector.w * vector.w;
    return sqrtf(length);
}

auto noz(V2 vector) -> V2 {
    f32 len = length(vector);
    if (len == 0)
        return vector;
    return 1.0f / len * vector;
}

auto noz(V3 vector) -> V3 {
    f32 len = length(vector);
    if (len == 0)
        return vector;
    return 1.0f / len * vector;
}

auto noz(V4 vector) -> V4 {
    f32 len = length(vector);
    if (len == 0)
        return vector;
    return (1.0f / len) * vector;
}

auto linerp(V2 from, f32 t, V2 to) -> V2 {
    return from + (to - from) * t;
}

auto linerp(V3 from, f32 t, V3 to) -> V3 {
    return from + (to - from) * t;
}

auto linerp(V4 from, f32 t, V4 to) -> V4 {
    return from + (to - from) * t;
}

auto operator==(V2 a, V2 b) -> bool {
    return
        a.x == b.x &&
        a.y == b.y;
}

auto operator==(V3 a, V3 b) -> bool {
    return
        a.x == b.x &&
        a.y == b.y &&
        a.z == b.z;
}

auto operator==(V4 a, V4 b) -> bool {
    return
        a.x == b.x &&
        a.y == b.y &&
        a.z == b.z &&
        a.w == b.w;
}


auto operator!=(V2 a, V2 b) -> bool {
    return
        a.x != b.x ||
        a.y != b.y;
}

auto operator!=(V3 a, V3 b) -> bool {
    return
        a.x != b.x ||
        a.y != b.y ||
        a.z != b.z;
}

auto operator!=(V4 a, V4 b) -> bool {
    return
        a.x != b.x ||
        a.y != b.y ||
        a.z != b.z ||
        a.w != b.w;
}

// ---------------------
//    matrix functions
// ---------------------

auto operator==(M2x2 a, M2x2 b) -> bool {
    return
        a.columns[0] == b.columns[0] &&
        a.columns[1] == b.columns[1];
}

auto operator==(M3x3 a, M3x3 b) -> bool {
    return
        a.columns[0] == b.columns[0] &&
        a.columns[1] == b.columns[1] &&
        a.columns[2] == b.columns[2];
}

auto operator==(M4x4 a, M4x4 b) -> bool {
    return
        a.columns[0] == b.columns[0] &&
        a.columns[1] == b.columns[1] &&
        a.columns[2] == b.columns[2] &&
        a.columns[3] == b.columns[3];
}


auto operator*(M2x2 a, M2x2 b) -> M2x2 {
    M2x2 result {
        dot(V2{a._00, a._10}, b.columns[0]), dot(V2{a._01, a._11}, b.columns[0]),
        dot(V2{a._00, a._10}, b.columns[1]), dot(V2{a._01, a._11}, b.columns[1]),
    };

    return result;
}


auto operator*(M3x3 a, M3x3 b) -> M3x3 {
    M3x3 result {
        dot({a._00, a._10, a._20}, b.columns[0]), dot({a._01, a._11, a._21}, b.columns[0]), dot({a._02, a._12, a._22}, b.columns[0]),
        dot({a._00, a._10, a._20}, b.columns[1]), dot({a._01, a._11, a._21}, b.columns[1]), dot({a._02, a._12, a._22}, b.columns[1]),
        dot({a._00, a._10, a._20}, b.columns[2]), dot({a._01, a._11, a._21}, b.columns[2]), dot({a._02, a._12, a._22}, b.columns[2]),

    };

    return result;
}

auto operator*(M4x4 a, M4x4 b) -> M4x4 {
    M4x4 result {
        dot({a._00, a._10, a._20, a._30}, b.columns[0]), dot({a._01, a._11, a._21, a._31}, b.columns[0]),
        dot({a._02, a._12, a._22, a._32}, b.columns[0]), dot({a._03, a._13, a._23, a._33}, b.columns[0]),

        dot({a._00, a._10, a._20, a._30}, b.columns[1]), dot({a._01, a._11, a._21, a._31}, b.columns[1]),
        dot({a._02, a._12, a._22, a._32}, b.columns[1]), dot({a._03, a._13, a._23, a._33}, b.columns[1]),

        dot({a._00, a._10, a._20, a._30}, b.columns[2]), dot({a._01, a._11, a._21, a._31}, b.columns[2]),
        dot({a._02, a._12, a._22, a._32}, b.columns[2]), dot({a._03, a._13, a._23, a._33}, b.columns[2]),

        dot({a._00, a._10, a._20, a._30}, b.columns[3]), dot({a._01, a._11, a._21, a._31}, b.columns[3]),
        dot({a._02, a._12, a._22, a._32}, b.columns[3]), dot({a._03, a._13, a._23, a._33}, b.columns[3]),
    };

    return result;
}

auto operator-(M2x2 a, M2x2 b) -> M2x2 {
    M2x2 result;
    result.columns[0] = a.columns[0] - b.columns[0];
    result.columns[1] = a.columns[1] - b.columns[1];
    return result;
}

auto operator-(M3x3 a, M3x3 b) -> M3x3 {
    M3x3 result;
    result.columns[0] = a.columns[0] - b.columns[0];
    result.columns[1] = a.columns[1] - b.columns[1];
    result.columns[2] = a.columns[2] - b.columns[2];
    return result;
}

auto operator-(M4x4 a, M4x4 b) -> M4x4 {
    M4x4 result;
    result.columns[0] = a.columns[0] - b.columns[0];
    result.columns[1] = a.columns[1] - b.columns[1];
    result.columns[2] = a.columns[2] - b.columns[2];
    result.columns[3] = a.columns[3] - b.columns[3];
    return result;
}

auto operator+(M2x2 a, M2x2 b) -> M2x2 {
    M2x2 result;
    result.columns[0] = a.columns[0] + b.columns[0];
    result.columns[1] = a.columns[1] + b.columns[1];
    return result;
}

auto operator+(M3x3 a, M3x3 b) -> M3x3 {
    M3x3 result;
    result.columns[0] = a.columns[0] + b.columns[0];
    result.columns[1] = a.columns[1] + b.columns[1];
    result.columns[2] = a.columns[2] + b.columns[2];
    return result;
}

auto operator+(M4x4 a, M4x4 b) -> M4x4 {
    M4x4 result;
    result.columns[0] = a.columns[0] + b.columns[0];
    result.columns[1] = a.columns[1] + b.columns[1];
    result.columns[2] = a.columns[2] + b.columns[2];
    result.columns[3] = a.columns[3] + b.columns[3];
    return result;
}

auto operator*(M2x2 a, f32 s) -> M2x2 {
    M2x2 result;
    result.columns[0] = a.columns[0] * s;
    result.columns[1] = a.columns[1] * s;
    return result;
}

auto operator*(M3x3 a, f32 s) -> M3x3 {
    M3x3 result;
    result.columns[0] = a.columns[0] * s;
    result.columns[1] = a.columns[1] * s;
    result.columns[2] = a.columns[2] * s;
    return result;
}

auto operator*(M4x4 a, f32 s)  -> M4x4 {
    M4x4 result;
    result.columns[0] = a.columns[0] * s;
    result.columns[1] = a.columns[1] * s;
    result.columns[2] = a.columns[2] * s;
    result.columns[3] = a.columns[3] * s;
    return result;
}

auto operator*(f32 s, M2x2 a)  -> M2x2 {
    return a * s;
}

auto operator*(f32 s, M3x3 a)  -> M3x3 {
    return a * s;
}

auto operator*(f32 s, M4x4 a)  -> M4x4 {
    return a * s;
}

auto m4x4(M3x3 m) -> M4x4 {
    M4x4 mat {};

    mat._00 = m._00;
    mat._01 = m._01;
    mat._02 = m._02;

    mat._10 = m._10;
    mat._11 = m._11;
    mat._12 = m._12;

    mat._20 = m._20;
    mat._21 = m._21;
    mat._22 = m._22;

    mat._33 = 1.0f;

    return mat;
}

M4x4 m4x4_identity() {
    M4x4 mat {};

    mat._00 = 1;
    mat._11 = 1;
    mat._22 = 1;
    mat._33 = 1;

    return mat;
}

auto m4x4_from_axis_angle(V3 axis, f32 angle_in_rad) -> M4x4 {
    f32 a = angle_in_rad;
    f32 c = cosf(a);
    f32 s = sinf(a);
    axis = noz(axis);
    V3 temp = (1 - c) * axis;

    M4x4 r {};
    r._00 = c + temp[0] * axis[0];
    r._01 = temp[1] * axis[0] - s * axis[2];
    r._02 = temp[2] * axis[0] + s * axis[1];

    r._10 = temp[0] * axis[1] + s * axis[2];
    r._11 = c + temp[1] * axis[1];
    r._12 = temp[2] * axis[1] - s * axis[0];

    r._20 = temp[0] * axis[2] - s * axis[1];
    r._21 = temp[1] * axis[2] + s * axis[0];
    r._22 = c + temp[2] * axis[2];

    r._33 = 1;

    return r;
}


auto m4x4_print(M4x4 m) -> void {
    println(" ╭                             ╮");
    println(" │ % 5.3f % 5.3f % 5.3f % 5.3f │", m._00, m._10, m._20, m._30);
    println(" │ % 5.3f % 5.3f % 5.3f % 5.3f │", m._01, m._11, m._21, m._31);
    println(" │ % 5.3f % 5.3f % 5.3f % 5.3f │", m._02, m._12, m._22, m._32);
    println(" │ % 5.3f % 5.3f % 5.3f % 5.3f │", m._03, m._13, m._23, m._33);
    println(" ╰                             ╯");
}

auto m3x3_print(M3x3 m) -> void {
    println(" ╭                      ╮");
    println(" │ % 5.3f % 5.3f % 5.3f │", m._00, m._10, m._20);
    println(" │ % 5.3f % 5.3f % 5.3f │", m._01, m._11, m._21);
    println(" │ % 5.3f % 5.3f % 5.3f │", m._02, m._12, m._22);
    println(" ╰                      ╯");
}

auto v3_print(V3 v) -> void {
    println(" ╭                      ╮T");
    println(" │ % 5.3f % 5.3f % 5.3f │", v.x, v.y, v.z);
    println(" ╰                      ╯");
}


auto m4x4_look_at(V3 cam_pos, V3 target, V3 up) -> M4x4
{
    V3 view_dir  =  noz(target - cam_pos);
    V3 right_dir =  noz(cross(view_dir, up));
    V3 up_dir    = -cross(right_dir, view_dir);

    /*
     *  ╭                ╮
     *  │ right_dir  p.x │
     *  │  up_dir    p.y │
     *  │  view_dir  p.z │
     *  │  0  0  0    1  │
     *  ╰                ╯
     */

    M4x4 result = m4x4_identity();
    // 1st row
    result._00 = right_dir.x;
    result._10 = right_dir.y;
    result._20 = right_dir.z;
    // 2nd row
    result._01 = up_dir.x;
    result._11 = up_dir.y;
    result._21 = up_dir.z;
    // 3rd row
    result._02 = view_dir.x;
    result._12 = view_dir.y;
    result._22 = view_dir.z;

    // 4th column
    result._30 = -dot(right_dir, cam_pos);
    result._31 = -dot(   up_dir, cam_pos);
    result._32 = -dot( view_dir, cam_pos);

    return result;
}


auto identity_look_at(V3* out_eye, V3* out_target, V3* out_up) -> void {
    *out_eye    = {0,  0,  0};
    *out_target = {0,  0,  1}; // vulkan z goes into the screen
    *out_up     = {0, -1,  0}; // vulkan y goes down in image space

    // {
    //     println("check ident:");
    //     m4x4_print(m4x4_look_at(*out_eye, *out_target, *out_up));
    // }
}

auto quick_invert_transformation_mat_without_scale(M4x4 mat) -> M4x4 {
    M3x3 rot_sheer;
    V3   trans;

    m4x4_decompose(mat, &rot_sheer, &trans);
    rot_sheer = m3x3_transpose(rot_sheer);
    trans     = (rot_sheer * -1.0) * trans;

    return m4x4_compose(rot_sheer, trans);
}

auto look_at_from_m4x4(M4x4 view_mat, V3* out_eye, V3* out_target, V3* out_up) -> void {
    V3 eye, target, up;
    identity_look_at(&eye, &target, &up);

    M4x4 inverse_view =  quick_invert_transformation_mat_without_scale(view_mat);

    V4 t_eye    = inverse_view * v4(eye,    1);
    V4 t_target = t_eye - V4{view_mat._02, view_mat._12, view_mat._22, view_mat._32};
    V4 t_up     =         V4{view_mat._01, view_mat._11, view_mat._21, view_mat._31};

    t_eye  = t_eye * (1.0f/t_eye.w);

    *out_eye    =    t_eye.xyz;
    *out_target = t_target.xyz;
    *out_up     = noz(t_up.xyz);

    // {
    //     println("before:");
    //     m4x4_print(view_mat);
    //     println("after:");
    //     m4x4_print(m4x4_look_at(*out_eye, *out_target, *out_up));
    //     println("");
    //     // exit(0);
    // }
}

auto m4x4_perspective(f32 fov_y, f32 aspect, f32 clip_near, f32 clip_far) -> M4x4 {
    f32 const tan_half_fov_y = tanf(fov_y / 2.0f);

    M4x4 result {};

    result._00 = 1.0f / (aspect * tan_half_fov_y);
    result._11 = 1.0f / (tan_half_fov_y);
    result._22 = clip_far / (clip_far - clip_near);
    result._23 = 1.0f;
    result._32 = -(clip_far * clip_near) / (clip_far - clip_near);

    return result;
}

auto m4x4_ortho(f32 left, f32 right, f32 bottom, f32 top) -> M4x4 {
    M4x4 result {};
    result._33 = 1;

    result._00 = 2.0f / (right - left);
    result._11 = 2.0f / (top - bottom);
    result._22 = -1.0f;
    result._30 = -(right + left) / (right - left);
    result._31 = -(top + bottom) / (top - bottom);

    return result;
}

auto m4x4_ortho(f32 x_extend, f32 aspect) -> M4x4 {
    f32 half = x_extend / 2.0f;
    return m4x4_ortho(-half, half,
                      -half/aspect, half/aspect);
}

auto m4x4_scale(V3 scale) -> M4x4 {
    M4x4 result = {};

    result._00 = scale.x;
    result._11 = scale.y;
    result._22 = scale.z;
    result._33 = 1;

    return result;
}

auto m4x4_translate(V3 tanslation) -> M4x4 {
    M4x4 result = {};

    result._00 = 1;
    result._11 = 1;
    result._22 = 1;

    result.columns[3].xyz = tanslation;
    result.columns[3].w = 1;

    return result;
}

auto m3x3_transpose(M3x3 mat) -> M3x3 {
    M3x3 result;

    for (int i = 0; i < 3; ++i) {
        for (int j = 0; j < 3; ++j) {
            result[i*3+j] = mat[j*3+i];
        }
    }

    return result;
}

auto m4x4_transpose(M4x4 m) -> M4x4 {
    M4x4 result;

    for (int i = 0; i < 4; ++i) {
        for (int j = 0; j < 4; ++j) {
            result[i*4+j] = m[j*4+i];
        }
    }

    return result;
}

auto m4x4_orientation(Quat q) -> M4x4 {
    M4x4 result = {};
    result._33 = 1;

    f32 qxx = q.x * q.x;
    f32 qyy = q.y * q.y;
    f32 qzz = q.z * q.z;
    f32 qxz = q.x * q.z;
    f32 qxy = q.x * q.y;
    f32 qyz = q.y * q.z;
    f32 qwx = q.w * q.x;
    f32 qwy = q.w * q.y;
    f32 qwz = q.w * q.z;

    result._00 = 1 - 2 * (qyy +  qzz);
    result._01 = 2 * (qxy + qwz);
    result._02 = 2 * (qxz - qwy);

    result._10 = 2 * (qxy - qwz);
    result._11 = 1 - 2 * (qxx +  qzz);
    result._12 = 2 * (qyz + qwx);

    result._20 = 2 * (qxz + qwy);
    result._21 = 2 * (qyz - qwx);
    result._22 = 1 - 2 * (qxx +  qyy);

    return result;
}

auto m4x4_model(V3 tanslation, Quat orientation, V3 scale) -> M4x4 {
    M4x4 t = m4x4_translate(tanslation);
    M4x4 o = m4x4_orientation(orientation);
    M4x4 s = m4x4_scale(scale);
    return t * o * s;
}

auto m4x4_decompose(M4x4 input, M3x3* out_rot_sheer, V3* out_translation) -> void {
    out_rot_sheer->columns[0] = input.columns[0].xyz;
    out_rot_sheer->columns[1] = input.columns[1].xyz;
    out_rot_sheer->columns[2] = input.columns[2].xyz;
    *out_translation          = input.columns[3].xyz;
}

auto m4x4_compose(M3x3 rot_sheer, V3 translation) -> M4x4 {
    M4x4 result {};
    result.columns[0].xyz = rot_sheer.columns[0];
    result.columns[1].xyz = rot_sheer.columns[1];
    result.columns[2].xyz = rot_sheer.columns[2];
    result.columns[3]     = v4(translation, 1);
    return result;
}

auto frobenius(M4x4 m) -> float {
    f32 sum = 0;
    for (s32 i = 0; i < array_length(m.elements); ++i) {
        sum += m.elements[i] * m.elements[i];
    }
    return sqrtf(sum);
}

auto frobenius(M3x3 m) -> float {
    f32 sum = 0;
    for (s32 i = 0; i < array_length(m.elements); ++i) {
        sum += m.elements[i] * m.elements[i];
    }
    return sqrtf(sum);
}

auto m3x3_identity() -> M3x3 {
    M3x3 mat {};

    mat._00 = 1;
    mat._11 = 1;
    mat._22 = 1;

    return mat;
}

auto m3x3_orientation(Quat q) -> M3x3 {
    M3x3 result;

    f32 qxx = q.x * q.x;
    f32 qyy = q.y * q.y;
    f32 qzz = q.z * q.z;
    f32 qxz = q.x * q.z;
    f32 qxy = q.x * q.y;
    f32 qyz = q.y * q.z;
    f32 qwx = q.w * q.x;
    f32 qwy = q.w * q.y;
    f32 qwz = q.w * q.z;

    result._00 = 1 - 2 * (qyy +  qzz);
    result._01 = 2 * (qxy + qwz);
    result._02 = 2 * (qxz - qwy);

    result._10 = 2 * (qxy - qwz);
    result._11 = 1 - 2 * (qxx +  qzz);
    result._12 = 2 * (qyz + qwx);

    result._20 = 2 * (qxz + qwy);
    result._21 = 2 * (qyz - qwx);
    result._22 = 1 - 2 * (qxx +  qyy);

    return result;
}

// ---------------------
//     quat functions
// ---------------------
auto quat_from_axis_angle(V3 axis, f32 angle) -> Quat {
    Quat out;
    out.x = sinf(angle / 2) * axis.x;
    out.y = sinf(angle / 2) * axis.y;
    out.z = sinf(angle / 2) * axis.z;
    out.w = cosf(angle / 2);
    return out;
}

auto quat_from_XYZ(f32 x, f32 y, f32 z) -> Quat {
    f32 sx = sinf(deg_to_rad(x) / 2);
    f32 sy = sinf(deg_to_rad(y) / 2);
    f32 sz = sinf(deg_to_rad(z) / 2);
    f32 cx = cosf(deg_to_rad(x) / 2);
    f32 cy = cosf(deg_to_rad(y) / 2);
    f32 cz = cosf(deg_to_rad(z) / 2);

    Quat result;
    result.x = sx*cy*cz - cx*sy*sz;
    result.y = cx*sy*cz + sx*cy*sz;
    result.z = cx*cy*sz - sx*sy*cz;
    result.w = cx*cy*cz + sx*sy*sz;

    return result;
}

auto quat_from_m4x4(M4x4 m) -> Quat {
    // source: https://math.stackexchange.com/a/3183435
    Quat q;
    f32 t;
    if (m._22 < 0) {
        if (m._00 > m._11) {
            t = 1 + m._00 - m._11 - m._22;
            q = Quat{ t, m._01+m._10, m._20+m._02, m._12-m._21 };
        } else {
            t = 1 - m._00 + m._11 - m._22;
            q = Quat{ m._01+m._10, t, m._12+m._21, m._20-m._02 };
        }
    } else {
        if (m._00 < -m._11) {
            t = 1 - m._00 - m._11 + m._22;
            q = Quat{ m._20+m._02, m._12+m._21, t, m._01-m._10 };
        } else {
            t = 1 + m._00 + m._11 + m._22;
            q = Quat{ m._12-m._21, m._20-m._02, m._01-m._10, t };
        }
    }
    q = q *(0.5f / sqrtf(t));

    return q;
}


auto quat_to_XYZ_Euler(Quat q) -> V3 {
    // source: https://en.wikipedia.org/wiki/Conversion_between_quaternions_and_Euler_angles
    V3 angles;

    // roll (x-axis rotation)
    f32 sinr_cosp = 2 * (q.w * q.x + q.y * q.z);
    f32 cosr_cosp = 1 - 2 * (q.x * q.x + q.y * q.y);
    angles.x = atan2f(sinr_cosp, cosr_cosp);

    // pitch (y-axis rotation)
    f32 sinp = sqrtf(1 + 2 * (q.w * q.y - q.x * q.z));
    f32 cosp = sqrtf(1 - 2 * (q.w * q.y - q.x * q.z));
    angles.y = 2 * atan2f(sinp, cosp) - pi / 2;

    // yaw (z-axis rotation)
    f32 siny_cosp = 2 * (q.w * q.z + q.x * q.y);
    f32 cosy_cosp = 1 - 2 * (q.y * q.y + q.z * q.z);
    angles.z = atan2f(siny_cosp, cosy_cosp);

    return angles;
}

auto closest_t_on_ray(Ray3 ray, V3 point) -> f32 {
    V3 v = point - ray.offset;
    f32 dotProd = dot(v, ray.direction);
    return dotProd;
}

auto closest_point_on_ray(Ray3 ray, V3 point) -> V3 {
    f32 t = closest_t_on_ray(ray, point);
    return ray.offset + ray.direction * t;
}

std::random_device rd;
std::mt19937 e2(rd());
std::uniform_real_distribution<> dist_0_1(0.0f, 1.0f);

auto rand_0_1() -> f32 {
    // return ((f32)rand())/RAND_MAX;

    return (f32)dist_0_1(e2);
}

auto rand_int(s32 lower_incl, s32 upper_incl) -> int {
    s32 dist = upper_incl-lower_incl;
    return lower_incl + round_to_int(rand_0_1() * dist);
}

#endif // FTB_MATH_IMPL
