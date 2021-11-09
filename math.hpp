#pragma once
#include <math.h>
#include <float.h>
#include "types.hpp"
#include "macros.hpp"

#define FTB_USING_MATH

extern f32 pi;
extern f32 two_pi;

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

union V4 {
    struct {
        f32 x, y, z, w;
    };
    struct {
        f32 r, g, b, a;
    };
    struct {
        V2 xy;
        V2 zw;
    };
    struct {
        f32 _padding_1;
        V2 yz;
        f32 _padding_2;
    };
    struct {
        V3 xyz;
        f32 _padding_3;
    };
    struct {
        f32 _padding_4;
        V3 yzw;
    };
    f32 elements[4];

    inline f32 &operator[](const int &index) {
        return elements[index];
    }
};

typedef V4 Quat;

union M2x2 {
    struct {
        f32 _00; f32 _01;
        f32 _10; f32 _11;
    };
    V2 rows[2];
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
    V3 rows[3];
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
    V4 rows[4];
    f32 elements[16];

    inline f32 &operator[](const int &index) {
        return elements[index];
    }
};


#ifndef FTB_MATH_IMPL

// ---------------------
//     f32 functions
// ---------------------
auto rad_to_dec(f32 rad) -> f32;
auto dec_to_rad(f32 dec) -> f32;

auto round_to_precision(f32 num, u32 decimals) ->  f32;

auto lerp(f32 from, f32 t, f32 to) -> f32;
auto unlerp(f32 from, f32 val, f32 to) -> f32;
auto remap(f32 from_a, f32 val, f32 to_a, f32 from_b, f32 to_b) -> f32;

auto clamp(f32 from, f32 x, f32 to) -> f32;
auto clamp01(f32 x) -> f32;
auto clamped_lerp(f32 from, f32 t, f32 to) -> f32;

// ---------------------
//   vector functions
// ---------------------
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

auto lerp(V2 from, f32 t, V2 to) -> V2;
auto lerp(V3 from, f32 t, V3 to) -> V3;
auto lerp(V4 from, f32 t, V4 to) -> V4;

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
auto operator*(M2x2 a, M2x2 b) -> M2x2;
auto operator*(M3x3 a, M3x3 b) -> M3x3;
auto operator*(M4x4 a, M4x4 b) -> M4x4;

auto operator+(M2x2 a, M2x2 b) -> M2x2;
auto operator+(M3x3 a, M3x3 b) -> M3x3;
auto operator+(M4x4 a, M4x4 b) -> M4x4;

auto operator*(M2x2 a, f32 s)  -> M2x2;
auto operator*(M3x3 a, f32 s)  -> M3x3;
auto operator*(M4x4 a, f32 s)  -> M4x4;

auto operator*(f32 s, M2x2 a)  -> M2x2;
auto operator*(f32 s, M3x3 a)  -> M3x3;
auto operator*(f32 s, M4x4 a)  -> M4x4;

// ---------------------
//     quat functions
// ---------------------
auto quat_from_axis_angle(V3 axis, f32 angle) -> Quat;
auto quat_from_XYZ(f32 x, f32 y, f32 z) -> Quat;
auto quat_to_m3x3(Quat q) -> M3x3;

#else // implementations

f32 pi     = 3.1415926535897932384626433832795;
f32 two_pi = 6.283185307179586476925286766559;

// ---------------------
//     f32 functions
// ---------------------
inline auto rad_to_deg(f32 rad) -> f32 {
    return rad / pi * 180;
}

inline auto deg_to_rad(f32 deg) -> f32 {
    return deg / 180 * pi;
}

inline auto round_to_precision(f32 num, u32 decimals) ->  f32 {
    f32 factor = powf(10.0f, (f32)decimals);
    return roundf(num * factor) / factor;
}

inline auto lerp(f32 from, f32 t, f32 to) -> f32 {
    return from + (to - from) * t;
}

inline auto unlerp(f32 from, f32 val, f32 to) -> f32 {
    return (val - from) / (to - from);
}

inline auto remap(f32 from_a, f32 val, f32 to_a, f32 from_b, f32 to_b) -> f32 {
    return lerp(from_b, unlerp(from_a, val, from_b), to_b);
}

inline auto clamp(f32 from, f32 x, f32 to) -> f32 {
    f32 t = x < from ? from : x;
    return t > to ? to : t;
}

inline auto clamp01(f32 x) -> f32 {
    return clamp(0, x, 1);
}

inline auto clamped_lerp(f32 from, f32 t, f32 to) -> f32 {
    t = clamp01(t);
    return from + (to - from) * t;
}

// ---------------------
//   vector functions
// ---------------------
inline auto dot(V2 a, V2 b) -> f32 {
    return a.x * b.x + a.y * b.y;
}

inline auto dot(V3 a, V3 b) -> f32 {
    return a.x * b.x + a.y * b.y + a.z * b.z;
}

inline auto dot(V4 a, V4 b) -> f32 {
    return a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;
}

inline auto cross(V3 a, V3 b) -> V3 {
    return {
        a.y*b.z - a.z*b.y,
        a.z*b.x - a.x*b.z,
        a.x*b.y - a.y*b.x};
}

inline auto hadamard(V2 a, V2 b) -> V2 {
    return {
        a.x * b.x,
        a.y * b.y,
    };
}

inline auto hadamard(V3 a, V3 b) -> V3 {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
    };
}

inline auto hadamard(V4 a, V4 b) -> V4 {
    return {
        a.x * b.x,
        a.y * b.y,
        a.z * b.z,
        a.w * b.w,
    };
}

inline auto operator+(V2 a, V2 b) -> V2 {
    return {
        a.x + b.x,
        a.y + b.y
    };
}

inline auto operator+(V3 a, V3 b) -> V3 {
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z
    };
}

inline auto operator+(V4 a, V4 b) -> V4 {
    return {
        a.x + b.x,
        a.y + b.y,
        a.z + b.z,
        a.w + b.w
    };
}

inline auto operator-(V2 a) -> V2 {
    return {
        -a.x,
        -a.y
    };
}

inline auto operator-(V3 a) -> V3 {
    return {
        -a.x,
        -a.y,
        -a.z
    };
}

inline auto operator-(V4 a) -> V4 {
    return {
        -a.x,
        -a.y,
        -a.z,
        -a.w
    };
}

inline auto operator-(V2 a, V2 b) -> V2 {
    return {
        a.x - b.x,
        a.y - b.y,
    };
}

inline auto operator-(V3 a, V3 b) -> V3 {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
    };
}

inline auto operator-(V4 a, V4 b) -> V4 {
    return {
        a.x - b.x,
        a.y - b.y,
        a.z - b.z,
        a.w - b.w,
    };
}


inline auto operator*(V2 v, f32 s) -> V2 {
    return {
        v.x * s,
        v.y * s,
    };
}

inline auto operator*(V3 v, f32 s) -> V3 {
    return {
        v.x * s,
        v.y * s,
        v.z * s,
    };
}

inline auto operator*(V4 v, f32 s) -> V4 {
    return {
        v.x * s,
        v.y * s,
        v.z * s,
        v.w * s,
    };
}

inline auto operator*(f32 s, V2 v) -> V2 {
    return v * s;
}

inline auto operator*(f32 s, V3 v) -> V3 {
    return v * s;
}

inline auto operator*(f32 s, V4 v) -> V4 {
    return v * s;
}

inline auto operator*(M2x2 m, V2 v) -> V2 {
    return {
        dot(m.rows[0], v),
        dot(m.rows[1], v),
    };
}

inline auto operator*(M3x3 m, V3 v) -> V3 {
    return {
        dot(m.rows[0], v),
        dot(m.rows[1], v),
        dot(m.rows[2], v),
    };
}

inline auto operator*(M4x4 m, V4 v) -> V4 {
    return {
        dot(m.rows[0], v),
        dot(m.rows[1], v),
        dot(m.rows[2], v),
        dot(m.rows[3], v),
    };
}

inline auto reflect(V2 vector, V2 normal) -> V2 {
    return 2 * dot(-vector, normal) * normal + vector;
}

inline auto reflect(V3 vector, V3 normal) -> V3 {
    return 2 * dot(-vector, normal) * normal + vector;
}

inline auto reflect(V4 vector, V4 normal) -> V4 {
    return 2 * dot(-vector, normal) * normal + vector;
}

inline auto length(V2 vector) -> f32 {
    f32 length =
        vector.x * vector.x +
        vector.y * vector.y;
    return sqrt(length);
}

inline auto length(V3 vector) -> f32 {
    f32 length =
        vector.x * vector.x +
        vector.y * vector.y +
        vector.z * vector.z;
    return sqrt(length);
}

inline auto length(V4 vector) -> f32 {
    f32 length =
        vector.x * vector.x +
        vector.y * vector.y +
        vector.z * vector.z +
        vector.w * vector.w;
    return sqrt(length);
}

inline auto noz(V2 vector) -> V2 {
    f32 len = length(vector);
    if (len == 0)
        return vector;
    return 1.0f / len * vector;
}

inline auto noz(V3 vector) -> V3 {
    f32 len = length(vector);
    if (len == 0)
        return vector;
    return 1.0f / len * vector;
}

inline auto noz(V4 vector) -> V4 {
    f32 len = length(vector);
    if (len == 0)
        return vector;
    return 1.0f / len * vector;
}

inline auto lerp(V2 from, f32 t, V2 to) -> V2 {
    return from + (to - from) * t;
}

inline auto lerp(V3 from, f32 t, V3 to) -> V3 {
    return from + (to - from) * t;
}

inline auto lerp(V4 from, f32 t, V4 to) -> V4 {
    return from + (to - from) * t;
}

// ---------------------
//    matrix functions
// ---------------------

inline auto operator*(M2x2 a, M2x2 b) -> M2x2 {
    M2x2 result {
        dot(a.rows[0], V2{b._00, b._10}), dot(a.rows[0], V2{b._01, b._11}),
        dot(a.rows[1], V2{b._00, b._10}), dot(a.rows[1], V2{b._01, b._11}),
    };

    return result;
}


inline auto operator*(M3x3 a, M3x3 b) -> M3x3 {
    M3x3 result {
        dot(a.rows[0], V3{b._00, b._10, b._20}), dot(a.rows[0], V3{b._01, b._11, b._21}), dot(a.rows[0], V3{b._02, b._12, b._22}),
        dot(a.rows[1], V3{b._00, b._10, b._20}), dot(a.rows[1], V3{b._01, b._11, b._21}), dot(a.rows[1], V3{b._02, b._12, b._22}),
        dot(a.rows[2], V3{b._00, b._10, b._20}), dot(a.rows[2], V3{b._01, b._11, b._21}), dot(a.rows[2], V3{b._02, b._12, b._22}),
    };

    return result;
}

inline auto operator*(M4x4 a, M4x4 b) -> M4x4 {
    M4x4 result {
        dot(a.rows[0], V4{b._00, b._10, b._20, b._30}), dot(a.rows[0], V4{b._01, b._11, b._21, b._31}),
        dot(a.rows[0], V4{b._02, b._12, b._22, b._32}), dot(a.rows[0], V4{b._03, b._13, b._23, b._33}),

        dot(a.rows[1], V4{b._00, b._10, b._20, b._30}), dot(a.rows[1], V4{b._01, b._11, b._21, b._31}),
        dot(a.rows[1], V4{b._02, b._12, b._22, b._32}), dot(a.rows[1], V4{b._03, b._13, b._23, b._33}),

        dot(a.rows[2], V4{b._00, b._10, b._20, b._30}), dot(a.rows[2], V4{b._01, b._11, b._21, b._31}),
        dot(a.rows[2], V4{b._02, b._12, b._22, b._32}), dot(a.rows[2], V4{b._03, b._13, b._23, b._33}),

        dot(a.rows[3], V4{b._00, b._10, b._20, b._30}), dot(a.rows[3], V4{b._01, b._11, b._21, b._31}),
        dot(a.rows[3], V4{b._02, b._12, b._22, b._32}), dot(a.rows[3], V4{b._03, b._13, b._23, b._33}),
    };

    return result;
}

inline auto operator+(M2x2 a, M2x2 b) -> M2x2 {
    M2x2 result;
    result.rows[0] = a.rows[0] + b.rows[0];
    result.rows[1] = a.rows[1] + b.rows[1];
    return result;
}

inline auto operator+(M3x3 a, M3x3 b) -> M3x3 {
    M3x3 result;
    result.rows[0] = a.rows[0] + b.rows[0];
    result.rows[1] = a.rows[1] + b.rows[1];
    result.rows[2] = a.rows[2] + b.rows[2];
    return result;
}

inline auto operator+(M4x4 a, M4x4 b) -> M4x4 {
    M4x4 result;
    result.rows[0] = a.rows[0] + b.rows[0];
    result.rows[1] = a.rows[1] + b.rows[1];
    result.rows[2] = a.rows[2] + b.rows[2];
    result.rows[3] = a.rows[3] + b.rows[3];
    return result;
}

auto operator*(M2x2 a, f32 s) -> M2x2 {
    M2x2 result;
    result.rows[0] = a.rows[0] * s;
    result.rows[1] = a.rows[1] * s;
    return result;
}

auto operator*(M3x3 a, f32 s) -> M3x3 {
    M3x3 result;
    result.rows[0] = a.rows[0] * s;
    result.rows[1] = a.rows[1] * s;
    result.rows[2] = a.rows[2] * s;
    return result;
}

auto operator*(M4x4 a, f32 s)  -> M4x4 {
    M4x4 result;
    result.rows[0] = a.rows[0] * s;
    result.rows[1] = a.rows[1] * s;
    result.rows[2] = a.rows[2] * s;
    result.rows[3] = a.rows[3] * s;
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
    f32 sx = sin(deg_to_rad(x) / 2);
    f32 sy = sin(deg_to_rad(y) / 2);
    f32 sz = sin(deg_to_rad(z) / 2);
    f32 cx = cos(deg_to_rad(x) / 2);
    f32 cy = cos(deg_to_rad(y) / 2);
    f32 cz = cos(deg_to_rad(z) / 2);

    return Quat {
        cx*cy*cz + sx*sy*sz,
        sx*cy*cz - cx*sy*sz,
        cx*sy*cz + sx*cy*sz,
        cx*cy*sz - sx*sy*cz
    };
}

auto quat_to_m3x3(Quat q) -> M3x3 {
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

#endif // FTB_MATH_IMPL
