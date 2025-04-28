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

    // Create all geometry without textures
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

    // Set floor texture coordinates
    floor_mesh.uv = { {0,0}, {1,0}, {1,1}, {0,1} };

    // Initialize the floor drawable
    floor.initialize_data_on_gpu(floor_mesh);

    // Simply use a color instead of texture
    floor.material.color = vec3{ 0.6f, 0.6f, 0.6f }; // Grey color default

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
    ceiling_mesh.uv = { {0,0}, {1,0}, {1,1}, {0,1} };

    // Initialize the ceiling drawable
    ceiling.initialize_data_on_gpu(ceiling_mesh);

    // Use a color for ceiling
    ceiling.material.color = vec3{ 0.95f, 0.95f, 0.95f }; // Light color for ceiling

    ceiling.material.phong.ambient = 0.5f;
    ceiling.material.phong.diffuse = 0.8f;
    ceiling.material.phong.specular = 0.1f;
}

void Apartment::create_walls()
{
    // Define wall positions and dimensions for a 3-room apartment
    // Living room, bedroom, and bathroom

    // Common wall color
    vec3 wall_color = { 0.9f, 0.9f, 0.85f }; // Off-white color

    // External walls
    // Wall 1: Back wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle({ -apartment_width / 2, -apartment_length / 2, 0 },
            { apartment_width / 2, -apartment_length / 2, 0 },
            { apartment_width / 2, -apartment_length / 2, room_height },
            { -apartment_width / 2, -apartment_length / 2, room_height });

        // Create a new mesh_drawable for this wall
        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh);
        wall.material.color = wall_color;
        wall.material.phong.ambient = 0.5f;
        wall.material.phong.diffuse = 0.7f;
        wall.material.phong.specular = 0.1f;
        walls.push_back(wall);

        // Store wall collision data
        wall_positions.push_back({ 0, -apartment_length / 2, room_height / 2 });
        wall_dimensions.push_back({ apartment_width, 0.1f, room_height });
    }

    // Wall 2: Front wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle({ -apartment_width / 2, apartment_length / 2, 0 },
            { apartment_width / 2, apartment_length / 2, 0 },
            { apartment_width / 2, apartment_length / 2, room_height },
            { -apartment_width / 2, apartment_length / 2, room_height });

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh);
        wall.material.color = wall_color;
        wall.material.phong.ambient = 0.5f;
        wall.material.phong.diffuse = 0.7f;
        wall.material.phong.specular = 0.1f;
        walls.push_back(wall);

        wall_positions.push_back({ 0, apartment_length / 2, room_height / 2 });
        wall_dimensions.push_back({ apartment_width, 0.1f, room_height });
    }

    // Wall 3: Left wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle({ -apartment_width / 2, -apartment_length / 2, 0 },
            { -apartment_width / 2, apartment_length / 2, 0 },
            { -apartment_width / 2, apartment_length / 2, room_height },
            { -apartment_width / 2, -apartment_length / 2, room_height });

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh);
        wall.material.color = wall_color;
        wall.material.phong.ambient = 0.5f;
        wall.material.phong.diffuse = 0.7f;
        wall.material.phong.specular = 0.1f;
        walls.push_back(wall);

        wall_positions.push_back({ -apartment_width / 2, 0, room_height / 2 });
        wall_dimensions.push_back({ 0.1f, apartment_length, room_height });
    }

    // Wall 4: Right wall
    {
        mesh wall_mesh = mesh_primitive_quadrangle({ apartment_width / 2, -apartment_length / 2, 0 },
            { apartment_width / 2, apartment_length / 2, 0 },
            { apartment_width / 2, apartment_length / 2, room_height },
            { apartment_width / 2, -apartment_length / 2, room_height });

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh);
        wall.material.color = wall_color;
        wall.material.phong.ambient = 0.5f;
        wall.material.phong.diffuse = 0.7f;
        wall.material.phong.specular = 0.1f;
        walls.push_back(wall);

        wall_positions.push_back({ apartment_width / 2, 0, room_height / 2 });
        wall_dimensions.push_back({ 0.1f, apartment_length, room_height });
    }

    // Interior Walls
    // Bedroom divider wall
    {
        float bedroom_x = 2.0f;
        mesh wall_mesh = mesh_primitive_quadrangle({ bedroom_x, -apartment_length / 2, 0 },
            { bedroom_x, apartment_length / 4, 0 },
            { bedroom_x, apartment_length / 4, room_height },
            { bedroom_x, -apartment_length / 2, room_height });

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh);
        wall.material.color = wall_color;
        wall.material.phong.ambient = 0.5f;
        wall.material.phong.diffuse = 0.7f;
        wall.material.phong.specular = 0.1f;
        walls.push_back(wall);

        wall_positions.push_back({ bedroom_x, -apartment_length / 8, room_height / 2 });
        wall_dimensions.push_back({ 0.1f, 3 * apartment_length / 8, room_height });
    }

    // Bathroom divider wall
    {
        float bathroom_y = apartment_length / 4;
        float bedroom_x = 2.0f;
        mesh wall_mesh = mesh_primitive_quadrangle({ -apartment_width / 2, bathroom_y, 0 },
            { bedroom_x, bathroom_y, 0 },
            { bedroom_x, bathroom_y, room_height },
            { -apartment_width / 2, bathroom_y, room_height });

        mesh_drawable wall;
        wall.initialize_data_on_gpu(wall_mesh);
        wall.material.color = wall_color;
        wall.material.phong.ambient = 0.5f;
        wall.material.phong.diffuse = 0.7f;
        wall.material.phong.specular = 0.1f;
        walls.push_back(wall);

        wall_positions.push_back({ -apartment_width / 4, bathroom_y, room_height / 2 });
        wall_dimensions.push_back({ apartment_width / 4, 0.1f, room_height });
    }
}

bool Apartment::check_collision(const cgp::vec3& position, float radius)
{
    // Check collision with walls
    for (size_t i = 0; i < wall_positions.size(); ++i) {
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

        // Check if distance to closest point is less than radius
        float distance = norm(position - closest_point);
        if (distance < radius) {
            return true;
        }
    }

    return false;
}
