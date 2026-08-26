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
#include <string>

extern RendererConfig rendererConfig;
extern glm::vec3 WORLD_UP;

int main(int argc, char* argv[]) {
  // (void)argc;
  // (void)argv;

  renderer_init();

  SceneHandle sh = scene_create();

  scene_load_obj(sh, "models/bmw.obj");
  std::cout << "Length " << scene_mesh_count(sh) << '\n';

  u64 meshCount = scene_mesh_count(sh);
  for (u64 i = 0; i < meshCount; i++) {
    MeshHandle mh = scene_mesh_at(sh, i);
    scene_add_instance(sh, mh,
                       {/*position*/ 0.0f, 0.0f, 0.0f,
                        /*scale*/    1.0f, 1.0f, 1.0f,
                        /*rotation*/ 0.0f, 0.0f, 0.0f});
  }
  
  renderer_rebuild_scene(sh);
  rendererConfig.keyboardSensitivity = 200.0;
  rendererConfig.mouseSensitivity = 50.0;
  bool running = true;
  while (running) {
    const f32 dt = time_dt();

    input_poll();

    if (input_should_close() || input_key_down(SDL_SCANCODE_ESCAPE)) {
      running = false;
    }

    const f32 wheel_x = input_mouse_wheel_x();
    const f32 wheel_y = input_mouse_wheel_y();
    const f32 dx = input_mouse_dx();
    const f32 dy = input_mouse_dy();

    const bool w_down = input_key_down(SDL_SCANCODE_W);
    const bool s_down = input_key_down(SDL_SCANCODE_S);
    const bool middle_click = input_mouse_down(MOUSE_BUTTON_MIDDLE);

    const f32 move_speed = rendererConfig.keyboardSensitivity * dt;
    const f32 look_speed = rendererConfig.mouseSensitivity * dt;

    renderer_camera_rotate(-wheel_x * look_speed, wheel_y * look_speed);
    renderer_camera_move(
			 (w_down ? move_speed : 0.0f) + (s_down ? -move_speed : 0.0f),
			 middle_click ? -dx * look_speed : 0.0f,
			 middle_click ? dy * look_speed : 0.0f
			 );
    renderer_render_begin();
    renderer_render_scene(sh);
    ui_begin();
    ui_begin_window("Inspector");
    u64 scCount = scene_count();
    for (u64 s = 0; s < scCount; s++) {
      SceneHandle sceneH = scene_at(s);
      std::string sceneLabel = "Scene #" + std::to_string(sceneH);
      if (ui_treenode(sceneLabel.c_str())) {
        u64 mCount = scene_mesh_count(sceneH);
        for (u64 m = 0; m < mCount; m++) {
          MeshHandle mh = scene_mesh_at(sceneH, m);
          const char* meshLabel = scene_mesh_alias(sceneH, mh);
          if (ui_treenode(meshLabel)) {
            u64 instCount = scene_mesh_instance_count(sceneH, mh);
            for (u64 inst = 0; inst < instCount; inst++) {
              InstanceHandle ih = scene_mesh_instance_at(sceneH, mh, inst);
              bool selected = false;
              if (ui_selectable(scene_instance_alias(sceneH, ih), selected)) {
                selected = true;
              }
            }
            ui_treepop();
          }
        }
        ui_treepop();
      }
    }
    ui_end_window();
    ui_end();
    renderer_render_end();
  }
  
  scene_destroy(sh);
  renderer_quit();
  return 0;
}
