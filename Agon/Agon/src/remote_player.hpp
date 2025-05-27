#pragma once

#include "cgp/cgp.hpp"
#include <cmath> // For std::atan2, std::isfinite
#include <iostream> // For std::cerr

struct RemotePlayer {
    cgp::vec3 position;
    cgp::rotation_transform orientation;
    cgp::mesh_drawable model_drawable;
    cgp::rotation_transform initial_model_rotation; // May become unused if base mesh is pre-rotated
    bool initialized_on_gpu;
    cgp::mesh stored_mesh; // Store mesh data until we can safely initialize on GPU

    RemotePlayer()
        : position({0,0,0}), 
          orientation(), 
          // Initialize initial_model_rotation to make model upright and face +X if Z is up
          // This assumes the raw .obj model is oriented along Y-axis as 'up' and looking towards +Z or -Z
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

    void update_state(const cgp::vec3& position_arg, const cgp::mat4& /* aim_direction_matrix_arg */) {
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
