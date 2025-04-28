#pragma once
#ifndef PLAYER_HPP

#include "cgp/cgp.hpp"

class Player {
private:
    // Private member variables
    float movement_speed;
    float height;
    cgp::vec3 position;

    // New smooth movement variables
    cgp::vec3 velocity;         // Current velocity vector
    float acceleration;         // How quickly speed builds up
    float deceleration;         // How quickly speed decreases
    float max_velocity;         // Maximum velocity magnitude

    // Camera rotation constraints
    float current_pitch;        // Track current pitch angle in degrees
    float max_pitch_up;         // Maximum upward angle (positive)
    float max_pitch_down;       // Maximum downward angle (negative)

    // Jump mechanic
    float verticalVelocity;
    float gravity;
    float jumpForce;
    bool isGrounded;

public:
    // Public interface and camera controller
    Player();
    cgp::camera_controller_first_person_euler camera;

    // Public methods
    void initialise(cgp::input_devices& inputs, cgp::window_structure& window);
    void update(float dt, const cgp::inputs_keyboard_parameters& keyboard, cgp::mat4& camera_view_matrix);
    void handle_mouse_move(cgp::vec2 const& mouse_position_current, cgp::vec2 const& mouse_position_previous, cgp::mat4& camera_view_matrix);
};

#endif // !1
