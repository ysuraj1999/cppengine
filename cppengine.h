#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL_scancode.h>

typedef uint64_t u64;
typedef float f32;
typedef u64 SceneHandle;
typedef u64 MeshHandle;
typedef u64 InstanceHandle;

// #define SDL_BUTTON_LEFT 1
// #define SDL_BUTTON_MIDDLE 2
// #define SDL_BUTTON_RIGHT 3
// #define SDL_BUTTON_X1 4
// #define SDL_BUTTON_X2 5

typedef enum MouseButton {
  MOUSE_BUTTON_LEFT = 1,
  MOUSE_BUTTON_MIDDLE = 2,
  MOUSE_BUTTON_RIGHT = 3,
  MOUSE_BUTTON_X1 = 4,
  MOUSE_BUTTON_X2 = 5,
  MOUSE_BUTTON_COUNT = 6,   // index 0 unused
} MouseButton;

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
void renderer_scene_instance_set_transform(SceneHandle sh,
					   InstanceHandle i,
					   f32 px, f32 py, f32 pz,
					   f32 sx, f32 sy, f32 sz,
					   f32 rx, f32 ry, f32 rz);
bool renderer_scene_remove_instance(SceneHandle sh, InstanceHandle i);

bool renderer_sync_scene(SceneHandle sh);

void renderer_render_begin(void);
void renderer_render(void);
void renderer_render_end(void);

void renderer_ui_begin(void);
void renderer_ui_end(void);
  
bool renderer_clear(void);
  
void renderer_quit();

u64 renderer_scene_count();
SceneHandle renderer_scene_at(u64 index);

u64 renderer_scene_mesh_count(SceneHandle sh);
MeshHandle renderer_scene_mesh_at(SceneHandle sh, u64 index);

u64 renderer_scene_mesh_instance_count(SceneHandle sh, MeshHandle mesh);
InstanceHandle renderer_scene_mesh_instance_at(SceneHandle sh, MeshHandle mesh, u64 index);

void renderer_poll_events();

bool renderer_should_close();

f32 renderer_mouse_x(void);
f32 renderer_mouse_y(void);
f32 renderer_mouse_dx(void);
f32 renderer_mouse_dy(void);
f32 renderer_mouse_wheel_x(void);
f32 renderer_mouse_wheel_y(void);
bool renderer_mouse_down(MouseButton button);
bool renderer_mouse_pressed(MouseButton button);
bool renderer_mouse_released(MouseButton button);

void renderer_camera_move(f32 forward, f32 right, f32 up);
void renderer_camera_rotate(f32 dx, f32 dy);
void renderer_camera_update(void);

bool renderer_key_down(SDL_Scancode key);
bool renderer_key_pressed(SDL_Scancode key);
bool renderer_key_released(SDL_Scancode key);

f32 renderer_dt(void);

void ui_begin_window(const char* name);
void ui_end_window();
bool ui_button(const char* label);
  
#ifdef __cplusplus
}
#endif
