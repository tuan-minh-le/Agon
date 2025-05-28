#pragma once
#ifndef PLAYER_HPP
#define PLAYER_HPP

#include "cgp/cgp.hpp"
#include "apartment.hpp"
#include <map>
#include <string>

// Forward declarations
struct RemotePlayer;
struct HitInfo;

#include "weapon.hpp"

class Player {
private:
    int hp;
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

    // Model for the player
    cgp::mesh_drawable player_visual_model; // Renamed from player_model for clarity if it was generic
    cgp::rotation_transform initial_model_rotation; // To make model stand upright

    // State flags
    bool shooting_flag;
    bool moving_flag;

public:
    // Default constructor
    Player();

    //Camera
    cgp::camera_controller_first_person_euler camera;

    void initialise(cgp::input_devices& inputs, cgp::window_structure& window);
    void set_initial_model_properties(const cgp::mesh& base_mesh_data, const cgp::rotation_transform& initial_rotation_transform); // New method
    void update(float dt, const cgp::inputs_keyboard_parameters& keyboard, const cgp::inputs_mouse_parameters& mouse, cgp::mat4& camera_view_matrix);
    void handle_mouse_move(cgp::vec2 const& mouse_position_current, cgp::vec2 const& mouse_position_previous, cgp::mat4& camera_view_matrix);

    void set_apartment(Apartment* apartment_ptr);
    cgp::vec3 compute_push_direction(const cgp::vec3& pos);
    const Weapon& getWeapon() const;
    Weapon& getWeaponMutable(); // Non-const version for shooting
    
    // Shooting system
    HitInfo performShoot(const std::map<std::string, RemotePlayer>& remote_players);
    
    // Model methods
    //void load_model(const std::string& model_path);
    //void draw(const cgp::environment_generic_structure& environment);
    void draw_model(const cgp::environment_generic_structure& environment); // New method to draw player model

    cgp::vec3 getPosition() const;

    int getHP() const {return hp;};
    
    // Health management
    void updateHealth(int healthChange);
    void setHP(int newHP);

    // New methods for state checking
    bool isShooting() const;
    bool isMoving() const;

};

#endif // !1
