#include "types.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_gpu.h>
#include <SDL3/SDL_main.h>
#include <SDL3/SDL_scancode.h>
#include <algorithm>
#include <cfloat>
#include <cstddef>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <format>
#include <glm/ext/matrix_clip_space.hpp>
#include <glm/ext/matrix_transform.hpp>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>
#include <string>
#include <unordered_set>
#include <vector>

#include "imgui/backends/imgui_impl_sdl3.h"
#include "imgui/backends/imgui_impl_sdlgpu3.h"
#include "imgui/imgui.h"

#include "obj.hpp"
#include "stb_image.h"
#include "types.hpp"
#include "utils.hpp"

#include <cfenv>

static std::fenv_t env;

void fpu_begin() {
    std::feholdexcept(&env);
}

void fpu_end() {
    std::fesetenv(&env);
}

// SDL boundary translation: SDL_Scancode -> canonical Key ABI value.
static Key sdl_to_key(SDL_Scancode scancode) {
    switch (scancode) {
    case SDL_SCANCODE_A:
        return KEY_A;
    case SDL_SCANCODE_B:
        return KEY_B;
    case SDL_SCANCODE_C:
        return KEY_C;
    case SDL_SCANCODE_D:
        return KEY_D;
    case SDL_SCANCODE_E:
        return KEY_E;
    case SDL_SCANCODE_F:
        return KEY_F;
    case SDL_SCANCODE_G:
        return KEY_G;
    case SDL_SCANCODE_H:
        return KEY_H;
    case SDL_SCANCODE_I:
        return KEY_I;
    case SDL_SCANCODE_J:
        return KEY_J;
    case SDL_SCANCODE_K:
        return KEY_K;
    case SDL_SCANCODE_L:
        return KEY_L;
    case SDL_SCANCODE_M:
        return KEY_M;
    case SDL_SCANCODE_N:
        return KEY_N;
    case SDL_SCANCODE_O:
        return KEY_O;
    case SDL_SCANCODE_P:
        return KEY_P;
    case SDL_SCANCODE_Q:
        return KEY_Q;
    case SDL_SCANCODE_R:
        return KEY_R;
    case SDL_SCANCODE_S:
        return KEY_S;
    case SDL_SCANCODE_T:
        return KEY_T;
    case SDL_SCANCODE_U:
        return KEY_U;
    case SDL_SCANCODE_V:
        return KEY_V;
    case SDL_SCANCODE_W:
        return KEY_W;
    case SDL_SCANCODE_X:
        return KEY_X;
    case SDL_SCANCODE_Y:
        return KEY_Y;
    case SDL_SCANCODE_Z:
        return KEY_Z;
    case SDL_SCANCODE_1:
        return KEY_1;
    case SDL_SCANCODE_2:
        return KEY_2;
    case SDL_SCANCODE_3:
        return KEY_3;
    case SDL_SCANCODE_4:
        return KEY_4;
    case SDL_SCANCODE_5:
        return KEY_5;
    case SDL_SCANCODE_6:
        return KEY_6;
    case SDL_SCANCODE_7:
        return KEY_7;
    case SDL_SCANCODE_8:
        return KEY_8;
    case SDL_SCANCODE_9:
        return KEY_9;
    case SDL_SCANCODE_0:
        return KEY_0;
    case SDL_SCANCODE_RETURN:
        return KEY_ENTER;
    case SDL_SCANCODE_ESCAPE:
        return KEY_ESCAPE;
    case SDL_SCANCODE_BACKSPACE:
        return KEY_BACKSPACE;
    case SDL_SCANCODE_TAB:
        return KEY_TAB;
    case SDL_SCANCODE_SPACE:
        return KEY_SPACE;
    case SDL_SCANCODE_MINUS:
        return KEY_MINUS;
    case SDL_SCANCODE_EQUALS:
        return KEY_EQUALS;
    case SDL_SCANCODE_LEFTBRACKET:
        return KEY_LEFT_BRACKET;
    case SDL_SCANCODE_RIGHTBRACKET:
        return KEY_RIGHT_BRACKET;
    case SDL_SCANCODE_BACKSLASH:
        return KEY_BACKSLASH;
    case SDL_SCANCODE_SEMICOLON:
        return KEY_SEMICOLON;
    case SDL_SCANCODE_APOSTROPHE:
        return KEY_APOSTROPHE;
    case SDL_SCANCODE_GRAVE:
        return KEY_GRAVE;
    case SDL_SCANCODE_COMMA:
        return KEY_COMMA;
    case SDL_SCANCODE_PERIOD:
        return KEY_PERIOD;
    case SDL_SCANCODE_SLASH:
        return KEY_SLASH;
    case SDL_SCANCODE_F1:
        return KEY_F1;
    case SDL_SCANCODE_F2:
        return KEY_F2;
    case SDL_SCANCODE_F3:
        return KEY_F3;
    case SDL_SCANCODE_F4:
        return KEY_F4;
    case SDL_SCANCODE_F5:
        return KEY_F5;
    case SDL_SCANCODE_F6:
        return KEY_F6;
    case SDL_SCANCODE_F7:
        return KEY_F7;
    case SDL_SCANCODE_F8:
        return KEY_F8;
    case SDL_SCANCODE_F9:
        return KEY_F9;
    case SDL_SCANCODE_F10:
        return KEY_F10;
    case SDL_SCANCODE_F11:
        return KEY_F11;
    case SDL_SCANCODE_F12:
        return KEY_F12;
    case SDL_SCANCODE_INSERT:
        return KEY_INSERT;
    case SDL_SCANCODE_HOME:
        return KEY_HOME;
    case SDL_SCANCODE_PAGEUP:
        return KEY_PAGE_UP;
    case SDL_SCANCODE_DELETE:
        return KEY_DELETE;
    case SDL_SCANCODE_END:
        return KEY_END;
    case SDL_SCANCODE_PAGEDOWN:
        return KEY_PAGE_DOWN;
    case SDL_SCANCODE_RIGHT:
        return KEY_RIGHT;
    case SDL_SCANCODE_LEFT:
        return KEY_LEFT;
    case SDL_SCANCODE_DOWN:
        return KEY_DOWN;
    case SDL_SCANCODE_UP:
        return KEY_UP;
    case SDL_SCANCODE_LCTRL:
        return KEY_LEFT_CTRL;
    case SDL_SCANCODE_LSHIFT:
        return KEY_LEFT_SHIFT;
    case SDL_SCANCODE_LALT:
        return KEY_LEFT_ALT;
    case SDL_SCANCODE_LGUI:
        return KEY_LEFT_GUI;
    case SDL_SCANCODE_RCTRL:
        return KEY_RIGHT_CTRL;
    case SDL_SCANCODE_RSHIFT:
        return KEY_RIGHT_SHIFT;
    case SDL_SCANCODE_RALT:
        return KEY_RIGHT_ALT;
    case SDL_SCANCODE_RGUI:
        return KEY_RIGHT_GUI;
    default:
        return KEY_UNKNOWN;
    }
}

// SDL boundary translation: SDL button number -> canonical MouseButton value.
static MouseButton sdl_to_mouse_button(Uint8 button) {
    switch (button) {
    case SDL_BUTTON_LEFT:
        return MOUSE_BUTTON_LEFT;
    case SDL_BUTTON_MIDDLE:
        return MOUSE_BUTTON_MIDDLE;
    case SDL_BUTTON_RIGHT:
        return MOUSE_BUTTON_RIGHT;
    case SDL_BUTTON_X1:
        return MOUSE_BUTTON_X1;
    case SDL_BUTTON_X2:
        return MOUSE_BUTTON_X2;
    default:
        return (MouseButton)button;
    }
}

//-----------------------------------------------------------------------------
// GPUResources
//-----------------------------------------------------------------------------

#define GPU_INIT_CHECK(cond, msg)                                                                                      \
    do {                                                                                                               \
        if (!(cond)) {                                                                                                 \
            std::fprintf(stderr, "%s:%d: gpu_init failed: %s (%s)\n", __FILE__, __LINE__, #cond, msg);                  \
            destroy();                                                                                                 \
            return false;                                                                                              \
        }                                                                                                              \
    } while (0)

bool GPUResources::init(RendererConfig &config) {
    if (gpuDevice != nullptr || window != nullptr) {
        std::fprintf(stderr, "GPUResources::init: already initialized (call destroy first)\n");
        return false;
    }

    GPU_INIT_CHECK(SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS), "SDL subsystem initialization");

    window = SDL_CreateWindow("Lens", (u32)config.resolution.x, (u32)config.resolution.y, 0);
    GPU_INIT_CHECK(window != nullptr, "window creation");

    gpuDevice = SDL_CreateGPUDevice(SDL_GPU_SHADERFORMAT_SPIRV, false, "vulkan");
    GPU_INIT_CHECK(gpuDevice != nullptr, "GPU device creation");

    GPU_INIT_CHECK(SDL_ClaimWindowForGPUDevice(gpuDevice, window), "claim window for GPU device");

    GPU_INIT_CHECK(
        SDL_SetGPUSwapchainParameters(gpuDevice, window, SDL_GPU_SWAPCHAINCOMPOSITION_SDR, SDL_GPU_PRESENTMODE_VSYNC),
        "swapchain configuration"
    );

    std::vector<u8> vertexShaderContents = readFile(SHADER_BIN_DIR "/shader.vertex.spv");
    ASSERT(!vertexShaderContents.empty(), SHADER_BIN_DIR "/shader.vertex.spv missing or empty");

    SDL_GPUShaderCreateInfo vertexShaderInfo = {
        .code_size = vertexShaderContents.size(),
        .code = vertexShaderContents.data(),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_VERTEX,
        .num_samplers = 0,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1,
    };
    vertexShader = SDL_CreateGPUShader(gpuDevice, &vertexShaderInfo);
    GPU_INIT_CHECK(vertexShader != nullptr, "vertex shader creation");

    std::vector<u8> fragmentShaderContents = readFile(SHADER_BIN_DIR "/shader.fragment.spv");
    ASSERT(!fragmentShaderContents.empty(), SHADER_BIN_DIR "/shader.fragment.spv missing or empty");

    SDL_GPUShaderCreateInfo fragmentShaderInfo = {
        .code_size = fragmentShaderContents.size(),
        .code = fragmentShaderContents.data(),
        .entrypoint = "main",
        .format = SDL_GPU_SHADERFORMAT_SPIRV,
        .stage = SDL_GPU_SHADERSTAGE_FRAGMENT,
        .num_samplers = 1,
        .num_storage_textures = 0,
        .num_storage_buffers = 0,
        .num_uniform_buffers = 1,
    };
    fragmentShader = SDL_CreateGPUShader(gpuDevice, &fragmentShaderInfo);
    GPU_INIT_CHECK(fragmentShader != nullptr, "fragment shader creation");

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
    vertexBufferDescription[1].instance_step_rate = 1;
    vertexBufferDescription[1].pitch = (u32)sizeof(glm::mat4);

    pipelineInfo.vertex_input_state.num_vertex_buffers = 2;
    pipelineInfo.vertex_input_state.vertex_buffer_descriptions = vertexBufferDescription;

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

    vertexAttributes[4] = {.location = 4, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = 0};
    vertexAttributes[5] = {
        .location = 5, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = sizeof(glm::vec4)
    };
    vertexAttributes[6] = {
        .location = 6, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = 2 * sizeof(glm::vec4)
    };
    vertexAttributes[7] = {
        .location = 7, .buffer_slot = 1, .format = SDL_GPU_VERTEXELEMENTFORMAT_FLOAT4, .offset = 3 * sizeof(glm::vec4)
    };

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
    GPU_INIT_CHECK(pipeline != nullptr, "graphics pipeline creation");

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
        GPU_INIT_CHECK(checkTexture != nullptr, "default texture creation");

        SDL_GPUTransferBufferCreateInfo transferToTextureInfo = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = TEXTURE_SIZE,
        };
        SDL_GPUTransferBuffer *transferToTexture = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToTextureInfo);
        GPU_INIT_CHECK(transferToTexture != nullptr, "texture transfer buffer creation");

        u8 *mappedTransferToBytes =
            reinterpret_cast<u8 *>(SDL_MapGPUTransferBuffer(gpuDevice, transferToTexture, false));
        GPU_INIT_CHECK(mappedTransferToBytes != nullptr, "texture transfer buffer map");
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
        SDL_GPUCommandBuffer *initCmd = SDL_AcquireGPUCommandBuffer(gpuDevice);
        GPU_INIT_CHECK(initCmd != nullptr, "texture upload command buffer acquisition");
        SDL_GPUCopyPass *copyPass = SDL_BeginGPUCopyPass(initCmd);
        GPU_INIT_CHECK(copyPass != nullptr, "texture copy pass begin");
        SDL_UploadToGPUTexture(copyPass, &textureTransferInfo, &textureRegion, false);
        SDL_EndGPUCopyPass(copyPass);
        GPU_INIT_CHECK(SDL_SubmitGPUCommandBuffer(initCmd), "texture upload submission");

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
        GPU_INIT_CHECK(sampler != nullptr, "sampler creation");

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
        GPU_INIT_CHECK(depthTexture != nullptr, "depth texture creation");
    }

    return true;
}

void GPUResources::rebuild(const GeometryStorage &geometry) {
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
    sdlAssert(vertexBuffer != nullptr);

    const u32 INDEX_BUFFER_SIZE = geometry.indices.size() * sizeof(u32);
    SDL_GPUBufferCreateInfo indexBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_INDEX,
        .size = INDEX_BUFFER_SIZE,
    };
    indexBuffer = SDL_CreateGPUBuffer(gpuDevice, &indexBufferInfo);
    sdlAssert(indexBuffer != nullptr);

    SDL_GPUBufferCreateInfo instanceBufferInfo = {
        .usage = SDL_GPU_BUFFERUSAGE_VERTEX,
        .size = INSTANCE_PREALLOCATED_SIZE,
    };
    instanceBuffer = SDL_CreateGPUBuffer(gpuDevice, &instanceBufferInfo);
    sdlAssert(instanceBuffer != nullptr);

    SDL_GPUTransferBufferCreateInfo transferToInstanceBufferInfo = {
        .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
        .size = INSTANCE_PREALLOCATED_SIZE,
    };
    transferToInstanceBuffer = SDL_CreateGPUTransferBuffer(gpuDevice, &transferToInstanceBufferInfo);
    sdlAssert(transferToInstanceBuffer != nullptr);

    if (VERTEX_BUFFER_SIZE > 0) {
        SDL_GPUTransferBufferCreateInfo transferToVertexBufferInfo = {
            .usage = SDL_GPU_TRANSFERBUFFERUSAGE_UPLOAD,
            .size = VERTEX_BUFFER_SIZE,
        };
        SDL_GPUTransferBuffer *transferToVertexBuffer =
            SDL_CreateGPUTransferBuffer(gpuDevice, &transferToVertexBufferInfo);
        sdlAssert(transferToVertexBuffer != nullptr);

        Vertex *mappedVertexInTransfer =
            reinterpret_cast<Vertex *>(SDL_MapGPUTransferBuffer(gpuDevice, transferToVertexBuffer, false));
        sdlAssert(mappedVertexInTransfer != nullptr);
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
            sdlAssert(transferToIndexBuffer != nullptr);

            u32 *mappedIndicesInTransfer =
                reinterpret_cast<u32 *>(SDL_MapGPUTransferBuffer(gpuDevice, transferToIndexBuffer, false));
            sdlAssert(mappedIndicesInTransfer != nullptr);
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
            sdlAssert(syncCmd != nullptr);
            SDL_GPUCopyPass *initialCopyPass = SDL_BeginGPUCopyPass(syncCmd);
            sdlAssert(initialCopyPass != nullptr);

            SDL_UploadToGPUBuffer(initialCopyPass, &transferToVertexBufferLocation, &vertexBufferRegion, false);
            SDL_UploadToGPUBuffer(initialCopyPass, &transferToIndexBufferLocation, &indexBufferRegion, false);
            SDL_EndGPUCopyPass(initialCopyPass);
            sdlAssert(SDL_SubmitGPUCommandBuffer(syncCmd));

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

void GPUResources::destroy() {
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

//-----------------------------------------------------------------------------
// Renderer
//-----------------------------------------------------------------------------

bool Renderer::init() {
    if (gpu.initialized()) {
        std::fprintf(stderr, "Renderer::init: already initialized (call quit first)\n");
        return false;
    }

    config = RendererConfig{
        .resolution = {1800.0f, 1000.0f},
        .backgroundColor = {47.0f / 255.0f, 149.0f / 255.0f, 78.0f / 255.0f, 1.0f},
        .keyboardSensitivity = 200.0f,
        .mouseSensitivity = 0.1f,
        .camera = {.position = {-5.0f, 0.0f, 0.0f}, .yaw = 0.0f, .pitch = 0.0f},
    };
    config.camera.update();

    shouldClose = false;

    fpu_begin();
    if (!gpu.init(config)) {
        fpu_end();
        return false;
    }
    frame.init();
    fpu_end();
    return true;
}

void Renderer::quit() {
    gpu.destroy();
    shouldClose = false;
}

bool Renderer::clear() {
    geometry.vertices.clear();
    geometry.indices.clear();
    geometry.meshRegions.clear();
    geometry.meshOrder.clear();
    return true;
}

void Renderer::rebuild() {
    gpu.rebuild(geometry);
}

u64 Renderer::load_obj(const char *path) {
    std::vector<Mesh> meshCollection = loadAllMeshFromObj(path);
    for (auto &m : meshCollection)
        geometry.add_mesh(std::move(m));
    return meshCollection.size();
}

void Renderer::camera_move(f32 forward, f32 right, f32 up) {
    config.camera.move(forward, right, up);
}

void Renderer::camera_rotate(f32 yaw, f32 pitch) {
    config.camera.rotate(yaw, pitch);
    config.camera.update();
}

void Renderer::camera_look_at(f32 x, f32 y, f32 z) {
    config.camera.look_at(glm::vec3(x, y, z));
}

//-----------------------------------------------------------------------------
// RendererFrame
//-----------------------------------------------------------------------------

void RendererFrame::init() {
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

void RendererFrame::begin() {
    instanceBufferOffset = 0;
    commandBuffer = SDL_AcquireGPUCommandBuffer(gpu.gpuDevice);
    sdlAssert(commandBuffer != nullptr);

    sdlAssert(SDL_WaitAndAcquireGPUSwapchainTexture(
        commandBuffer, gpu.window, &swapchainTexture, &swapchainWidth, &swapchainHeight
    ));

    colorTargetInfo.texture = swapchainTexture;

    renderPass = SDL_BeginGPURenderPass(commandBuffer, &colorTargetInfo, 1, &depthTargetInfo);
    sdlAssert(renderPass != nullptr);

    ASSERT(renderPass != nullptr && commandBuffer != nullptr, "draw called outside render_begin/end block");

    glm::mat4 view = config.camera.getViewMatrix();
    glm::mat4 projection =
        glm::perspectiveLH_ZO(glm::radians(60.0f), config.resolution.x / config.resolution.y, 0.1f, 10000.0f);
    glm::mat4 viewProjection = projection * view;

    if (!scene.instances.empty()) {
        SDL_BindGPUGraphicsPipeline(renderPass, gpu.pipeline);
        SDL_PushGPUVertexUniformData(commandBuffer, 0, &viewProjection, (u32)sizeof(viewProjection));
        SDL_PushGPUFragmentUniformData(commandBuffer, 0, &config.resolution, (u32)sizeof(config.resolution));
        SDL_BindGPUVertexBuffers(renderPass, 0, gpu.vertexBufferBinding, 1);
        SDL_BindGPUVertexBuffers(renderPass, 1, gpu.instanceBufferBinding, 1);
        SDL_BindGPUIndexBuffer(renderPass, gpu.indexBufferBinding, SDL_GPU_INDEXELEMENTSIZE_32BIT);
        SDL_BindGPUFragmentSamplers(renderPass, 0, gpu.textureSamplerBindings, 1);
    }
}

void RendererFrame::render_scene() {
    u64 i = 0;
    while (i < scene.instances.size()) {
        MeshHandle meshHandle = scene.instances[i].mesh;
        u64 j = i;
        while (j < scene.instances.size() && scene.instances[j].mesh == meshHandle)
            j++;
        auto regionIt = geometry.meshRegions.find(meshHandle);
        ASSERT(regionIt != geometry.meshRegions.end(), "mesh never synced - call rebuild first");
        draw_mesh(regionIt->second, scene.instances, i, j - i);
        i = j;
    }
}

void RendererFrame::end() {
    SDL_EndGPURenderPass(renderPass);
    sdlAssert(SDL_SubmitGPUCommandBuffer(commandBuffer));
}

void RendererFrame::draw_mesh(MeshRegion &meshRegion, const Instances &instances, u64 start, u64 count) {
    SDL_GPUCopyPass *frameCopyPass = SDL_BeginGPUCopyPass(commandBuffer);
    sdlAssert(frameCopyPass != nullptr);

    const u32 requiredInstanceCopySize = count * sizeof(glm::mat4);
    ASSERT(
        instanceBufferOffset + requiredInstanceCopySize <= gpu.INSTANCE_PREALLOCATED_SIZE,
        "instance buffer overflow - raise INSTANCE_PREALLOCATED_SIZE"
    );

    glm::mat4 *mappedToInstanceInTransfer =
        reinterpret_cast<glm::mat4 *>(SDL_MapGPUTransferBuffer(gpu.gpuDevice, gpu.transferToInstanceBuffer, false));
    sdlAssert(mappedToInstanceInTransfer != nullptr);

    glm::mat4 *dst = (glm::mat4 *)((u8 *)mappedToInstanceInTransfer + instanceBufferOffset);
    for (u64 k = 0; k < count; k++)
        *dst++ = transformToMat4(instances[start + k].transform);

    SDL_GPUTransferBufferLocation transferToInstanceBufferLocation = {
        .transfer_buffer = gpu.transferToInstanceBuffer,
        .offset = instanceBufferOffset,
    };
    SDL_GPUBufferRegion instanceBufferRegion = {
        .buffer = gpu.instanceBuffer,
        .offset = instanceBufferOffset,
        .size = requiredInstanceCopySize,
    };

    SDL_UploadToGPUBuffer(frameCopyPass, &transferToInstanceBufferLocation, &instanceBufferRegion, false);
    SDL_EndGPUCopyPass(frameCopyPass);

    SDL_DrawGPUIndexedPrimitives(
        renderPass, meshRegion.indexCount, count, meshRegion.indexOffset, meshRegion.vertexOffset, 0
    );
    instanceBufferOffset += requiredInstanceCopySize;
}

//-----------------------------------------------------------------------------
// UI
//-----------------------------------------------------------------------------

bool UI::init(GPUResources &gpu) {
    if (ImGui::GetCurrentContext() != nullptr) {
        std::fprintf(stderr, "UI::init: already initialized (call quit first)\n");
        return false;
    }
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ctx = ImGui::GetCurrentContext();
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

void UI::quit() {
    if (!ImGui::GetCurrentContext())
        return;
    ImGuiIO &io = ImGui::GetIO();
    if (io.BackendRendererUserData)
        ImGui_ImplSDLGPU3_Shutdown();
    if (io.BackendPlatformUserData)
        ImGui_ImplSDL3_Shutdown();
    ImGui::DestroyContext();
    ctx = nullptr;
}

void UI::begin() {
    fpu_begin();
    ImGui_ImplSDLGPU3_NewFrame();
    ImGui_ImplSDL3_NewFrame();
    ImGui::NewFrame();
}

void UI::end(SDL_GPUCommandBuffer *commandBuffer, SDL_GPURenderPass *renderPass) {
    ImGui::Render();
    ImDrawData *draw_data = ImGui::GetDrawData();
    ImGui_ImplSDLGPU3_PrepareDrawData(draw_data, commandBuffer);
    ImGui_ImplSDLGPU3_RenderDrawData(draw_data, commandBuffer, renderPass);
    fpu_end();
}

bool UI::begin_window(const char *name) {
    return ImGui::Begin(name);
}
void UI::end_window() {
    ImGui::End();
}
bool UI::button(const char *label) {
    return ImGui::Button(label);
}
bool UI::begin_tree(const char *label) {
    return ImGui::TreeNode(label);
}
void UI::end_tree() {
    ImGui::TreePop();
}
bool UI::selectable(const char *label, bool selected) {
    return ImGui::Selectable(label, selected);
}
void UI::sameline() {
    ImGui::SameLine();
}
void UI::text(const char *text) {
    ImGui::Text("%s", text);
}
void UI::push_id(int id) {
    ImGui::PushID(id);
}
void UI::pop_id() {
    ImGui::PopID();
}
bool UI::begin_context_menu_item() {
    return ImGui::BeginPopupContextItem();
}
bool UI::begin_context_menu_window() {
    return ImGui::BeginPopupContextWindow(nullptr, ImGuiPopupFlags_NoOpenOverItems);
}
void UI::end_context_menu() {
    ImGui::EndPopup();
}
bool UI::menu_item(const char *label) {
    return ImGui::MenuItem(label);
}

static ImGuiMouseButton imgui_button(MouseButton button) {
    switch (button) {
    case MOUSE_BUTTON_LEFT:
        return ImGuiMouseButton_Left;
    case MOUSE_BUTTON_RIGHT:
        return ImGuiMouseButton_Right;
    case MOUSE_BUTTON_MIDDLE:
        return ImGuiMouseButton_Middle;
    case MOUSE_BUTTON_X1:
        return ImGuiMouseButton_Left;
    case MOUSE_BUTTON_X2:
        return ImGuiMouseButton_Right;
    default:
        return ImGuiMouseButton_Left;
    }
}

bool UI::item_double_clicked(MouseButton button) {
    return ImGui::IsMouseDoubleClicked(imgui_button(button)) && ImGui::IsItemHovered(ImGuiHoveredFlags_None);
}
bool UI::item_clicked(MouseButton button) {
    return ImGui::IsItemClicked(imgui_button(button));
}

//-----------------------------------------------------------------------------
// Input
//-----------------------------------------------------------------------------

void Keyboard::end() {
    const bool *sdl_state = SDL_GetKeyboardState(nullptr);
    memset(current, 0, sizeof(current));
    for (int i = 0; i < SDL_SCANCODE_COUNT; i++)
        if (sdl_state[i]) {
            Key k = sdl_to_key((SDL_Scancode)i);
            if (k != KEY_UNKNOWN)
                current[k] = true;
        }
}

void Input::poll() {
    begin();

    ImGuiIO &io = ImGui::GetIO();

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        ImGui_ImplSDL3_ProcessEvent(&event);

        switch (event.type) {
        case SDL_EVENT_QUIT: {
            shouldClose = true;
            break;
        }
        case SDL_EVENT_MOUSE_MOTION: {
            const f32 xrel = io.WantCaptureMouse ? 0.0f : event.motion.xrel;
            const f32 yrel = io.WantCaptureMouse ? 0.0f : event.motion.yrel;
            mouse.on_motion(event.motion.x, event.motion.y, xrel, yrel);
            push_event(
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
            const MouseButton button = sdl_to_mouse_button(event.button.button);
            mouse.on_button_down(button, event.button.x, event.button.y, !io.WantCaptureMouse);
            push_event(
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
            const MouseButton button = sdl_to_mouse_button(event.button.button);
            mouse.on_button_up(button, event.button.x, event.button.y);
            push_event(
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
                push_event(
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
                const Key key = sdl_to_key(event.key.scancode);
                const bool down = event.type == SDL_EVENT_KEY_DOWN;
                push_event(
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
        keyboard.clear_current_state();
}

f32 Input::time_dt() {
    static const u64 timerFrequency = SDL_GetPerformanceFrequency();
    const u64 now = SDL_GetPerformanceCounter();
    const f32 dt = static_cast<f32>(now - prevCounter) / static_cast<f32>(timerFrequency);
    prevCounter = now;
    return dt;
}