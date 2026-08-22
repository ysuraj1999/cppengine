cmake_minimum_required(VERSION 3.16)
project(cppengine CXX)

# LSP
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(SDL3 REQUIRED)

include(FetchContent)
FetchContent_Declare(
    glm
    GIT_REPOSITORY https://github.com/g-truc/glm.git
    GIT_TAG        1.0.1
)
FetchContent_MakeAvailable(glm)

add_library(cppengine SHARED
  cppengine.cpp
  obj.cpp
  utils.cpp
  stb_image.cpp

  includes/imgui/imgui.cpp
  includes/imgui/imgui_draw.cpp
  includes/imgui/imgui_tables.cpp
  includes/imgui/imgui_widgets.cpp
  includes/imgui/backends/imgui_impl_sdl3.cpp
  includes/imgui/backends/imgui_impl_sdlgpu3.cpp
)

target_include_directories(cppengine PUBLIC
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/includes
  ${CMAKE_CURRENT_SOURCE_DIR}/includes/imgui)

target_link_libraries(cppengine PUBLIC
  SDL3::SDL3
  glm::glm)

add_executable(playground
  playground.cpp
)

target_link_libraries(playground PRIVATE cppengine)

# SHADERS
set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)
set(TEXTURE_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/textures)

target_compile_definitions(cppengine PRIVATE
  SHADER_BIN_DIR="${SHADER_BIN_DIR}"
  TEXTURE_BIN_DIR="${TEXTURE_BIN_DIR}"
)

find_program(GLSLANG_VALIDATOR glslangValidator)

if(GLSLANG_VALIDATOR)
    add_custom_command(
        OUTPUT
            ${SHADER_BIN_DIR}/shader.vertex.spv
            ${SHADER_BIN_DIR}/shader.fragment.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BIN_DIR}
        COMMAND ${GLSLANG_VALIDATOR} -V -S vert ${SHADER_SRC_DIR}/shader.vert.glsl -o ${SHADER_BIN_DIR}/shader.vertex.spv
        COMMAND ${GLSLANG_VALIDATOR} -V -S frag ${SHADER_SRC_DIR}/shader.frag.glsl -o ${SHADER_BIN_DIR}/shader.fragment.spv
        DEPENDS
            ${SHADER_SRC_DIR}/shader.vert.glsl
            ${SHADER_SRC_DIR}/shader.frag.glsl
        COMMENT "Compiling shaders with glslangValidator"
        VERBATIM
    )
else()
    message(WARNING "glslangValidator not found")
endif()

add_custom_target(shaders ALL
    DEPENDS
        ${SHADER_BIN_DIR}/shader.vertex.spv
        ${SHADER_BIN_DIR}/shader.fragment.spv
)

add_dependencies(playground shaders)

# TEXTURES
set(TEXTURE_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/textures)
set(TEXTURE_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/textures)

add_custom_target(textures ALL
    COMMAND ${CMAKE_COMMAND} -E copy_directory ${TEXTURE_SRC_DIR} ${TEXTURE_BIN_DIR}
    COMMENT "Copying textures to build directory"
    VERBATIM
)

add_dependencies(playground textures)
