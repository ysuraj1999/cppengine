#include "cppengine.cpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    Renderer renderer;
    renderer.init();

    u64 meshCount = renderer.load_obj("models/bmw.obj");
    std::cout << "Length " << meshCount << '\n';

    renderer.scene.resize(meshCount);
    for (u64 i = 0; i < meshCount; i++) {
        renderer.scene[i].meshHandle = renderer.geometry[i];
        renderer.scene[i].transform = {/*position*/ 0.0f, 0.0f, 0.0f, /*scale*/ 1.0f, 1.0f, 1.0f, /*rotation*/ 0.0f, 0.0f, 0.0f};
    }

    renderer.rebuild();
    renderer.config.keyboardSensitivity = 200.0;
    renderer.config.mouseSensitivity = 50.0;

    bool running = true;
    while (running) {
        const f32 dt = renderer.input.time_dt();

        renderer.input.poll();

        if (renderer.input.should_close() || renderer.input.keyboard.down(KEY_ESCAPE)) {
            running = false;
        }

        const f32 wheel_x = renderer.input.mouse.wheel_x;
        const f32 wheel_y = renderer.input.mouse.wheel_y;
        const f32 dx = renderer.input.mouse.dx;
        const f32 dy = renderer.input.mouse.dy;

        const bool w_down = renderer.input.keyboard.down(KEY_W);
        const bool s_down = renderer.input.keyboard.down(KEY_S);
        const bool middle_click = renderer.input.mouse.down(MOUSE_BUTTON_MIDDLE);

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
        ui::begin();
        ui::begin_window("Inspector");
        u64 mCount = renderer.geometry.size();
        for (u64 m = 0; m < mCount; m++) {
            MeshHandle mh = renderer.geometry[m];
            std::string meshLabel = "Mesh " + std::to_string(m);
            if (ui::begin_tree(meshLabel.c_str())) {
                ui::text(("handle " + std::to_string(mh)).c_str());
                ui::end_tree();
            }
        }
        ui::end_window();
        ui::end(renderer.command_buffer(), renderer.render_pass());
        renderer.render_end();
    }

    renderer.quit();
    return 0;
}