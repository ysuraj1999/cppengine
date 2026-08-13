#pragma once
#include <cstdint>

typedef int8_t i8;
typedef int16_t i16;
typedef int32_t i32;
typedef int64_t i64;
typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float f32;
typedef double f64;
typedef std::size_t usize;

#define static_global   static
#define static_local    static
#define static_internal static

struct Vec2 {
    f32 x = 0.0f;
    f32 y = 0.0f;
};

struct Vec3 {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
};

struct Vec4 {
    f32 x = 0.0f;
    f32 y = 0.0f;
    f32 z = 0.0f;
    f32 w = 0.0f;
};

struct Vertex {
    Vec3 position;
    Vec4 color;
    Vec2 uv;
};

typedef struct Camera {
    Vec3 position;
    f32 yaw = 0.0f;
    f32 pitch = 0.0f;
} Camera;

struct Mat4 {
    f32 m0 = 0.0f;
    f32 m1 = 0.0f;
    f32 m2 = 0.0f;
    f32 m3 = 0.0f;
    f32 m4 = 0.0f;
    f32 m5 = 0.0f;
    f32 m6 = 0.0f;
    f32 m7 = 0.0f;
    f32 m8 = 0.0f;
    f32 m9 = 0.0f;
    f32 m10 = 0.0f;
    f32 m11 = 0.0f;
    f32 m12 = 0.0f;
    f32 m13 = 0.0f;
    f32 m14 = 0.0f;
    f32 m15 = 0.0f;
};

Mat4 translateMatrix(const f32 v[4]);
Mat4 translateMatrix(const Vec3& v);
