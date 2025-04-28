#include "player.hpp"

Player::Player()
    :movement_speed(0.f), height(0.f), position(0, 0, 0),
    velocity(0, 0, 0), acceleration(15.0f), deceleration(10.0f), max_velocity(6.0f),
    current_pitch(0.0f), max_pitch_up(85.0f), max_pitch_down(-85.0f), isGrounded(true) {
}

void Player::initialise(cgp::input_devices& inputs, cgp::window_structure& window) {
    movement_speed = 6.0f;
    height = 1.7f;
    position = cgp::vec3(0, 0, height);

    // Initialize smooth movement variables
    velocity = cgp::vec3(0, 0, 0);
    acceleration = 15.0f;    // Accelerate to full speed in ~0.4 seconds
    deceleration = 10.0f;    // Decelerate to stop in ~0.6 seconds
    max_velocity = movement_speed;

    // Initialize jumping attributes
    gravity = 11.f;
    jumpForce = 4.5f;
    isGrounded = true;

    // Initialize camera rotation limits
    current_pitch = 0.0f;
    max_pitch_up = 85.0f;     // Can look up to 85 degrees
    max_pitch_down = -85.0f;  // Can look down to 85 degrees

    camera.initialize(inputs, window);
    camera.set_rotation_axis_z(); // Sets z axis as the height for camera rotation and (x,y) for ground
    camera.look_at(position, position + cgp::vec3(1, 0, 0)); // Sets camera at position and looks at 1 unit along x axis

    // Enable cursor trapping for no-click rotation
    camera.is_cursor_trapped = true;
}

void Player::update(float dt, const cgp::inputs_keyboard_parameters& keyboard, cgp::mat4& camera_view_matrix) {
    static cgp::vec3 forward;
    static cgp::vec3 right;
    static cgp::vec3 desired_direction(0, 0, 0);
    static cgp::vec3 velocity_change;

    forward = camera.camera_model.front();
    right = camera.camera_model.right();

    forward.z = 0;
    right.z = 0;


    // Normalize directions
    if (cgp::norm(forward) > 0.01f) forward = cgp::normalize(forward);
    if (cgp::norm(right) > 0.01f) right = cgp::normalize(right);

    // Calculate desired movement direction based on keyboard input
    desired_direction = { 0,0,0 };
    if (keyboard.is_pressed(GLFW_KEY_W)) {
        desired_direction += forward;
    }
    if (keyboard.is_pressed(GLFW_KEY_S)) {
        desired_direction -= forward;
    }
    if (keyboard.is_pressed(GLFW_KEY_D)) {
        desired_direction += right;
    }
    if (keyboard.is_pressed(GLFW_KEY_A)) {
        desired_direction -= right;
    }

    // Normalize the desired direction if it's not zero
    if (cgp::norm(desired_direction) > 0.01f) {
        desired_direction = cgp::normalize(desired_direction);
    }

    // Calculate target velocity
    float target_speed = keyboard.shift ? max_velocity * 1.8f : max_velocity;
    cgp::vec3 target_velocity = desired_direction * target_speed;

    // Smoothly interpolate between current velocity and target velocity
    if (cgp::norm(desired_direction) > 0.01f) {
        // Acceleration
        velocity_change = target_velocity - velocity;
        float change_rate = acceleration * dt;
        float max_change = cgp::norm(velocity_change);

        if (max_change > 0.01f) {
            velocity_change = velocity_change * std::min(change_rate / max_change, 1.0f);
            velocity += velocity_change;
        }
    }
    else {
        // Deceleration (no input)
        float current_speed = cgp::norm(velocity);
        if (current_speed > 0.01f) {
            float decel_amount = deceleration * dt;
            if (decel_amount >= current_speed) {
                velocity = { 0,0,0 };  // Stop completely
            }
            else {
                velocity *= (1.0f - decel_amount / current_speed);
            }
        }
    }




    // Jumping and Gravity Logic

    // Check if the player is grounded
    if (position.z <= height) {
        position.z = height;  // Reset to ground level
        isGrounded = true;    // Player is now grounded
        verticalVelocity = 0.0f; // Reset vertical velocity
    }

    if (isGrounded) {
        verticalVelocity = 0.0f; // Reset vertical velocity when grounded
        if (keyboard.is_pressed(GLFW_KEY_SPACE)) {
            verticalVelocity = jumpForce; // Apply jump force
            isGrounded = false;          // Player is now mid-air
        }
    }
    else {
        verticalVelocity -= gravity * dt; // Apply gravity
    }

    // Update vertical position
    position.z += verticalVelocity * dt;



    // Apply velocity to position
    position += velocity * dt;



    // Update camera position
    camera.camera_model.position_camera = position;
    camera_view_matrix = camera.camera_model.matrix_view();
}

void Player::handle_mouse_move(cgp::vec2 const& mouse_position_current, cgp::vec2 const& mouse_position_previous, cgp::mat4& camera_view_matrix) {
    // Calculate mouse movement delta
    static cgp::vec2 dp;
    dp = mouse_position_current - mouse_position_previous;

    // Only process if movement is meaningful
    if (std::abs(dp.x) > 0.001f || std::abs(dp.y) > 0.001f) {
        // Calculate pitch change in degrees (approximate)
        float pitch_change_degrees = dp.y * 180 / cgp::Pi; // Convert from radians to degrees (57.3 = 180/?)

        // Check if the new pitch would exceed limits
        float new_pitch = current_pitch + pitch_change_degrees;

        // Clamp pitch within limits
        if (new_pitch > max_pitch_up) {
            pitch_change_degrees = max_pitch_up - current_pitch;
            current_pitch = max_pitch_up;
        }
        else if (new_pitch < max_pitch_down) {
            pitch_change_degrees = max_pitch_down - current_pitch;
            current_pitch = max_pitch_down;
        }
        else {
            current_pitch = new_pitch;
        }

        // Apply rotation with constrained pitch
        if (pitch_change_degrees != 0.0f) {
            // Convert back to radians for the camera system
            float pitch_change_radians = pitch_change_degrees * cgp::Pi / 180;
            camera.camera_model.manipulator_rotate_roll_pitch_yaw(0, pitch_change_radians, 0);
        }

        // Apply yaw (horizontal rotation) without restrictions
        camera.camera_model.manipulator_rotate_roll_pitch_yaw(0, 0, -dp.x);

        // Update the camera view matrix
        camera_view_matrix = camera.camera_model.matrix_view();
    }
}

//void Player::load_model(const std::string& model_path)
//{
//    cgp::mesh model_mesh = cgp::mesh_load_file_obj("assets/player.obj");
//}
//
//void Player::draw(const cgp::environment_generic_structure& environment)
//{
//}
