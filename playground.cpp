#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <cassert>
#include <cstdint>
#include <cstdio>
#include <cstring>
#include <fstream>
#include <vector>

#include "types.hpp"

#define sdlAssert(cond)                                                        \
    do {                                                                       \
        if (!(cond)) {                                                         \
            std::fprintf(stderr, "SDL error: %s\n", SDL_GetError());           \
            assert(cond);                                                      \
        }                                                                      \
    } while (0)

static std::vector<uint8_t> readFile(const char* path) {
    std::ifstream file(path, std::ios::binary | std::ios::ate);
    if (!file) {
        return {};
    }
    std::streamsize size = file.tellg();
    file.seekg(0, std::ios::beg);
    std::vector<uint8_t> buffer(static_cast<size_t>(size));
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

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;

    camera.position = Vec3{-10.0f, 0.0f, 0.0f};
    camera.yaw = 0.0f;
    camera.pitch = 0.0f;

    assert(SDL_Init(SDL_INIT_VIDEO));
    // defer: SDL_Quit();

    SDL_Window* window = SDL_CreateWindow("Window", 800, 800, 0);
    sdlAssert(window != nullptr);
    // defer: SDL_DestroyWindow(window);

    SDL_GPUTexture* swapchainTexture = nullptr;
    Uint32 width = 0;
    Uint32 height = 0;

    SDL_GPUDevice* gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    sdlAssert(gpuDevice != nullptr);
    // defer: SDL_DestroyGPUDevice(gpuDevice);

    sdlAssert(SDL_ClaimWindowForGPUDevice(gpuDevice, window));

    bool setSwapchainParams = SDL_SetGPUSwapchainParameters(
        gpuDevice,
        window,
        SDL_GPU_SWAPCHAINCOMPOSITION_SDR,
        SDL_GPU_PRESENTMODE_VSYNC
    );

    sdlAssert(setSwapchainParams);

    SDL_GPUCommandBuffer* commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);
    sdlAssert(commandBuffer != nullptr);

    SDL_GPUBufferCreateInfo gpuBufferInfo = {};
    gpuBufferInfo.size = (Uint32)sizeof(vertices);
    gpuBufferInfo.usage = SDL_GPU_BUFFERUSAGE_VERTEX;
    SDL_GPUBuffer* vertexBuffer = SDL_CreateGPUBuffer(gpuDevice, &gpuBufferInfo);
    sdlAssert(vertexBuffer != nullptr);
    // defer: SDL_ReleaseGPUBuffer(gpuDevice, vertexBuffer);

    SDL_GPUTransferBufferCreateInfo transferBufferInfo = {};
    transferBufferInfo.size = (Uint32)sizeof(vertices);
    transferBufferInfo.usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD;
    SDL_GPUTransferBuffer* transferBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferBufferInfo);
    sdlAssert(transferBuffer != nullptr);

    SDL_GPUBufferRegion gpuBufferRegion = {};
    gpuBufferRegion.offset = 0;
    gpuBufferRegion.buffer = vertexBuffer;
    gpuBufferRegion.size = (Uint32)sizeof(vertices);

    // block copy:
    {
        Vertex* mappedVertexInTransfer = reinterpret_cast<Vertex*>(
            SDL_MapGPUTransferBuffer(gpuDevice, transferBuffer, false));
        sdlAssert(mappedVertexInTransfer != nullptr);
        std::memcpy(mappedVertexInTransfer, vertices, sizeof(vertices));

        SDL_GPUCopyPass* copyPass = SDL_BeginGPUCopyPass(commandBuffer);
        sdlAssert(copyPass != nullptr);
        // defer: SDL_EndGPUCopyPass(copyPass);

        SDL_GPUTransferBufferLocation transferBufferLocation = {};
        transferBufferLocation.offset = 0;
        transferBufferLocation.transfer_buffer = transferBuffer;

        SDL_UploadToGPUBuffer(copyPass, &transferBufferLocation, &gpuBufferRegion, false);
        sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));

        // Unmap and release transfer buffer once uploaded
        SDL_UnmapGPUTransferBuffer(gpuDevice, transferBuffer);
        SDL_ReleaseGPUTransferBuffer(gpuDevice, transferBuffer);
    }

    std::vector<uint8_t> vertexShaderContents = readFile("./shaders/shader.vertex.spv");
    // echo "Vertex", vertexShaderContents

    SDL_GPUShaderCreateInfo vertexShaderInfo = {};
    vertexShaderInfo.entrypoint = "main";
    vertexShaderInfo.code = vertexShaderContents.data();
    vertexShaderInfo.code_size = vertexShaderContents.size();
    vertexShaderInfo.stage = SDL_GPU_SHADERSTAGE_VERTEX;
    vertexShaderInfo.format = SDL_GPU_SHADERFORMAT_SPIRV;
    vertexShaderInfo.num_samplers = 0;
    vertexShaderInfo.num_storage_textures = 0;
    vertexShaderInfo.num_storage_buffers = 0;
    vertexShaderInfo.num_uniform_buffers = 0;

    SDL_GPUShader* vertexShader = SDL_CreateGPUShader(gpuDevice, &vertexShaderInfo);
    sdlAssert(vertexShader != nullptr);
    // defer: SDL_ReleaseGPUShader(gpuDevice, vertexShader);

    std::vector<uint8_t> fragmentShaderContents = readFile("./shaders/shader.fragment.spv");
    // echo "Fragment", fragmentShaderContents

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
    // defer: SDL_ReleaseGPUShader(gpuDevice, fragmentShader);

    SDL_GPUGraphicsPipelineCreateInfo pipelineInfo = {};
    pipelineInfo.vertex_shader = vertexShader;
    pipelineInfo.fragment_shader = fragmentShader;
    pipelineInfo.primitive_type = SDL_GPU_PRIMITIVETYPE_TRIANGLELIST;

    SDL_GPUVertexBufferDescription bufferDescription[1] = {};
    // echo "Buffer length ", bufferDescription.len

    // How each vertex is defined
    bufferDescription[0].slot = 0;
    bufferDescription[0].input_rate = SDL_GPU_VERTEXINPUTRATE_VERTEX;
    bufferDescription[0].instance_step_rate = 0;
    bufferDescription[0].pitch = (Uint32)sizeof(Vertex);

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
    vertexAttributes[1].offset = (Uint32)(sizeof(float) * 3);

    pipelineInfo.vertex_input_state.num_vertex_attributes = 2;
    pipelineInfo.vertex_input_state.vertex_attributes = vertexAttributes;

    SDL_GPUColorTargetDescription colorTargetDescriptions[1] = {};
    colorTargetDescriptions[0].format = SDL_GetGPUSwapchainTextureFormat(gpuDevice, window);

    pipelineInfo.target_info.num_color_targets = 1;
    pipelineInfo.target_info.color_target_descriptions = colorTargetDescriptions;

    SDL_GPUGraphicsPipeline* pipeline = SDL_CreateGPUGraphicsPipeline(gpuDevice, &pipelineInfo);
    sdlAssert(pipeline != nullptr);
    // defer: SDL_ReleaseGPUGraphicsPipeline(gpuDevice, pipeline);

    SDL_GPUBufferBinding bufferBindings[1] = {};
    bufferBindings[0].buffer = vertexBuffer;
    bufferBindings[0].offset = 0;

    bool running = true;
    SDL_GPURenderPass* renderPass = nullptr;

    while (running) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                running = false;
            }
        }

        Mat4 viewMat = translateMatrix(camera.position);

        commandBuffer = SDL_AcquireGPUCommandBuffer(gpuDevice);

        sdlAssert(SDL_WaitAndAcquireGPUSwapchainTexture(
            commandBuffer,
            window,
            &swapchainTexture,
            &width,
            &height));

        SDL_GPUColorTargetInfo colorTargetInfo = {};
        colorTargetInfo.clear_color = SDL_FColor{0.25f, 0.0f, 0.0f, 1.0f};
        colorTargetInfo.load_op = SDL_GPU_LOADOP_CLEAR;
        colorTargetInfo.store_op = SDL_GPU_STOREOP_STORE;
        colorTargetInfo.texture = swapchainTexture;

        renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, nullptr);
        sdlAssert(renderPass != nullptr);

        SDL_PushGPUVertexUniformData(commandBuffer, 0, &viewMat, (Uint32)sizeof(viewMat));

        SDL_BindGPUGraphicsPipeline(renderPass, pipeline);
        SDL_BindGPUVertexBuffers(renderPass, 0, bufferBindings, 1);
        SDL_DrawGPUPrimitives(renderPass, 3, 1, 0, 0);
        SDL_EndGPURenderPass(renderPass);
        sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));
    }

    return 0;
}
