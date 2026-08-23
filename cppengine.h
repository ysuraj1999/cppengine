#pragma once
#include <stdint.h>
#include <stdbool.h>
#include <SDL3/SDL_scancode.h>

typedef uint64_t u64;
typedef float f32;
typedef u64 SceneHandle;
typedef u64 MeshHandle;
typedef u64 InstanceHandle;

typedef struct Transform {
  f32 px; f32 py; f32 pz;
  f32 sx; f32 sy; f32 sz;
  f32 rx; f32 ry; f32 rz;
} Transform;

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

bool renderer_init(void);
void renderer_render_begin(void);
void renderer_render_scene(SceneHandle sh);
void renderer_render_end(void);
bool renderer_clear(void);
void renderer_quit(void);
bool renderer_sync_scene(SceneHandle sh);

void renderer_camera_move(f32 forward, f32 right, f32 up);
void renderer_camera_rotate(f32 dx, f32 dy);
void renderer_camera_update(void);

SceneHandle scene_create(void);
bool scene_destroy(SceneHandle sh);
bool scene_load_obj(SceneHandle sh, const char *path);
bool scene_remove_mesh(SceneHandle sh, MeshHandle mh);
InstanceHandle scene_add_instance(SceneHandle sh, MeshHandle m, Transform t);
const char* scene_mesh_alias(SceneHandle sh, MeshHandle mh);
const char* scene_instance_alias(SceneHandle sh, InstanceHandle ih);
void scene_instance_set_transform(SceneHandle sh,
                                  InstanceHandle i,
                                  Transform t);
Transform scene_instance_get_transform(SceneHandle sh, InstanceHandle i);
bool scene_remove_instance(SceneHandle sh, InstanceHandle i);

u64 scene_count(void);
SceneHandle scene_at(u64 index);
u64 scene_mesh_count(SceneHandle sh);
MeshHandle scene_mesh_at(SceneHandle sh, u64 index);
u64 scene_mesh_instance_count(SceneHandle sh, MeshHandle mesh);
InstanceHandle scene_mesh_instance_at(SceneHandle sh, MeshHandle mesh, u64 index);

void input_poll(void);
bool input_should_close(void);

f32 input_mouse_x(void);
f32 input_mouse_y(void);
f32 input_mouse_dx(void);
f32 input_mouse_dy(void);
f32 input_mouse_wheel_x(void);
f32 input_mouse_wheel_y(void);
bool input_mouse_down(MouseButton button);
bool input_mouse_pressed(MouseButton button);
bool input_mouse_released(MouseButton button);

bool input_key_down(SDL_Scancode key);
bool input_key_pressed(SDL_Scancode key);
bool input_key_released(SDL_Scancode key);

f32 time_dt(void);

void ui_begin(void);
void ui_end(void);
void ui_begin_window(const char* name);
void ui_end_window(void);
bool ui_button(const char* label);
bool ui_treenode(const char* label);
void ui_treepop(void);
bool ui_selectable(const char* label);
void ui_sameline(void);
bool ui_drag_float3(const char* label, f32 *v);
void ui_text(const char* text);

#ifdef __cplusplus
}
#endif
