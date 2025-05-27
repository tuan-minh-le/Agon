#pragma once

#include "cgp/cgp.hpp"
#include <cmath> // For std::atan2, std::isfinite
#include <iostream> // For std::cerr

struct RemotePlayer {
    cgp::vec3 position;
    cgp::rotation_transform orientation;
    cgp::mesh_drawable model_drawable;
    cgp::rotation_transform initial_model_rotation; // May become unused if base mesh is pre-rotated

    RemotePlayer()
        : position({0,0,0}), 
          orientation(), 
          // Initialize initial_model_rotation to make model upright and face +X if Z is up
          // This assumes the raw .obj model is oriented along Y-axis as 'up' and looking towards +Z or -Z
          initial_model_rotation(cgp::rotation_transform::from_axis_angle({1,0,0}, cgp::Pi/2.0f) * cgp::rotation_transform::from_axis_angle({0,0,1}, cgp::Pi))
    {}

    void initialize_data_on_gpu(cgp::mesh const& mesh_shape) {
        model_drawable.initialize_data_on_gpu(mesh_shape);
    }

    void update_state(const cgp::vec3& position_arg, const cgp::mat4& aim_direction_matrix_arg) {
        // Simple and safe approach - just ignore the aim direction for now to prevent crashes
        position = position_arg;
        
        // Validate position values
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
            std::cerr << "Invalid position values in RemotePlayer::update_state, using (0,0,0)" << std::endl;
            position = cgp::vec3(0, 0, 0);
        }
        
        // For now, just keep the remote player facing forward (identity rotation)
        // This prevents all the matrix inversion issues that were causing crashes
        orientation = cgp::rotation_transform();
        
        // Update the model transform safely
        try {
            model_drawable.model.translation = position;
            model_drawable.model.rotation = orientation;
        } catch (const std::exception& e) {
            std::cerr << "Exception setting model transform in RemotePlayer::update_state: " << e.what() << std::endl;
        } 
        // If the base mesh isn't pre-rotated fully (including the 180-degree turn for man.obj), 
        // we might need: model_drawable.model.rotation = orientation * initial_model_rotation_for_facing_direction;
        // However, consistency with local player setup (which now pre-rotates the mesh fully) is better.
    }

    void draw(cgp::environment_generic_structure const& environment) const {
        cgp::draw(model_drawable, environment);
    }
};
