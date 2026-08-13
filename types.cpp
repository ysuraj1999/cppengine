#include "types.hpp"
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

Mat4 translateMatrix(const float v[4]) {
    Mat4 result;
    result.m0 = 1.0f;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = -v[0];
    result.m4 = 0.0f;
    result.m5 = 1.0f;
    result.m6 = 0.0f;
    result.m7 = -v[1];
    result.m8 = 0.0f;
    result.m9 = 0.0f;
    result.m10 = 1.0f;
    result.m11 = -v[2];
    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;
    return result;
}

Mat4 translateMatrix(const Vec3& v) {
    Mat4 result;
    result.m0 = 1.0f;
    result.m1 = 0.0f;
    result.m2 = 0.0f;
    result.m3 = -v.x;
    result.m4 = 0.0f;
    result.m5 = 1.0f;
    result.m6 = 0.0f;
    result.m7 = -v.y;
    result.m8 = 0.0f;
    result.m9 = 0.0f;
    result.m10 = 1.0f;
    result.m11 = -v.z;
    result.m12 = 0.0f;
    result.m13 = 0.0f;
    result.m14 = 0.0f;
    result.m15 = 1.0f;
    return result;
}
