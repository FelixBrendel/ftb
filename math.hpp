#pragma once
#include <math.h>
#include "types.hpp"
#include "macros.hpp"

// TODO(Felix): Operations missing:
//   - unlerp
//   - remap

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


#define vector_length(v) array_length(v)

template<typename VecT>
inline auto operator+(VecT a, VecT b) -> VecT {
    VecT result;

    for (u32 i = 0; i < vector_length(a); ++i) {
        result[i] = a[i] + b[i];
    }

    return result;
}

template<typename VecT>
inline auto operator-(VecT a) -> VecT {
    VecT result;

    for (u32 i = 0; i < vector_length(a); ++i) {
        result[i] = -a[i];
    }

    return result;
}

template<typename VecT>
inline auto operator-(VecT a, VecT b) -> VecT {
    VecT result;

    for (u32 i = 0; i < vector_length(a); ++i) {
        result[i] = a[i] - b[i];
    }

    return result;
}


template<typename VecT>
inline auto operator*(VecT v, f32 s) -> VecT {
    VecT result;

    for (u32 i = 0; i < vector_length(v); ++i) {
        result[i] = v[i] * s;
    }

    return result;
}

template<typename VecT>
inline auto operator*(f32 s, VecT v) -> VecT {
    return v * s;
}

template<typename VecT, typename MatT>
inline auto operator*(MatT m, VecT v) -> VecT {
    static_assert(sizeof(m.rows[0]) == sizeof(v));

    VecT result;

    for (u32 i = 0; i < vector_length(v); ++i) {
        result[i] = dot(m.rows[i], v);
    }

    return result;
}

template<typename VecT>
inline auto dot(VecT a, VecT b) -> f32 {
    f32 result = 0;

    for (u32 i = 0; i < vector_length(a); ++i) {
        result += a[i] * b[i];
    }

    return result;
}

template<typename VecT>
inline auto hadamard(VecT a, VecT b) -> VecT {
    VecT result;

    for (u32 i = 0; i < vector_length(a); ++i) {
        result[i] = a[i] * b[i];
    }

    return result;
}

template<typename VecT>
inline auto reflect(VecT vector, VecT normal) -> VecT {
    return 2 * dot(-vector, normal) * normal + vector;
}

template<typename VecT>
inline auto length(VecT vector) -> f32 {
    f32 length = 0;

    for (u32 i = 0; i < vector_length(vector); ++i) {
        length += vector[i] * vector[i];
    }

    return sqrt(length);
}

template<typename VecT>
inline auto noz(VecT vector) -> VecT {
    f32 len = length(vector);

    if (len == 0)
        return vector;

    return 1.0f / len * vector;
}

template<typename VecT>
inline auto lerp(VecT from, f32 t, VecT to) -> VecT {
    return from + (to - from) * t;
}


#ifndef FTB_MATH_IMPL

// f32 functions
auto round_to_precision(f32 num, u32 decimals) ->  f32;
auto lerp(f32 from, f32 t, f32 to) -> f32;
auto clamp(f32 from, f32 x, f32 to) -> f32;
auto clamp01(f32 x) -> f32;
auto clamped_lerp(f32 from, f32 t, f32 to) -> f32;

/* matrix functions */
// NOTE(Felix): The operator + and - and * <scalar> are already covered by the
//   vector templates
auto operator*(M2x2 a, M2x2 b) -> M2x2;
auto operator*(M3x3 a, M3x3 b) -> M3x3;
auto operator*(M4x4 a, M4x4 b) -> M4x4;

#else // implementations

// ---------------------
//     f32 functions
// ---------------------
inline auto round_to_precision(f32 num, u32 decimals) ->  f32 {
    f32 factor = powf(10.0f, (f32)decimals);
    return roundf(num * factor) / factor;
}

inline auto lerp(f32 from, f32 t, f32 to) -> f32 {
    return from + (to - from) * t;
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
//    matrix functions
// ---------------------

auto operator*(M2x2 a, M2x2 b) -> M2x2 {
    M2x2 result {
        dot(a.rows[0], V2{b._00, b._10}), dot(a.rows[0], V2{b._01, b._11}),
        dot(a.rows[1], V2{b._00, b._10}), dot(a.rows[1], V2{b._01, b._11}),
    };

    return result;
}


auto operator*(M3x3 a, M3x3 b) -> M3x3 {
    M3x3 result {
        dot(a.rows[0], V3{b._00, b._10, b._20}), dot(a.rows[0], V3{b._01, b._11, b._21}), dot(a.rows[0], V3{b._02, b._12, b._22}),
        dot(a.rows[1], V3{b._00, b._10, b._20}), dot(a.rows[1], V3{b._01, b._11, b._21}), dot(a.rows[1], V3{b._02, b._12, b._22}),
        dot(a.rows[2], V3{b._00, b._10, b._20}), dot(a.rows[2], V3{b._01, b._11, b._21}), dot(a.rows[2], V3{b._02, b._12, b._22}),
    };

    return result;
}

auto operator*(M4x4 a, M4x4 b) -> M4x4 {
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

#endif // FTB_MATH_IMPL
