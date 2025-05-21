#pragma once

#include "cgp/cgp.hpp"
#include <cmath> // For std::atan2

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
        position = position_arg;
        
        // aim_direction_matrix_arg is the camera view matrix of the remote player.
        // It transforms world coordinates to the remote player's camera coordinates.
        // V_cam = R_cam * T_cam
        // The 3x3 part of V_cam is R_cam (world to camera orientation)
        // We need R_model_world = (R_cam)^-1 to get camera orientation in world.
        // R_model_world = Rz(yaw)Rx(pitch)
        
        cgp::mat3 R_world_to_camera = cgp::mat3(aim_direction_matrix_arg); 
        cgp::mat3 R_camera_to_world = cgp::inverse(R_world_to_camera); // Corrected: Use cgp::inverse()

        // Extract yaw from R_camera_to_world matrix
        // Assuming R_camera_to_world = Rz(yaw) * Rx(pitch)
        // R_camera_to_world(0,0) = cos(yaw)
        // R_camera_to_world(1,0) = sin(yaw)
        float remote_yaw = std::atan2(R_camera_to_world(1,0), R_camera_to_world(0,0));

        // This orientation is only the yaw around Z-axis.
        // The base mesh (provided during initialize_data_on_gpu) should already be
        // rotated to be upright and face the correct "base" direction (e.g. +X).
        // The local player model faces +X after its rotations in scene.cpp, and its final rotation is also just yaw.
        orientation = cgp::rotation_transform::from_axis_angle({0,0,1}, remote_yaw); 
        
        model_drawable.model.translation = position; // Corrected: Use .translation instead of .position
        model_drawable.model.rotation = orientation; 
        // If the base mesh isn't pre-rotated fully (including the 180-degree turn for man.obj), 
        // we might need: model_drawable.model.rotation = orientation * initial_model_rotation_for_facing_direction;
        // However, consistency with local player setup (which now pre-rotates the mesh fully) is better.
    }

    void draw(cgp::environment_generic_structure const& environment) const {
        cgp::draw(model_drawable, environment);
    }
};
