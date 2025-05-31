#pragma once

#include "cgp/cgp.hpp"
#include <cmath> 
#include <iostream>

struct RemotePlayer {
    cgp::vec3 position;
    cgp::rotation_transform orientation;
    cgp::mesh_drawable model_drawable;
    cgp::rotation_transform initial_model_rotation;
    bool initialized_on_gpu;
    cgp::mesh stored_mesh;

    RemotePlayer()
        : position({0,0,0}), 
          orientation(), 
          initial_model_rotation(cgp::rotation_transform::from_axis_angle({1,0,0}, cgp::Pi/2.0f) * cgp::rotation_transform::from_axis_angle({0,0,1}, cgp::Pi)),
          initialized_on_gpu(false)
    {}

    void store_mesh_data(cgp::mesh const& mesh_shape) {
        // Just store the mesh data, don't initialize on GPU yet
        stored_mesh = mesh_shape;
        std::cout << "Stored mesh data for remote player (vertices: " << mesh_shape.position.size() << ")" << std::endl;
    }

    void initialize_data_on_gpu_if_needed() {
        if (initialized_on_gpu) return;
        
        try {
            std::cout << "RemotePlayer::initialize_data_on_gpu_if_needed called" << std::endl;
            
            // Validate mesh data before GPU upload
            if (stored_mesh.position.size() == 0) {
                std::cerr << "ERROR: Stored mesh has no vertices, cannot initialize on GPU" << std::endl;
                return;
            }
            
            std::cout << "Mesh validation passed, calling model_drawable.initialize_data_on_gpu" << std::endl;
            model_drawable.initialize_data_on_gpu(stored_mesh);
            model_drawable.model.set_scaling(0.6f);
            initialized_on_gpu = true;
            std::cout << "model_drawable.initialize_data_on_gpu completed successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Exception in RemotePlayer::initialize_data_on_gpu_if_needed: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in RemotePlayer::initialize_data_on_gpu_if_needed" << std::endl;
        }
    }

    void initialize_data_on_gpu(cgp::mesh const& mesh_shape) {
        try {
            std::cout << "RemotePlayer::initialize_data_on_gpu called" << std::endl;
            
            // Validate mesh data before GPU upload
            if (mesh_shape.position.size() == 0) {
                std::cerr << "ERROR: Mesh has no vertices, cannot initialize on GPU" << std::endl;
                return;
            }
            
            std::cout << "Mesh validation passed, calling model_drawable.initialize_data_on_gpu" << std::endl;
            model_drawable.initialize_data_on_gpu(mesh_shape);
            initialized_on_gpu = true;
            std::cout << "model_drawable.initialize_data_on_gpu completed successfully" << std::endl;
            
        } catch (const std::exception& e) {
            std::cerr << "Exception in RemotePlayer::initialize_data_on_gpu: " << e.what() << std::endl;
        } catch (...) {
            std::cerr << "Unknown exception in RemotePlayer::initialize_data_on_gpu" << std::endl;
        }
    }

    void update_state(const cgp::vec3& position_arg, const cgp::mat4& aim_direction_matrix_arg) {
        position = position_arg;
        
        // Validate position values
        if (!std::isfinite(position.x) || !std::isfinite(position.y) || !std::isfinite(position.z)) {
            std::cerr << "Invalid position values in RemotePlayer::update_state, using (0,0,0)" << std::endl;
            position = cgp::vec3(0, 0, 0);
        }
        
        try {
            // Get the inverse of the view matrix to get the camera frame matrix
            cgp::mat4 camera_frame_matrix = inverse(aim_direction_matrix_arg);
            
            // Extract the rotation part from the camera frame matrix (upper-left 3x3)
            cgp::mat3 camera_rotation_matrix = cgp::mat3(
                cgp::vec3(camera_frame_matrix(0,0), camera_frame_matrix(0,1), camera_frame_matrix(0,2)),
                cgp::vec3(camera_frame_matrix(1,0), camera_frame_matrix(1,1), camera_frame_matrix(1,2)),
                cgp::vec3(camera_frame_matrix(2,0), camera_frame_matrix(2,1), camera_frame_matrix(2,2))
            );
            
            // Create rotation transform from the camera's rotation matrix
            cgp::rotation_transform camera_rotation = cgp::rotation_transform::from_matrix(camera_rotation_matrix);
            
            // For the player model, we only want rotation around the Z-axis (yaw)
            // Extract the Z-axis rotation component from the full camera rotation
            cgp::vec3 front_direction = camera_rotation * cgp::vec3(0, 0, -1); // Camera's front direction
            
            // Project onto XY plane and compute yaw rotation around Z-axis
            cgp::vec3 front_xy = cgp::vec3(front_direction.x, front_direction.y, 0);
            if (norm(front_xy) > 1e-6f) {
                front_xy = normalize(front_xy);
                
                // Create rotation that aligns default front direction (0,1,0) with projected front
                cgp::vec3 default_front = cgp::vec3(0, 1, 0);
                
                // Compute angle between default front and projected front
                float cos_angle = dot(default_front, front_xy);
                float sin_angle = front_xy.x; // Cross product z-component: default_front × front_xy
                
                // Create Z-axis rotation
                float yaw_angle = std::atan2(sin_angle, cos_angle);
                
                // Invert the yaw angle to fix rotation direction (when player turns right, object should turn right)
                yaw_angle = -yaw_angle;
                
                // Apply Z-axis rotation to match the local player's rotation system
                orientation = cgp::rotation_transform::from_axis_angle({0,0,1}, yaw_angle);
            } else {
                // If front direction is purely vertical, use identity rotation
                orientation = cgp::rotation_transform();
            }
            
        } catch (const std::exception& e) {
            std::cerr << "Exception extracting rotation from camera matrix in RemotePlayer::update_state: " << e.what() << std::endl;
            orientation = cgp::rotation_transform();
        }
        
        // Update the model transform safely
        try {
            model_drawable.model.translation = position;
            model_drawable.model.translation.z -= 0.8f; // Lower the remote player to match local player ground level
            model_drawable.model.rotation = orientation;
        } catch (const std::exception& e) {
            std::cerr << "Exception setting model transform in RemotePlayer::update_state: " << e.what() << std::endl;
        } 
        // The model rotation now uses direct matrix rotation extraction for unlimited rotation
    }

    void draw(cgp::environment_generic_structure const& environment) const {
        // Try to initialize GPU data if not already done (const_cast is safe here for deferred initialization)
        const_cast<RemotePlayer*>(this)->initialize_data_on_gpu_if_needed();
        
        // Only draw if properly initialized on GPU
        if (initialized_on_gpu) {
            try {
                cgp::draw(model_drawable, environment);
            } catch (const std::exception& e) {
                std::cerr << "Exception in RemotePlayer::draw: " << e.what() << std::endl;
            } catch (...) {
                std::cerr << "Unknown exception in RemotePlayer::draw" << std::endl;
            }
        }
    }
};
