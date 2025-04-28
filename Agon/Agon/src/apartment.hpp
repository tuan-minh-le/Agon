#pragma once

#include "cgp/cgp.hpp"
#include <vector>

class Apartment {
public:
    // Constructor
    Apartment();

    // Initialize the apartment meshes and structures
    void initialize();

    // Clear all mesh data
    void clear();

    // Draw the apartment
    void draw(const cgp::environment_generic_structure& environment);

    // Collision detection for player movement
    bool check_collision(const cgp::vec3& position, float radius);

    std::vector<cgp::vec3> wall_positions;
    std::vector<cgp::vec3> wall_dimensions;

private:
    // Apartment structure elements
    cgp::mesh_drawable floor;
    cgp::mesh_drawable ceiling;
    std::vector<cgp::mesh_drawable> walls;

    // Texture identifiers
    cgp::opengl_texture_image_structure floor_texture;
    cgp::opengl_texture_image_structure ceiling_texture;
    cgp::opengl_texture_image_structure wall_texture;

    // Room sizes and layout
    float apartment_width;
    float apartment_length;
    float room_height;

    // Helper functions to create apartment components
    void create_floor();
    void create_ceiling();
    void create_walls();

    // Collision meshes for walls

};
