#include "types.hpp"
#include <glm/geometric.hpp>
#include <glm/glm.hpp>
#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    GPUResources gpu;
    GeometryStorage geometry;
    Scene scene;
    Renderer renderer{gpu, geometry, scene};
    Input input{gpu};
    UI ui;

    renderer.init();
    ui.init(gpu);

    u64 meshCount = renderer.load_obj("models/bmw.obj");
    std::cout << "Length " << meshCount << '\n';

    scene.ensure_instance_count(meshCount);
    for (u64 i = 0; i < meshCount; i++) {
        scene.set_instance(
            i, geometry.mesh_at(i),
            {/*position*/ 0.0f, 0.0f, 0.0f, /*scale*/ 1.0f, 1.0f, 1.0f, /*rotation*/ 0.0f, 0.0f, 0.0f}
        );
    }

    renderer.rebuild();
    renderer.config.keyboardSensitivity = 200.0;
    renderer.config.mouseSensitivity = 50.0;

    bool running = true;
    while (running) {
        const f32 dt = input.time_dt();

        input.poll();

        if (input.should_close() || input.key_down(KEY_ESCAPE)) {
            running = false;
        }

        const f32 wheel_x = input.mouse_wheel_x();
        const f32 wheel_y = input.mouse_wheel_y();
        const f32 dx = input.mouse_dx();
        const f32 dy = input.mouse_dy();

        const bool w_down = input.key_down(KEY_W);
        const bool s_down = input.key_down(KEY_S);
        const bool middle_click = input.mouse_down(MOUSE_BUTTON_MIDDLE);

        const f32 move_speed = renderer.config.keyboardSensitivity * dt;
        const f32 look_speed = renderer.config.mouseSensitivity * dt;

        renderer.camera_rotate(-wheel_x * look_speed, wheel_y * look_speed);
        renderer.camera_move(
            (w_down ? move_speed : 0.0f) + (s_down ? -move_speed : 0.0f),
            middle_click ? -dx * look_speed : 0.0f,
            middle_click ? dy * look_speed : 0.0f
        );

        renderer.render_begin();
        renderer.render_scene();
        ui.begin();
        ui.begin_window("Inspector");
        u64 mCount = geometry.mesh_count();
        for (u64 m = 0; m < mCount; m++) {
            MeshHandle mh = geometry.mesh_at(m);
            std::string meshLabel = "Mesh " + std::to_string(m);
            if (ui.begin_tree(meshLabel.c_str())) {
                ui.text(("handle " + std::to_string(mh)).c_str());
                ui.end_tree();
            }
        }
        ui.end_window();
        ui.end(renderer.command_buffer(), renderer.render_pass());
        renderer.render_end();
    }

    ui.quit();
    renderer.quit();
    return 0;
}