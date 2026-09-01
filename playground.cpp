#include "cppengine.h"
#include "types.hpp"
#include <SDL3/SDL.h>
#include <SDL3/SDL_events.h>
#include <cmath>
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <glm/trigonometric.hpp>
#include <iostream>
#include <string>

extern RendererConfig rendererConfig;
extern glm::vec3 WORLD_UP;

int main(int argc, char *argv[]) {
    // (void)argc;
    // (void)argv;

    renderer_init();

    u64 meshCount = renderer_load_obj("models/bmw.obj");
    std::cout << "Length " << meshCount << '\n';

    ensure_instance_count(meshCount);
    for (u64 i = 0; i < meshCount; i++) {
        instance_set(
            i, mesh_at(i),
            {/*position*/ 0.0f,
             0.0f,
             0.0f,
             /*scale*/ 1.0f,
             1.0f,
             1.0f,
             /*rotation*/ 0.0f,
             0.0f,
             0.0f}
        );
    }

    renderer_rebuild();
    rendererConfig.keyboardSensitivity = 200.0;
    rendererConfig.mouseSensitivity = 50.0;
    bool running = true;
    while (running) {
        const f32 dt = time_dt();

        input_poll();

        if (input_should_close() || input_key_down(KEY_ESCAPE)) {
            running = false;
        }

        const f32 wheel_x = input_mouse_wheel_x();
        const f32 wheel_y = input_mouse_wheel_y();
        const f32 dx = input_mouse_dx();
        const f32 dy = input_mouse_dy();

        const bool w_down = input_key_down(KEY_W);
        const bool s_down = input_key_down(KEY_S);
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
        renderer_render_scene();
        ui_begin();
        ui_begin_window("Inspector");
        u64 mCount = mesh_count();
        for (u64 m = 0; m < mCount; m++) {
            MeshHandle mh = mesh_at(m);
            std::string meshLabel = "Mesh " + std::to_string(m);
            if (ui_begin_tree(meshLabel.c_str())) {
                u64 instCount = instance_count();
                for (u64 inst = 0; inst < instCount; inst++) {
                    if (instance_mesh(inst) != mh)
                        continue;
                    bool selected = false;
                    std::string label = "Instance " + std::to_string(inst);
                    if (ui_selectable(label.c_str(), selected)) {
                        selected = true;
                    }
                }
                ui_end_tree();
            }
        }
        ui_end_window();
        ui_end();
        renderer_render_end();
    }

    renderer_quit();
    return 0;
}
