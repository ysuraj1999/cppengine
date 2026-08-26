#include "cppengine.h"
#include "types.hpp"
#include <SDL3/SDL_gpu.h>
#include <algorithm>
#include <SDL3/SDL.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <cfloat>
#include <cstddef>
#include <glm/glm.hpp>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <vector>
#include <string>
#include <unordered_set>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <format>

#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_sdlgpu3.h"

#include "stb_image.h"
#include "types.hpp"
#include "obj.hpp"
#include "utils.hpp"

#include <cfenv>

static std::fenv_t env;

void fpu_begin()
{
  std::feholdexcept(&env);
}

void fpu_end()
{
  std::fesetenv(&env);
}

RendererConfig rendererConfig = {};
glm::vec3 WORLD_UP = glm::vec3(0.0f, 1.0f, 0.0f);

// Monotonically increasing counters
static_global uint64_t next_mesh_handle = 0;
static_global uint64_t next_instance_handle = 0;
static_global uint64_t next_scene_handle = 0;

static_global std::unordered_map<SceneHandle, Scene> sceneRegistry;
static_global std::vector<SceneHandle> sceneOrder;
static_global SDL_Window *window;
static_global SDL_GPUDevice *gpuDevice;
static_global SDL_GPUBuffer *vertexBuffer;
static_global SDL_GPUTexture *checkTexture;
static_global SDL_GPUShader *vertexShader;
static_global SDL_GPUShader *fragmentShader;
static_global SDL_GPUSampler *sampler;
static_global SDL_GPUBuffer* instanceBuffer;
static_global SDL_GPUBuffer* indexBuffer;
static_global SDL_GPUTexture* depthTexture;
static_global SDL_GPUGraphicsPipeline* pipeline;
static_global SDL_GPUBufferBinding vertexBufferBinding[1] = {};
static_global SDL_GPUBufferBinding instanceBufferBinding[1] = {};
static_global SDL_GPUBufferBinding indexBufferBinding[1] = {};
static_global SDL_GPUTextureSamplerBinding textureSamplerBindings[1] = {};
static_global SDL_GPUTransferBuffer* transferToInstanceBuffer;
static_global u32 INSTANCE_PREALLOCATED_SIZE = 10 * 1024 * 1024;
static_global RendererState rendererState;
static_global bool shouldClose = false;
static_global std::unordered_set<std::string> uiSelectedItems;

struct Keyboard {
  bool current[SDL_SCANCODE_COUNT] = {};
  bool previous[SDL_SCANCODE_COUNT] = {};
  KeyboardEvent buffer[KEYBOARD_EVENT_CAPACITY] = {};
  u32 count = 0;

  void copy_current_state_into_previous_state() {
    memcpy(previous, current, sizeof(current));
  }
  void clear_events() { count = 0; }

  void begin() {
    copy_current_state_into_previous_state();
    clear_events();
  }
  void end() { memcpy(current, SDL_GetKeyboardState(nullptr), sizeof(current)); }
  void clear_current_state() { memset(current, 0, sizeof(current)); }

  void push_event(int scancode, bool down, bool repeat) {
    if (count >= KEYBOARD_EVENT_CAPACITY) return;
    buffer[count++] = {scancode, down, repeat};
  }

  bool down(SDL_Scancode k) const { ASSERT(k >= 0 && k < SDL_SCANCODE_COUNT, "scancode out of range"); return current[k]; }
  bool pressed(SDL_Scancode k) const { ASSERT(k >= 0 && k < SDL_SCANCODE_COUNT, "scancode out of range"); return current[k] && !previous[k]; }
  bool released(SDL_Scancode k) const { ASSERT(k >= 0 && k < SDL_SCANCODE_COUNT, "scancode out of range"); return !current[k] && previous[k]; }
};

struct Mouse {
  f32 x = 0, y = 0;
  f32 dx = 0, dy = 0;
  f32 wheel_x = 0, wheel_y = 0;
  bool down_current[MOUSE_BUTTON_COUNT] = {};
  bool down_previous[MOUSE_BUTTON_COUNT] = {};

  void copy_current_state_into_previous_state() { memcpy(down_previous, down_current, sizeof(down_current)); }
  void clear_transient() { dx = 0; dy = 0; wheel_x = 0; wheel_y = 0; }

  void begin() {
    copy_current_state_into_previous_state();
    clear_transient();
  }
  void end() {}

  void on_motion(f32 nx, f32 ny, f32 xrel, f32 yrel) { x = nx; y = ny; dx += xrel; dy += yrel; }
  void on_button_down(int button, f32 bx, f32 by, bool track) { x = bx; y = by; if (track && button > 0 && button < MOUSE_BUTTON_COUNT) down_current[button] = true; }
  void on_button_up(int button, f32 bx, f32 by) { x = bx; y = by; if (button > 0 && button < MOUSE_BUTTON_COUNT) down_current[button] = false; }
  void on_wheel(f32 wx, f32 wy) { wheel_x += wx; wheel_y += wy; }

  bool down(MouseButton b) const { ASSERT(b > 0 && b < MOUSE_BUTTON_COUNT, "invalid mouse button"); return down_current[b]; }
  bool pressed(MouseButton b) const { ASSERT(b > 0 && b < MOUSE_BUTTON_COUNT, "invalid mouse button"); return down_current[b] && !down_previous[b]; }
  bool released(MouseButton b) const { ASSERT(b > 0 && b < MOUSE_BUTTON_COUNT, "invalid mouse button"); return !down_current[b] && down_previous[b]; }
};

struct Input {
  Keyboard keyboard;
  Mouse mouse;
  void poll();
};

static_global Input input = {};

static_global const u64 timerFrequency = SDL_GetPerformanceFrequency();
static_global RendererFrame frame = {};

SceneHandle scene_create() {
  SceneHandle sh = next_scene_handle++;
  sceneRegistry[sh] = Scene{};
  sceneOrder.push_back(sh);
  return sh;
}

bool scene_destroy(SceneHandle sh) {
  bool erased = sceneRegistry.erase(sh) > 0;
  if (erased) {
    auto it = std::find(sceneOrder.begin(), sceneOrder.end(), sh);
    sceneOrder.erase(it);
  }
  return erased;
}

static_internal Scene* scene_get(SceneHandle sh) {
  auto it = sceneRegistry.find(sh);
  ASSERT(it != sceneRegistry.end(), "invalid scene handle");
  return &it->second;
}

static_internal Mesh* mesh_get(Scene* scene, MeshHandle mh) {
  auto it = scene->meshes.find(mh);
  ASSERT(it != scene->meshes.end(), "invalid mesh handle");
  return &it->second;
}

static_internal Instance* instance_get(Scene* scene, InstanceHandle ih) {
  for (auto &[meshHandle, instances] : scene->objects) {
    for (auto &inst : instances) {
      if (inst.instanceHandle == ih)
        return &inst;
    }
  }
  ASSERT(false, "instance handle not found");
  return nullptr;
}

static_internal MeshHandle mesh_add(Scene* scene, Mesh m) {
  MeshHandle newMeshHandle = next_mesh_handle++;
  m.alias = (m.alias.empty() ? "Mesh" : m.alias) + "#" + std::to_string(newMeshHandle);
  scene->meshes[newMeshHandle] = m;
  scene->meshOrder.push_back(newMeshHandle);
  return newMeshHandle;
}

static_internal InstanceHandle instance_add(Scene* scene, MeshHandle m, Transform t) {
  Mesh* mesh = mesh_get(scene, m);
  Instance newInstance = {};
  newInstance.instanceHandle = next_instance_handle++;
  newInstance.mesh = m;
  newInstance.transform = t;
  newInstance.alias = mesh->alias + "-instance#" + std::to_string(newInstance.instanceHandle);
  scene->objects[m].push_back(newInstance);
  return newInstance.instanceHandle;
}

static_internal void camera_update(Camera& cam) {
  cam.forward = glm::normalize(glm::vec3{
      cos(glm::radians(cam.pitch)) * cos(glm::radians(cam.yaw)),
      sin(glm::radians(cam.pitch)),
      cos(glm::radians(cam.pitch)) * sin(glm::radians(cam.yaw))});
  cam.right = glm::cross(WORLD_UP, cam.forward);
}

// Lifecycle init check: log, release any partial state, fail gracefully.
#define INIT_CHECK(cond, msg)						\
  do {									\
    if (!(cond)) {							\
      std::fprintf(stderr, "%s:%d: renderer_init failed: %s (%s)\n",	\
                   __FILE__, __LINE__, #cond, msg);			\
      fpu_end();							\
      renderer_quit();							\
      return false;							\
    }									\
  } while (0)

bool ui_init() {
  if (ImGui::GetCurrentContext() != nullptr) {
    std::fprintf(stderr, "ui_init: already initialized (call ui_quit first)\n");
    return false;
  }
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO(); (void)io;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
  
  ImGui::StyleColorsDark();
  ImGuiStyle& style = ImGui::GetStyle();
  style.ScaleAllSizes(1.0f);
  style.FontScaleDpi = 1.25f;

  // Setup Platform/Renderer backends
  ImGui_ImplSDL3_InitForSDLGPU(window);
  ImGui_ImplSDLGPU3_InitInfo init_info = {};
  init_info.Device = gpuDevice;
  init_info.ColorTargetFormat = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window);
  init_info.MSAASamples = SDL_GPU_SAMPLECOUNT_1;                      // Only used in multi-viewports mode.
  init_info.SwapchainComposition = SDL_GPU_SWAPCHAINCOMPOSITION_SDR;  // Only used in multi-viewports mode.
  init_info.PresentMode = SDL_GPU_PRESENTMODE_VSYNC;
  ImGui_ImplSDLGPU3_Init(&init_info);
  return true;
}

void ui_quit() {
  if (!ImGui::GetCurrentContext())
    return;
  ImGuiIO& io = ImGui::GetIO();
  if (io.BackendRendererUserData)
    ImGui_ImplSDLGPU3_Shutdown();
  if (io.BackendPlatformUserData)
    ImGui_ImplSDL3_Shutdown();
  ImGui::DestroyContext();
}

bool renderer_init() {
  if (gpuDevice != nullptr || window != nullptr) {
    std::fprintf(stderr,
                 "renderer_init: already initialized (call renderer_quit first)\n");
    return false;
  }

  fpu_begin();
  RendererConfig config = {
    .resolution = {1800.0f, 1000.0f},
    .backgroundColor = {47.0f / 255.0f, 149.0f / 255.0f, 78.0f / 255.0f,
			1.0f},
    .keyboardSensitivity = 200.0f,
    .mouseSensitivity = 0.1f,
    .cameraEnabled = true,
    .camera = {.position = {-5.0f, 0.0f, 0.0f}, .yaw = 0.0f, .pitch = 0.0f},
  };

  shouldClose = false;

  rendererConfig = config;

  camera_update(rendererConfig.camera);

  INIT_CHECK(SDL_Init(SDL_INIT_VIDEO), "SDL subsystem initialization");

  window = SDL_CreateWindow("Lens", (u32)rendererConfig.resolution.x, (u32)rendererConfig.resolution.y, 0);
  INIT_CHECK(window != nullptr, "window creation");

  // TODO - flag cameraRelative
  // if (rendererConfig.cameraEnabled)
  //   sdlAssert(SDL_SetWindowRelativeMouseMode(window, true));

  gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
  INIT_CHECK(gpuDevice != nullptr, "GPU device creation");

  INIT_CHECK(SDL_ClaimWindowForGPUDevice(gpuDevice, window),
             "claim window for GPU device");

  INIT_CHECK(SDL_SetGPUSwapchainParameters(gpuDevice, window,
					   SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
					   SDL_GPU_PRESENTMODE_VSYNC),
             "swapchain configuration");

  INIT_CHECK(ui_init(), "imgui initialization");
  // IMGUI END
  
  frame.swapchainTexture = nullptr;
  frame.swapchainWidth = 0;
  frame.swapchainHeight = 0;

  std::vector<u8> vertexShaderContents =
    readFile(SHADER_BIN_DIR "/shader.vertex.spv");
  ASSERT(!vertexShaderContents.empty(),
         SHADER_BIN_DIR "/shader.vertex.spv missing or empty");

  SDL_GPUShaderCreateInfo vertexShaderInfo = {};
  vertexShaderInfo.entrypoint = "main";
  vertexShaderInfo.code = vertexShaderContents.data();
  vertexShaderInfo.code_size = vertexShaderContents.size();
  vertexShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
  vertexShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
  vertexShaderInfo.num_samplers = 0;
  vertexShaderInfo.num_storage_textures = 0;
  vertexShaderInfo.num_storage_buffers = 0;
  vertexShaderInfo.num_uniform_buffers = 1;

  vertexShader = SDL_CreateGPUShader(gpuDevice, &vertexShaderInfo);
  INIT_CHECK(vertexShader != nullptr, "vertex shader creation");

  std::vector<u8> fragmentShaderContents =
    readFile(SHADER_BIN_DIR "/shader.fragment.spv");
  ASSERT(!fragmentShaderContents.empty(),
         SHADER_BIN_DIR "/shader.fragment.spv missing or empty");

  SDL_GPUShaderCreateInfo fragmentShaderInfo = {};
  fragmentShaderInfo.entrypoint = "main";
  fragmentShaderInfo.code = fragmentShaderContents.data();
  fragmentShaderInfo.code_size = fragmentShaderContents.size();
  fragmentShaderInfo.stage = SDL_GPU_SHADERSTAGE_FRAGMENT;
  fragmentShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
  fragmentShaderInfo.num_samplers = 1;
  fragmentShaderInfo.num_storage_textures = 0;
  fragmentShaderInfo.num_storage_buffers = 0;
  fragmentShaderInfo.num_uniform_buffers = 1;

  fragmentShader = SDL_CreateGPUShader(gpuDevice, &fragmentShaderInfo);
  INIT_CHECK(fragmentShader != nullptr, "fragment shader creation");

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.vertex_shader = vertexShader;
  pipelineInfo.fragment_shader = fragmentShader;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

  SDL_GPUVertexBufferDescription vertexBufferDescription[2] = {};

  // How each vertex is defined
  vertexBufferDescription[0].slot = 0;
  vertexBufferDescription[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  vertexBufferDescription[0].instance_step_rate = 0;
  vertexBufferDescription[0].pitch = (u32)sizeof(Vertex);

  vertexBufferDescription[1].slot = 1;
  vertexBufferDescription[1].input_rate = SDL_GPU_VERTEXINPUTRATE_INSTANCE;
  vertexBufferDescription[1].instance_step_rate = 1;
  vertexBufferDescription[1].pitch = (u32)sizeof(glm::mat4);

  
  pipelineInfo.vertex_input_state.num_vertex_buffers = 2;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescription;

  // Vertex attributes
  SDL_GPUVertexAttribute vertexAttributes[8] = {};
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

  // Instance attributes
  vertexAttributes[4] = {
    .location = 4,
    .buffer_slot = 1,
    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
    .offset = 0};

  vertexAttributes[5] = {
    .location = 5,
    .buffer_slot = 1,
    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
    .offset = sizeof(glm::vec4)};

  vertexAttributes[6] = {
    .location = 6,
    .buffer_slot = 1,
    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
    .offset = 2 * sizeof(glm::vec4)};

  vertexAttributes[7] = {
    .location = 7,
    .buffer_slot = 1,
    .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4,
    .offset = 3 * sizeof(glm::vec4)};

  pipelineInfo.vertex_input_state.num_vertex_attributes = 8;
  pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;
  
  SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {};
  colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window);

  pipelineInfo.target_info.num_color_targets = 1;
  pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

  // Culling
  pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
  pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_CLOCKWISE;

  // Depth test
  SDL_GPUDepthStencilState depthStencilState = {};
  depthStencilState.enable_depth_write = true;
  depthStencilState.enable_depth_test = true;
  depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS;

  pipelineInfo.depth_stencil_state = depthStencilState;
  pipelineInfo.target_info.has_depth_stencil_target = true;
  pipelineInfo.target_info.depth_stencil_format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
      
  pipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineInfo);
  INIT_CHECK(pipeline != nullptr, "graphics pipeline creation");

  {
    SDL_GPUTextureCreateInfo textureInfo = {};
    textureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    textureInfo.format = SDL_GPU_TEXTUREFORMAT_R8G8B8A8_UNORM;
    textureInfo.usage = SDL_GPU_TEXTUREUSAGE_SAMPLER;
    textureInfo.num_levels = 1;
    textureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    textureInfo.layer_count_or_depth = 1;

    // LOAD TEXTURE
    int w, h, channels;
    u8 *pixels =
      stbi_load(TEXTURE_BIN_DIR "/default.png", &w, &h, &channels, 4);
    ASSERT(pixels != nullptr, TEXTURE_BIN_DIR "/default.png failed to load");
    defer(stbi_image_free(pixels));
      
    u32 tWidth = static_cast<u32>(w);
    u32 tHeight = static_cast<u32>(h);
    textureInfo.width = tWidth;
    textureInfo.height = tHeight;
    const u32 TEXTURE_SIZE = tWidth * tHeight * 4;

    checkTexture = SDL_CreateGPUTexture(gpuDevice, &textureInfo);
    INIT_CHECK(checkTexture != nullptr, "default texture creation");

    
    SDL_GPUTransferBufferCreateInfo transferToTextureInfo = {};
    transferToTextureInfo.size = TEXTURE_SIZE;
    transferToTextureInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transferToTexture = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToTextureInfo);
    INIT_CHECK(transferToTexture != nullptr, "texture transfer buffer creation");

    u8* mappedTransferToBytes = reinterpret_cast<u8*>(
						      SDL_MapGPUTransferBuffer(gpuDevice, transferToTexture, false));
    INIT_CHECK(mappedTransferToBytes != nullptr, "texture transfer buffer map");
    std::memcpy(mappedTransferToBytes, pixels, TEXTURE_SIZE);

    SDL_GPUTextureTransferInfo textureTransferInfo = {};
    textureTransferInfo.transfer_buffer = transferToTexture;
    textureTransferInfo.offset = 0;
    textureTransferInfo.pixels_per_row = tWidth;
    textureTransferInfo.rows_per_layer = tHeight;

    SDL_GPUTextureRegion textureRegion = {};
    textureRegion.texture = checkTexture;
    textureRegion.mip_level = 0;
    textureRegion.layer = 0;
    textureRegion.x = 0;
    textureRegion.y = 0;
    textureRegion.z = 0;
    textureRegion.w = 512;
    textureRegion.h = 512;
    textureRegion.d = 1;
    SDL_GPUCommandBuffer* initCmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
    INIT_CHECK(initCmd != nullptr, "texture upload command buffer acquisition");
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(initCmd);
    INIT_CHECK(copyPass != nullptr, "texture copy pass begin");
    SDL_UploadToGPUTexture(copyPass, &textureTransferInfo,
                           &textureRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    INIT_CHECK(SDL_SubmitGPUCommandBuffer(initCmd), "texture upload submission");

    // make the sampler here
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    sampler = SDL_CreateGPUSampler(gpuDevice, &samplerInfo);
    INIT_CHECK(sampler != nullptr, "sampler creation");
    
    // bind texture and sampler here
    textureSamplerBindings[0].sampler = sampler;
    textureSamplerBindings[0].texture = checkTexture;

    frame.renderPass = nullptr;
    frame.colorTargetInfo = {};
    frame.colorTargetInfo.clear_color = SDL_FColor{rendererConfig.backgroundColor.r, rendererConfig.backgroundColor.g, rendererConfig.backgroundColor.b, rendererConfig.backgroundColor.a};
    frame.colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    frame.colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

    // make a depth texture here
    SDL_GPUTextureCreateInfo depthTextureInfo = {};
    depthTextureInfo.type = SDL_GPU_TEXTURETYPE_2D;
    depthTextureInfo.format = SDL_GPU_TEXTUREFORMAT_D32_FLOAT;
    depthTextureInfo.usage = SDL_GPU_TEXTUREUSAGE_DEPTH_STENCIL_TARGET;
    depthTextureInfo.num_levels = 1;
    depthTextureInfo.sample_count = SDL_GPU_SAMPLECOUNT_1;
    depthTextureInfo.layer_count_or_depth = 1;
    depthTextureInfo.width = (u32)rendererConfig.resolution.x;
    depthTextureInfo.height = (u32)rendererConfig.resolution.y;

    depthTexture =
      SDL_CreateGPUTexture(gpuDevice, &depthTextureInfo);
    INIT_CHECK(depthTexture != nullptr, "depth texture creation");
  
    frame.depthTargetInfo = {};
    frame.depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    frame.depthTargetInfo.clear_depth = 1.0f;
    frame.depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
    frame.depthTargetInfo.texture = depthTexture;
  
    // std::cout << "width " << tWidth << " height " << tHeight << '\n';
  }

  fpu_end();
  return true;
}

bool scene_load_obj(SceneHandle sh, const char *path) {
  Scene* scene = scene_get(sh);

  std::vector<Mesh> meshCollection = loadAllMeshFromObj(path);

  for (const auto &m : meshCollection) {
    mesh_add(scene, m);
  }
  
  return true;
}

bool scene_remove_mesh(SceneHandle sh, MeshHandle mh) {
  Scene* scene = scene_get(sh);

  if (!scene->meshes.contains(mh)) {
    return false;
  }
  scene->objects.erase(mh);
  scene->meshes.erase(mh);
  auto it = std::find(scene->meshOrder.begin(), scene->meshOrder.end(), mh);
  scene->meshOrder.erase(it);
  return true;
}

static_internal Transform make_transform(f32 px, f32 py, f32 pz, f32 sx, f32 sy, f32 sz,
					 f32 rx, f32 ry, f32 rz) {
  return {px, py, pz, sx, sy, sz, rx, ry, rz};
}

InstanceHandle scene_add_instance(SceneHandle sh, MeshHandle m, Transform t) {
  Scene* scene = scene_get(sh);
  return instance_add(scene, m, t);
}

const char* scene_mesh_alias(SceneHandle sh, MeshHandle mh) {
  return mesh_get(scene_get(sh), mh)->alias.c_str();
}

const char* scene_instance_alias(SceneHandle sh, InstanceHandle ih) {
  return instance_get(scene_get(sh), ih)->alias.c_str();
}

bool scene_remove_instance(SceneHandle sh, InstanceHandle ih) {
  Scene* scene = scene_get(sh);

  for (auto &[meshHandle, instances] : scene->objects) {
    for (u64 i = 0; i < instances.size(); i++){
      if (instances[i].instanceHandle == ih) {
        instances.erase(instances.begin() + i);
	return true;
      }
    }
  }
  return false;
}

bool renderer_sync_scene(SceneHandle sh) {
  ASSERT(gpuDevice != nullptr, "renderer not initialized");
  Scene* scene = scene_get(sh);

  renderer_clear();
  
  u32 vertIndexOffset = 0;
  u32 indexIndexOffset = 0;
  for (MeshHandle meshHandle : scene->meshOrder) {
    const Mesh& mesh = scene->meshes[meshHandle];
    MeshRegion meshRegionStorage = {};
    meshRegionStorage.vertexOffset = vertIndexOffset;
    meshRegionStorage.indexOffset = indexIndexOffset;
    meshRegionStorage.indexCount = (u32)mesh.indices.size();
    rendererState.meshRegions[meshHandle] = meshRegionStorage;
    
    for (auto &vertex : mesh.vertices) {
      rendererState.vertices.push_back(vertex);
    }
    for (auto &index : mesh.indices) {
      rendererState.indices.push_back(index);
    }

    vertIndexOffset += mesh.vertices.size();
    indexIndexOffset += mesh.indices.size();
  }

  SDL_GPUBufferCreateInfo vertexBufferInfo = {};
  const u32 VERTEX_BUFFER_SIZE = rendererState.vertices.size() * sizeof(Vertex);
  vertexBufferInfo.size = VERTEX_BUFFER_SIZE;
  vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufferInfo);
  sdlAssert(vertexBuffer != nullptr);
  
  SDL_GPUBufferCreateInfo indexBufferInfo = {};
  const u32 INDEX_BUFFER_SIZE = rendererState.indices.size() * sizeof(u32);
  indexBufferInfo.size = INDEX_BUFFER_SIZE;
  indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  indexBuffer = SDL_CreateGPUBuffer(gpuDevice, &indexBufferInfo);
  sdlAssert(indexBuffer != nullptr);
  
  SDL_GPUBufferCreateInfo instanceBufferInfo = {};
  instanceBufferInfo.size = INSTANCE_PREALLOCATED_SIZE;
  instanceBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  instanceBuffer = SDL_CreateGPUBuffer(gpuDevice, &instanceBufferInfo);
  sdlAssert(instanceBuffer != nullptr);

  SDL_GPUTransferBufferCreateInfo transferToInstanceBufferInfo = {};
  transferToInstanceBufferInfo.size = INSTANCE_PREALLOCATED_SIZE;
  transferToInstanceBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  transferToInstanceBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToInstanceBufferInfo);
  sdlAssert(transferToInstanceBuffer != nullptr);
  
  // COPY:
  {
    // VERTEX BUFFER
    SDL_GPUTransferBufferCreateInfo transferToVertexBufferInfo = {};
    transferToVertexBufferInfo.size = VERTEX_BUFFER_SIZE;
    transferToVertexBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transferToVertexBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToVertexBufferInfo);
    sdlAssert(transferToVertexBuffer != nullptr);

    Vertex* mappedVertexInTransfer = reinterpret_cast<Vertex*>(
							       SDL_MapGPUTransferBuffer(gpuDevice, transferToVertexBuffer, false));
    sdlAssert(mappedVertexInTransfer != nullptr);
    std::memcpy(mappedVertexInTransfer, rendererState.vertices.data(), VERTEX_BUFFER_SIZE);

    SDL_GPUTransferBufferLocation transferToVertexBufferLocation = {};
    transferToVertexBufferLocation.offset = 0;
    transferToVertexBufferLocation.transfer_buffer = transferToVertexBuffer;

    SDL_GPUBufferRegion vertexBufferRegion = {};
    vertexBufferRegion.offset = 0;
    vertexBufferRegion.buffer = vertexBuffer;
    vertexBufferRegion.size = VERTEX_BUFFER_SIZE;

    // INDEX BUFFER
    SDL_GPUTransferBufferCreateInfo transferToIndexBufferInfo = {};
    transferToIndexBufferInfo.size = INDEX_BUFFER_SIZE;
    transferToIndexBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transferToIndexBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToIndexBufferInfo);
    sdlAssert(transferToIndexBuffer != nullptr);

    u32* mappedIndicesInTransfer = reinterpret_cast<u32*>(
							  SDL_MapGPUTransferBuffer(gpuDevice, transferToIndexBuffer, false));
    sdlAssert(mappedIndicesInTransfer != nullptr);
    std::memcpy(mappedIndicesInTransfer, rendererState.indices.data(), INDEX_BUFFER_SIZE);

    SDL_GPUTransferBufferLocation transferToIndexBufferLocation = {};
    transferToIndexBufferLocation.offset = 0;
    transferToIndexBufferLocation.transfer_buffer = transferToIndexBuffer;

    SDL_GPUBufferRegion indexBufferRegion = {};
    indexBufferRegion.offset = 0;
    indexBufferRegion.buffer = indexBuffer;
    indexBufferRegion.size = INDEX_BUFFER_SIZE;


    SDL_GPUCommandBuffer* syncCmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
    sdlAssert(syncCmd != nullptr);
    SDL_GPUCopyPass* initialCopyPass = SDL_BeginGPUCopyPass(syncCmd);
    sdlAssert(initialCopyPass != nullptr);

    SDL_UploadToGPUBuffer(initialCopyPass, &transferToVertexBufferLocation,
                          &vertexBufferRegion, false);
    SDL_UploadToGPUBuffer(initialCopyPass, &transferToIndexBufferLocation,
			  &indexBufferRegion, false);
    SDL_EndGPUCopyPass(initialCopyPass);
    sdlAssert(SDL_SubmitGPUCommandBuffer(syncCmd));

    // Unmap and release transfer buffer once uploaded
    SDL_UnmapGPUTransferBuffer(gpuDevice, transferToVertexBuffer);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToVertexBuffer);

    SDL_UnmapGPUTransferBuffer(gpuDevice, transferToIndexBuffer);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToIndexBuffer);
  }

  vertexBufferBinding[0].buffer = vertexBuffer;
  vertexBufferBinding[0].offset = 0;

  instanceBufferBinding[0].buffer = instanceBuffer;
  instanceBufferBinding[0].offset = 0;

  indexBufferBinding[0].buffer = indexBuffer;
  indexBufferBinding[0].offset = 0;
  return true;
}

u64 scene_count() {
  return sceneOrder.size();
}

SceneHandle scene_at(u64 index) {
  ASSERT(index < sceneOrder.size(), "scene index out of bounds");
  return sceneOrder[index];
}

u64 scene_mesh_count(SceneHandle sh) {
  Scene* scene = scene_get(sh);
  return scene->meshOrder.size();
}

MeshHandle scene_mesh_at(SceneHandle sh, u64 index) {
  Scene* scene = scene_get(sh);
  ASSERT(index < scene->meshOrder.size(), "mesh index out of bounds");
  return scene->meshOrder[index];
}

u64 scene_mesh_instance_count(SceneHandle sh, MeshHandle mesh) {
  Scene* scene = scene_get(sh);
  mesh_get(scene, mesh);
  auto it = scene->objects.find(mesh);
  return it == scene->objects.end() ? 0 : it->second.size();
}

InstanceHandle scene_mesh_instance_at(SceneHandle sh, MeshHandle mesh, u64 index) {
  Scene* scene = scene_get(sh);
  mesh_get(scene, mesh);
  auto it = scene->objects.find(mesh);
  ASSERT(it != scene->objects.end(), "mesh has no instances");
  ASSERT(index < it->second.size(), "instance index out of bounds");
  return it->second[index].instanceHandle;
}

static_internal void draw_mesh(MeshRegion &meshRegion, Instances &instances) {
  ASSERT(frame.renderPass != nullptr && frame.commandBuffer != nullptr,
         "draw called outside renderer_render_begin/end block");
  glm::mat4 view = glm::lookAtLH(
				 rendererConfig.camera.position,
				 rendererConfig.camera.position + rendererConfig.camera.forward, WORLD_UP);
  // TODO move near and far into config
  glm::mat4 projection = glm::perspectiveLH_ZO(
					       glm::radians(60.0f),
					       rendererConfig.resolution.x / rendererConfig.resolution.y,
					       0.1f,
					       10000.0f
					       );
  glm::mat4 viewProjection = projection * view;

  SDL_PushGPUVertexUniformData(frame.commandBuffer, 0, &viewProjection,
			       (u32)sizeof(viewProjection));
  SDL_PushGPUFragmentUniformData(frame.commandBuffer, 0, &rendererConfig.resolution, (u32)sizeof(rendererConfig.resolution));
				   
  SDL_BindGPUGraphicsPipeline(frame.renderPass, pipeline);
  SDL_BindGPUVertexBuffers(frame.renderPass, 0, vertexBufferBinding, 1);
  SDL_BindGPUVertexBuffers(frame.renderPass, 1, instanceBufferBinding, 1);
  SDL_BindGPUIndexBuffer(frame.renderPass, indexBufferBinding,
			 SDL_GPU_INDEXELEMENTSIZE_32BIT);
  SDL_BindGPUFragmentSamplers(frame.renderPass, 0, textureSamplerBindings, 1);

  SDL_GPUCopyPass* frameCopyPass = SDL_BeginGPUCopyPass(frame.commandBuffer);
  sdlAssert(frameCopyPass != nullptr);

  const u32 requiredInstanceCopySize =
    instances.size() * sizeof(glm::mat4);
  ASSERT(frame.instanceBufferOffset + requiredInstanceCopySize
	 <= INSTANCE_PREALLOCATED_SIZE,
         "instance buffer overflow - raise INSTANCE_PREALLOCATED_SIZE");

  glm::mat4 *mappedToInstanceInTransfer =
    reinterpret_cast<glm::mat4*>(
				 SDL_MapGPUTransferBuffer(gpuDevice, transferToInstanceBuffer, false));
  sdlAssert(mappedToInstanceInTransfer != nullptr);

  glm::mat4* dst =
    (glm::mat4*)((u8*)mappedToInstanceInTransfer +
                 frame.instanceBufferOffset);
  for (auto &t : instances) {
    *dst++ = transformToMat4(t.transform);
  }

  SDL_GPUTransferBufferLocation transferToInstanceBufferLocation = {};
  transferToInstanceBufferLocation.offset = frame.instanceBufferOffset;
  transferToInstanceBufferLocation.transfer_buffer =
    transferToInstanceBuffer;

  SDL_GPUBufferRegion instanceBufferRegion = {};
  instanceBufferRegion.offset = frame.instanceBufferOffset;
  instanceBufferRegion.buffer = instanceBuffer;
  instanceBufferRegion.size = requiredInstanceCopySize;

  SDL_UploadToGPUBuffer(frameCopyPass, &transferToInstanceBufferLocation,
			&instanceBufferRegion, false);
  SDL_EndGPUCopyPass(frameCopyPass);

  SDL_DrawGPUIndexedPrimitives(frame.renderPass, meshRegion.indexCount,
			       instances.size(), meshRegion.indexOffset,
			       meshRegion.vertexOffset, 0);
  frame.instanceBufferOffset += requiredInstanceCopySize;
}


void renderer_render_begin(void) {
  ASSERT(gpuDevice != nullptr, "renderer not initialized");

  frame.instanceBufferOffset = 0;
  frame.commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
  sdlAssert(frame.commandBuffer != nullptr);

  sdlAssert(SDL_WaitAndAcquireGPUSwapchainTexture(
						  frame.commandBuffer,
						  window,
						  &frame.swapchainTexture,
						  &frame.swapchainWidth,
						  &frame.swapchainHeight));

  frame.colorTargetInfo.texture = frame.swapchainTexture;

  frame.renderPass = SDL_BeginGPURenderPass(frame.commandBuffer, &frame.colorTargetInfo, 1, &frame.depthTargetInfo);
  sdlAssert(frame.renderPass != nullptr);
}

void ui_begin() {
  fpu_begin();
  ImGui_ImplSDLGPU3_NewFrame();
  ImGui_ImplSDL3_NewFrame();
  ImGui::NewFrame();
}

void ui_end() {
  // edit styles
  // ImGui::ShowStyleEditor();
  
  ImGui::Render();
  ImDrawData *draw_data = ImGui::GetDrawData();
  ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, frame.commandBuffer);
  ImGui_ImplSDLGPU3_RenderDrawData(draw_data, frame.commandBuffer,
                                   frame.renderPass);
  fpu_end();
}

void renderer_render_scene(SceneHandle sh) {
  Scene *scene = scene_get(sh);
  for (auto &[meshHandle, instances] : scene->objects) {
    auto regionIt = rendererState.meshRegions.find(meshHandle);
    ASSERT(regionIt != rendererState.meshRegions.end(),
           "mesh never synced - call renderer_sync_scene first");
    MeshRegion meshRegion = regionIt->second;
    draw_mesh(meshRegion, instances);
  }
}

void renderer_render_end() {
  SDL_EndGPURenderPass(frame.renderPass);
  sdlAssert(SDL_SubmitGPUCommandBuffer(frame.commandBuffer));
}

bool renderer_clear(void) {
  rendererState.vertices.clear();
  rendererState.indices.clear();
  rendererState.meshRegions.clear();
  return true;
}

void Input::poll() {
  keyboard.begin();
  mouse.begin();

  ImGuiIO& io = ImGui::GetIO();

  SDL_Event event;
  while (SDL_PollEvent(&event)) {
    ImGui_ImplSDL3_ProcessEvent(&event);

    switch (event.type) {
    case SDL_EVENT_QUIT: {
      shouldClose = true;
      break;
    }
    case SDL_EVENT_MOUSE_MOTION: {
      if (io.WantCaptureMouse)
        mouse.on_motion(event.motion.x, event.motion.y, 0.0f, 0.0f);
      else
        mouse.on_motion(event.motion.x, event.motion.y,
                        event.motion.xrel, event.motion.yrel);
      break;
    }
    case SDL_EVENT_MOUSE_BUTTON_DOWN: {
      mouse.on_button_down(event.button.button, event.button.x, event.button.y,
                           !io.WantCaptureMouse);
      break;
    }
    case SDL_EVENT_MOUSE_BUTTON_UP: {
      mouse.on_button_up(event.button.button, event.button.x, event.button.y);
      break;
    }
    case SDL_EVENT_MOUSE_WHEEL: {
      if (!io.WantCaptureMouse)
        mouse.on_wheel(event.wheel.x, event.wheel.y);
      break;
    }
    case SDL_EVENT_KEY_DOWN:
    case SDL_EVENT_KEY_UP: {
      if (!io.WantCaptureKeyboard)
        keyboard.push_event((int)event.key.scancode,
                            event.type == SDL_EVENT_KEY_DOWN,
                            event.key.repeat);
      break;
    }
    }
  }

  mouse.end();
  keyboard.end();
  if (io.WantCaptureKeyboard) keyboard.clear_current_state();
}

void input_poll(void) {
  input.poll();
}

bool input_should_close() {
  return shouldClose;
}

f32 input_mouse_x(void) {
  return input.mouse.x;
}

f32 input_mouse_y(void) {
  return input.mouse.y;
}

f32 input_mouse_dx(void) {
  return input.mouse.dx;
}

f32 input_mouse_dy(void) {
  return input.mouse.dy;
}

f32 input_mouse_wheel_x(void) {
  return input.mouse.wheel_x;
}

f32 input_mouse_wheel_y(void) {
  return input.mouse.wheel_y;
}

bool input_mouse_down(MouseButton button) {
  return input.mouse.down(button);
}

bool input_mouse_pressed(MouseButton button) {
  return input.mouse.pressed(button);
}

bool input_mouse_released(MouseButton button) {
  return input.mouse.released(button);
}

void renderer_quit() {
  ui_quit();

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

  gpuDevice = nullptr;
  window = nullptr;
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
  frame = RendererFrame{};

  rendererState.vertices.clear();
  rendererState.indices.clear();
  rendererState.meshRegions.clear();
  sceneRegistry.clear();
  sceneOrder.clear();
  uiSelectedItems.clear();
  next_mesh_handle = 0;
  next_instance_handle = 0;
  next_scene_handle = 0;

  shouldClose = false;
  input = Input{};
}

void renderer_camera_move(f32 forward, f32 right, f32 up) {
  auto &camera = rendererConfig.camera;
  camera_update(camera);

  camera.position += camera.forward * forward;
  camera.position += camera.right * right;
  camera.position += WORLD_UP * up;
}

void renderer_camera_rotate(f32 yaw, f32 pitch) {
  auto& camera = rendererConfig.camera;

  camera.yaw = glm::mod(camera.yaw + yaw, 360.0f);
  camera.pitch = glm::clamp(camera.pitch + pitch, -89.99f, 89.99f);
}

void renderer_camera_update(void) {
  camera_update(rendererConfig.camera);
}

bool input_key_down(SDL_Scancode key) {
  return input.keyboard.down(key);
}

bool input_key_pressed(SDL_Scancode key) {
  return input.keyboard.pressed(key);
}

bool input_key_released(SDL_Scancode key) {
  return input.keyboard.released(key);
}

u64 input_keyboard_event_count(void) {
  return input.keyboard.count;
}

KeyboardEvent input_keyboard_event_at(u64 index) {
  ASSERT(index < input.keyboard.count, "keyboard event index out of bounds");
  return input.keyboard.buffer[index];
}

f32 time_dt(void) {
  static_local u64 previous = SDL_GetPerformanceCounter();
  const u64 now = SDL_GetPerformanceCounter();
  const f32 dt = static_cast<f32>(now - previous) / static_cast<f32>(timerFrequency);
  previous = now;
  return dt;
}

void ui_begin_window(const char* name) {
  ImGui::Begin(name);
}

void ui_end_window() { ImGui::End(); }

bool ui_button(const char* label) {
  return ImGui::Button(label);
}

bool ui_treenode(const char* label) {
  return ImGui::TreeNode(label);
}

void ui_treepop() {
  ImGui::TreePop();
}

bool ui_selectable(const char* label) {
  std::string key(label);
  bool isSelected = uiSelectedItems.contains(key);

  if (ImGui::Selectable(label, isSelected)) {
    ImGuiIO& io = ImGui::GetIO();
    bool ctrlPressed = io.KeyCtrl;

    if (ctrlPressed) {
      if (isSelected) {
        uiSelectedItems.erase(key);
        isSelected = false;
      } else {
        uiSelectedItems.insert(key);
        isSelected = true;
      }
    } else {
      uiSelectedItems.clear();
      uiSelectedItems.insert(key);
      isSelected = true;
    }
  }

  return isSelected;
}

void ui_sameline() {
  ImGui::SameLine();
}

bool ui_drag_float3(const char* label, f32 *v) {
  return ImGui::DragFloat3(label, v);
}

void ui_text(const char* text) {
  ImGui::Text("%s", text);
}

void scene_instance_set_transform(SceneHandle sh, InstanceHandle i,
				  Transform t) {
  Scene *s = scene_get(sh);
  Instance *instance = instance_get(s, i);
  instance->transform = t;
}

Transform scene_instance_get_transform(SceneHandle sh, InstanceHandle i) {
  Scene *s = scene_get(sh);
  return instance_get(s, i)->transform;
}
