#pragma once
#include <stdint.h>
#include <stdbool.h>

typedef uint64_t u64;
typedef float f32;
typedef u64 SceneHandle;
typedef u64 MeshHandle;
typedef u64 InstanceHandle;

#ifdef __cplusplus
extern "C" {
#endif

bool renderer_init();

SceneHandle renderer_scene_create();
bool renderer_scene_destroy(SceneHandle sh);

bool renderer_scene_load_obj(SceneHandle sh, const char *path);

bool renderer_scene_remove_mesh(SceneHandle sh, MeshHandle mh);

InstanceHandle renderer_scene_add_instance(SceneHandle sh, MeshHandle m,
                                           f32 px, f32 py, f32 pz,
                                           f32 sx, f32 sy, f32 sz,
                                           f32 rx, f32 ry, f32 rz);
bool renderer_scene_remove_instance(SceneHandle sh, InstanceHandle i);

bool renderer_sync_scene(SceneHandle sh);

void renderer_render(void);
bool renderer_clear(void);

void renderer_quit();

u64 renderer_scene_count();
SceneHandle renderer_scene_at(u64 index);

u64 renderer_scene_mesh_count(SceneHandle sh);
MeshHandle renderer_scene_mesh_at(SceneHandle sh, u64 index);

u64 renderer_scene_mesh_instance_count(SceneHandle sh, MeshHandle mesh);
InstanceHandle renderer_scene_mesh_instance_at(SceneHandle sh, MeshHandle mesh, u64 index);

void renderer_camera_set(f32 px, f32 py, f32 pz, f32 yaw, f32 pitch);

void renderer_poll_events();

bool renderer_should_close();
  
#ifdef __cplusplus
}
#endif
