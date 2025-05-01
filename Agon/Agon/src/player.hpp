#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "cgp/cgp.hpp"
#include "apartment.hpp"

#include "weapon.hpp"

class Player {
private:
    // Private member variables
    float movement_speed;
    float height;
    cgp::vec3 position;
    float collision_radius;

    Weapon weapon;

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

    // Model
    cgp::mesh_drawable player_model;

    //Apartment for collision detection
    Apartment* apartment = nullptr;

public:
    // Default constructor
    Player();

    //Camera
    cgp::camera_controller_first_person_euler camera;

    void initialise(cgp::input_devices& inputs, cgp::window_structure& window);
    void update(float dt, const cgp::inputs_keyboard_parameters& keyboard, const cgp::inputs_mouse_parameters& mouse, cgp::mat4& camera_view_matrix);
    void handle_mouse_move(cgp::vec2 const& mouse_position_current, cgp::vec2 const& mouse_position_previous, cgp::mat4& camera_view_matrix);

    void set_apartment(Apartment* apartment_ptr);
    cgp::vec3 compute_push_direction(const cgp::vec3& pos);
    const Weapon& getWeapon() ;
    // Model methods
    //void load_model(const std::string& model_path);
    //void draw(const cgp::environment_generic_structure& environment);

    cgp::vec3 getPosition() const;

};

#endif // !1
