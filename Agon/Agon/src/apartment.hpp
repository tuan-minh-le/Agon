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

    // CSV grid loader and wall/door generator for flexible apartment layout
    std::vector<std::vector<char>> load_layout_from_csv(const std::string& filename);
    void create_walls_from_grid(const std::vector<std::vector<char>>& grid);
    
    // Generate doors from grid layout 
    void create_door(float x1, float x2, float y, float z0, float z1, float wall_thickness, bool isHorizontal);
    
    // Helper for simple wall segment creation
    void create_wall_segment(float x1, float y1, float x2, float y2, float z1, float z2, float thickness, bool isHorizontal);
    
    // Texture for doors
    cgp::opengl_texture_image_structure door_texture;

    // Collision meshes for walls

    // Helper to compute bounds of non-empty cells in the grid
    void compute_grid_bounds(const std::vector<std::vector<char>>& grid, int& min_i, int& max_i, int& min_j, int& max_j);
};
