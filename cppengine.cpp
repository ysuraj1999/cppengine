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
static_global SDL_GPUTexture *swapchainTexture;
static_global u32 swapchainWidth;
static_global u32 swapchainHeight;
static_global SDL_GPUCommandBuffer *commandBuffer;
static_global SDL_GPUColorTargetInfo colorTargetInfo;
static_global SDL_GPURenderPass *renderPass;
static_global SDL_GPUBuffer *vertexBuffer;
static_global SDL_GPUTexture *checkTexture;
static_global SDL_GPUShader *vertexShader;
static_global SDL_GPUShader *fragmentShader;
static_global SDL_GPUDepthStencilTargetInfo depthTargetInfo;
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

static_internal SceneHandle scene_create() {
  SceneHandle sh = next_scene_handle++;
  sceneRegistry[sh] = Scene{};
  sceneOrder.push_back(sh);
  return sh;
}

static_internal bool scene_destroy(SceneHandle sh) {
  bool erased = sceneRegistry.erase(sh) > 0;
  if (erased) {
    auto it = std::find(sceneOrder.begin(), sceneOrder.end(), sh);
    sceneOrder.erase(it);
  }
  return erased;
}

static_internal Scene* scene_get(SceneHandle sh) {
  auto it = sceneRegistry.find(sh);
  assert(it != sceneRegistry.end());
  return &it->second;
}

static_internal Mesh* mesh_get(Scene* scene, MeshHandle mh) {
  auto it = scene->meshes.find(mh);
  assert(it != scene->meshes.end());
  return &it->second;
}

static_internal Instance* instance_get(Scene* scene, InstanceHandle ih) {
  for (auto &[meshHandle, instances] : scene->objects) {
    for (auto &inst : instances) {
      if (inst.instanceHandle == ih)
        return &inst;
    }
  }
  assert(false && "instance handle not found");
  return nullptr;
}

static_internal MeshHandle mesh_add(Scene* scene, Mesh m) {
  MeshHandle newMeshHandle = next_mesh_handle++;
  scene->meshes[newMeshHandle] = m;
  scene->meshOrder.push_back(newMeshHandle);
  return newMeshHandle;
}

static_internal InstanceHandle instance_add(Scene* scene, MeshHandle m, Transform t) {
  mesh_get(scene, m);
  Instance newInstance = {};
  newInstance.instanceHandle = next_instance_handle++;
  newInstance.mesh = m;
  newInstance.transform = t;
  scene->objects[m].push_back(newInstance);
  return newInstance.instanceHandle;
}

bool renderer_init() {
  RendererConfig config = {
    .resolution = {1000.0f, 1000.0f},
    .backgroundColor = {47.0f / 255.0f, 149.0f / 255.0f, 78.0f / 255.0f, 1.0f},
    .keyboardSensitivity = 200.0f,
    .mouseSensitivity = 0.1f,
    .cameraEnabled = true,
    .camera = {.position = {-5.0f, 0.0f, 0.0f}, .yaw = 0.0f, .pitch = 0.0f},
  };

  rendererConfig = config;
  
  sdlAssert(SDL_Init(SDL_INIT_VIDEO));

  window = SDL_CreateWindow("Lens", (u32)rendererConfig.resolution.x, (u32)rendererConfig.resolution.y, 0);
  sdlAssert(window != nullptr);

  if (rendererConfig.cameraEnabled)
    sdlAssert(SDL_SetWindowRelativeMouseMode(window, true));

  gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
  sdlAssert(gpuDevice != nullptr);

  sdlAssert(SDL_ClaimWindowForGPUDevice(gpuDevice, window));

  sdlAssert(SDL_SetGPUSwapchainParameters(gpuDevice, window,
                                          SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
                                          SDL_GPU_PRESENTMODE_VSYNC));
  swapchainTexture = nullptr;
  swapchainWidth = 0;
  swapchainHeight = 0;

  std::vector<u8> vertexShaderContents =
    readFile(SHADER_BIN_DIR "/shader.vertex.spv");
  assert(!vertexShaderContents.empty());

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
  sdlAssert(vertexShader != nullptr);

  std::vector<u8> fragmentShaderContents =
    readFile(SHADER_BIN_DIR "/shader.fragment.spv");
  assert(!fragmentShaderContents.empty());

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
  sdlAssert(fragmentShader != nullptr);

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
  sdlAssert(pipeline != nullptr);

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
    assert(pixels != nullptr);
    defer(stbi_image_free(pixels));
      
    u32 tWidth = static_cast<u32>(w);
    u32 tHeight = static_cast<u32>(h);
    textureInfo.width = tWidth;
    textureInfo.height = tHeight;
    const u32 TEXTURE_SIZE = tWidth * tHeight * 4;

    checkTexture = SDL_CreateGPUTexture(gpuDevice, &textureInfo);
    sdlAssert(checkTexture != nullptr);

    
    SDL_GPUTransferBufferCreateInfo transferToTextureInfo = {};
    transferToTextureInfo.size = TEXTURE_SIZE;
    transferToTextureInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transferToTexture = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToTextureInfo);
    sdlAssert(transferToTexture != nullptr);

    u8* mappedTransferToBytes = reinterpret_cast<u8*>(
						      SDL_MapGPUTransferBuffer(gpuDevice, transferToTexture, false));
    sdlAssert(mappedTransferToBytes != nullptr);
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
    sdlAssert(initCmd != nullptr);
    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(initCmd);
    sdlAssert(copyPass != nullptr);
    SDL_UploadToGPUTexture(copyPass, &textureTransferInfo,
                           &textureRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    sdlAssert(SDL_SubmitGPUCommandBuffer(initCmd));

    // make the sampler here
    SDL_GPUSamplerCreateInfo samplerInfo = {};
    samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
    samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
    samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
    samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

    sampler = SDL_CreateGPUSampler(gpuDevice, &samplerInfo);
    sdlAssert(sampler != nullptr);
    
    // bind texture and sampler here
    textureSamplerBindings[0].sampler = sampler;
    textureSamplerBindings[0].texture = checkTexture;

    renderPass = nullptr;
    colorTargetInfo = {};
    colorTargetInfo.clear_color = SDL_FColor{rendererConfig.backgroundColor.r, rendererConfig.backgroundColor.g, rendererConfig.backgroundColor.b, rendererConfig.backgroundColor.a};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;

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
    sdlAssert(depthTexture);
  
    depthTargetInfo = {};
    depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    depthTargetInfo.clear_depth = 1.0f;
    depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
    depthTargetInfo.texture = depthTexture;
  
    // std::cout << "width " << tWidth << " height " << tHeight << '\n';
  }

  
  return true;
}

SceneHandle renderer_scene_create() {
  return scene_create();
}

bool renderer_scene_destroy(SceneHandle sh) {
  return scene_destroy(sh);
}

bool renderer_scene_load_obj(SceneHandle sh, const char *path) {
  Scene* scene = scene_get(sh);

  std::vector<Mesh> meshCollection = loadAllMeshFromObj(path);

  for (const auto &m : meshCollection) {
    mesh_add(scene, m);
  }
  
  return true;
}

bool renderer_scene_remove_mesh(SceneHandle sh, MeshHandle mh) {
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

InstanceHandle renderer_scene_add_instance(SceneHandle sh, MeshHandle m,
                                           f32 px, f32 py, f32 pz,
                                           f32 sx, f32 sy, f32 sz,
                                           f32 rx, f32 ry, f32 rz) {
  Scene* scene = scene_get(sh);

  Transform t = {};
  t.position = {px, py, pz};
  t.scale    = {sx, sy, sz};
  t.rotation = {rx, ry, rz};

  return instance_add(scene, m, t);
}

bool renderer_scene_remove_instance(SceneHandle sh, InstanceHandle ih) {
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

  rendererState.objects = scene->objects;

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

u64 renderer_scene_count() {
  return sceneOrder.size();
}

SceneHandle renderer_scene_at(u64 index) {
  assert(index < sceneOrder.size());
  return sceneOrder[index];
}

u64 renderer_scene_mesh_count(SceneHandle sh) {
  Scene* scene = scene_get(sh);
  return scene->meshOrder.size();
}

MeshHandle renderer_scene_mesh_at(SceneHandle sh, u64 index) {
  Scene* scene = scene_get(sh);
  assert(index < scene->meshOrder.size());
  return scene->meshOrder[index];
}

u64 renderer_scene_mesh_instance_count(SceneHandle sh, MeshHandle mesh) {
  Scene* scene = scene_get(sh);
  mesh_get(scene, mesh);
  auto it = scene->objects.find(mesh);
  return it == scene->objects.end() ? 0 : it->second.size();
}

InstanceHandle renderer_scene_mesh_instance_at(SceneHandle sh, MeshHandle mesh, u64 index) {
  Scene* scene = scene_get(sh);
  mesh_get(scene, mesh);
  auto it = scene->objects.find(mesh);
  assert(it != scene->objects.end());
  assert(index < it->second.size());
  return it->second[index].instanceHandle;
}

void renderer_render(void) {
  glm::mat4 view =
    glm::lookAtLH(rendererConfig.camera.position, rendererConfig.camera.position + rendererConfig.camera.forward, WORLD_UP);
  glm::mat4 projection = glm::perspectiveLH_ZO(
					       glm::radians(60.0f),
					       rendererConfig.resolution.x / rendererConfig.resolution.y,
					       0.1f,
					       10000.0f
					       );
  glm::mat4 viewProjection = projection * view;
  commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);

  sdlAssert(SDL_WaitAndAcquireGPUSwapchainTexture(
						  commandBuffer,
						  window,
						  &swapchainTexture,
						  &swapchainWidth,
						  &swapchainHeight));

  colorTargetInfo.texture = swapchainTexture;

  renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
  sdlAssert(renderPass != nullptr);

  SDL_PushGPUVertexUniformData(commandBuffer, 0, &viewProjection,
			       (u32)sizeof(viewProjection));
  SDL_PushGPUFragmentUniformData(commandBuffer, 0, &rendererConfig.resolution, (u32)sizeof(rendererConfig.resolution));
				   
  SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
  SDL_BindGPUVertexBuffers(renderPass, 0, vertexBufferBinding, 1);
  SDL_BindGPUVertexBuffers(renderPass, 1, instanceBufferBinding, 1);
  SDL_BindGPUIndexBuffer(renderPass, indexBufferBinding,
			 SDL_GPU_INDEXELEMENTSIZE_32BIT);
  SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBindings, 1);

  u32 transferBufferOffsetPerMesh = 0;
  for (auto& entry : rendererState.objects) {
    auto meshHandle = entry.first;
    auto &instances = entry.second;

    SDL_GPUCopyPass* frameCopyPass = SDL_BeginGPUCopyPass(commandBuffer);
    sdlAssert(frameCopyPass != nullptr);

    std::vector<glm::mat4> modelMatrices;
    modelMatrices.reserve(instances.size());
    for (auto &t : instances) {
      modelMatrices.push_back(transformToMat4(t.transform));
    }

    const u32 requiredInstanceCopySize =
        modelMatrices.size() * sizeof(glm::mat4);
    assert(transferBufferOffsetPerMesh + requiredInstanceCopySize <= INSTANCE_PREALLOCATED_SIZE);

    glm::mat4 *mappedToInstanceInTransfer =
      reinterpret_cast<glm::mat4*>(
				   SDL_MapGPUTransferBuffer(gpuDevice, transferToInstanceBuffer, false));
    sdlAssert(mappedToInstanceInTransfer != nullptr);
    std::memcpy((u8 *)mappedToInstanceInTransfer + transferBufferOffsetPerMesh,
		modelMatrices.data(), requiredInstanceCopySize);

    SDL_GPUTransferBufferLocation transferToInstanceBufferLocation = {};
    transferToInstanceBufferLocation.offset = transferBufferOffsetPerMesh;
    transferToInstanceBufferLocation.transfer_buffer =
      transferToInstanceBuffer;

    SDL_GPUBufferRegion instanceBufferRegion = {};
    instanceBufferRegion.offset = transferBufferOffsetPerMesh;
    instanceBufferRegion.buffer = instanceBuffer;
    instanceBufferRegion.size = requiredInstanceCopySize;

    MeshRegion region = rendererState.meshRegions[meshHandle];

    SDL_UploadToGPUBuffer(frameCopyPass, &transferToInstanceBufferLocation,
                          &instanceBufferRegion, false);
    SDL_EndGPUCopyPass(frameCopyPass);

    SDL_DrawGPUIndexedPrimitives(renderPass, region.indexCount,
				 instances.size(), region.indexOffset,
				 region.vertexOffset, 0);
    transferBufferOffsetPerMesh += requiredInstanceCopySize;
  }

    
  SDL_EndGPURenderPass(renderPass);
  sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));
}

bool renderer_clear(void) {
  rendererState.vertices.clear();
  rendererState.indices.clear();
  rendererState.meshRegions.clear();
  rendererState.objects.clear();
  return true;
}

void renderer_quit() {
  SDL_ReleaseGPUTexture(gpuDevice, checkTexture);
  SDL_ReleaseGPUTexture(gpuDevice, depthTexture);
  SDL_ReleaseGPUGraphicsPipeline(gpuDevice, pipeline);
  SDL_ReleaseGPUShader(gpuDevice, fragmentShader);
  SDL_ReleaseGPUShader(gpuDevice, vertexShader);
  SDL_ReleaseGPUBuffer(gpuDevice, vertexBuffer);
  SDL_ReleaseGPUBuffer(gpuDevice, indexBuffer);
  SDL_ReleaseGPUBuffer(gpuDevice, instanceBuffer);
  SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToInstanceBuffer);
  SDL_ReleaseGPUSampler(gpuDevice, sampler);
  SDL_DestroyGPUDevice(gpuDevice);
  SDL_DestroyWindow(window);
  SDL_Quit();
}

