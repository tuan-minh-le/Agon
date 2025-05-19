#include "player.hpp"

Player::Player()
    :movement_speed(0.f), height(0.f), position(0, 0, 0),
    velocity(0, 0, 0), acceleration(15.0f), deceleration(10.0f), max_velocity(6.0f),
    current_pitch(0.0f), max_pitch_up(85.0f), max_pitch_down(-85.0f), isGrounded(true), collision_radius(0.f) {
}

void Player::initialise(cgp::input_devices& inputs, cgp::window_structure& window) {
    hp = 100;

    movement_speed = 6.0f;
    height = 1.7f;
    position = cgp::vec3(-20.f, -20.f, height);
    collision_radius = 0.5f;

    // Initialize smooth movement variables
    velocity = cgp::vec3(0, 0, 0);
    acceleration = 15.0f;    // Accelerate to full speed in ~0.4 seconds
    deceleration = 12.0f;    // Decelerate to stop in ~0.6 seconds
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

    weapon.initialize();
}

void Player::update(float dt, const cgp::inputs_keyboard_parameters& keyboard, const cgp::inputs_mouse_parameters& mouse, cgp::mat4& camera_view_matrix) {
    static cgp::vec3 forward;
    static cgp::vec3 right;
    static cgp::vec3 desired_direction(0, 0, 0);
    static cgp::vec3 velocity_change;

    forward = camera.camera_model.front();
    right = camera.camera_model.right();

    forward.z = 0;
    right.z = 0;

    if (mouse.click.left) {
        weapon.shoot();
    }

    if (keyboard.is_pressed(GLFW_KEY_R)) {
        weapon.reload();
    }

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

    cgp::vec3 previous_position = position;

    // Update vertical position first (jumping/falling)
    position.z += verticalVelocity * dt;

    // Step 1: Try full movement
    cgp::vec3 intended_position = position;
    intended_position.x += velocity.x * dt;
    intended_position.y += velocity.y * dt;

    // Check if full movement would cause a collision
    bool has_collision = (apartment != nullptr && apartment->check_collision(intended_position, collision_radius));

    if (!has_collision) {
        // No collision, apply full movement
        position = intended_position;
    }
    else {
        // Step 2: Try sliding along walls
        // Try moving only on X axis
        cgp::vec3 x_only_position = position;
        x_only_position.x += velocity.x * dt;
        bool x_collision = (apartment != nullptr && apartment->check_collision(x_only_position, collision_radius));

        // Try moving only on Y axis
        cgp::vec3 y_only_position = position;
        y_only_position.y += velocity.y * dt;
        bool y_collision = (apartment != nullptr && apartment->check_collision(y_only_position, collision_radius));

        // Apply movement on non-colliding axes
        if (!x_collision) {
            position.x = x_only_position.x;
        }

        if (!y_collision) {
            position.y = y_only_position.y;
        }

        // Step 3: If still colliding, implement "push away" behavior
        if (apartment != nullptr && apartment->check_collision(position, collision_radius)) {
            // Find nearest wall and compute push-back direction
            cgp::vec3 push_direction = compute_push_direction(position);

            // Push player away from the wall slightly
            float push_strength = 0.01f; // Small push to avoid sticking
            position += push_direction * push_strength;
        }
    }

    weapon.update(dt);

    // Update camera position
    camera.camera_model.position_camera = position;
    camera_view_matrix = camera.camera_model.matrix_view();
}



cgp::vec3 Player::compute_push_direction(const cgp::vec3& pos) {
    if (apartment == nullptr) return cgp::vec3(0, 0, 0);

    cgp::vec3 closest_point(0, 0, 0);
    float min_distance = 1000.0f;

    // For each wall, find the closest one
    for (size_t i = 0; i < apartment->wall_positions.size(); ++i) {
        const cgp::vec3& wall_pos = apartment->wall_positions[i];
        const cgp::vec3& wall_dim = apartment->wall_dimensions[i];

        // Calculate minimum and maximum points of wall
        cgp::vec3 wall_min = wall_pos - wall_dim * 0.5f;
        cgp::vec3 wall_max = wall_pos + wall_dim * 0.5f;

        // Calculate closest point on wall to position
        cgp::vec3 point;
        point.x = std::max(wall_min.x, std::min(pos.x, wall_max.x));
        point.y = std::max(wall_min.y, std::min(pos.y, wall_max.y));
        point.z = std::max(wall_min.z, std::min(pos.z, wall_max.z));

        float distance = cgp::norm(pos - point);
        if (distance < min_distance) {
            min_distance = distance;
            closest_point = point;
        }
    }

    // Compute push direction away from closest wall point
    cgp::vec3 direction = pos - closest_point;
    if (cgp::norm(direction) < 0.001f) {
        // If too close, use a default direction (up)
        return cgp::vec3(0, 0, 1);
    }
    return cgp::normalize(direction);
}

cgp::vec3 Player::getPosition() const
{
    return position;
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

void Player::set_apartment(Apartment* apartment_ptr)
{
    apartment = apartment_ptr;
}

const Weapon& Player::getWeapon(){
    return weapon;
}

//void Player::load_model(const std::string& model_path)
//{
//    cgp::mesh model_mesh = cgp::mesh_load_file_obj("assets/player.obj");
//}
//
//void Player::draw(const cgp::environment_generic_structure& environment)
//{
//}
