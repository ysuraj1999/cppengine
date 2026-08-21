#include "cppengine.h"
#include "types.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <SDL3/SDL_scancode.h>
#include <SDL3/SDL_gpu.h>
#include <glm/glm.hpp>
#include <glm/geometric.hpp>
#include <glm/trigonometric.hpp>
#include <cmath>
#include <iostream>

extern RendererConfig rendererConfig;
extern glm::vec3 WORLD_UP;

int main(int argc, char* argv[]) {
  // (void)argc;
  // (void)argv;

  renderer_init();

  SceneHandle sh = renderer_scene_create();

  renderer_scene_load_obj(sh, "models/bmw.obj");
  std::cout << "Length " << renderer_scene_mesh_count(sh) << '\n';

  u64 meshCount = renderer_scene_mesh_count(sh);
  for (u64 i = 0; i < meshCount; i++) {
    MeshHandle mh = renderer_scene_mesh_at(sh, i);
    renderer_scene_add_instance(sh, mh,
                                0.0f, 0.0f, 0.0f,   // position
                                1.0f, 1.0f, 1.0f,   // scale
                                0.0f, 0.0f, 0.0f);  // rotation
  }

  // for (u32 x = 0; x < GRID_SIZE; x++){
  //   for (u32 y = 0; y < GRID_SIZE; y++){
  //     for (u32 z = 0; z < GRID_SIZE; z++) {
  // 	scene_add_instance(&scene, 0, Transform{
  //           .position = {1.0f * x, 1.0f * y, 1.0f * z},
  //           .scale = {1.0f, 1.0f, 1.0f},
  //           .rotation = {0.0f, 0.0f, 0.0f}
  // 	  });
  //     }
  //   }
  // }
  
  renderer_sync_scene(sh);

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
    rendererConfig.camera.forward = forward;
    glm::vec3 right = glm::cross(WORLD_UP, forward); // reversed for Left handed
    rendererConfig.camera.right = right;

    renderer_poll_events();

    if (keys[SDL_SCANCODE_ESCAPE]) {
      running = false;
    }

    if (rendererConfig.cameraEnabled){
      u32 mouseButton = SDL_GetRelativeMouseState(&delta_x, &delta_y);
      rendererConfig.camera.yaw -= delta_x * rendererConfig.mouseSensitivity;
      rendererConfig.camera.yaw = std::fmod(rendererConfig.camera.yaw, 360.0f);
      rendererConfig.camera.pitch -= delta_y * rendererConfig.mouseSensitivity;
      rendererConfig.camera.pitch = glm::clamp(rendererConfig.camera.pitch, -90.0f, 90.0f);

      f32 keyboardSensitivity = rendererConfig.keyboardSensitivity;
      if (keys[SDL_SCANCODE_LSHIFT]) {
	keyboardSensitivity *= 2.0f;
      }
      if (keys[SDL_SCANCODE_W]) {
	rendererConfig.camera.position += forward * dt * keyboardSensitivity;
      }
      if (keys[SDL_SCANCODE_S]) {
	rendererConfig.camera.position -= forward * dt * keyboardSensitivity;
      }
      if (keys[SDL_SCANCODE_A]) {
	rendererConfig.camera.position -= right * dt * keyboardSensitivity;
      }
      if (keys[SDL_SCANCODE_D]) {
	rendererConfig.camera.position += right * dt * keyboardSensitivity;
      }
      if (keys[SDL_SCANCODE_SPACE]) {
	rendererConfig.camera.position += WORLD_UP * dt * keyboardSensitivity;
      }
      if (keys[SDL_SCANCODE_LCTRL]) {
	rendererConfig.camera.position -= WORLD_UP * dt * keyboardSensitivity;
      }
    }

    renderer_render();
  }
  
  renderer_scene_destroy(sh);
  renderer_quit();
  return 0;
}
