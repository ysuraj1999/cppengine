#pragma once
#include <cstdint>
#include <glm/glm.hpp>
#include <unordered_map>
#include <vector>
#include "cppengine.h"

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
  glm::vec3 forward; // cached forward & right
  glm::vec3 right;
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

using MeshHandle = u64;
using InstanceHandle = u64;
using SceneHandle = u64;

struct Instance {
  InstanceHandle instanceHandle;
  MeshHandle mesh; // redundant, for direct mapping
  Transform transform;
};

using Instances = std::vector<Instance>;

struct Scene {
  std::unordered_map<MeshHandle, Mesh> meshes;
  std::vector<MeshHandle> meshOrder;
  std::unordered_map<MeshHandle, Instances> objects;
};

struct RendererState {
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::unordered_map<MeshHandle, MeshRegion> meshRegions;
  std::unordered_map<MeshHandle, Instances> objects;
};

struct RendererConfig {
  glm::vec2 resolution;
  glm::vec4 backgroundColor;
  f32 keyboardSensitivity;
  f32 mouseSensitivity;
  bool cameraEnabled;
  Camera camera;
};

struct MouseState {
  f32 x = 0.0f, y = 0.0f;              // absolute position (persists)
  f32 dx = 0.0f, dy = 0.0f;            // movement this frame (reset each poll)
  f32 wheel_x = 0.0f, wheel_y = 0.0f;  // scroll this frame (reset each poll)
  bool down[MOUSE_BUTTON_COUNT] = {};      // held state (persists)
  bool pressed[MOUSE_BUTTON_COUNT] = {};   // just pressed this poll
  bool released[MOUSE_BUTTON_COUNT] = {};  // just released this poll
};
