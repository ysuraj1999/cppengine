#pragma once
#include <SDL3/SDL_gpu.h>
#include <cstdint>
#include <cstdlib>
#include <algorithm>
#include <glm/glm.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include "cppengine.h"
#include <glm/ext/matrix_transform.hpp>
#include <glm/trigonometric.hpp>
#include <glm/geometric.hpp>

#define ASSERT(cond, msg)                                          \
  do {                                                             \
    if (!(cond)) {                                                 \
      std::fprintf(stderr, "%s:%d: ASSERT FAILED: %s - %s\n",      \
                   __FILE__, __LINE__, #cond, msg);                \
      std::abort();                                                \
    }                                                              \
  } while (0)

struct HandleCounter {
  u64 next = 0;
  u64 operator()() { return next++; }
};

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

extern glm::vec3 WORLD_UP;

struct Camera {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  f32 yaw = 0.0f;
  f32 pitch = 0.0f;
  glm::vec3 forward{0.0f, 0.0f, 1.0f};
  glm::vec3 right{1.0f, 0.0f, 0.0f};

  void update() {
    forward = glm::normalize(glm::vec3{
        cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
        sin(glm::radians(pitch)),
        cos(glm::radians(pitch)) * sin(glm::radians(yaw))});
    right = glm::cross(WORLD_UP, forward);
  }

  void move(f32 fwd, f32 rght, f32 up) {
    position += forward * fwd;
    position += right * rght;
    position += WORLD_UP * up;
  }

  void rotate(f32 dYaw, f32 dPitch) {
    yaw = glm::mod(yaw + dYaw, 360.0f);
    pitch = glm::clamp(pitch + dPitch, -89.99f, 89.99f);
  }

  glm::mat4 getViewMatrix() const {
    return glm::lookAtLH(position, position + forward, WORLD_UP);
  }
};

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
  std::string alias;
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
  std::string alias;
};

using Instances = std::vector<Instance>;

struct Scene {
  HandleCounter meshCounter;
  HandleCounter instanceCounter;

  std::unordered_map<MeshHandle, Mesh> meshes;
  std::vector<MeshHandle> meshOrder;
  std::unordered_map<MeshHandle, Instances> objects;

  MeshHandle add_mesh(Mesh m) {
    MeshHandle h = meshCounter();
    m.alias = (m.alias.empty() ? "Mesh" : m.alias) + "#" + std::to_string(h);
    meshes[h] = m;
    meshOrder.push_back(h);
    return h;
  }
  bool remove_mesh(MeshHandle mh) {
    if (!meshes.contains(mh)) return false;
    objects.erase(mh);
    meshes.erase(mh);
    meshOrder.erase(std::find(meshOrder.begin(), meshOrder.end(), mh));
    return true;
  }
  Mesh& get_mesh(MeshHandle mh) {
    auto it = meshes.find(mh);
    ASSERT(it != meshes.end(), "invalid mesh handle");
    return it->second;
  }
  u64 mesh_count() const { return meshOrder.size(); }
  MeshHandle mesh_at(u64 index) const {
    ASSERT(index < meshOrder.size(), "mesh index out of bounds");
    return meshOrder[index];
  }

  InstanceHandle add_instance(MeshHandle m, Transform t) {
    Mesh& mesh = get_mesh(m);
    Instance inst = {};
    inst.instanceHandle = instanceCounter();
    inst.mesh = m;
    inst.transform = t;
    inst.alias = mesh.alias + "-instance#" + std::to_string(inst.instanceHandle);
    objects[m].push_back(inst);
    return inst.instanceHandle;
  }
  bool remove_instance(InstanceHandle ih) {
    for (auto &[mh, instances] : objects) {
      (void)mh;
      for (u64 i = 0; i < instances.size(); i++)
        if (instances[i].instanceHandle == ih) {
          instances.erase(instances.begin() + i);
          return true;
        }
    }
    return false;
  }
  Instance& get_instance(InstanceHandle ih) {
    for (auto &[mh, instances] : objects) {
      (void)mh;
      for (auto &inst : instances)
        if (inst.instanceHandle == ih) return inst;
    }
    ASSERT(false, "instance handle not found");
    return *((Instance*)nullptr);
  }
  u64 instance_count(MeshHandle mh) const {
    auto it = objects.find(mh);
    return it == objects.end() ? 0 : it->second.size();
  }
  InstanceHandle instance_at(MeshHandle mh, u64 index) const {
    auto it = objects.find(mh);
    ASSERT(it != objects.end(), "mesh has no instances");
    ASSERT(index < it->second.size(), "instance index out of bounds");
    return it->second[index].instanceHandle;
  }

  void set_instance_transform(InstanceHandle ih, Transform t) {
    get_instance(ih).transform = t;
  }
  Transform get_instance_transform(InstanceHandle ih) const {
    for (auto &[mh, instances] : objects) {
      (void)mh;
      for (auto &inst : instances)
        if (inst.instanceHandle == ih) return inst.transform;
    }
    ASSERT(false, "instance handle not found");
    return Transform{};
  }
  const char* mesh_alias(MeshHandle mh) const {
    auto it = meshes.find(mh);
    ASSERT(it != meshes.end(), "invalid mesh handle");
    return it->second.alias.c_str();
  }
  const char* instance_alias(InstanceHandle ih) const {
    for (auto &[mh, instances] : objects) {
      (void)mh;
      for (auto &inst : instances)
        if (inst.instanceHandle == ih) return inst.alias.c_str();
    }
    ASSERT(false, "instance handle not found");
    return nullptr;
  }
};

struct SceneRegistry {
  std::unordered_map<SceneHandle, Scene> scenes;
  std::vector<SceneHandle> order;
  HandleCounter sceneCounter;

  SceneHandle create() {
    SceneHandle sh = sceneCounter();
    scenes[sh] = Scene{};
    order.push_back(sh);
    return sh;
  }
  bool destroy(SceneHandle sh) {
    bool erased = scenes.erase(sh) > 0;
    if (erased)
      order.erase(std::find(order.begin(), order.end(), sh));
    return erased;
  }
  Scene& get(SceneHandle sh) {
    auto it = scenes.find(sh);
    ASSERT(it != scenes.end(), "invalid scene handle");
    return it->second;
  }
  u64 count() const { return order.size(); }
  SceneHandle at(u64 index) const {
    ASSERT(index < order.size(), "scene index out of bounds");
    return order[index];
  }
};

struct RendererState {
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::unordered_map<MeshHandle, MeshRegion> meshRegions;
};

struct RendererConfig {
  glm::vec2 resolution;
  glm::vec4 backgroundColor;
  f32 keyboardSensitivity;
  f32 mouseSensitivity;
  Camera camera;
};


