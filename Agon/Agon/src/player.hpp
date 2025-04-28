#pragma once
#ifndef PLAYER_HPP

#include "cgp/cgp.hpp"

class Player {
private:
    // Private member variables
    float movement_speed;
    float height;
    cgp::vec3 position;

public:
    // Public interface and camera controller
    Player();
    cgp::camera_controller_first_person_euler camera;

    // Public methods
    void initialise(cgp::input_devices& inputs, cgp::window_structure& window);
    void update(float dt, const cgp::inputs_keyboard_parameters& keyboard, cgp::mat4& camera_view_matrix);
};


#endif // !1
