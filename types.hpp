#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <map>
#include <vector>

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

struct Vertex {
  glm::vec3 position;
  glm::vec4 color;
  glm::vec3 normal;
  glm::vec2 uv;
};

typedef struct Camera {
  glm::vec3 position;
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

struct Mesh {
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
};

struct Transform {
  glm::vec3 position;
  glm::vec3 scale;
  glm::vec3 rotation; // roll, yaw, pitch
};

struct MeshRegion {
    u32 vertexOffset;
    u32 indexOffset;
    u32 indexCount;
};

using MeshHandle = u32;
using InstanceHandle = u32;

struct Instance {
  MeshHandle mesh;
  Transform transform;
};

using Instances = std::vector<Transform>;

struct Scene {
  std::vector<Mesh> meshes;
  std::map<MeshHandle, Instances> objects;
};

struct RendererState {
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<MeshRegion> meshRegions;
};

struct RendererConfig {
  glm::vec2 resolution;
  glm::vec4 backgroundColor;
  f32 keyboardSensitivity;
  f32 mouseSensitivity;
  bool cameraEnabled;
  Camera camera;
};

Mat4 translateMatrix(const f32 v[4]);
Mat4 translateMatrix(const glm::vec3& v);
