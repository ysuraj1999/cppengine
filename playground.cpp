#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <glm/glm.hpp>

#include <cassert>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <fstream>
#include <iostream>
#include <vector>

#include "glm/ext/matrix_clip_space.hpp"
#include "glm/ext/matrix_transform.hpp"
#include "glm/geometric.hpp"
#include "glm/trigonometric.hpp"
#include "types.hpp"

#define sdlAssert(cond)					\
  do {							\
    if (!(cond)) {					\
      std::fprintf(stderr, "%s:%d: SDL error: %s\n",	\
		   __FILE__, __LINE__, SDL_GetError());	\
      std::abort();					\
    }							\
  } while (0)

static std::vector<u8> readFile(const char* path) {
  std::ifstream file(path, std::ios::binary | std::ios::ate);
  if (!file) {
    return {};
  }
  std::streamsize size = file.tellg();
  file.seekg(0, std::ios::beg);
  std::vector<u8> buffer(static_cast<size_t>(size));
  if (size > 0) {
    file.read(reinterpret_cast<char*>(buffer.data()), size);
  }
  return buffer;
}

static Camera camera;

static const Vertex vertices[3] = {
  { {0.0f, 0.5f, 0.0f},   {1.0f, 0.0f, 0.0f, 1.0f}, {} },
  { {0.0f, -0.5f, -0.5f}, {1.0f, 1.0f, 0.0f, 1.0f}, {} },
  { {0.0f, -0.5f, 0.5f},  {1.0f, 0.0f, 1.0f, 1.0f}, {} },
};

auto WORLD_UP = glm::vec3(0.0, 1.0, 0.0);

int main(int argc, char* argv[]) {
  // (void)argc;
  // (void)argv;

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

  SDL_GPUBufferCreateInfo gpuBufferInfo = {};
  gpuBufferInfo.size = (u32)sizeof(vertices);
  gpuBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
  SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &gpuBufferInfo);
  sdlAssert(vertexBuffer != nullptr);

  SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
  transferBufferInfo.size = (u32)sizeof(vertices);
  transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
  SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferBufferInfo);
  sdlAssert(transferBuffer != nullptr);

  SDL_GPUBufferRegion gpuBufferRegion = {};
  gpuBufferRegion.offset = 0;
  gpuBufferRegion.buffer = vertexBuffer;
  gpuBufferRegion.size = (u32)sizeof(vertices);

  // block copy:
  {
    Vertex* mappedVertexInTransfer = reinterpret_cast<Vertex*>(
							       SDL_MapGPUTransferBuffer(gpuDevice, transferBuffer, false));
    sdlAssert(mappedVertexInTransfer != nullptr);
    std::memcpy(mappedVertexInTransfer, vertices, sizeof(vertices));

    SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
    sdlAssert(copyPass != nullptr);


    SDL_GPUTransferBufferLocation transferBufferLocation = {};
    transferBufferLocation.offset = 0;
    transferBufferLocation.transfer_buffer = transferBuffer;

    SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation,
			  &gpuBufferRegion, false);
    SDL_EndGPUCopyPass(copyPass);
    sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));

    // Unmap and release transfer buffer once uploaded
    SDL_UnmapGPUTransferBuffer(gpuDevice, transferBuffer);
    SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);

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
  fragmentShaderInfo.num_samplers = 0;
  fragmentShaderInfo.num_storage_textures = 0;
  fragmentShaderInfo.num_storage_buffers = 0;
  fragmentShaderInfo.num_uniform_buffers = 0;

  SDL_GPUShader* fragmentShader = SDL_CreateGPUShader(gpuDevice, &fragmentShaderInfo);
  sdlAssert(fragmentShader != nullptr);

  SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
  pipelineInfo.vertex_shader = vertexShader;
  pipelineInfo.fragment_shader = fragmentShader;
  pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

  SDL_GPUVertexBufferDescription bufferDescription[1] = {};

  // How each vertex is defined
  bufferDescription[0].slot = 0;
  bufferDescription[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
  bufferDescription[0].instance_step_rate = 0;
  bufferDescription[0].pitch = (u32)sizeof(Vertex);

  pipelineInfo.vertex_input_state.num_vertex_buffers = 1;
  pipelineInfo.vertex_input_state.vertex_buffer_descriptions = bufferDescription;

  // Vertex attributes
  SDL_GPUVertexAttribute vertexAttributes[2] = {};
  vertexAttributes[0].buffer_slot = 0;
  vertexAttributes[0].location = 0;
  vertexAttributes[0].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT3;
  vertexAttributes[0].offset = 0;

  vertexAttributes[1].buffer_slot = 0;
  vertexAttributes[1].location = 1;
  vertexAttributes[1].format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4;
  vertexAttributes[1].offset = (u32)(sizeof(float) * 3);

  pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
  pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

  SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {};
  colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window);

  pipelineInfo.target_info.num_color_targets = 1;
  pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

  SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineInfo);
  sdlAssert(pipeline != nullptr);

  SDL_GPUBufferBinding bufferBindings[1] = {};
  bufferBindings[0].buffer = vertexBuffer;
  bufferBindings[0].offset = 0;

  bool running = true;
  SDL_GPURenderPass* renderPass = nullptr;

  u64 previous = SDL_GetPerformanceCounter();
  const u64 freq = SDL_GetPerformanceFrequency();
  const f32 keyboard_sensitivity = 5.0f;
  const f32 mouse_sensitivity = 0.05f;
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

    SDL_GPUColorTargetInfo colorTargetInfo = {};
    colorTargetInfo.clear_color = SDL_FColor{13.0f/255.0f, 115.0f/255.0f, 89.0f/255.0f, 1.0f};
    colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
    colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
    colorTargetInfo.texture = swapchainTexture;

    renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);
    sdlAssert(renderPass != nullptr);

    SDL_PushGPUVertexUniformData(commandBuffer, 0, &viewProjection, (u32)sizeof(viewProjection));

    SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
    SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);
    SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
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
