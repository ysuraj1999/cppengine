#pragma once
#include <stdint.h>
#include <stdbool.h>

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

typedef enum Key {
  KEY_UNKNOWN = 0,           // not a supported key; index 0 unused
  // Letters
  KEY_A = 4, KEY_B = 5, KEY_C = 6, KEY_D = 7, KEY_E = 8, KEY_F = 9,
  KEY_G = 10, KEY_H = 11, KEY_I = 12, KEY_J = 13, KEY_K = 14,
  KEY_L = 15, KEY_M = 16, KEY_N = 17, KEY_O = 18, KEY_P = 19,
  KEY_Q = 20, KEY_R = 21, KEY_S = 22, KEY_T = 23, KEY_U = 24,
  KEY_V = 25, KEY_W = 26, KEY_X = 27, KEY_Y = 28, KEY_Z = 29,
  // Numbers
  KEY_1 = 30, KEY_2 = 31, KEY_3 = 32, KEY_4 = 33, KEY_5 = 34,
  KEY_6 = 35, KEY_7 = 36, KEY_8 = 37, KEY_9 = 38, KEY_0 = 39,
  // Common
  KEY_ENTER = 40, KEY_ESCAPE = 41, KEY_BACKSPACE = 42, KEY_TAB = 43,
  KEY_SPACE = 44,
  // Punctuation
  KEY_MINUS = 45, KEY_EQUALS = 46, KEY_LEFT_BRACKET = 47,
  KEY_RIGHT_BRACKET = 48, KEY_BACKSLASH = 49, KEY_SEMICOLON = 51,
  KEY_APOSTROPHE = 52, KEY_GRAVE = 53, KEY_COMMA = 54, KEY_PERIOD = 55,
  KEY_SLASH = 56,
  // Function keys
  KEY_F1 = 58, KEY_F2 = 59, KEY_F3 = 60, KEY_F4 = 61, KEY_F5 = 62,
  KEY_F6 = 63, KEY_F7 = 64, KEY_F8 = 65, KEY_F9 = 66, KEY_F10 = 67,
  KEY_F11 = 68, KEY_F12 = 69,
  // Navigation
  KEY_INSERT = 73, KEY_HOME = 74, KEY_PAGE_UP = 75, KEY_DELETE = 76,
  KEY_END = 77, KEY_PAGE_DOWN = 78, KEY_RIGHT = 79, KEY_LEFT = 80,
  KEY_DOWN = 81, KEY_UP = 82,
  // Modifiers
  KEY_LEFT_CTRL = 224, KEY_LEFT_SHIFT = 225, KEY_LEFT_ALT = 226,
  KEY_LEFT_GUI = 227, KEY_RIGHT_CTRL = 228, KEY_RIGHT_SHIFT = 229,
  KEY_RIGHT_ALT = 230, KEY_RIGHT_GUI = 231,
  // Convenience aliases that resolve to the left modifier
  KEY_CTRL = 224, KEY_SHIFT = 225, KEY_ALT = 226, KEY_GUI = 227,
  KEY_COUNT = 232,          // one past KEY_RIGHT_GUI; index 0 unused
} Key;

typedef enum MouseButton {
  MOUSE_BUTTON_NONE = 0,       // no button; index 0 sentinel
  MOUSE_BUTTON_LEFT = 1,
  MOUSE_BUTTON_MIDDLE = 2,
  MOUSE_BUTTON_RIGHT = 3,
  MOUSE_BUTTON_X1 = 4,
  MOUSE_BUTTON_X2 = 5,
  MOUSE_BUTTON_COUNT = 6,      // one past MOUSE_BUTTON_X2
} MouseButton;

typedef enum InputEventType {
  INPUT_EVENT_KEY_DOWN = 1,
  INPUT_EVENT_KEY_UP,
  INPUT_EVENT_MOUSE_MOTION,
  INPUT_EVENT_MOUSE_BUTTON_DOWN,
  INPUT_EVENT_MOUSE_BUTTON_UP,
  INPUT_EVENT_MOUSE_WHEEL,
} InputEventType;

#define INPUT_EVENT_CAPACITY 512

typedef struct InputEvent {
  InputEventType type;   // INPUT_EVENT_*
  Key key;               // KEY_* (key events, else KEY_UNKNOWN)
  MouseButton button;    // MOUSE_BUTTON_* (button events, else MOUSE_BUTTON_NONE)
  bool down;             // true = press, false = release (key/button events)
  bool repeat;           // OS auto-repeat flag (key down only)
  f32  x, y;             // absolute cursor position (mouse events)
  f32  dx, dy;           // motion delta (motion events)
  f32  wheel_x, wheel_y; // wheel delta (wheel events)
} InputEvent;

#ifdef __cplusplus
extern "C" {
#endif

bool renderer_init(void);
void renderer_render_begin(void);
void renderer_render_scene(SceneHandle sh);
void renderer_render_end(void);
bool renderer_clear(void);
void renderer_quit(void);
bool renderer_rebuild_scene(SceneHandle sh);

void renderer_camera_move(f32 forward, f32 right, f32 up);
void renderer_camera_rotate(f32 dx, f32 dy);
void renderer_camera_look_at(InstanceHandle ih);

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

void input_set_cursor_captured(bool captured);
bool input_cursor_captured(void);

f32 input_mouse_x(void);
f32 input_mouse_y(void);
f32 input_mouse_dx(void);
f32 input_mouse_dy(void);
f32 input_mouse_wheel_x(void);
f32 input_mouse_wheel_y(void);
bool input_mouse_down(MouseButton button);
bool input_mouse_pressed(MouseButton button);
bool input_mouse_released(MouseButton button);

bool input_key_down(Key key);
bool input_key_pressed(Key key);
bool input_key_released(Key key);

u64 input_event_count(void);
InputEvent input_event_at(u64 index);

f32 time_dt(void);

bool ui_init(void);
void ui_quit(void);
void ui_begin(void);
void ui_end(void);
void ui_begin_window(const char* name);
void ui_end_window(void);
bool ui_button(const char* label);
bool ui_treenode(const char* label);
void ui_treepop(void);
bool ui_selectable(const char* label, bool selected);
void ui_sameline(void);
void ui_text(const char* text);
void ui_push_id(int id);
void ui_pop_id(void);
bool ui_begin_context_menu_item(void);
bool ui_begin_context_menu_window(void);
void ui_end_context_menu(void);
bool ui_menu_item(const char* label);
bool ui_double_click(MouseButton button);
bool ui_item_clicked(MouseButton button);

#ifdef __cplusplus
}
#endif
