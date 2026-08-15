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

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"

#include "stb_image.h"

#include "types.hpp"
#include "obj.hpp"
#include "utils.cpp"

static Camera camera;

const auto WORLD_UP = glm::vec3(0.0, 1.0, 0.0);

int main(int argc, char* argv[]) {
  // (void)argc;
  // (void)argv;

  std::vector<Mesh> meshContainer;
  loadObj("models/breakfast_room.obj", meshContainer);

  const u32 modelIndex = 1;
  std::vector<Vertex> worldVertices = {};
  std::vector<u32> worldIndices = {};

  u32 indexOffset = 0;
  for (auto &mesh : meshContainer) {
    for (auto &vertex : mesh.vertices) {
      worldVertices.push_back(vertex);
    }
    for (auto &index : mesh.indices) {
      worldIndices.push_back(index + indexOffset);
    }

    indexOffset += mesh.vertices.size();
  }

  // std::cout << vertices << '\n';
  // std::cout << indices << '\n';
  
  camera.position = glm::vec3{-5.0f, 0.0f, 0.0f};
  camera.yaw = 0.0f;
  camera.pitch = 0.0f;

  sdlAssert(SDL_Init(SDL_INIT_VIDEO));

  SDL_Window* window = SDL_CreateWindow("Lens", 800, 800, 0);
  sdlAssert(window != nullptr);

  sdlAssert(SDL_SetWindowRelativeMouseMode(window, true));

  SDL_GPUTexture* swapchainTexture = nullptr;
  u32 width = 0;
  u32 height = 0;

  SDL_GPUDevice* gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
  sdlAssert(gpuDevice != nullptr);

  sdlAssert(SDL_ClaimWindowForGPUDevice(gpuDevice, window));

  sdlAssert(SDL_SetGPUSwapchainParameters(
					  gpuDevice,
					  window,
					  SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
					  SDL_GPU_PRESENTMODE_VSYNC
					  ));

  SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
  sdlAssert(commandBuffer != nullptr);

  SDL_GPUBufferCreateInfo vertexBufferInfo = {};
  const u32 VERTEX_BUFFER_SIZE = worldVertices.size() * sizeof(Vertex);
  vertexBufferInfo.size = VERTEX_BUFFER_SIZE;
  vertexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &vertexBufferInfo);
  sdlAssert(vertexBuffer != nullptr);

  SDL_GPUBufferCreateInfo indexBufferInfo = {};
  const u32 INDEX_BUFFER_SIZE = worldIndices.size() * sizeof(u32);
  indexBufferInfo.size = INDEX_BUFFER_SIZE;
  indexBufferInfo.usage = SDL_GPU_BUFFERUSAGE_INDEX;
  SDL_GPUBuffer* indexBuffer = SDL_CreateGPUBuffer(gpuDevice, &indexBufferInfo);
  sdlAssert(indexBuffer != nullptr);

  SDL_GPUTexture *checkTexture;
  defer(SDL_ReleaseGPUTexture(gpuDevice, checkTexture));
  
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
    std::memcpy(mappedVertexInTransfer, worldVertices.data(), VERTEX_BUFFER_SIZE);

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

    Vertex* mappedIndicesInTransfer = reinterpret_cast<Vertex*>(
							       SDL_MapGPUTransferBuffer(gpuDevice, transferToIndexBuffer, false));
    sdlAssert(mappedIndicesInTransfer != nullptr);
    std::memcpy(mappedIndicesInTransfer, worldIndices.data(), INDEX_BUFFER_SIZE);

    SDL_GPUTransferBufferLocation transferToIndexBufferLocation = {};
    transferToIndexBufferLocation.offset = 0;
    transferToIndexBufferLocation.transfer_buffer = transferToIndexBuffer;

    SDL_GPUBufferRegion indexBufferRegion = {};
    indexBufferRegion.offset = 0;
    indexBufferRegion.buffer = indexBuffer;
    indexBufferRegion.size = INDEX_BUFFER_SIZE;


    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    sdlAssert(copyPass != nullptr);

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

      SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);

      // std::cout << "width " << tWidth << " height " << tHeight << '\n';
    }

    
    SDL_UploadToGPUBuffer(copyPass, &transferToVertexBufferLocation,
                          &vertexBufferRegion, false);
    SDL_UploadToGPUBuffer(copyPass, &transferToIndexBufferLocation,
			      &indexBufferRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));

    // Unmap and release transfer buffer once uploaded
    SDL_UnmapGPUTransferBuffer(gpuDevice, transferToVertexBuffer);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToVertexBuffer);

    SDL_UnmapGPUTransferBuffer(gpuDevice, transferToIndexBuffer);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, transferToIndexBuffer);
  }


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

  SDL_GPUShader* vertexShader = SDL_CreateGPUShader(gpuDevice, &vertexShaderInfo);
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
  fragmentShaderInfo.num_uniform_buffers = 0;

  SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(gpuDevice, &fragmentShaderInfo);
  sdlAssert(fragmentShader != nullptr);

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.vertex_shader = vertexShader;
  pipelineInfo.fragment_shader = fragmentShader;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

  SDL_GPUVertexBufferDescription vertexBufferDescription[1] = {};

  // How each vertex is defined
  vertexBufferDescription[0].slot = 0;
  vertexBufferDescription[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  vertexBufferDescription[0].instance_step_rate = 0;
  vertexBufferDescription[0].pitch = (u32)sizeof(Vertex);

  pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescription;

  // Vertex attributes
  SDL_GPUVertexAttribute vertexAttributes[4] = {};
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

  pipelineInfo.vertex_input_state.num_vertex_attributes = 4;
  pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;
  
  SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {};
  colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window);

  pipelineInfo.target_info.num_color_targets = 1;
  pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

  pipelineInfo.rasterizer_state.cull_mode = SDL_GPU_CULLMODE_BACK;
  pipelineInfo.rasterizer_state.front_face = SDL_GPU_FRONTFACE_COUNTER_CLOCKWISE;

  // Depth test
  SDL_GPUDepthStencilState depthStencilState = {};
  depthStencilState.enable_depth_write = true;
  depthStencilState.enable_depth_test = true;
  depthStencilState.compare_op = SDL_GPU_COMPAREOP_LESS;
  
  pipelineInfo.depth_stencil_state = depthStencilState;
      
  SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineInfo);
  sdlAssert(pipeline != nullptr);

  SDL_GPUBufferBinding vertexBufferBinding[1] = {};
  vertexBufferBinding[0].buffer = vertexBuffer;
  vertexBufferBinding[0].offset = 0;

  SDL_GPUBufferBinding indexBufferBinding[1] = {};
  indexBufferBinding[0].buffer = indexBuffer;
  indexBufferBinding[0].offset = 0;

  // make the sampler here
  SDL_GPUSamplerCreateInfo samplerInfo = {};
  samplerInfo.mipmap_mode = SDL_GPU_SAMPLERMIPMAPMODE_NEAREST;
  samplerInfo.min_filter = SDL_GPU_FILTER_NEAREST;
  samplerInfo.mag_filter = SDL_GPU_FILTER_NEAREST;
  samplerInfo.address_mode_u = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;
  samplerInfo.address_mode_v = SDL_GPU_SAMPLERADDRESSMODE_CLAMP_TO_EDGE;

  SDL_GPUSampler *sampler = SDL_CreateGPUSampler(gpuDevice, &samplerInfo);
  sdlAssert(sampler != nullptr);
    
  // bind texture and sampler here
  SDL_GPUTextureSamplerBinding textureSamplerBindings[1] = {};
  textureSamplerBindings[0].sampler = sampler;
  textureSamplerBindings[0].texture = checkTexture;

  bool running = true;
  SDL_GPURenderPass* renderPass = nullptr;

  SDL_GPUColorTargetInfo colorTargetInfo = {};
  colorTargetInfo.clear_color = SDL_FColor{13.0f/255.0f, 115.0f/255.0f, 89.0f/255.0f, 1.0f};
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
  depthTextureInfo.width = 800;
  depthTextureInfo.height = 800;

  SDL_GPUTexture *depthTexture =
      SDL_CreateGPUTexture(gpuDevice, &depthTextureInfo);
  sdlAssert(depthTexture);
  
  SDL_GPUDepthStencilTargetInfo depthTargetInfo = {};
  depthTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
  depthTargetInfo.clear_depth = 1.0f;
  depthTargetInfo.store_op = SDL_GPU_STOREOP_DONT_CARE;
  depthTargetInfo.texture = depthTexture;

  u64 previous = SDL_GetPerformanceCounter();
  const u64 freq = SDL_GetPerformanceFrequency();
  const f32 keyboard_sensitivity = 50.0f;
  const f32 mouse_sensitivity = 0.1f;
  float delta_x, delta_y;
  while (running) {
    const u64 now = SDL_GetPerformanceCounter();
    const f32 dt = static_cast<f32>(now - previous) / static_cast<f32>(freq);
    previous = now;

    const bool* keys =  SDL_GetKeyboardState(nullptr);
    
    glm::vec3 forward{
      cos(glm::radians(camera.pitch)) * cos(glm::radians(camera.yaw)),
      sin(glm::radians(camera.pitch)),
      cos(glm::radians(camera.pitch)) * sin(glm::radians(camera.yaw))};

    forward = glm::normalize(forward);
    glm::vec3 right = glm::cross(WORLD_UP, forward); // reversed for Left handed

    u32 mouseButton = SDL_GetRelativeMouseState(&delta_x, &delta_y);
    camera.yaw -= delta_x * mouse_sensitivity;
    camera.yaw = std::fmod(camera.yaw, 360.0f);
    camera.pitch -= delta_y * mouse_sensitivity;
    camera.pitch = glm::clamp(camera.pitch, -90.0f, 90.0f);

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      if (event.type == SDL_EVENT_QUIT) {
	running = false;
      }
    }

    if (keys[SDL_SCANCODE_ESCAPE]) {
      running = false;
    }
    if (keys[SDL_SCANCODE_W]) {
      camera.position += forward * dt * keyboard_sensitivity;
    }
    if (keys[SDL_SCANCODE_S]) {
      camera.position -= forward * dt * keyboard_sensitivity;
    }
    if (keys[SDL_SCANCODE_A]) {
      camera.position -= right * dt * keyboard_sensitivity;
    }
    if (keys[SDL_SCANCODE_D]) {
      camera.position += right * dt * keyboard_sensitivity;
    }
    if (keys[SDL_SCANCODE_SPACE]) {
      camera.position += WORLD_UP * dt * keyboard_sensitivity;
    }
    if (keys[SDL_SCANCODE_LCTRL]) {
      camera.position -= WORLD_UP * dt * keyboard_sensitivity;
    }

    glm::mat4 view =
      glm::lookAtLH(camera.position, camera.position + forward, WORLD_UP);
    glm::mat4 projection = glm::perspectiveLH_ZO(
					    glm::radians(60.0f),
					    800.0f / 800.0f,
					    0.1f,
					    100.0f
					    );
    glm::mat4 viewProjection = projection * view;
    commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);

    sdlAssert(SDL_WaitAndAcquireGPUSwapchainTexture(
						    commandBuffer,
						    window,
						    &swapchainTexture,
						    &width,
						    &height));

    colorTargetInfo.texture = swapchainTexture;

    renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
    sdlAssert(renderPass != nullptr);

    SDL_PushGPUVertexUniformData(commandBuffer, 0, &viewProjection, (u32)sizeof(viewProjection));

    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
    SDL_BindGPUVertexBuffers(renderPass, 0, vertexBufferBinding, 1);
    SDL_BindGPUIndexBuffer(renderPass, indexBufferBinding,
                           SDL_GPU_INDEXELEMENTSIZE_32BIT);
    SDL_BindGPUFragmentSamplers(renderPass, 0, textureSamplerBindings, 1);
    SDL_DrawGPUIndexedPrimitives(
				 renderPass,
				 worldIndices.size(),
				 1,
				 0,
				 0,
				 0);
    SDL_EndGPURenderPass(renderPass);
    sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));
  }

  SDL_ReleaseGPUGraphicsPipeline(gpuDevice, pipeline);
  SDL_ReleaseGPUShader(gpuDevice, fragmentShader);
  SDL_ReleaseGPUShader(gpuDevice, vertexShader);
  SDL_ReleaseGPUBuffer(gpuDevice, vertexBuffer);
  SDL_DestroyGPUDevice(gpuDevice);    
  SDL_DestroyWindow(window);
  SDL_Quit();
  return 0;
}
