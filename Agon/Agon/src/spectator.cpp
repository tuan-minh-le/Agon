#include "spectator.hpp"

using namespace cgp;

Spectator::Spectator()
    : movement_speed(8.0f), position(-20.f, -20.f, 1.7f),
      velocity(0, 0, 0), acceleration(15.0f), deceleration(12.0f),
      max_velocity(8.0f), current_pitch(0.0f),
      max_pitch_up(85.0f), max_pitch_down(-85.0f) {}

void Spectator::initialise(input_devices& inputs, window_structure& window) {
    camera.initialize(inputs, window);
    camera.set_rotation_axis_z();
    camera.look_at(position, position + vec3(1, 0, 0));
    camera.is_cursor_trapped = true;
}

void Spectator::update(float dt, const inputs_keyboard_parameters& keyboard,
                       const inputs_mouse_parameters& mouse, mat4& camera_view_matrix) {
    vec3 forward = camera.camera_model.front();
    vec3 right = camera.camera_model.right();
    forward.z = 0;
    right.z = 0;

    if (norm(forward) > 0.01f) forward = normalize(forward);
    if (norm(right) > 0.01f) right = normalize(right);

    vec3 desired_direction = {0, 0, 0};
    if (keyboard.is_pressed(GLFW_KEY_W)) desired_direction += forward;
    if (keyboard.is_pressed(GLFW_KEY_S)) desired_direction -= forward;
    if (keyboard.is_pressed(GLFW_KEY_D)) desired_direction += right;
    if (keyboard.is_pressed(GLFW_KEY_A)) desired_direction -= right;
    if (keyboard.is_pressed(GLFW_KEY_SPACE)) desired_direction += {0, 0, 1};
    if (keyboard.is_pressed(GLFW_KEY_LEFT_CONTROL)) desired_direction -= {0, 0, 1};

    if (norm(desired_direction) > 0.01f)
        desired_direction = normalize(desired_direction);

    float target_speed = keyboard.shift ? max_velocity * 1.8f : max_velocity;
    vec3 target_velocity = desired_direction * target_speed;

    if (norm(desired_direction) > 0.01f) {
        vec3 velocity_change = target_velocity - velocity;
        float change_rate = acceleration * dt;
        float max_change = norm(velocity_change);

        if (max_change > 0.01f) {
            velocity_change = velocity_change * std::min(change_rate / max_change, 1.0f);
            velocity += velocity_change;
        }
    } else {
        float current_speed = norm(velocity);
        if (current_speed > 0.01f) {
            float decel_amount = deceleration * dt;
            if (decel_amount >= current_speed)
                velocity = {0, 0, 0};
            else
                velocity *= (1.0f - decel_amount / current_speed);
        }
    }

    // Appliquer le déplacement
    position += velocity * dt;

    // Bloquer la descente sous une certaine altitude
    if (position.x < -23.45)
        position.x = -23.45;
    if (position.x > 23.45f)
        position.x = 23.45f;

    if (position.y < -25.45f)
        position.y = -25.45f;
    if (position.y > 25.45f)
        position.y = 25.45f;

    if (position.z < 0.1)
        position.z = 0.1;
    if (position.z > 2.7f)
        position.z = 2.7f;


    camera.camera_model.position_camera = position;
    camera_view_matrix = camera.camera_model.matrix_view();
}

void Spectator::handle_mouse_move(vec2 const& current, vec2 const& previous, mat4& camera_view_matrix) {
    vec2 dp = current - previous;
    if (std::abs(dp.x) > 0.001f || std::abs(dp.y) > 0.001f) {
        float pitch_deg = dp.y * 180 / Pi;
        float new_pitch = current_pitch + pitch_deg;

        if (new_pitch > max_pitch_up) {
            pitch_deg = max_pitch_up - current_pitch;
            current_pitch = max_pitch_up;
        } else if (new_pitch < max_pitch_down) {
            pitch_deg = max_pitch_down - current_pitch;
            current_pitch = max_pitch_down;
        } else {
            current_pitch = new_pitch;
        }

        float pitch_rad = pitch_deg * Pi / 180;
        camera.camera_model.manipulator_rotate_roll_pitch_yaw(0, pitch_rad, 0);
        camera.camera_model.manipulator_rotate_roll_pitch_yaw(0, 0, -dp.x);

        camera_view_matrix = camera.camera_model.matrix_view();
    }
}

void Spectator::set_apartment(Apartment* apartment_ptr) {
    apartment = apartment_ptr;
}
