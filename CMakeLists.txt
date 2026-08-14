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

add_executable(playground
    playground.cpp
    types.cpp
    stb_image.cpp
)

target_include_directories(playground PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/includes)
target_link_libraries(playground PRIVATE SDL3::SDL3 glm::glm)

# SHADERS
set(SHADER_SRC_DIR ${CMAKE_CURRENT_SOURCE_DIR}/shaders)
set(SHADER_BIN_DIR ${CMAKE_CURRENT_BINARY_DIR}/shaders)

target_compile_definitions(playground PRIVATE
    SHADER_BIN_DIR="${SHADER_BIN_DIR}"
)

find_program(GLSLANG_VALIDATOR glslangValidator)

if(GLSLANG_VALIDATOR)
    add_custom_command(
        OUTPUT
            ${SHADER_BIN_DIR}/shader.vertex.spv
            ${SHADER_BIN_DIR}/shader.fragment.spv
        COMMAND ${CMAKE_COMMAND} -E make_directory ${SHADER_BIN_DIR}
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SRC_DIR}/shader.vert -o ${SHADER_BIN_DIR}/shader.vertex.spv
        COMMAND ${GLSLANG_VALIDATOR} -V ${SHADER_SRC_DIR}/shader.frag -o ${SHADER_BIN_DIR}/shader.fragment.spv
        DEPENDS
            ${SHADER_SRC_DIR}/shader.vert
            ${SHADER_SRC_DIR}/shader.frag
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
