#include "types.hpp"

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
