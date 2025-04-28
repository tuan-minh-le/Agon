#include "scene.hpp"


using namespace cgp;


void scene_structure::initialize()
{
	camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
	camera_control.set_rotation_axis_z();
	camera_control.look_at({ 3.0f, 2.0f, 2.0f }, {0,0,0}, {0,0,1});
	global_frame.initialize_data_on_gpu(mesh_primitive_frame());
    player.initialise(inputs, window);
    fps_mode = true;
}



void scene_structure::display_frame()
{
    // Update the camera view based on the current mode
    if (fps_mode) {
        // Use player's camera view
        environment.camera_view = player.camera.camera_model.matrix_view();
    }
    else {
        // Use orbit camera view
        environment.camera_view = camera_control.camera_model.matrix_view();
    }

    // Set the light to the current position of the camera
    environment.light = camera_control.camera_model.position();

    // Draw the frame if enabled
    if (gui.display_frame)
        draw(global_frame, environment);

    // Draw other scene elements here
}

void scene_structure::display_gui()
{
	ImGui::Checkbox("Frame", &gui.display_frame);
	ImGui::Checkbox("Wireframe", &gui.display_wireframe);
}

void scene_structure::mouse_move_event()
{
	if (!inputs.keyboard.shift)
		camera_control.action_mouse_move(environment.camera_view);
}
void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click(environment.camera_view);
}

void scene_structure::keyboard_event()
{
    if (fps_mode) {
        // Update player based on keyboard input
        std::cout << "FPS mode active. Processing keyboard input." << std::endl;
        player.update(inputs.time_interval, inputs.keyboard, environment.camera_view);

        // Exit FPS mode with Escape key
        if (inputs.keyboard.is_pressed(GLFW_KEY_ESCAPE)) {
            toggle_fps_mode();
        }
    }
    else {
        // Standard orbit camera control
        camera_control.action_keyboard(environment.camera_view);

        // Enter FPS mode with F key
        if (inputs.keyboard.is_pressed('`')) {
            std::cout << "` pressed " << std::endl;
            toggle_fps_mode();
        }
    }
}
void scene_structure::idle_frame()
{
    if (!fps_mode) {
        // Only use standard idle update for orbit camera
        camera_control.idle_frame(environment.camera_view);
    }
}

void scene_structure::toggle_fps_mode()
{
    fps_mode = !fps_mode;

    if (fps_mode) {
        // Hide cursor for FPS mode
        glfwSetInputMode(window.glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else {
        // Show cursor for normal mode
        glfwSetInputMode(window.glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}