#include "cppengine.cpp"

#include <iostream>
#include <string>

int main(int argc, char *argv[]) {
    (void)argc;
    (void)argv;

    Renderer renderer;
    renderer.init();

    u64 meshCount = renderer.load_obj("models/cube.obj");
    std::cout << "Length " << meshCount << '\n';

    f32 spacing = 2.0f;
    u64 GRID = 10;
    for (u64 x = 0; x < GRID; x++) {
      for (u64 y = 0; y < GRID; y++) {
	for (u64 z = 0; z < GRID; z++){
	  for (u64 i = 0; i < meshCount; i++) {
	    Transform t {
	      /*position*/ spacing *x, spacing *y, spacing *z,
	      /*scale*/ 1.0f, 1.0f, 1.0f,
	      /*rotation*/ 0.0f, 0.0f, 0.0f
	    };
	    renderer.scene.add(renderer.geometry[i], t);
	  }
	}
      }
    }

    renderer.rebuild();
    renderer.config.keyboardSensitivity = 20.0;
    renderer.config.mouseSensitivity = 10.0;

    bool running = true;
    renderer.input.set_cursor_captured(true);
    while (running) {
        const f32 dt = renderer.input.time_dt();

        renderer.input.poll();

        if (renderer.input.events.close_requested() || renderer.input.keyboard.down(KEY_ESCAPE)) {
            running = false;
        }

        const glm::vec2 wheel = renderer.input.mouse.wheel();
        const glm::vec2 delta = renderer.input.mouse.delta();

        const bool w_down = renderer.input.keyboard.down(KEY_W);
	const bool a_down = renderer.input.keyboard.down(KEY_A);
        const bool s_down = renderer.input.keyboard.down(KEY_S);
        const bool d_down = renderer.input.keyboard.down(KEY_D);
	const bool ctrl_down = renderer.input.keyboard.down(KEY_CTRL);
        const bool space_down = renderer.input.keyboard.down(KEY_SPACE);

        const f32 move_speed = renderer.config.keyboardSensitivity * dt;
        const f32 look_speed = renderer.config.mouseSensitivity * dt;

        renderer.camera.rotate(delta.x * look_speed, delta.y * look_speed);
        renderer.camera.move(
			     (w_down ? move_speed : 0.0f) + (s_down ? -move_speed : 0.0f),
			     (a_down ? -move_speed : 0.0f) + (d_down ? move_speed : 0.0f),
			     (space_down ? move_speed : 0.0f) + (ctrl_down ? -move_speed : 0.0f)
			     );

        renderer.begin();
        renderer.prepare(); // copy pass

        FPUEnvironment::save();
        ui::begin_frame();
        ui::begin_window("Inspector");
        u64 mCount = renderer.geometry.size();
        for (u64 m = 0; m < mCount; m++) {
	  MeshHandle mh = renderer.geometry[m];
	  std::string meshLabel = renderer.geometry.meshRegions[mh].alias;
	  if (ui::begin_tree(meshLabel.c_str())) {
	    ui::text(("Instance " + std::to_string(mh)).c_str());
	    ui::end_tree();
	  }
        }
        ui::end_window();
        ImDrawData *draw_data = ui::end_frame();

        ui::prepare(draw_data, renderer.command_buffer()); // copy pass

        renderer.render(); // render start

        ui::render(draw_data, renderer.command_buffer(), renderer.render_pass());
        FPUEnvironment::restore();
        renderer.end();
    }

    renderer.quit();
    return 0;
}
