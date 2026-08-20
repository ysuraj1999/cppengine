#pragma once

#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <cfloat>
#include <cstddef>
#include <glm/glm.hpp>
#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>

#include "stb_image.h"
#include "types.hpp"
#include "obj.hpp"
#include "utils.hpp"

// CONSTANTS
extern glm::vec3 WORLD_UP;
extern RendererConfig rendererConfig;

bool renderer_init();
bool renderer_scene_load_obj(Scene *scene, const char *path);
MeshHandle renderer_scene_add_mesh(Scene *scene, Mesh m);
bool renderer_scene_remove_mesh(Scene *scene, MeshHandle mh);
InstanceHandle renderer_scene_add_instance(Scene *scene, MeshHandle m, Transform t);
bool renderer_scene_remove_instance(Scene *scene, InstanceHandle i);
bool renderer_sync_scene(const Scene *scene);
void renderer_render(void);
bool renderer_clear(void);
void renderer_quit();

// Assuming only 1 scene exists for now
// get all mesh handles in scene
u64 renderer_get_scene_count();
u64* renderer_get_all_scenes();

u64 renderer_get_mesh_count(SceneHandle sh);
u64* renderer_get_all_meshes(SceneHandle sh);

u64 renderer_get_instance_count(SceneHandle sh);
u64* renderer_get_all_instances(SceneHandle sh);

