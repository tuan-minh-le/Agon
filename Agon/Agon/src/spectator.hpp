#pragma once
#ifndef SPECTATOR_HPP
#define SPECTATOR_HPP

#include "cgp/cgp.hpp"
#include "apartment.hpp"

class Spectator {
public:
    cgp::camera_controller_first_person_euler camera;
    cgp::vec3 position;

    // Mouvement
    cgp::vec3 velocity;
    float movement_speed;
    float acceleration;
    float deceleration;
    float max_velocity;
    float current_pitch;
    float max_pitch_up;
    float max_pitch_down;

    float collision_radius = 1.2f;
    float min_altitude = 1.0f; // Pour bloquer la descente sous un certain z

    Spectator();

    void initialise(cgp::input_devices& inputs, cgp::window_structure& window);
    void update(float dt, const cgp::inputs_keyboard_parameters& keyboard,
                const cgp::inputs_mouse_parameters& mouse, cgp::mat4& camera_view_matrix);
    void handle_mouse_move(cgp::vec2 const& current, cgp::vec2 const& previous, cgp::mat4& camera_view_matrix);

    void set_apartment(Apartment* apartment_ptr);
    cgp::vec3 getPosition() const;

private:
    Apartment* apartment = nullptr; // inutile ici mais laissé pour compatibilité
};

#endif