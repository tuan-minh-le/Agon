#include "player.hpp"



Player::Player() :movement_speed(0.f), height(0.f), position(0,0,0) {}

void Player::initialise(cgp::input_devices& inputs, cgp::window_structure& window){



	movement_speed = 2.0f;
	height = 1.7f;
	position = cgp::vec3(0, 0, height);

	std::cout << "Player initialised with [speed, height, position]: " << movement_speed << ", " <<
		height << ", " << position << std::endl;

	camera.initialize(inputs, window);
	camera.set_rotation_axis_z(); // Sets z axis as the height for camera rotation and (x,y) for ground
	camera.look_at(position, position + cgp::vec3(1, 0, 0)); // Sets camera at position and looks at 1 unit along x axis


}

void Player::update(float dt, const cgp::inputs_keyboard_parameters& keyboard, cgp::mat4& camera_view_matrix){
	camera.action_mouse_move(camera_view_matrix); // Handle mouse input for camera rotation

	cgp::vec3 forward = camera.camera_model.front();
	cgp::vec3 right = camera.camera_model.right();

	forward.z = 0;
	right.z = 0;

	// Normalize directions
	if (cgp::norm(forward) > 0.01f) forward = cgp::normalize(forward);
	if (cgp::norm(right) > 0.01f) right = cgp::normalize(right);

	std::cout << "Forward vec3:" << forward << std::endl;
	std::cout << "Right vec3 :" << right << std::endl;

	// Calculate movement based on keyboard input
	cgp::vec3 movement = { 0,0,0 };
	if (keyboard.is_pressed(GLFW_KEY_W)) {
		std::cout << "W key pressed!" << std::endl;
		movement += forward;
	}
	if (keyboard.is_pressed(GLFW_KEY_S)) {
		std::cout << "S key pressed!" << std::endl;
		movement -= forward;
	}
	if (keyboard.is_pressed(GLFW_KEY_D)) {
		std::cout << "D key pressed!" << std::endl;
		movement += right;
	}
	if (keyboard.is_pressed(GLFW_KEY_A)) {
		std::cout << "A key pressed!" << std::endl;
		movement -= right;
	}

	if (cgp::norm(movement) > 0.01f) {
		movement = cgp::normalize(movement);
		float current_speed = keyboard.shift ? movement_speed * 1.8f : movement_speed; // Twice the speed when sprinting (SHIFT pressed)
		movement *= current_speed * dt; // Scale movement direction with speed and tick time
	}

	position += movement; // Updates position
	position.z = height; // z coords must still be same height

	std::cout << "Position updated: " << position << std::endl;

	camera.look_at(position, position + camera.camera_model.front()); // Updates the looking direction
}
