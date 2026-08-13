#pragma once

struct Vec2 {
    float x = 0.0f;
    float y = 0.0f;
};

struct Vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Vec4 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
    float w = 0.0f;
};

struct Vertex {
    Vec3 position;
    Vec4 color;
    Vec2 uv;
};

struct Camera {
    Vec3 position;
    float yaw = 0.0f;
    float pitch = 0.0f;
};

struct Mat4 {
    float m0 = 0.0f;
    float m1 = 0.0f;
    float m2 = 0.0f;
    float m3 = 0.0f;
    float m4 = 0.0f;
    float m5 = 0.0f;
    float m6 = 0.0f;
    float m7 = 0.0f;
    float m8 = 0.0f;
    float m9 = 0.0f;
    float m10 = 0.0f;
    float m11 = 0.0f;
    float m12 = 0.0f;
    float m13 = 0.0f;
    float m14 = 0.0f;
    float m15 = 0.0f;
};

Mat4 translateMatrix(const float v[4]);
Mat4 translateMatrix(const Vec3& v);
