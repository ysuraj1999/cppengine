#pragma once
#include <SDL3/SDL.h>
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

using MeshHandle = u64;
using InstanceHandle = u64;

#define static_global   static
#define static_local    static
#define static_internal static

struct HandleCounter {
    u64 next = 0;
    u64 operator()() {
        return next++;
    }
};

typedef struct Transform {
    f32 px;
    f32 py;
    f32 pz;
    f32 sx;
    f32 sy;
    f32 sz;
    f32 rx;
    f32 ry;
    f32 rz;
} Transform;

typedef enum Key {
    KEY_UNKNOWN = 0, // not a supported key; index 0 unused
    // Letters
    KEY_A = 4,
    KEY_B = 5,
    KEY_C = 6,
    KEY_D = 7,
    KEY_E = 8,
    KEY_F = 9,
    KEY_G = 10,
    KEY_H = 11,
    KEY_I = 12,
    KEY_J = 13,
    KEY_K = 14,
    KEY_L = 15,
    KEY_M = 16,
    KEY_N = 17,
    KEY_O = 18,
    KEY_P = 19,
    KEY_Q = 20,
    KEY_R = 21,
    KEY_S = 22,
    KEY_T = 23,
    KEY_U = 24,
    KEY_V = 25,
    KEY_W = 26,
    KEY_X = 27,
    KEY_Y = 28,
    KEY_Z = 29,
    // Numbers
    KEY_1 = 30,
    KEY_2 = 31,
    KEY_3 = 32,
    KEY_4 = 33,
    KEY_5 = 34,
    KEY_6 = 35,
    KEY_7 = 36,
    KEY_8 = 37,
    KEY_9 = 38,
    KEY_0 = 39,
    // Common
    KEY_ENTER = 40,
    KEY_ESCAPE = 41,
    KEY_BACKSPACE = 42,
    KEY_TAB = 43,
    KEY_SPACE = 44,
    // Punctuation
    KEY_MINUS = 45,
    KEY_EQUALS = 46,
    KEY_LEFT_BRACKET = 47,
    KEY_RIGHT_BRACKET = 48,
    KEY_BACKSLASH = 49,
    KEY_SEMICOLON = 51,
    KEY_APOSTROPHE = 52,
    KEY_GRAVE = 53,
    KEY_COMMA = 54,
    KEY_PERIOD = 55,
    KEY_SLASH = 56,
    // Function keys
    KEY_F1 = 58,
    KEY_F2 = 59,
    KEY_F3 = 60,
    KEY_F4 = 61,
    KEY_F5 = 62,
    KEY_F6 = 63,
    KEY_F7 = 64,
    KEY_F8 = 65,
    KEY_F9 = 66,
    KEY_F10 = 67,
    KEY_F11 = 68,
    KEY_F12 = 69,
    // Navigation
    KEY_INSERT = 73,
    KEY_HOME = 74,
    KEY_PAGE_UP = 75,
    KEY_DELETE = 76,
    KEY_END = 77,
    KEY_PAGE_DOWN = 78,
    KEY_RIGHT = 79,
    KEY_LEFT = 80,
    KEY_DOWN = 81,
    KEY_UP = 82,
    // Modifiers
    KEY_LEFT_CTRL = 224,
    KEY_LEFT_SHIFT = 225,
    KEY_LEFT_ALT = 226,
    KEY_LEFT_GUI = 227,
    KEY_RIGHT_CTRL = 228,
    KEY_RIGHT_SHIFT = 229,
    KEY_RIGHT_ALT = 230,
    KEY_RIGHT_GUI = 231,
    // Convenience aliases that resolve to the left modifier
    KEY_CTRL = 224,
    KEY_SHIFT = 225,
    KEY_ALT = 226,
    KEY_GUI = 227,
    KEY_COUNT = 232, // one past KEY_RIGHT_GUI; index 0 unused
} Key;

typedef enum MouseButton {
    MOUSE_BUTTON_NONE = 0, // no button; index 0 sentinel
    MOUSE_BUTTON_LEFT = 1,
    MOUSE_BUTTON_MIDDLE = 2,
    MOUSE_BUTTON_RIGHT = 3,
    MOUSE_BUTTON_X1 = 4,
    MOUSE_BUTTON_X2 = 5,
    MOUSE_BUTTON_COUNT = 6, // one past MOUSE_BUTTON_X2
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
    InputEventType type;  // INPUT_EVENT_*
    Key key;              // KEY_* (key events, else KEY_UNKNOWN)
    MouseButton button;   // MOUSE_BUTTON_* (button events, else MOUSE_BUTTON_NONE)
    bool down;            // true = press, false = release (key/button events)
    bool repeat;          // OS auto-repeat flag (key down only)
    f32 x, y;             // absolute cursor position (mouse events)
    f32 dx, dy;           // motion delta (motion events)
    f32 wheel_x, wheel_y; // wheel delta (wheel events)
} InputEvent;

struct Vertex {
    glm::vec3 position;
    glm::vec4 color;
    glm::vec3 normal;
    glm::vec2 uv;
};

inline constexpr glm::vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};

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

// Forward for UI's ImGui context pointer (keeps this header ImGui-free).
struct ImGuiContext;

//-----------------------------------------------------------------------------
// GPUResources: owns every SDL window / device / buffer / texture / sampler /
// pipeline object and the per-frame bindings.
//-----------------------------------------------------------------------------
struct GPUResources {
    SDL_Window *window = nullptr;
    SDL_GPUDevice *gpuDevice = nullptr;

    SDL_GPUBuffer *vertexBuffer = nullptr;
    SDL_GPUBuffer *instanceBuffer = nullptr;
    SDL_GPUBuffer *indexBuffer = nullptr;
    SDL_GPUTransferBuffer *transferToInstanceBuffer = nullptr;

    SDL_GPUTexture *checkTexture = nullptr;
    SDL_GPUTexture *depthTexture = nullptr;
    SDL_GPUSampler *sampler = nullptr;

    SDL_GPUShader *vertexShader = nullptr;
    SDL_GPUShader *fragmentShader = nullptr;
    SDL_GPUGraphicsPipeline *pipeline = nullptr;

    SDL_GPUBufferBinding vertexBufferBinding[1] = {};
    SDL_GPUBufferBinding instanceBufferBinding[1] = {};
    SDL_GPUBufferBinding indexBufferBinding[1] = {};
    SDL_GPUTextureSamplerBinding textureSamplerBindings[1] = {};

    u32 INSTANCE_PREALLOCATED_SIZE = 10 * 1024 * 1024;

    bool initialized() const {
        return gpuDevice != nullptr;
    }

    bool init(RendererConfig &config);
    void rebuild(const GeometryStorage &geometry);
    void destroy();
};

//-----------------------------------------------------------------------------
// Keyboard / Mouse: polled input state.
//-----------------------------------------------------------------------------
struct Keyboard {
    bool current[KEY_COUNT] = {};
    bool previous[KEY_COUNT] = {};

    void copy_current_state_into_previous_state() {
        memcpy(previous, current, sizeof(current));
    }
    void begin() {
        copy_current_state_into_previous_state();
    }
    void end();
    void clear_current_state() {
        memset(current, 0, sizeof(current));
    }

    bool down(Key k) const {
        ASSERT(k > KEY_UNKNOWN && k < KEY_COUNT, "key out of range");
        return current[k];
    }
    bool up(Key k) const {
        ASSERT(k > KEY_UNKNOWN && k < KEY_COUNT, "key out of range");
        return !current[k];
    }
    bool pressed(Key k) const {
        ASSERT(k > KEY_UNKNOWN && k < KEY_COUNT, "key out of range");
        return current[k] && !previous[k];
    }
    bool released(Key k) const {
        ASSERT(k > KEY_UNKNOWN && k < KEY_COUNT, "key out of range");
        return !current[k] && previous[k];
    }
};

struct Mouse {
    f32 x = 0, y = 0;
    f32 dx = 0, dy = 0;
    f32 wheel_x = 0, wheel_y = 0;
    bool down_current[MOUSE_BUTTON_COUNT] = {};
    bool down_previous[MOUSE_BUTTON_COUNT] = {};

    void copy_current_state_into_previous_state() {
        memcpy(down_previous, down_current, sizeof(down_current));
    }
    void clear_transient() {
        dx = 0;
        dy = 0;
        wheel_x = 0;
        wheel_y = 0;
    }
    void begin() {
        copy_current_state_into_previous_state();
        clear_transient();
    }
    void end() {
    }

    void on_motion(f32 nx, f32 ny, f32 xrel, f32 yrel) {
        x = nx;
        y = ny;
        dx += xrel;
        dy += yrel;
    }
    void on_button_down(MouseButton button, f32 bx, f32 by, bool track) {
        x = bx;
        y = by;
        if (track && button > 0 && button < MOUSE_BUTTON_COUNT)
            down_current[button] = true;
    }
    void on_button_up(MouseButton button, f32 bx, f32 by) {
        x = bx;
        y = by;
        if (button > 0 && button < MOUSE_BUTTON_COUNT)
            down_current[button] = false;
    }
    void on_wheel(f32 wx, f32 wy) {
        wheel_x += wx;
        wheel_y += wy;
    }

    bool down(MouseButton b) const {
        ASSERT(b > 0 && b < MOUSE_BUTTON_COUNT, "invalid mouse button");
        return down_current[b];
    }
    bool up(MouseButton b) const {
        ASSERT(b > 0 && b < MOUSE_BUTTON_COUNT, "invalid mouse button");
        return !down_current[b];
    }
    bool pressed(MouseButton b) const {
        ASSERT(b > 0 && b < MOUSE_BUTTON_COUNT, "invalid mouse button");
        return down_current[b] && !down_previous[b];
    }
    bool released(MouseButton b) const {
        ASSERT(b > 0 && b < MOUSE_BUTTON_COUNT, "invalid mouse button");
        return !down_current[b] && down_previous[b];
    }
};

//-----------------------------------------------------------------------------
// Input: polls SDL events, exposes keyboard / mouse / event-buffer queries.
//-----------------------------------------------------------------------------
struct Input {
    Keyboard keyboard;
    Mouse mouse;
    InputEvent buffer[INPUT_EVENT_CAPACITY] = {};
    u32 count = 0;
    bool shouldClose = false;

    GPUResources &gpu;
    u64 prevCounter = 0;

    Input(GPUResources &g) : gpu(g) {
        prevCounter = SDL_GetPerformanceCounter();
    }

    void push_event(InputEvent e) {
        if (count >= INPUT_EVENT_CAPACITY)
            return;
        buffer[count++] = e;
    }
    void clear_events() {
        count = 0;
    }
    void begin() {
        keyboard.begin();
        mouse.begin();
        clear_events();
    }
    void poll();

    bool should_close() {
        return shouldClose;
    }
    void set_cursor_captured(bool captured) {
        if (gpu.window)
            SDL_SetWindowRelativeMouseMode(gpu.window, captured != false);
    }
    bool cursor_captured() {
        return gpu.window && SDL_GetWindowRelativeMouseMode(gpu.window);
    }

    f32 mouse_x() {
        return mouse.x;
    }
    f32 mouse_y() {
        return mouse.y;
    }
    f32 mouse_dx() {
        return mouse.dx;
    }
    f32 mouse_dy() {
        return mouse.dy;
    }
    f32 mouse_wheel_x() {
        return mouse.wheel_x;
    }
    f32 mouse_wheel_y() {
        return mouse.wheel_y;
    }
    bool mouse_down(MouseButton button) {
        return mouse.down(button);
    }
    bool mouse_up(MouseButton button) {
        return mouse.up(button);
    }
    bool mouse_pressed(MouseButton button) {
        return mouse.pressed(button);
    }
    bool mouse_released(MouseButton button) {
        return mouse.released(button);
    }
    bool key_down(Key key) {
        return keyboard.down(key);
    }
    bool key_up(Key key) {
        return keyboard.up(key);
    }
    bool key_pressed(Key key) {
        return keyboard.pressed(key);
    }
    bool key_released(Key key) {
        return keyboard.released(key);
    }
    u64 event_count() {
        return count;
    }
    InputEvent event_at(u64 index) {
        ASSERT(index < count, "input event index out of bounds");
        return buffer[index];
    }
    f32 time_dt();
};

//-----------------------------------------------------------------------------
// UI: wraps the (global) ImGui API.
//-----------------------------------------------------------------------------
struct UI {
    ImGuiContext *ctx = nullptr;

    bool init(GPUResources &gpu);
    void quit();
    void begin();
    void end(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass);

    bool begin_window(const char *name);
    void end_window();
    bool button(const char *label);
    bool begin_tree(const char *label);
    void end_tree();
    bool selectable(const char *label, bool selected);
    void sameline();
    void text(const char *text);
    void push_id(int id);
    void pop_id();
    bool begin_context_menu_item();
    bool begin_context_menu_window();
    void end_context_menu();
    bool menu_item(const char *label);
    bool item_double_clicked(MouseButton button);
    bool item_clicked(MouseButton button);
};

//-----------------------------------------------------------------------------
// RendererFrame: per-frame draw-pass bookkeeping + draw submission. Holds
// references to the app-owned GPU / geometry / scene / config components.
//-----------------------------------------------------------------------------
struct RendererFrame {
    SDL_GPURenderPass *renderPass = nullptr;
    SDL_GPUCommandBuffer *commandBuffer = nullptr;
    SDL_GPUTexture *swapchainTexture = nullptr;
    u32 swapchainWidth = 0;
    u32 swapchainHeight = 0;
    SDL_GPUColorTargetInfo colorTargetInfo;
    SDL_GPUDepthStencilTargetInfo depthTargetInfo;
    u32 instanceBufferOffset = 0;

    GPUResources &gpu;
    GeometryStorage &geometry;
    Scene &scene;
    RendererConfig &config;

    RendererFrame(GPUResources &g, GeometryStorage &geo, Scene &s, RendererConfig &c)
        : gpu(g), geometry(geo), scene(s), config(c) {
    }

    void init();
    void begin();
    void render_scene();
    void end();

    void draw_mesh(MeshRegion &meshRegion, const Instances &instances, u64 start, u64 count);
};

//-----------------------------------------------------------------------------
// Renderer: high-level create/draw/camera flow driven by the app.
//-----------------------------------------------------------------------------
struct Renderer {
    GPUResources &gpu;
    GeometryStorage &geometry;
    Scene &scene;
    RendererConfig config;
    RendererFrame frame;
    bool shouldClose = false;

    Renderer(GPUResources &g, GeometryStorage &geo, Scene &s)
        : gpu(g), geometry(geo), scene(s), frame(g, geo, s, config) {
    }

    bool init();
    void quit();
    bool clear();
    void rebuild();
    u64 load_obj(const char *path);

    void camera_move(f32 forward, f32 right, f32 up);
    void camera_rotate(f32 yaw, f32 pitch);
    void camera_look_at(f32 x, f32 y, f32 z);

    void render_begin() {
        frame.begin();
    }
    void render_scene() {
        frame.render_scene();
    }
    void render_end() {
        frame.end();
    }

    SDL_GPUCommandBuffer *command_buffer() {
        return frame.commandBuffer;
    }
    SDL_GPURenderPass *render_pass() {
        return frame.renderPass;
    }
};
