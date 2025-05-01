#include "apartment.hpp"
#include <iostream>
#include <fstream>

using namespace cgp;

Apartment::Apartment()
    : apartment_width(10.0f), apartment_length(12.0f), room_height(2.8f)
{
}

void Apartment::initialize()
{
    // Clear any existing data before initialization
    clear();

    // Load texture resources
    floor_texture.load_and_initialize_texture_2d_on_gpu("assets/floor.jpg", GL_REPEAT, GL_REPEAT);
    ceiling_texture.load_and_initialize_texture_2d_on_gpu("assets/ceiling.jpg", GL_REPEAT, GL_REPEAT);
    wall_texture.load_and_initialize_texture_2d_on_gpu("assets/wall.jpg", GL_REPEAT, GL_REPEAT);

    // Create all geometry with textures
    create_floor();
    create_ceiling();
    create_walls();
}

void Apartment::clear()
{
    // Clear any existing mesh drawables
    floor.clear();
    ceiling.clear();
    walls.clear();
    wall_positions.clear();
    wall_dimensions.clear();

    // Only clear texture resources if they have been initialized
    if (floor_texture.id != 0)
        floor_texture.clear();
    if (ceiling_texture.id != 0)
        ceiling_texture.clear();
    if (wall_texture.id != 0)
        wall_texture.clear();
}

void Apartment::draw(const cgp::environment_generic_structure& environment)
{
    // Draw floor and ceiling
    cgp::draw(floor, environment);
    cgp::draw(ceiling, environment);

    // Draw all walls
    for (const auto& wall : walls) {
        cgp::draw(wall, environment);
    }
}

void Apartment::create_floor()
{
    // Create a floor mesh
    mesh floor_mesh = mesh_primitive_quadrangle({ -apartment_width / 2, -apartment_length / 2, 0 },
        { apartment_width / 2, -apartment_length / 2, 0 },
        { apartment_width / 2, apartment_length / 2, 0 },
        { -apartment_width / 2, apartment_length / 2, 0 });

    // Set floor texture coordinates - using a tiled approach for larger floors
    float tiling_factor = 4.0f; // Adjust this to change texture repetition
    floor_mesh.uv = { {0,0}, {tiling_factor,0}, {tiling_factor,tiling_factor}, {0,tiling_factor} };

    // Initialize the floor drawable with texture
    floor.initialize_data_on_gpu(floor_mesh, mesh_drawable::default_shader, floor_texture);

    // Adjust material properties for the floor
    floor.material.phong.ambient = 0.5f;
    floor.material.phong.diffuse = 0.6f;
    floor.material.phong.specular = 0.2f;
}

void Apartment::create_ceiling()
{
    // Create a ceiling mesh
    mesh ceiling_mesh = mesh_primitive_quadrangle({ -apartment_width / 2, -apartment_length / 2, room_height },
        { apartment_width / 2, -apartment_length / 2, room_height },
        { apartment_width / 2, apartment_length / 2, room_height },
        { -apartment_width / 2, apartment_length / 2, room_height });

    // Set ceiling texture coordinates
    float tiling_factor = 3.0f; // Adjust for ceiling texture density
    ceiling_mesh.uv = { {0,0}, {tiling_factor,0}, {tiling_factor,tiling_factor}, {0,tiling_factor} };

    // Initialize the ceiling drawable with texture
    ceiling.initialize_data_on_gpu(ceiling_mesh, mesh_drawable::default_shader, ceiling_texture);

}

void Apartment::create_walls()
{
    // Define wall positions and dimensions for a 3-room apartment
    // Living room, bedroom, and bathroom
    walls.clear(); // Ensure walls container is empty before adding new walls
    wall_positions.clear(); // Clear collision data
    wall_dimensions.clear();

    // Wall texture tiling factors
    float horizontal_tiling = 4.0f;
    float vertical_tiling = 1.0f;

    // Calculate specific positions for clarity
    float left_edge = -apartment_width / 2;    // -5.0
    float right_edge = apartment_width / 2;    // 5.0
    float back_edge = -apartment_length / 2;   // -6.0
    float front_edge = apartment_length / 2;   // 6.0

    // Interior wall positions
    float bedroom_x = 2.0f;
    float bathroom_y = apartment_length / 4;   // 3.0

    // External walls
    // Wall 1: Back wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle(
            { left_edge, back_edge, 0 },
            { right_edge, back_edge, 0 },
            { right_edge, back_edge, room_height },
            { left_edge, back_edge, room_height });

        wall_mesh.uv = { {0,0}, {horizontal_tiling,0}, {horizontal_tiling,vertical_tiling}, {0,vertical_tiling} };

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh, mesh_drawable::default_shader, wall_texture);
        walls.push_back(wall);

        // Store wall collision data - centered at middle of wall
        wall_positions.push_back({ 0, back_edge, room_height / 2 });
        wall_dimensions.push_back({ apartment_width, 0.2f, room_height }); // Slightly thicker for better collision
    }

    // Wall 2: Front wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle(
            { left_edge, front_edge, 0 },
            { right_edge, front_edge, 0 },
            { right_edge, front_edge, room_height },
            { left_edge, front_edge, room_height });

        wall_mesh.uv = { {0,0}, {horizontal_tiling,0}, {horizontal_tiling,vertical_tiling}, {0,vertical_tiling} };

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh, mesh_drawable::default_shader, wall_texture);
        walls.push_back(wall);

        wall_positions.push_back({ 0, front_edge, room_height / 2 });
        wall_dimensions.push_back({ apartment_width, 0.2f, room_height });
    }

    // Wall 3: Left wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle(
            { left_edge, back_edge, 0 },
            { left_edge, front_edge, 0 },
            { left_edge, front_edge, room_height },
            { left_edge, back_edge, room_height });

        wall_mesh.uv = { {0,0}, {horizontal_tiling,0}, {horizontal_tiling,vertical_tiling}, {0,vertical_tiling} };

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh, mesh_drawable::default_shader, wall_texture);
        walls.push_back(wall);

        wall_positions.push_back({ left_edge, 0, room_height / 2 });
        wall_dimensions.push_back({ 0.2f, apartment_length, room_height });
    }

    // Wall 4: Right wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle(
            { right_edge, back_edge, 0 },
            { right_edge, front_edge, 0 },
            { right_edge, front_edge, room_height },
            { right_edge, back_edge, room_height });

        wall_mesh.uv = { {0,0}, {horizontal_tiling,0}, {horizontal_tiling,vertical_tiling}, {0,vertical_tiling} };

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh, mesh_drawable::default_shader, wall_texture);
        walls.push_back(wall);

        wall_positions.push_back({ right_edge, 0, room_height / 2 });
        wall_dimensions.push_back({ 0.2f, apartment_length, room_height });
    }

    // Interior Walls
    // Bedroom divider wall - full length from back to bathroom wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle(
            { bedroom_x, back_edge, 0 },
            { bedroom_x, bathroom_y, 0 },
            { bedroom_x, bathroom_y, room_height },
            { bedroom_x, back_edge, room_height });

        wall_mesh.uv = { {0,0}, {horizontal_tiling / 2,0}, {horizontal_tiling / 2,vertical_tiling}, {0,vertical_tiling} };

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh, mesh_drawable::default_shader, wall_texture);
        walls.push_back(wall);

        // Position at center of wall section
        float midpoint_y = (back_edge + bathroom_y) / 2;
        float length_y = bathroom_y - back_edge;
        wall_positions.push_back({ bedroom_x, midpoint_y, room_height / 2 });
        wall_dimensions.push_back({ 0.2f, length_y, room_height });
    }

    // Bathroom divider wall - full length from left to bedroom wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle(
            { left_edge, bathroom_y, 0 },
            { bedroom_x, bathroom_y, 0 },
            { bedroom_x, bathroom_y, room_height },
            { left_edge, bathroom_y, room_height });

        wall_mesh.uv = { {0,0}, {horizontal_tiling / 2,0}, {horizontal_tiling / 2,vertical_tiling}, {0,vertical_tiling} };

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh, mesh_drawable::default_shader, wall_texture);
        walls.push_back(wall);

        // Position at center of wall section
        float midpoint_x = (left_edge + bedroom_x) / 2;
        float length_x = bedroom_x - left_edge;
        wall_positions.push_back({ midpoint_x, bathroom_y, room_height / 2 });
        wall_dimensions.push_back({ length_x, 0.2f, room_height });
    }

    // Add an extra collision box at the corner junction of bedroom and bathroom walls
    // This prevents the player from sliding through the corner
    {
        // Corner collision box
        wall_positions.push_back({ bedroom_x, bathroom_y, room_height / 2 });
        wall_dimensions.push_back({ 0.3f, 0.3f, room_height }); // Small box at the junction
    }

    // Debug output of all wall positions and dimensions
    std::cout << "--- Wall Collision Data ---" << std::endl;
    for (int i = 0; i < wall_positions.size(); ++i) {
        std::cout << "Wall " << i << ": Position = ("
            << wall_positions[i].x << ", " << wall_positions[i].y << ", " << wall_positions[i].z
            << "), Dimensions = ("
            << wall_dimensions[i].x << ", " << wall_dimensions[i].y << ", " << wall_dimensions[i].z
            << ")" << std::endl;
    }
}


bool Apartment::check_collision(const cgp::vec3& position, float radius)
{
    // Add a small buffer to the radius to account for floating-point precision errors
    const float buffer = 0.01f;
    float effective_radius = radius + buffer;

    // Check collision with walls
    for (int i = 0; i < wall_positions.size(); ++i) {
        const vec3& wall_pos = wall_positions[i];
        const vec3& wall_dim = wall_dimensions[i];

        // Calculate minimum and maximum points of wall
        vec3 wall_min = wall_pos - wall_dim / 2.0f;
        vec3 wall_max = wall_pos + wall_dim / 2.0f;

        // Calculate closest point on wall to position
        vec3 closest_point;
        closest_point.x = std::max(wall_min.x, std::min(position.x, wall_max.x));
        closest_point.y = std::max(wall_min.y, std::min(position.y, wall_max.y));
        closest_point.z = std::max(wall_min.z, std::min(position.z, wall_max.z));

        // Check if distance to closest point is less than effective radius
        float distance = norm(position - closest_point);
        if (distance < effective_radius) {
            return true;
        }
    }

    return false;
}
