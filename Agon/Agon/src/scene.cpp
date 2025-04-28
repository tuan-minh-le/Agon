#include "scene.hpp"


using namespace cgp;


void scene_structure::initialize()
{
    camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
    camera_control.set_rotation_axis_z();
    camera_control.look_at({ 3.0f, 2.0f, 2.0f }, { 0,0,0 }, { 0,0,1 });
    global_frame.initialize_data_on_gpu(mesh_primitive_frame());
    player.initialise(inputs, window);
    fps_mode = true;

    mesh_obj = mesh_load_file_obj("assets/man.obj");


    mesh_obj.centered();
    mesh_obj.scale(0.16f);
	mesh_obj.rotate({ 1, 0, 0 }, 90.0f * cgp::Pi / 180.0f);

    obj_man.initialize_data_on_gpu(mesh_obj);

}



void scene_structure::display_frame()
{
    // Only recreate model data when needed
    static bool model_needs_update = false;

    if (gui.x_rotation != previous_x_rotation) {
        mesh_obj.rotate({ 1, 0, 0 }, gui.x_rotation - previous_x_rotation);
        previous_x_rotation = gui.x_rotation;
        model_needs_update = true;
    }

    if (gui.y_rotation != previous_y_rotation) {
        mesh_obj.rotate({ 0, 1, 0 }, gui.y_rotation - previous_y_rotation);
        previous_y_rotation = gui.y_rotation;
        model_needs_update = true;
    }

    // Only update GPU data when needed
    if (model_needs_update) {
        obj_man.initialize_data_on_gpu(mesh_obj);
        model_needs_update = false;
    }


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
    if (gui.display_wireframe) {
        draw_wireframe(obj_man, environment);
    }
    else {
        draw(obj_man, environment);
    }
}

void scene_structure::display_gui()
{
	ImGui::Checkbox("Frame", &gui.display_frame);
	ImGui::Checkbox("Wireframe", &gui.display_wireframe);
    ImGui::SliderFloat("X rotation", &gui.x_rotation, 0, 90.0f);
    ImGui::SliderFloat("Y rotation", &gui.y_rotation, 0, 90.0f);
}

void scene_structure::mouse_move_event()
{
    if (fps_mode) {
        // Only process mouse movement if not on GUI
        if (!inputs.mouse.on_gui) {
            // Send the mouse positions to the player for handling
            player.handle_mouse_move(inputs.mouse.position.current, inputs.mouse.position.previous, environment.camera_view);
        }
    }
    else if (!inputs.keyboard.shift) {
        // Standard orbit camera control for non-FPS mode
        camera_control.action_mouse_move(environment.camera_view);
    }
}

void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click(environment.camera_view);
}

void scene_structure::keyboard_event()
{
    if (fps_mode) {
        // Exit FPS mode with Escape key
        if (inputs.keyboard.is_pressed(GLFW_KEY_ESCAPE)) {
            toggle_fps_mode();
        }
        // Note: Actual player movement is now handled in idle_frame
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


void scene_structure::idle_frame() {
    if (fps_mode) {
        // Limit update frequency for player movement
        static float update_timer = 0;
        update_timer += inputs.time_interval;

        if (update_timer >= 0.016f) { // ~60 fps
            player.update(update_timer, inputs.keyboard, environment.camera_view);
            update_timer = 0;
        }
    }
    else {
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