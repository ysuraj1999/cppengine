#include "glm/ext/vector_float4.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>
#include <string>
#include <unordered_map>
#include <vector>

#define ASSERT(cond, msg)						\
  do {									\
    if (!(cond)) {							\
      std::fprintf(stderr, "%s:%d: ASSERT FAILED: %s - %s\n", __FILE__, __LINE__, #cond, msg); \
      std::abort();							\
    }									\
  } while (0)

#define SDL_ASSERT(cond)						\
  do {									\
    if (!(cond)) {							\
      std::fprintf(stderr, "%s:%d: SDL error: %s\n", __FILE__, __LINE__, SDL_GetError()); \
      std::abort();							\
    }									\
  } while (0)

template <typename F> class ScopeGuard {
  F f;
  bool active = true;

public:
  explicit ScopeGuard(F &&f) : f(std::forward<F>(f)) {
  }

  ScopeGuard(ScopeGuard &&other) : f(std::move(other.f)), active(other.active) {
    other.active = false;
  }

  ~ScopeGuard() {
    if (active)
      f();
  }

  ScopeGuard(const ScopeGuard &) = delete;
  ScopeGuard &operator=(const ScopeGuard &) = delete;
};

template <typename F> ScopeGuard(F &&) -> ScopeGuard<F>;

#define DEFER_IMPL(x, y) x##y
#define DEFER(x, y)      DEFER_IMPL(x, y)

#define defer(code) auto DEFER(_defer_, __COUNTER__) = ScopeGuard([&] { code; })

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

typedef u64 MeshHandle;
typedef u64 InstanceHandle;
typedef u64 MaterialHandle;

#define static_global   static
#define static_local    static
#define static_internal static

// CONSTANTS
inline constexpr glm::vec3 WORLD_UP = {0.0f, 1.0f, 0.0f};

struct HandleCounter {
  u64 next = 0;
  u64 operator()() {
    return next++;
  }
  void reset() {
    next = 0;
  }
};

typedef struct Transform Transform;
glm::mat4 transformToMat4(const Transform &t);

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

  const glm::mat4 matrix() const {
    return transformToMat4(*this);
  }
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

#define INPUT_EVENT_CAPACITY 256

typedef struct InputEvent {
  InputEventType type;
  Key key;
  MouseButton button;
  bool down;
  bool repeat;
  f32 x, y;
  f32 dx, dy;
  f32 wheel_x, wheel_y;
} InputEvent;

struct Vertex {
  glm::vec3 position;
  glm::vec4 color;
  glm::vec3 normal;
  glm::vec2 uv;
};

struct Camera {
  glm::vec3 position{0.0f, 0.0f, 0.0f};
  f32 yaw = 0.0f;
  f32 pitch = 0.0f;
  glm::vec3 forward{1.0f, 0.0f, 0.0f};
  glm::vec3 right{0.0f, 0.0f, 1.0f};

  void move(f32 fwd, f32 rght, f32 up) {
    position += forward * fwd;
    position += right * rght;
    position += WORLD_UP * up;
  }

  void rotate(f32 dYaw, f32 dPitch) {
    yaw = glm::mod(yaw + dYaw, 360.0f);
    pitch = glm::clamp(pitch + dPitch, -89.99f, 89.99f);
    _update();
  }

  void look_at(glm::vec3 target) {
    glm::vec3 dir = target - position;
    if (glm::dot(dir, dir) < 1e-12f)
      return;
    dir = glm::normalize(dir);

    yaw = glm::mod(glm::degrees(glm::atan(dir.z, dir.x)), 360.0f);
    pitch = glm::clamp(glm::degrees(glm::asin(glm::clamp(dir.y, -1.0f, 1.0f))), -89.99f, 89.99f);
    _update();
  }

  glm::mat4 viewMatrix() const {
    return glm::lookAtLH(position, position + forward, WORLD_UP);
  }
  void _update() {
    forward = glm::normalize(
			     glm::vec3{
			       cos(glm::radians(pitch)) * cos(glm::radians(yaw)),
			       sin(glm::radians(pitch)),
			       cos(glm::radians(pitch)) * sin(glm::radians(yaw))
			     }
			     );
    right = glm::cross(WORLD_UP, forward);
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
  std::vector<MaterialHandle> materialHandles;
  std::string alias;
};

struct MaterialRegion {
    u32 indexOffset;
    u32 indexCount;
    MaterialHandle materialHandle;
};

struct MeshRegion {
  u32 vertexOffset;
  u32 indexOffset;
  u32 indexCount;
  std::vector<MaterialRegion> materialRegions;
  std::string alias;
};



#include <cfenv>
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_sdlgpu3.h"
#include "imgui/imgui.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// FPUContext: RAII-style FPU exception-state save/restore via static env and
// begin()/end() methods. Static env defined in the folded-in section below.
struct FPUEnvironment {
  static std::fenv_t env;
  static void save() {
    std::feholdexcept(&env);
  }
  static void restore() {
    std::fesetenv(&env);
  }
};
std::fenv_t FPUEnvironment::env;


Key SDL_Scancode_to_Key(SDL_Scancode scancode);
MouseButton SDL_MouseButton_to_MouseButton(Uint8 button);
ImGuiMouseButton MouseButton_to_imgui_button(MouseButton button);

// Implemented in utils.cpp.
std::vector<u8> readFile(const char *path);
glm::mat4 transformToMat4(const Transform &t);

struct Material {
  glm::vec4 baseColor;
};

struct MaterialStorage {
  std::vector<Material> materials;
  std::vector<MaterialHandle> materialOrder;
  std::unordered_map<MaterialHandle, u32> materialIndices;
  HandleCounter materialCounter;

  MaterialHandle add(Material &&mat) {
    mat.baseColor = glm::vec4(
			      rand() / (float)RAND_MAX,
			      rand() / (float)RAND_MAX,
			      rand() / (float)RAND_MAX,
			      1.0f
			      );
    MaterialHandle handle = materialCounter();
    u32 index = (u32)materials.size();
    materials.push_back(std::move(mat));
    materialOrder.push_back(handle);
    materialIndices[handle] = index;
    return handle;
  }

  Material& get(MaterialHandle handle) {
    return materials.at(materialIndices.at(handle));
  }
  
  u64 size() const {
    return materialOrder.size();
  }

  MaterialHandle operator[](u64 i) const {
    return materialOrder.at(i);
  }

  void clear() {
    materials.clear();
    materialOrder.clear();
    materialIndices.clear();
    materialCounter.reset();
  }
};

struct GeometryStorage {
  std::vector<Vertex> vertices;
  std::vector<u32> indices;
  std::vector<MeshHandle> meshOrder;
  std::unordered_map<MeshHandle, MeshRegion> meshRegions;
  HandleCounter meshCounter;

  MeshHandle add(Mesh &&mesh) {
    MeshHandle handle = meshCounter();
    MeshRegion region{};
    region.vertexOffset = (u32)vertices.size();
    region.indexOffset = (u32)indices.size();
    region.indexCount = (u32)mesh.indices.size();
    region.alias = mesh.alias;
    vertices.insert(vertices.end(), mesh.vertices.begin(), mesh.vertices.end());
    indices.insert(indices.end(), mesh.indices.begin(), mesh.indices.end());

    u32 i = 0;
    while (i < mesh.materialHandles.size()) {
      u32 j = i;
      while (j < mesh.materialHandles.size() && mesh.materialHandles[i] == mesh.materialHandles[j]) {
	j++;
      }
      region.materialRegions.push_back(
          MaterialRegion{.indexOffset = i * 3, .indexCount = (j - i) * 3, .materialHandle = mesh.materialHandles[i]}
      );
      i = j;
    }
    meshRegions[handle] = region;
    meshOrder.push_back(handle);
    return handle;
  }

  u64 size() const {
    return meshOrder.size();
  }

  MeshHandle operator[](u64 i) const {
    return meshOrder.at(i);
  }

  void clear() {
    vertices.clear();
    indices.clear();
    meshOrder.clear();
    meshRegions.clear();
    meshCounter.reset();
  }
};

struct Instance {
  MeshHandle meshHandle;
  Transform transform;
  mutable glm::mat4 cached{};
  mutable bool computed = false;

  const glm::mat4 &matrix() const {
    if (!computed) {
      cached = transform.matrix();
      computed = true;
    }
    return cached;
  }
};

typedef std::vector<Instance> Instances;

struct Scene {
  Instances instances;

  Instance &operator[](u64 i) {
    return instances.at(i);
  }
  const Instance &operator[](u64 i) const {
    return instances.at(i);
  }
  u64 size() const {
    return instances.size();
  }
  Instance& add(MeshHandle mesh, Transform transform) {
    instances.push_back(Instance{mesh, transform, {}, false});
    return instances.back();
  }
  void remove(u64 i) {
    ASSERT(i < size(), "scene index out of bounds");
    instances.erase(instances.begin() + i);
  }
  void clear() {
    instances.clear();
  }
};

struct RendererConfig {
  glm::vec2 resolution;
  glm::vec4 backgroundColor;
  f32 keyboardSensitivity;
  f32 mouseSensitivity;
};

struct GPUResources {
  SDL_Window *window = nullptr;
  SDL_GPUDevice *gpuDevice = nullptr;

  SDL_GPUBuffer *vertexBuffer = nullptr;
  SDL_GPUBuffer *indexBuffer = nullptr;
  SDL_GPUBuffer *instanceBuffer = nullptr;
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

  u32 INSTANCE_PREALLOCATED_SIZE = 100 * 1024 * 1024;

  bool initialized() const {
    return gpuDevice != nullptr;
  }

  bool request_cursor_capture(bool captured) {
    if (window)
      SDL_SetWindowRelativeMouseMode(window, captured != false);
    return true;
  }
  bool cursor_captured() const {
    return window && SDL_GetWindowRelativeMouseMode(window);
  }

  bool init(RendererConfig &config) {
    if (gpuDevice != nullptr || window != nullptr) {
      std::fprintf(stderr, "GPUResources::init: already initialized (call destroy first)\n");
      return false;
    }

    SDL_ASSERT(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS));

    window = SDL_CreateWindow("Lens", (u32)config.resolution.x, (u32)config.resolution.y, 0);
    SDL_ASSERT(window != nullptr);

    #if defined(__APPLE__)
    constexpr SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_MSL;
    constexpr const char *GPUDriver = "metal";
    constexpr const char *shaderEntryPoint = "main0";
    constexpr const char *vertexShaderPath = SHADER_BIN_DIR "/shader.vertex.metal";
    constexpr const char *fragmentShaderPath = SHADER_BIN_DIR "/shader.fragment.metal";
#elif defined(__linux__)
    constexpr SDL_GPUShaderFormat shaderFormat = SDL_GPU_SHADERFORMAT_SPIRV;
    constexpr const char *GPUDriver = "vulkan";
    constexpr const char *shaderEntryPoint = "main";
    constexpr const char *vertexShaderPath = SHADER_BIN_DIR "/shader.vertex.spv";
    constexpr const char *fragmentShaderPath = SHADER_BIN_DIR "/shader.fragment.spv";
#else
#error "Unsupported platform: only macOS and Linux are supported"
#endif

    gpuDevice = SDL_CreateGPUDevice(shaderFormat, true, GPUDriver);
    SDL_ASSERT(gpuDevice != nullptr);

    SDL_ASSERT(SDL_ClaimWindowForGPUDevice(gpuDevice, window));

    SDL_ASSERT(SDL_SetGPUSwapchainParameters(
					     gpuDevice, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC
					     ));

    std::vector<u8> vertexShaderContents = readFile(vertexShaderPath);
    ASSERT(!vertexShaderContents.empty(), vertexShaderPath);

    SDL_GPUShaderCreateInfo vertexShaderInfo = {
      .code_size = vertexShaderContents.size(),
      .code = vertexShaderContents.data(),
      .entrypoint = shaderEntryPoint,
      .format = shaderFormat,
      .stage = SDL_GPU_SHADERSTAGE_VERTEX,
      .num_samplers = 0,
      .num_storage_textures = 0,
      .num_storage_buffers = 0,
      .num_uniform_buffers = 1,
    };
    vertexShader = SDL_CreateGPUShader(gpuDevice, &vertexShaderInfo);
    SDL_ASSERT(vertexShader != nullptr);

    std::vector<u8> fragmentShaderContents = readFile(fragmentShaderPath);
    ASSERT(!fragmentShaderContents.empty(), fragmentShaderPath);

    SDL_GPUShaderCreateInfo fragmentShaderInfo = {
      .code_size = fragmentShaderContents.size(),
      .code = fragmentShaderContents.data(),
      .entrypoint = shaderEntryPoint,
      .format = shaderFormat,
      .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
      .num_samplers = 1,
      .num_storage_textures = 0,
      .num_storage_buffers = 0,
      .num_uniform_buffers = 2,
    };
    fragmentShader = SDL_CreateGPUShader(gpuDevice, &fragmentShaderInfo);
    SDL_ASSERT(fragmentShader != nullptr);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {
      .vertex_shader = vertexShader,
      .fragment_shader = fragmentShader,
      .primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST,
    };

    SDL_GPUVertexBufferDescription vertexBufferDescription[2] = {};

    vertexBufferDescription[0].slot = 0;
    vertexBufferDescription[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    vertexBufferDescription[0].instance_step_rate = 0;
    vertexBufferDescription[0].pitch = (u32)sizeof(Vertex);

    vertexBufferDescription[1].slot = 1;
    vertexBufferDescription[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
    vertexBufferDescription[1].instance_step_rate = 0;
    vertexBufferDescription[1].pitch = (u32)sizeof(glm::mat4);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 2;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescription;

    SDL_GPUVertexAttribute vertexAttributes[8] = {};
    // VERTICES
    vertexAttributes[0].buffer_slot = 0;
    vertexAttributes[0].location = 0;
    vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[0].offset = offsetof(Vertex, position);

    vertexAttributes[1].buffer_slot = 0;
    vertexAttributes[1].location = 1;
    vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[1].offset = offsetof(Vertex, color);

    vertexAttributes[2].buffer_slot = 0;
    vertexAttributes[2].location = 2;
    vertexAttributes[2].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
    vertexAttributes[2].offset = offsetof(Vertex, normal);

    vertexAttributes[3].buffer_slot = 0;
    vertexAttributes[3].location = 3;
    vertexAttributes[3].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT2;
    vertexAttributes[3].offset = offsetof(Vertex, uv);

    // INSTANCES
    vertexAttributes[4].buffer_slot = 1;
    vertexAttributes[4].location = 4;
    vertexAttributes[4].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[4].offset = 0;

    vertexAttributes[5].buffer_slot = 1;
    vertexAttributes[5].location = 5;
    vertexAttributes[5].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[5].offset = sizeof(glm::vec4);

    vertexAttributes[6].buffer_slot = 1;
    vertexAttributes[6].location = 6;
    vertexAttributes[6].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[6].offset = 2 * sizeof(glm::vec4);

    vertexAttributes[7].buffer_slot = 1;
    vertexAttributes[7].location = 7;
    vertexAttributes[7].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
    vertexAttributes[7].offset = 3 * sizeof(glm::vec4);

    pipelineInfo.vertex_input_state.num_vertex_attributes = 8;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {{
	.format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window),
      }};
    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

    pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
    pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

    SDL_GPUDepthStencilState depthStencilState = {
      .compare_op = SDL_GPU_COMPAREOP_LESS,
      .enable_depth_test = true,
      .enable_depth_write = true,
    };
    pipelineInfo.depth_stencil_state = depthStencilState;
    pipelineInfo.target_info.has_depth_stencil_target = true;
    pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;

    pipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineInfo);
    SDL_ASSERT(pipeline != nullptr);

    {
      SDL_GPUTextureCreateInfo textureInfo = {};
      textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
      textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
      textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
      textureInfo.num_levels = 1;
      textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
      textureInfo.layer_count_or_depth = 1;

      int w, h, channels;
      u8 *pixels = stbi_load(TEXTURE_BIN_DIR "/default.png", &w, &h, &channels, 4);
      ASSERT(pixels != nullptr, TEXTURE_BIN_DIR "/default.png failed to load");
      defer(stbi_image_free(pixels));

      u32 tWidth = static_cast<u32>(w);
      u32 tHeight = static_cast<u32>(h);
      textureInfo.width = tWidth;
      textureInfo.height = tHeight;
      const u32 TEXTURE_SIZE = tWidth * tHeight * 4;

      checkTexture = SDL_CreateGPUTexture(gpuDevice, &textureInfo);
      SDL_ASSERT(checkTexture != nullptr);

      SDL_GPUTransferBufferCreateInfo transferToTextureInfo = {
	.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	.size = TEXTURE_SIZE,
      };
      SDL_GPUTransferBuffer *transferToTexture = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToTextureInfo);
      SDL_ASSERT(transferToTexture != nullptr);

      u8 *mappedTransferToBytes =
	reinterpret_cast<u8 *>(SDL_MapGPUTransferBuffer(gpuDevice, transferToTexture, false));
      SDL_ASSERT(mappedTransferToBytes != nullptr);
      std::memcpy(mappedTransferToBytes, pixels, TEXTURE_SIZE);

      SDL_GPUTextureTransferInfo textureTransferInfo = {
	.transfer_buffer = transferToTexture,
	.offset = 0,
	.pixels_per_row = tWidth,
	.rows_per_layer = tHeight,
      };
      SDL_GPUTextureRegion textureRegion = {
	.texture = checkTexture,
	.mip_level = 0,
	.layer = 0,
	.x = 0,
	.y = 0,
	.z = 0,
	.w = tWidth,
	.h = tHeight,
	.d = 1,
      };
      SDL_GPUCommandBuffer *initCommandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
      SDL_ASSERT(initCommandBuffer != nullptr);
      SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(initCommandBuffer);
      SDL_ASSERT(copyPass != nullptr);
      SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
      SDL_EndGPUCopyPass(copyPass);
      SDL_ASSERT(SDL_SubmitGPUCommandBuffer(initCommandBuffer));

      SDL_UnmapGPUTransferBuffer(gpuDevice, transferToTexture);
      SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToTexture);

      SDL_GPUSamplerCreateInfo samplerInfo = {
	.min_filter = SDL_GPU_FILTER_NEAREST,
	.mag_filter = SDL_GPU_FILTER_NEAREST,
	.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST,
	.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
	.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE,
      };
      sampler = SDL_CreateGPUSampler(gpuDevice, &samplerInfo);
      SDL_ASSERT(sampler != nullptr);

      textureSamplerBindings[0].sampler = sampler;
      textureSamplerBindings[0].texture = checkTexture;

      SDL_GPUTextureCreateInfo depthTextureInfo = {
	.type = SDL_GPU_TEXTURETYPE_2D,
	.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT,
	.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET,
	.width = (u32)config.resolution.x,
	.height = (u32)config.resolution.y,
	.layer_count_or_depth = 1,
	.num_levels = 1,
	.sample_count = SDL_GPU_SAMPLECOUNT_1,
      };
      depthTexture = SDL_CreateGPUTexture(gpuDevice, &depthTextureInfo);
      SDL_ASSERT(depthTexture != nullptr);
    }

    return true;
  }

  void rebuild(const GeometryStorage &geometry) {
    ASSERT(gpuDevice != nullptr, "renderer not initialized");

    if (vertexBuffer) {
      SDL_ReleaseGPUBuffer(gpuDevice, vertexBuffer);
      vertexBuffer = nullptr;
    }
    if (indexBuffer) {
      SDL_ReleaseGPUBuffer(gpuDevice, indexBuffer);
      indexBuffer = nullptr;
    }

    const u32 VERTEX_BUFFER_SIZE = geometry.vertices.size() * sizeof(Vertex);
    SDL_GPUBufferCreateInfo vertexBufferInfo = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
      .size = VERTEX_BUFFER_SIZE,
    };
    vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufferInfo);
    SDL_ASSERT(vertexBuffer != nullptr);

    const u32 INDEX_BUFFER_SIZE = geometry.indices.size() * sizeof(u32);
    SDL_GPUBufferCreateInfo indexBufferInfo = {
      .usage = SDL_GPU_BUFFERUSAGE_INDEX,
      .size = INDEX_BUFFER_SIZE,
    };
    indexBuffer = SDL_CreateGPUBuffer(gpuDevice, &indexBufferInfo);
    SDL_ASSERT(indexBuffer != nullptr);

    SDL_GPUBufferCreateInfo instanceBufferInfo = {
      .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
      .size = INSTANCE_PREALLOCATED_SIZE,
    };
    instanceBuffer = SDL_CreateGPUBuffer(gpuDevice, &instanceBufferInfo);
    SDL_ASSERT(instanceBuffer != nullptr);

    SDL_GPUTransferBufferCreateInfo transferToInstanceBufferInfo = {
      .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
      .size = INSTANCE_PREALLOCATED_SIZE,
    };
    transferToInstanceBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToInstanceBufferInfo);
    SDL_ASSERT(transferToInstanceBuffer != nullptr);

    if (VERTEX_BUFFER_SIZE > 0) {
      SDL_GPUTransferBufferCreateInfo transferToVertexBufferInfo = {
	.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	.size = VERTEX_BUFFER_SIZE,
      };
      SDL_GPUTransferBuffer *transferToVertexBuffer =
	SDL_CreateGPUTransferBuffer(gpuDevice, &transferToVertexBufferInfo);
      SDL_ASSERT(transferToVertexBuffer != nullptr);

      Vertex *mappedVertexInTransfer =
	reinterpret_cast<Vertex *>(SDL_MapGPUTransferBuffer(gpuDevice, transferToVertexBuffer, false));
      SDL_ASSERT(mappedVertexInTransfer != nullptr);
      std::memcpy(mappedVertexInTransfer, geometry.vertices.data(), VERTEX_BUFFER_SIZE);

      SDL_GPUTransferBufferLocation transferToVertexBufferLocation = {
	.transfer_buffer = transferToVertexBuffer,
	.offset = 0,
      };
      SDL_GPUBufferRegion vertexBufferRegion = {
	.buffer = vertexBuffer,
	.offset = 0,
	.size = VERTEX_BUFFER_SIZE,
      };

      if (INDEX_BUFFER_SIZE > 0) {
	SDL_GPUTransferBufferCreateInfo transferToIndexBufferInfo = {
	  .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
	  .size = INDEX_BUFFER_SIZE,
	};
	SDL_GPUTransferBuffer *transferToIndexBuffer =
	  SDL_CreateGPUTransferBuffer(gpuDevice, &transferToIndexBufferInfo);
	SDL_ASSERT(transferToIndexBuffer != nullptr);

	u32 *mappedIndicesInTransfer =
	  reinterpret_cast<u32 *>(SDL_MapGPUTransferBuffer(gpuDevice, transferToIndexBuffer, false));
	SDL_ASSERT(mappedIndicesInTransfer != nullptr);
	std::memcpy(mappedIndicesInTransfer, geometry.indices.data(), INDEX_BUFFER_SIZE);

	SDL_GPUTransferBufferLocation transferToIndexBufferLocation = {
	  .transfer_buffer = transferToIndexBuffer,
	  .offset = 0,
	};
	SDL_GPUBufferRegion indexBufferRegion = {
	  .buffer = indexBuffer,
	  .offset = 0,
	  .size = INDEX_BUFFER_SIZE,
	};

	SDL_GPUCommandBuffer *syncCmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
	SDL_ASSERT(syncCmd != nullptr);
	SDL_GPUCopyPass *initialCopyPass = SDL_BeginGPUCopyPass(syncCmd);
	SDL_ASSERT(initialCopyPass != nullptr);

	SDL_UploadToGPUBuffer(initialCopyPass, &transferToVertexBufferLocation, &vertexBufferRegion, false);
	SDL_UploadToGPUBuffer(initialCopyPass, &transferToIndexBufferLocation, &indexBufferRegion, false);
	SDL_EndGPUCopyPass(initialCopyPass);
	SDL_ASSERT(SDL_SubmitGPUCommandBuffer(syncCmd));

	SDL_UnmapGPUTransferBuffer(gpuDevice, transferToVertexBuffer);
	SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToVertexBuffer);
	SDL_UnmapGPUTransferBuffer(gpuDevice, transferToIndexBuffer);
	SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToIndexBuffer);
      }
    }

    vertexBufferBinding[0].buffer = vertexBuffer;
    vertexBufferBinding[0].offset = 0;
    instanceBufferBinding[0].buffer = instanceBuffer;
    instanceBufferBinding[0].offset = 0;
    indexBufferBinding[0].buffer = indexBuffer;
    indexBufferBinding[0].offset = 0;
  }

  void destroy() {
    if (gpuDevice) {
      if (checkTexture)
	SDL_ReleaseGPUTexture(gpuDevice, checkTexture);
      if (depthTexture)
	SDL_ReleaseGPUTexture(gpuDevice, depthTexture);
      if (pipeline)
	SDL_ReleaseGPUGraphicsPipeline(gpuDevice, pipeline);
      if (fragmentShader)
	SDL_ReleaseGPUShader(gpuDevice, fragmentShader);
      if (vertexShader)
	SDL_ReleaseGPUShader(gpuDevice, vertexShader);
      if (vertexBuffer)
	SDL_ReleaseGPUBuffer(gpuDevice, vertexBuffer);
      if (indexBuffer)
	SDL_ReleaseGPUBuffer(gpuDevice, indexBuffer);
      if (instanceBuffer)
	SDL_ReleaseGPUBuffer(gpuDevice, instanceBuffer);
      if (transferToInstanceBuffer)
	SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToInstanceBuffer);
      if (sampler)
	SDL_ReleaseGPUSampler(gpuDevice, sampler);
      SDL_DestroyGPUDevice(gpuDevice);
    }

    if (window)
      SDL_DestroyWindow(window);
    SDL_Quit();

    window = nullptr;
    gpuDevice = nullptr;
    vertexBuffer = nullptr;
    indexBuffer = nullptr;
    instanceBuffer = nullptr;
    transferToInstanceBuffer = nullptr;
    checkTexture = nullptr;
    depthTexture = nullptr;
    pipeline = nullptr;
    sampler = nullptr;
    vertexShader = nullptr;
    fragmentShader = nullptr;
    vertexBufferBinding[0].buffer = nullptr;
    instanceBufferBinding[0].buffer = nullptr;
    indexBufferBinding[0].buffer = nullptr;
    textureSamplerBindings[0].texture = nullptr;
    textureSamplerBindings[0].sampler = nullptr;
  }
};

//-----------------------------------------------------------------------------
// Input: polls SDL events, exposes keyboard / mouse / event-buffer queries.
//-----------------------------------------------------------------------------
struct Input {
  struct Keyboard {
    bool current[KEY_COUNT] = {};
    bool previous[KEY_COUNT] = {};

    void begin() {
      _copy_current_state_into_previous_state();
    }
    void end() {
      const bool *sdl_state = SDL_GetKeyboardState(nullptr);
      memset(current, 0, sizeof(current));
      for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
        if (sdl_state[i]) {
          Key k = SDL_Scancode_to_Key((SDL_Scancode)i);
          if (k != KEY_UNKNOWN)
            current[k] = true;
        }
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
    void clear() {
      memset(current, 0, sizeof(current));
    }

    void _copy_current_state_into_previous_state() {
      memcpy(previous, current, sizeof(current));
    }
  };

  struct Mouse {
    f32 x = 0, y = 0;
    f32 dx = 0, dy = 0;
    f32 wheel_x = 0, wheel_y = 0;
    bool down_current[MOUSE_BUTTON_COUNT] = {};
    bool down_previous[MOUSE_BUTTON_COUNT] = {};

    glm::vec2 position() const {
      return {x, y};
    }
    glm::vec2 delta() const {
      return {dx, dy};
    }
    glm::vec2 wheel() const {
      return {wheel_x, wheel_y};
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

    void clear() {
      dx = 0;
      dy = 0;
      wheel_x = 0;
      wheel_y = 0;
    }
    void begin() {
      _copy_current_state_into_previous_state();
      clear();
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

    void _copy_current_state_into_previous_state() {
      memcpy(down_previous, down_current, sizeof(down_current));
    }
  };

  Keyboard keyboard;
  Mouse mouse;

  GPUResources &gpu;
  u64 prevCounter = 0;

  Input(GPUResources &g) : gpu(g) {
    prevCounter = SDL_GetPerformanceCounter();
  }

  struct Events {
    InputEvent buffer[INPUT_EVENT_CAPACITY] = {};
    u32 count = 0;
    bool closeRequested = false;

    void push(InputEvent e) {
      if (count >= INPUT_EVENT_CAPACITY)
	return;
      buffer[count++] = e;
    }
    void clear() {
      count = 0;
      closeRequested = false;
    }
    void request_close() {
      closeRequested = true;
    }
    bool close_requested() const {
      return closeRequested;
    }
    u32 size() const {
      return count;
    }
    InputEvent &operator[](u32 index) {
      ASSERT(index < count, "event index out of bounds");
      return buffer[index];
    }
  };

  Events events;
  void begin() {
    keyboard.begin();
    mouse.begin();
    events.clear();
  }
  void poll() {
    begin();

    ImGuiIO &io = ImGui::GetIO();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL3_ProcessEvent(&event);

      switch (event.type) {
      case SDL_EVENT_QUIT: {
	events.request_close();
	break;
      }
      case SDL_EVENT_MOUSE_MOTION: {
	const f32 xrel = io.WantCaptureMouse ? 0.0f : event.motion.xrel;
	const f32 yrel = io.WantCaptureMouse ? 0.0f : event.motion.yrel;
	mouse.on_motion(event.motion.x, event.motion.y, xrel, yrel);
	events.push(
                    {INPUT_EVENT_MOUSE_MOTION,
                     KEY_UNKNOWN,
                     MOUSE_BUTTON_NONE,
                     false,
                     false,
                     event.motion.x,
                     event.motion.y,
                     xrel,
                     yrel,
                     0.0f,
                     0.0f}
		    );
	break;
      }
      case SDL_EVENT_MOUSE_BUTTON_DOWN: {
	const MouseButton button = SDL_MouseButton_to_MouseButton(event.button.button);
	mouse.on_button_down(button, event.button.x, event.button.y, !io.WantCaptureMouse);
	events.push(
                    {INPUT_EVENT_MOUSE_BUTTON_DOWN,
                     KEY_UNKNOWN,
                     button,
                     true,
                     false,
                     event.button.x,
                     event.button.y,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f}
		    );
	break;
      }
      case SDL_EVENT_MOUSE_BUTTON_UP: {
	const MouseButton button = SDL_MouseButton_to_MouseButton(event.button.button);
	mouse.on_button_up(button, event.button.x, event.button.y);
	events.push(
                    {INPUT_EVENT_MOUSE_BUTTON_UP,
                     KEY_UNKNOWN,
                     button,
                     false,
                     false,
                     event.button.x,
                     event.button.y,
                     0.0f,
                     0.0f,
                     0.0f,
                     0.0f}
		    );
	break;
      }
      case SDL_EVENT_MOUSE_WHEEL: {
	if (!io.WantCaptureMouse) {
	  mouse.on_wheel(event.wheel.x, event.wheel.y);
	  events.push(
		      {INPUT_EVENT_MOUSE_WHEEL,
		       KEY_UNKNOWN,
		       MOUSE_BUTTON_NONE,
		       false,
		       false,
		       mouse.x,
		       mouse.y,
		       0.0f,
		       0.0f,
		       event.wheel.x,
		       event.wheel.y}
		      );
	}
	break;
      }
      case SDL_EVENT_KEY_DOWN:
      case SDL_EVENT_KEY_UP: {
	if (!io.WantCaptureKeyboard) {
	  const Key key = SDL_Scancode_to_Key(event.key.scancode);
	  const bool down = event.type == SDL_EVENT_KEY_DOWN;
	  events.push(
		      {down ? INPUT_EVENT_KEY_DOWN : INPUT_EVENT_KEY_UP,
		       key,
		       MOUSE_BUTTON_NONE,
		       down,
		       event.key.repeat,
		       0.0f,
		       0.0f,
		       0.0f,
		       0.0f,
		       0.0f,
		       0.0f}
		      );
	}
	break;
      }
      }
    }

    mouse.end();
    keyboard.end();
    if (io.WantCaptureKeyboard)
      keyboard.clear();
  }

  void set_cursor_captured(bool captured) {
    gpu.request_cursor_capture(captured);
  }
  bool cursor_captured() {
    return gpu.cursor_captured();
  }

  void clear() {
    keyboard.clear();
    mouse.clear();
    events.clear();
  }
  
  f32 time_dt() {
    static const u64 timerFrequency = SDL_GetPerformanceFrequency();
    const u64 now = SDL_GetPerformanceCounter();
    const f32 dt = static_cast<f32>(now - prevCounter) / static_cast<f32>(timerFrequency);
    prevCounter = now;
    return dt;
  }
};

//-----------------------------------------------------------------------------
// ui: stateless wrapper over the (global) ImGui API, so a namespace rather
// than a struct.
//-----------------------------------------------------------------------------
namespace ui {
  inline bool init(GPUResources &gpu) {
    if (ImGui::GetCurrentContext() != nullptr) {
      std::fprintf(stderr, "ui::init: already initialized (call shutdown first)\n");
      return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO &io = ImGui::GetIO();
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

    ImGui::StyleColorsDark();
    ImGuiStyle &style = ImGui::GetStyle();
    style.ScaleAllSizes(1.0f);
    style.FontScaleDpi = 1.25f;

    ImGui_ImplSDL3_InitForSDLGPU(gpu.window);
    ImGui_ImplSDLGPU3_InitInfo init_info = {
      .Device = gpu.gpuDevice,
      .ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpu.gpuDevice, gpu.window),
      .MSAASamples = SDL_GPU_SAMPLECOUNT_1,
      .SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
      .PresentMode = SDL_GPU_PRESENTMODE_VSYNC,
    };
    ImGui_ImplSDLGPU3_Init(&init_info);
    return true;
  }

  inline void shutdown() {
    if (!ImGui::GetCurrentContext())
      return;
    ImGuiIO &io = ImGui::GetIO();
    if (io.BackendRendererUserData)
      ImGui_ImplSDLGPU3_Shutdown();
    if (io.BackendPlatformUserData)
      ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
  }

  inline void begin_frame() {
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
  }

  inline ImDrawData *end_frame() {
    ImGui::Render();
    return ImGui::GetDrawData();
  }

  inline void prepare(ImDrawData *draw_data, SDL_GPUCommandBuffer *commandBuffer) {
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, commandBuffer);
  }

  inline void render(ImDrawData *draw_data, SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) {
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, commandBuffer, renderPass);
  }

  inline bool begin_window(const char *name) {
    return ImGui::Begin(name);
  }
  inline void end_window() {
    ImGui::End();
  }
  inline bool button(const char *label) {
    return ImGui::Button(label);
  }
  inline bool begin_tree(const char *label) {
    return ImGui::TreeNode(label);
  }
  inline void end_tree() {
    ImGui::TreePop();
  }
  inline bool selectable(const char *label, bool selected) {
    return ImGui::Selectable(label, selected);
  }
  inline void sameline() {
    ImGui::SameLine();
  }
  inline void text(const char *text) {
    ImGui::Text("%s", text);
  }
  inline void push_id(int id) {
    ImGui::PushID(id);
  }
  inline void pop_id() {
    ImGui::PopID();
  }
  inline bool begin_context_menu_item() {
    return ImGui::BeginPopupContextItem();
  }
  inline bool begin_context_menu_window() {
    return ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems);
  }
  inline void end_context_menu() {
    ImGui::EndPopup();
  }
  inline bool menu_item(const char *label) {
    return ImGui::MenuItem(label);
  }
  inline bool item_double_clicked(MouseButton button) {
    return ImGui::IsMouseDoubleClicked(MouseButton_to_imgui_button(button)) && ImGui::IsItemHovered(ImGuiHoveredFlags_None);
  }
  inline bool item_clicked(MouseButton button) {
    return ImGui::IsItemClicked(MouseButton_to_imgui_button(button));
  }
}

//-----------------------------------------------------------------------------
// RendererFrame: per-frame draw-pass bookkeeping + draw submission. Holds
// references to the renderer-owned GPU / geometry / scene / config pieces.
//-----------------------------------------------------------------------------

struct DrawBatch {
  u32 vertexOffset;
  u32 indexOffset;
  u32 indexCount;
  MaterialHandle materialHandle;
  u32 firstInstance;
  u32 instanceCount;
};

struct RendererFrame {
  SDL_GPURenderPass *renderPass = nullptr;
  SDL_GPUCommandBuffer *commandBuffer = nullptr;
  SDL_GPUTexture *swapchainTexture = nullptr;
  u32 swapchainWidth = 0;
  u32 swapchainHeight = 0;
  SDL_GPUColorTargetInfo colorTargetInfo;
  SDL_GPUDepthStencilTargetInfo depthTargetInfo;
  std::vector<DrawBatch> drawBatches;

  GPUResources &gpu;
  GeometryStorage &geometry;
  MaterialStorage &materials;
  Scene &scene;
  Camera &camera;
  RendererConfig &config;

  RendererFrame(GPUResources &g, GeometryStorage &geo, MaterialStorage &mat, Scene &s, Camera &cam, RendererConfig &c)
    : gpu(g), geometry(geo), materials(mat), scene(s), camera(cam), config(c) {
  }

  void init() {
    colorTargetInfo = {
      .clear_color =
      {config.backgroundColor.r, config.backgroundColor.g, config.backgroundColor.b, config.backgroundColor.a},
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_STORE,
    };
    depthTargetInfo = {
      .texture = gpu.depthTexture,
      .clear_depth = 1.0f,
      .load_op = SDL_GPU_LOADOP_CLEAR,
      .store_op = SDL_GPU_STOREOP_DONT_CARE,
    };
  }
  void begin() {
    commandBuffer = SDL_AcquireGPUCommandBuffer(gpu.gpuDevice);
    SDL_ASSERT(commandBuffer != nullptr);

    SDL_ASSERT(SDL_WaitAndAcquireGPUSwapchainTexture(
					     commandBuffer, gpu.window, &swapchainTexture, &swapchainWidth, &swapchainHeight
					     ));

    colorTargetInfo.texture = swapchainTexture;
  }
  void prepare() {
    drawBatches.clear();
    // Collect work
    u32 i = 0;
    while (i < scene.size()) {
      MeshHandle meshHandle = scene[i].meshHandle;
      u32 j = i;
      while (j < scene.size() && scene[j].meshHandle == meshHandle)
	j++;
      auto regionIt = geometry.meshRegions.find(meshHandle);
      ASSERT(regionIt != geometry.meshRegions.end(), "mesh never synced - call rebuild first");
      auto& meshRegion = regionIt->second;
      for (auto &materialRegion : meshRegion.materialRegions) {
          drawBatches.push_back(
              DrawBatch{
		.vertexOffset = meshRegion.vertexOffset,
		.indexOffset = meshRegion.indexOffset + materialRegion.indexOffset,
		.indexCount = materialRegion.indexCount,
		.materialHandle = materialRegion.materialHandle,
		.firstInstance = i,
		.instanceCount = j - i
	  });
      }
      i = j;
    }

    // Upload
    u32 count = scene.size();
    
    SDL_GPUCopyPass *frameCopyPass = SDL_BeginGPUCopyPass(commandBuffer);
    SDL_ASSERT(frameCopyPass != nullptr);

    const u32 requiredInstanceCopySize = count * sizeof(glm::mat4);
    ASSERT(
	   requiredInstanceCopySize <= gpu.INSTANCE_PREALLOCATED_SIZE,
	   "instance buffer overflow - raise INSTANCE_PREALLOCATED_SIZE"
	   );

    glm::mat4 *mappedToInstanceInTransfer =
      reinterpret_cast<glm::mat4 *>(SDL_MapGPUTransferBuffer(gpu.gpuDevice, gpu.transferToInstanceBuffer, false));
    SDL_ASSERT(mappedToInstanceInTransfer != nullptr);

    glm::mat4 *dst = (glm::mat4 *)((u8 *)mappedToInstanceInTransfer);
    for (u64 i = 0; i < count; i++)
      *dst++ = scene.instances[i].matrix();

    SDL_GPUTransferBufferLocation transferToInstanceBufferLocation = {
      .transfer_buffer = gpu.transferToInstanceBuffer,
      .offset = 0,
    };
    SDL_GPUBufferRegion instanceBufferRegion = {
      .buffer = gpu.instanceBuffer,
      .offset = 0,
      .size = requiredInstanceCopySize,
    };

    SDL_UploadToGPUBuffer(frameCopyPass, &transferToInstanceBufferLocation, &instanceBufferRegion, false);
    SDL_EndGPUCopyPass(frameCopyPass);

  }
  void render_scene() {
    renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
    SDL_ASSERT(renderPass != nullptr);

    glm::mat4 view = camera.viewMatrix();
    glm::mat4 projection =
      glm::perspectiveLH_ZO(glm::radians(60.0f), config.resolution.x / config.resolution.y, 0.1f, 10000.0f);
    glm::mat4 viewProjection = projection * view;

    if (!drawBatches.empty()) {
      SDL_BindGPUGraphicsPipeline(renderPass, gpu.pipeline);
      SDL_PushGPUVertexUniformData(commandBuffer, 0, &viewProjection, (u32)sizeof(viewProjection));
      SDL_PushGPUFragmentUniformData(commandBuffer, 0, &config.resolution, (u32)sizeof(config.resolution));
      SDL_BindGPUVertexBuffers(renderPass, 0, gpu.vertexBufferBinding, 1);
      SDL_BindGPUVertexBuffers(renderPass, 1, gpu.instanceBufferBinding, 1);
      SDL_BindGPUIndexBuffer(renderPass, gpu.indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
      SDL_BindGPUFragmentSamplers(renderPass, 0, gpu.textureSamplerBindings, 1);
    }

    // Draw
    for (auto &drawBatch : drawBatches) {
      auto vertexOffset = drawBatch.vertexOffset;
      auto indexOffset = drawBatch.indexOffset;
      auto indexCount = drawBatch.indexCount;
      auto materialHandle = drawBatch.materialHandle;
      auto instanceStart = drawBatch.firstInstance;
      auto instanceCount = drawBatch.instanceCount;
      auto material = materials.get(materialHandle);
      SDL_PushGPUFragmentUniformData(commandBuffer, 1, &material.baseColor, (u32)sizeof(glm::vec4));
      _draw_batch(vertexOffset, indexOffset, indexCount, instanceStart, instanceCount);
    }
  }

  void end() {
    SDL_EndGPURenderPass(renderPass);
    SDL_ASSERT(SDL_SubmitGPUCommandBuffer(commandBuffer));
  }

  void _draw_batch(u32 vertexOffset, u32 indexOffset, u32 indexCount, u32 instanceStart, u32 instanceCount){
    SDL_DrawGPUIndexedPrimitives(
				 renderPass,
				 indexCount,
				 instanceCount,
				 indexOffset,
				 vertexOffset,
				 0
				 );
  }
};

//-----------------------------------------------------------------------------
// Renderer: sole owner of GPUResources, geometry storage, scene, input, and
// the per-frame draw state. The app drives it through renderer.<member>.*.
//-----------------------------------------------------------------------------
u64 load_obj(const char *path, GeometryStorage &g, MaterialStorage &m);
struct Renderer {
  GPUResources gpu;
  GeometryStorage geometry;
  MaterialStorage materialStorage;
  Scene scene;
  Camera camera;
  RendererConfig config;

  Input input;
  RendererFrame frame;

  Renderer() : input(gpu), frame(gpu, geometry, materialStorage, scene, camera, config) {
  }

  bool init() {
    if (gpu.initialized()) {
      std::fprintf(stderr, "Renderer::init: already initialized (call quit first)\n");
      return false;
    }

    config = RendererConfig{
      .resolution = {1800.0f, 1000.0f},
      .backgroundColor = {47.0f / 255.0f, 149.0f / 255.0f, 78.0f / 255.0f, 1.0f},
      .keyboardSensitivity = 200.0f,
      .mouseSensitivity = 0.1f,
    };
    camera = Camera{.position = {-5.0f, 0.0f, 0.0f}, .yaw = 0.0f, .pitch = 0.0f};

    input.clear();

    FPUEnvironment::save();
    gpu.init(config);
    if (!ui::init(gpu)) {
      FPUEnvironment::restore();
      gpu.destroy();
      return false;
    }
    frame.init();
    FPUEnvironment::restore();
    return true;
  }
  void quit() {
    ui::shutdown();
    gpu.destroy();
  }
  bool clear() {
    geometry.clear();
    materialStorage.clear();
    return true;
  }
  void rebuild() {
    gpu.rebuild(geometry);
  }
  u64 load_obj(const char *path) {
    return ::load_obj(path, geometry, materialStorage);
  }

  SDL_GPUCommandBuffer *command_buffer() {
    return frame.commandBuffer;
  }
  SDL_GPURenderPass *render_pass() {
    return frame.renderPass;
  }
};

//=============================================================================
// FOLDED-IN IMPLEMENTATIONS (was utils.cpp / obj.cpp / stb_image.cpp / cppengine.cpp)
//=============================================================================
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <map>
#include <random>
#include <tuple>
#include <vector>

#define TINYOBJLOADER_IMPLEMENTATION
#define TINYOBJLOADER_USE_MULTITHREADING
#define TINYOBJLOADER_USE_SIMD
#include "tiny_obj_loader.h"

//---- utils ------------------------------------------------------------------
static std::random_device rd;
static std::mt19937 gen(rd());
static std::uniform_real_distribution<float> dist(0.0f, 1.0f);

std::vector<u8> readFile(const char *path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    std::fprintf(stderr, "%s:%d: readFile: cannot open '%s'\n", __FILE__, __LINE__, path);
    std::abort();
  }
  std::streamsize size = file.tellg();
  if (size < 0) {
    std::fprintf(stderr, "%s:%d: readFile: cannot stat '%s'\n", __FILE__, __LINE__, path);
    std::abort();
  }
  file.seekg(0, std::ios::beg);
  std::vector<u8> buffer(static_cast<size_t>(size));
  if (size > 0) {
    file.read(reinterpret_cast<char *>(buffer.data()), size);
  }
  return buffer;
}

std::ostream &operator<<(std::ostream &os, const Vertex &v) {
  return os << "Vertex{"
	    << "position=(" << v.position.x << ", " << v.position.y << ", " << v.position.z << "), "
	    << "color=(" << v.color.r << ", " << v.color.g << ", " << v.color.b << ", " << v.color.a << "), "
	    << "normal=(" << v.normal.x << ", " << v.normal.y << ", " << v.normal.z << "), "
	    << "uv=(" << v.uv.x << ", " << v.uv.y << ")"
	    << "}";
}

std::ostream &operator<<(std::ostream &os, const glm::vec2 &v) {
  return os << "vec2(" << v.x << ", " << v.y << ")";
}

std::ostream &operator<<(std::ostream &os, const glm::vec3 &v) {
  return os << "vec3(" << v.x << ", " << v.y << ", " << v.z << ")";
}

std::ostream &operator<<(std::ostream &os, const glm::vec4 &v) {
  return os << "vec4(" << v.x << ", " << v.y << ", " << v.z << ", " << v.w << ")";
}

std::ostream &operator<<(std::ostream &os, const MeshRegion &r) {
  return os << "MeshRegion("
	    << "vertexOffset=" << r.vertexOffset << ", indexOffset=" << r.indexOffset
	    << ", indexCount=" << r.indexCount << ")";
}

glm::mat4 transformToMat4(const Transform& t)
{
  float x = glm::radians(t.rx);
  float y = glm::radians(t.ry);
  float z = glm::radians(t.rz);

  float cx = cos(x), sx = sin(x);
  float cy = cos(y), sy = sin(y);
  float cz = cos(z), sz = sin(z);

  return glm::mat4{
    cy * cz * t.sx,  cy * sz * t.sx, -sy * t.sx, 0.0f,
    (sx * sy * cz - cx * sz) * t.sy,
    (sx * sy * sz + cx * cz) * t.sy,
    sx * cy * t.sy, 0.0f,
    (cx * sy * cz + sx * sz) * t.sz,
    (cx * sy * sz - sx * cz) * t.sz,
    cx * cy * t.sz, 0.0f,
    t.px, t.py, t.pz, 1.0f
  };
}

//---- obj loading -------------------------------------------------------------
u64 load_obj(const char *path, GeometryStorage &g, MaterialStorage &m) {
  std::string warn, err;

  tinyobj::ObjReaderConfig readerConfig;
  readerConfig.vertex_color = true;
  readerConfig.triangulate = true;

  tinyobj::ObjReader reader;

  if (!reader.ParseFromFile(path, readerConfig)) {
    std::fprintf(
		 stderr, "%s:%d: obj load failed: '%s': %s\n", __FILE__, __LINE__, path, reader.Error().c_str()
		 );
    std::abort();
  }

  if (!reader.Warning().empty()) {
    std::cout << "TinyObjReader: " << reader.Warning();
  }

  auto &attrib = reader.GetAttrib();
  auto &shapes = reader.GetShapes();
  auto &objMaterials = reader.GetMaterials();

  std::vector<MaterialHandle> materialIdToMaterialHandle;
  materialIdToMaterialHandle.reserve(objMaterials.size());
  for (auto &mat : objMaterials) {
    Material engineMaterial{};
    engineMaterial.baseColor = {
      mat.diffuse[0], mat.diffuse[1], mat.diffuse[2], 1.0f,
    };
    materialIdToMaterialHandle.push_back(m.add(std::move(engineMaterial)));
  }

  Material defaultMaterial{};
  defaultMaterial.baseColor = glm::vec4(1.0f);
  MaterialHandle defaultMaterialHandle = m.add(std::move(defaultMaterial));

  for (u32 shapeIndex = 0; shapeIndex < shapes.size(); shapeIndex++) {
    auto &shape = shapes[shapeIndex];

    Mesh mesh{};
    mesh.alias = shape.name;
    std::map<std::tuple<int, int, int>, u32> vertexMap;
    u32 indexOffset = 0;
    for (u32 faceIndex = 0; faceIndex < shape.mesh.num_face_vertices.size(); faceIndex++) {

      int materialId = shape.mesh.material_ids[faceIndex];
      MaterialHandle faceMaterialHandle =
	materialId < 0 ? defaultMaterialHandle : materialIdToMaterialHandle[(u32)materialId];

      Vertex vertexStorage{};
      for (u32 vertIndex = 0; vertIndex < 3; vertIndex++) {
	auto idx = shape.mesh.indices[indexOffset + vertIndex];

	tinyobj::real_t vx = attrib.vertices[3 * idx.vertex_index + 0];
	tinyobj::real_t vy = attrib.vertices[3 * idx.vertex_index + 1];
	tinyobj::real_t vz = attrib.vertices[3 * idx.vertex_index + 2];

	vertexStorage.position = {vx, vy, vz};

	if (idx.normal_index >= 0) {
	  tinyobj::real_t nx = attrib.normals[3 * idx.normal_index + 0];
	  tinyobj::real_t ny = attrib.normals[3 * idx.normal_index + 1];
	  tinyobj::real_t nz = attrib.normals[3 * idx.normal_index + 2];

	  vertexStorage.normal = {nx, ny, nz};
	}

	if (idx.texcoord_index >= 0) {
	  tinyobj::real_t tx = attrib.texcoords[2 * idx.texcoord_index + 0];
	  tinyobj::real_t ty = attrib.texcoords[2 * idx.texcoord_index + 1];

	  vertexStorage.uv = {tx, ty};
	}

	if (!attrib.colors.empty()) {
	  tinyobj::real_t red = attrib.colors[3 * idx.vertex_index + 0];
	  tinyobj::real_t green = attrib.colors[3 * idx.vertex_index + 1];
	  tinyobj::real_t blue = attrib.colors[3 * idx.vertex_index + 2];
	  vertexStorage.color = {red, green, blue, 1.0f};
	} else {
	  vertexStorage.color = glm::vec4(1.0f);
	}

	auto key = std::make_tuple(idx.vertex_index, idx.texcoord_index, idx.normal_index);

	auto [it, inserted] = vertexMap.emplace(key, mesh.vertices.size());

	if (inserted) {
	  mesh.vertices.push_back(vertexStorage);
	}

	mesh.indices.push_back(it->second);
      }
      mesh.materialHandles.push_back(faceMaterialHandle);

      indexOffset += 3;
    }

    g.add(std::move(mesh));
  }

  return shapes.size();
}

//---- boundary helpers --------------------------------------------------------

Key SDL_Scancode_to_Key(SDL_Scancode scancode) {
  switch (scancode) {
  case SDL_SCANCODE_A: return KEY_A;
  case SDL_SCANCODE_B: return KEY_B;
  case SDL_SCANCODE_C: return KEY_C;
  case SDL_SCANCODE_D: return KEY_D;
  case SDL_SCANCODE_E: return KEY_E;
  case SDL_SCANCODE_F: return KEY_F;
  case SDL_SCANCODE_G: return KEY_G;
  case SDL_SCANCODE_H: return KEY_H;
  case SDL_SCANCODE_I: return KEY_I;
  case SDL_SCANCODE_J: return KEY_J;
  case SDL_SCANCODE_K: return KEY_K;
  case SDL_SCANCODE_L: return KEY_L;
  case SDL_SCANCODE_M: return KEY_M;
  case SDL_SCANCODE_N: return KEY_N;
  case SDL_SCANCODE_O: return KEY_O;
  case SDL_SCANCODE_P: return KEY_P;
  case SDL_SCANCODE_Q: return KEY_Q;
  case SDL_SCANCODE_R: return KEY_R;
  case SDL_SCANCODE_S: return KEY_S;
  case SDL_SCANCODE_T: return KEY_T;
  case SDL_SCANCODE_U: return KEY_U;
  case SDL_SCANCODE_V: return KEY_V;
  case SDL_SCANCODE_W: return KEY_W;
  case SDL_SCANCODE_X: return KEY_X;
  case SDL_SCANCODE_Y: return KEY_Y;
  case SDL_SCANCODE_Z: return KEY_Z;
  case SDL_SCANCODE_1: return KEY_1;
  case SDL_SCANCODE_2: return KEY_2;
  case SDL_SCANCODE_3: return KEY_3;
  case SDL_SCANCODE_4: return KEY_4;
  case SDL_SCANCODE_5: return KEY_5;
  case SDL_SCANCODE_6: return KEY_6;
  case SDL_SCANCODE_7: return KEY_7;
  case SDL_SCANCODE_8: return KEY_8;
  case SDL_SCANCODE_9: return KEY_9;
  case SDL_SCANCODE_0: return KEY_0;
  case SDL_SCANCODE_RETURN: return KEY_ENTER;
  case SDL_SCANCODE_ESCAPE: return KEY_ESCAPE;
  case SDL_SCANCODE_BACKSPACE: return KEY_BACKSPACE;
  case SDL_SCANCODE_TAB: return KEY_TAB;
  case SDL_SCANCODE_SPACE: return KEY_SPACE;
  case SDL_SCANCODE_MINUS: return KEY_MINUS;
  case SDL_SCANCODE_EQUALS: return KEY_EQUALS;
  case SDL_SCANCODE_LEFTBRACKET: return KEY_LEFT_BRACKET;
  case SDL_SCANCODE_RIGHTBRACKET: return KEY_RIGHT_BRACKET;
  case SDL_SCANCODE_BACKSLASH: return KEY_BACKSLASH;
  case SDL_SCANCODE_SEMICOLON: return KEY_SEMICOLON;
  case SDL_SCANCODE_APOSTROPHE: return KEY_APOSTROPHE;
  case SDL_SCANCODE_GRAVE: return KEY_GRAVE;
  case SDL_SCANCODE_COMMA: return KEY_COMMA;
  case SDL_SCANCODE_PERIOD: return KEY_PERIOD;
  case SDL_SCANCODE_SLASH: return KEY_SLASH;
  case SDL_SCANCODE_F1: return KEY_F1;
  case SDL_SCANCODE_F2: return KEY_F2;
  case SDL_SCANCODE_F3: return KEY_F3;
  case SDL_SCANCODE_F4: return KEY_F4;
  case SDL_SCANCODE_F5: return KEY_F5;
  case SDL_SCANCODE_F6: return KEY_F6;
  case SDL_SCANCODE_F7: return KEY_F7;
  case SDL_SCANCODE_F8: return KEY_F8;
  case SDL_SCANCODE_F9: return KEY_F9;
  case SDL_SCANCODE_F10: return KEY_F10;
  case SDL_SCANCODE_F11: return KEY_F11;
  case SDL_SCANCODE_F12: return KEY_F12;
  case SDL_SCANCODE_INSERT: return KEY_INSERT;
  case SDL_SCANCODE_HOME: return KEY_HOME;
  case SDL_SCANCODE_PAGEUP: return KEY_PAGE_UP;
  case SDL_SCANCODE_DELETE: return KEY_DELETE;
  case SDL_SCANCODE_END: return KEY_END;
  case SDL_SCANCODE_PAGEDOWN: return KEY_PAGE_DOWN;
  case SDL_SCANCODE_RIGHT: return KEY_RIGHT;
  case SDL_SCANCODE_LEFT: return KEY_LEFT;
  case SDL_SCANCODE_DOWN: return KEY_DOWN;
  case SDL_SCANCODE_UP: return KEY_UP;
  case SDL_SCANCODE_LCTRL: return KEY_LEFT_CTRL;
  case SDL_SCANCODE_LSHIFT: return KEY_LEFT_SHIFT;
  case SDL_SCANCODE_LALT: return KEY_LEFT_ALT;
  case SDL_SCANCODE_LGUI: return KEY_LEFT_GUI;
  case SDL_SCANCODE_RCTRL: return KEY_RIGHT_CTRL;
  case SDL_SCANCODE_RSHIFT: return KEY_RIGHT_SHIFT;
  case SDL_SCANCODE_RALT: return KEY_RIGHT_ALT;
  case SDL_SCANCODE_RGUI: return KEY_RIGHT_GUI;
  default: return KEY_UNKNOWN;
  }
}

MouseButton SDL_MouseButton_to_MouseButton(Uint8 button) {
  switch (button) {
  case SDL_BUTTON_LEFT: return MOUSE_BUTTON_LEFT;
  case SDL_BUTTON_MIDDLE: return MOUSE_BUTTON_MIDDLE;
  case SDL_BUTTON_RIGHT: return MOUSE_BUTTON_RIGHT;
  case SDL_BUTTON_X1: return MOUSE_BUTTON_X1;
  case SDL_BUTTON_X2: return MOUSE_BUTTON_X2;
  default: return (MouseButton)button;
  }
}

ImGuiMouseButton MouseButton_to_imgui_button(MouseButton button) {
  switch (button) {
  case MOUSE_BUTTON_LEFT: return ImGuiMouseButton_Left;
  case MOUSE_BUTTON_RIGHT: return ImGuiMouseButton_Right;
  case MOUSE_BUTTON_MIDDLE: return ImGuiMouseButton_Middle;
  case MOUSE_BUTTON_X1: return ImGuiMouseButton_Left;
  case MOUSE_BUTTON_X2: return ImGuiMouseButton_Right;
  default: return ImGuiMouseButton_Left;
  }
}
