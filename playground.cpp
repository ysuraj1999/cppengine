#include "playground.h"
#include "types.hpp"

// CONSTANTS
const glm::vec3 WORLD_UP = glm::vec3(0.0, 1.0, 0.0);

// Monotonically increasing counters
static_global uint64_t next_mesh_handle = 0;
static_global uint64_t next_instance_handle = 0;
static_global RendererConfig rendererConfig = {};
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
static_global SDL_GPUGraphicsPipeline* pipeline;
static_global SDL_GPUBufferBinding vertexBufferBinding[1] = {};
static_global SDL_GPUBufferBinding instanceBufferBinding[1] = {};
static_global SDL_GPUBufferBinding indexBufferBinding[1] = {};
static_global SDL_GPUTextureSamplerBinding textureSamplerBindings[1] = {};
static_global SDL_GPUTransferBuffer* transferToInstanceBuffer;
static_global u32 INSTANCE_PREALLOCATED_SIZE = 10 * 1024 * 1024;

bool renderer_init(RendererConfig config) {
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

  commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
  sdlAssert(commandBuffer != nullptr);

  // from sync
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

  
  return true;
}

bool scene_load_obj(Scene *scene, const char *path) {
  std::vector<Mesh> meshCollection = loadAllMeshFromObj(path);

  for (const auto &m : meshCollection) {
    scene_add_mesh(scene, m);
  }
  
  return true;
}

MeshHandle scene_add_mesh(Scene *scene, Mesh m) {
  MeshEntry mEntry = {};
  mEntry.handle = next_mesh_handle++;
  mEntry.mesh = m;
  (*scene).meshes.push_back(mEntry);
  return mEntry.handle;
}

bool scene_remove_mesh(Scene *scene, MeshHandle mh) {
  for (auto it = scene->meshes.begin(); it != scene->meshes.end(); ++it) {
    if (it->handle == mh) {
      // erase instances
      scene->objects[mh].clear();
      
      scene->meshes.erase(it);
      return true;
    }
  }
  return false;
}

InstanceHandle scene_add_instance(Scene *scene, MeshHandle m, Transform t) {
  Instance newInstance = {};
  newInstance.instanceHandle = next_instance_handle++;
  newInstance.mesh = m;
  newInstance.transform = t;

  scene->objects[m].push_back(newInstance);
  return newInstance.instanceHandle;
}

bool scene_remove_instance(Scene *scene, InstanceHandle ih) {
  for (auto &[meshHandle, instances] : scene->objects) {
    for (auto it = instances.begin(); it != instances.end(); it++) {
      if (it->instanceHandle == ih) {
        instances.erase(it);
	return true;
      }
    }
  }
  return false;
}

bool renderer_sync_scene(RendererState *rendererState, const Scene *scene) {
  rendererState->vertices.clear();
  rendererState->indices.clear();
  rendererState->meshRegions.clear();
  rendererState->objects.clear();
  
  u32 vertIndexOffset = 0;
  u32 indexIndexOffset = 0;
  for (const MeshEntry &meshEntry : scene->meshes) {
    const Mesh mesh = meshEntry.mesh;
    const MeshHandle meshHandle = meshEntry.handle;
    MeshRegion meshRegionStorage = {};
    meshRegionStorage.vertexOffset = vertIndexOffset;
    meshRegionStorage.indexOffset = indexIndexOffset;
    meshRegionStorage.indexCount = (u32)mesh.indices.size();
    rendererState->meshRegions[meshHandle] = meshRegionStorage;
    
    for (auto &vertex : mesh.vertices) {
      rendererState->vertices.push_back(vertex);
    }
    for (auto &index : mesh.indices) {
      rendererState->indices.push_back(index);
    }

    vertIndexOffset += mesh.vertices.size();
    indexIndexOffset += mesh.indices.size();
  }

  rendererState->objects = scene->objects;

  SDL_GPUBufferCreateInfo vertexBufferInfo = {};
  const u32 VERTEX_BUFFER_SIZE = rendererState->vertices.size() * sizeof(Vertex);
  vertexBufferInfo.size = VERTEX_BUFFER_SIZE;
  vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufferInfo);
  sdlAssert(vertexBuffer != nullptr);
  
  SDL_GPUBufferCreateInfo indexBufferInfo = {};
  const u32 INDEX_BUFFER_SIZE = rendererState->indices.size() * sizeof(u32);
  indexBufferInfo.size = INDEX_BUFFER_SIZE;
  indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(gpuDevice, &indexBufferInfo);
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
    std::memcpy(mappedVertexInTransfer, rendererState->vertices.data(), VERTEX_BUFFER_SIZE);

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
    std::memcpy(mappedIndicesInTransfer, rendererState->indices.data(), INDEX_BUFFER_SIZE);

    SDL_GPUTransferBufferLocation transferToIndexBufferLocation = {};
    transferToIndexBufferLocation.offset = 0;
    transferToIndexBufferLocation.transfer_buffer = transferToIndexBuffer;

    SDL_GPUBufferRegion indexBufferRegion = {};
    indexBufferRegion.offset = 0;
    indexBufferRegion.buffer = indexBuffer;
    indexBufferRegion.size = INDEX_BUFFER_SIZE;


    SDL_GPUCopyPass* initialCopyPass = SDL_BeginGPUCopyPass(commandBuffer);
    sdlAssert(initialCopyPass != nullptr);

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

      SDL_UploadToGPUTexture(initialCopyPass, &textureTransferInfo, &textureRegion, false);

      // std::cout << "width " << tWidth << " height " << tHeight << '\n';
    }


    SDL_UploadToGPUBuffer(initialCopyPass, &transferToVertexBufferLocation,
                          &vertexBufferRegion, false);
    SDL_UploadToGPUBuffer(initialCopyPass, &transferToIndexBufferLocation,
			  &indexBufferRegion, false);
    SDL_EndGPUCopyPass(initialCopyPass);
    sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));

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

  SDL_GPUTexture *depthTexture =
    SDL_CreateGPUTexture(gpuDevice, &depthTextureInfo);
  sdlAssert(depthTexture);
  
  depthTargetInfo = {};
  depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
  depthTargetInfo.clear_depth = 1.0f;
  depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
  depthTargetInfo.texture = depthTexture;
  
  return true;
}

int main(int argc, char* argv[]) {
  // (void)argc;
  // (void)argv;

  RendererConfig config = {
      .resolution = {800.0f, 800.0f},
      .backgroundColor = {47.0f / 255.0f, 149.0f / 255.0f, 78.0f / 255.0f, 1.0f},
      .keyboardSensitivity = 10.0f,
      .mouseSensitivity = 0.1f,
      .cameraEnabled = true,
      .camera = {.position = {-5.0f, 0.0f, 0.0f}, .yaw = 0.0f, .pitch = 0.0f},
  };
  renderer_init(config);

  Scene scene = {};
  RendererState renderState = {};

  scene_load_obj(&scene, "models/cube.obj");
  scene_load_obj(&scene, "models/bunny.obj");

  u32 GRID_SIZE = 10;
  for (u32 x = 0; x < GRID_SIZE; x++){
    for (u32 y = 0; y < GRID_SIZE; y++){
      for (u32 z = 0; z < GRID_SIZE; z++) {
	scene_add_instance(&scene, 0, Transform{
            .position = {1.0f * x, 1.0f * y, 1.0f * z},
            .scale = {1.0f, 1.0f, 1.0f},
            .rotation = {0.0f, 0.0f, 0.0f}
	  });
      }
    }
  }
  
  renderer_sync_scene(&renderState, &scene);

  bool running = true;
  
  u64 previous = SDL_GetPerformanceCounter();
  const u64 freq = SDL_GetPerformanceFrequency();
  float delta_x, delta_y;
  while (running) {
    const u64 now = SDL_GetPerformanceCounter();
    const f32 dt = static_cast<f32>(now - previous) / static_cast<f32>(freq);
    previous = now;

    const bool* keys =  SDL_GetKeyboardState(nullptr);
    
    glm::vec3 forward{
      cos(glm::radians(rendererConfig.camera.pitch)) * cos(glm::radians(rendererConfig.camera.yaw)),
      sin(glm::radians(rendererConfig.camera.pitch)),
      cos(glm::radians(rendererConfig.camera.pitch)) * sin(glm::radians(rendererConfig.camera.yaw))};

    forward = glm::normalize(forward);
    glm::vec3 right = glm::cross(WORLD_UP, forward); // reversed for Left handed

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
	running = false;
      }
    }

    if (keys[SDL_SCANCODE_ESCAPE]) {
      running = false;
    }

    if (rendererConfig.cameraEnabled){
      u32 mouseButton = SDL_GetRelativeMouseState(&delta_x, &delta_y);
      rendererConfig.camera.yaw -= delta_x * rendererConfig.mouseSensitivity;
      rendererConfig.camera.yaw = std::fmod(rendererConfig.camera.yaw, 360.0f);
      rendererConfig.camera.pitch -= delta_y * rendererConfig.mouseSensitivity;
      rendererConfig.camera.pitch = glm::clamp(rendererConfig.camera.pitch, -90.0f, 90.0f);

      f32 speed = rendererConfig.keyboardSensitivity;
      if (keys[SDL_SCANCODE_LSHIFT]) {
	speed *= 2.0f;
      }
      if (keys[SDL_SCANCODE_W]) {
	rendererConfig.camera.position += forward * dt * speed;
      }
      if (keys[SDL_SCANCODE_S]) {
	rendererConfig.camera.position -= forward * dt * speed;
      }
      if (keys[SDL_SCANCODE_A]) {
	rendererConfig.camera.position -= right * dt * speed;
      }
      if (keys[SDL_SCANCODE_D]) {
	rendererConfig.camera.position += right * dt * speed;
      }
      if (keys[SDL_SCANCODE_SPACE]) {
	rendererConfig.camera.position += WORLD_UP * dt * speed;
      }
      if (keys[SDL_SCANCODE_LCTRL]) {
	rendererConfig.camera.position -= WORLD_UP * dt * speed;
      }
    }

    glm::mat4 view =
      glm::lookAtLH(rendererConfig.camera.position, rendererConfig.camera.position + forward, WORLD_UP);
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
    for (auto& entry : renderState.objects) {
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
      assert(requiredInstanceCopySize <= INSTANCE_PREALLOCATED_SIZE);

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

      MeshRegion region = renderState.meshRegions[meshHandle];

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
  
  SDL_ReleaseGPUTexture(gpuDevice, checkTexture);
  SDL_ReleaseGPUGraphicsPipeline(gpuDevice, pipeline);
  SDL_ReleaseGPUShader(gpuDevice, fragmentShader);
  SDL_ReleaseGPUShader(gpuDevice, vertexShader);
  SDL_ReleaseGPUBuffer(gpuDevice, vertexBuffer);
  SDL_DestroyGPUDevice(gpuDevice);    
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
