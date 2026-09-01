#pragma once
#include "cppengine.h"
#include <SDL3/SDL_gpu.h>
#include <algorithm>
#include <cstdint>
#include <cstdlib>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#define ASSERT(cond, msg)                                                                                              \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::fprintf(stderr, "%s:%d: ASSERT FAILED: %s - %s\n", __FILE__, __LINE__, #cond, msg);                   \
            std::abort();                                                                                              \
        }                                                                                                              \
    } while (0)

struct HandleCounter {
    u64 next = 0;
    u64 operator()() {
        return next++;
    }
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
    glm::vec3 forward{1.0f, 0.0f, 0.0f};
    glm::vec3 right{0.0f, 0.0f, 1.0f};

    void update() {
        forward = glm::normalize(
            glm::vec3{
                cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
                sin(glm::radians(pitch)),
                cos(glm::radians(pitch)) * sin(glm::radians(yaw))
            }
        );
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

    // Aim the camera at TARGET from the current position, keeping the
    // same distance.  No-op when the camera already sits on TARGET.
    void look_at(glm::vec3 target) {
        glm::vec3 dir = target - position;
        if (glm::dot(dir, dir) < 1e-12f)
            return;
        dir = glm::normalize(dir);
        yaw = glm::mod(glm::degrees(glm::atan(dir.z, dir.x)), 360.0f);
        pitch = glm::clamp(glm::degrees(glm::asin(glm::clamp(dir.y, -1.0f, 1.0f))), -89.99f, 89.99f);
        update();
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

std::vector<Mesh> loadAllMeshFromObj(const char *inputfile);

struct GeometryStorage {
    std::vector<Vertex> vertices;
    std::vector<u32> indices;
    std::vector<MeshHandle> meshOrder;
    std::unordered_map<MeshHandle, MeshRegion> meshRegions;
    HandleCounter meshCounter;

    MeshHandle add_mesh(Mesh &&m) {
        MeshHandle h = meshCounter();
        MeshRegion r{};
        r.vertexOffset = (u32)vertices.size();
        r.indexOffset = (u32)indices.size();
        r.indexCount = (u32)m.indices.size();
        vertices.insert(vertices.end(), m.vertices.begin(), m.vertices.end());
        indices.insert(indices.end(), m.indices.begin(), m.indices.end());
        meshRegions[h] = r;
        meshOrder.push_back(h);
        return h;
    }
    u64 mesh_count() const {
        return meshOrder.size();
    }
    MeshHandle mesh_at(u64 index) const {
        ASSERT(index < meshOrder.size(), "mesh index out of bounds");
        return meshOrder.at(index);
    }
};

using MeshHandle = u64;
using InstanceHandle = u64;

struct Instance {
    MeshHandle mesh;
    Transform transform;
};

using Instances = std::vector<Instance>;

struct Scene {
    std::vector<Instance> instances;

    void ensure_instance_count(u64 n) {
        instances.resize(n);
    }
    void set_instance(u64 index, MeshHandle meshHandle, Transform t) {
        ASSERT(index < instances.size(), "instance index out of bounds");
        instances[index].mesh = meshHandle;
        instances[index].transform = t;
    }
};

struct RendererConfig {
    glm::vec2 resolution;
    glm::vec4 backgroundColor;
    f32 keyboardSensitivity;
    f32 mouseSensitivity;
    Camera camera;
};
