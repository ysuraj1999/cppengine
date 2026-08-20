#include "cppengine.h"
#include "types.hpp"
#include <SDL3/SDL_gpu.h>

int main(int argc, char* argv[]) {
  // (void)argc;
  // (void)argv;

  renderer_init();

  Scene scene = {};

  renderer_scene_load_obj(&scene, "models/bmw.obj");
  std::cout << "Length " << scene.meshes.size() << '\n';

  u32 GRID_SIZE = 1;
  for (int i = 0; i < scene.meshes.size(); i++) {
    renderer_scene_add_instance(&scene, i, Transform{
            .position = {0.0f, 0.0f, 0.0f},
            .scale = {1.0f, 1.0f, 1.0f},
            .rotation = {0.0f, 0.0f, 0.0f}});
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
  
  renderer_sync_scene(&scene);

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
  
  renderer_quit();
  return 0;
}
