#include "apartment.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <vector>

using namespace cgp;

Apartment::Apartment()
    : apartment_width(10.0f), apartment_length(12.0f), room_height(2.8f)
{
}

void Apartment::initialize()
{
    // Clear any existing data before initialization
    clear();

    // Load texture resources with REPEAT wrapping for better tiling
    floor_texture.load_and_initialize_texture_2d_on_gpu("assets/floor.jpg", GL_REPEAT, GL_REPEAT);
    ceiling_texture.load_and_initialize_texture_2d_on_gpu("assets/ceiling.jpg", GL_REPEAT, GL_REPEAT);
    
    // Use enhanced texture filtering for wall texture to prevent stretching and aliasing
    wall_texture.load_and_initialize_texture_2d_on_gpu(
        "assets/wall.jpg", 
        GL_REPEAT, GL_REPEAT,
        GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
    
    // Also apply enhanced filtering to door texture
    door_texture.load_and_initialize_texture_2d_on_gpu(
        "assets/wall.jpg", 
        GL_REPEAT, GL_REPEAT, 
        GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);

    // Create all geometry with textures
    create_floor();
    create_ceiling();

    // Load grid and create walls/doors
    auto grid = load_layout_from_csv("assets/layout.csv");
    create_walls_from_grid(grid);
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
    if (door_texture.id != 0)
        door_texture.clear();
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

// Helper: Compute bounds of non-empty cells in the grid
void Apartment::compute_grid_bounds(const std::vector<std::vector<char>>& grid, int& min_i, int& max_i, int& min_j, int& max_j) {
    int rows = grid.size();
    int cols = (rows > 0) ? grid[0].size() : 0;
    min_i = rows; max_i = -1; min_j = cols; max_j = -1;
    for (int i = 0; i < rows; ++i) {
        for (int j = 0; j < cols; ++j) {
            if (grid[i][j] != '.') {
                if (i < min_i) min_i = i;
                if (i > max_i) max_i = i;
                if (j < min_j) min_j = j;
                if (j > max_j) max_j = j;
            }
        }
    }
    if (min_i > max_i || min_j > max_j) {
        min_i = min_j = 0; max_i = max_j = 0; // fallback if empty
    }
}

void Apartment::create_floor() {
    auto grid = load_layout_from_csv("assets/layout.csv");
    int min_i, max_i, min_j, max_j;
    compute_grid_bounds(grid, min_i, max_i, min_j, max_j);
    float cell_size = 1.0f;
    float width = (max_j - min_j + 1) * cell_size;
    float length = (max_i - min_i + 1) * cell_size;
    float x0 = (min_j + max_j + 1) * cell_size / 2.0f - (grid[0].size() * cell_size / 2.0f);
    float y0 = (min_i + max_i + 1) * cell_size / 2.0f - (grid.size() * cell_size / 2.0f);
    mesh floor_mesh = mesh_primitive_quadrangle({ x0 - width / 2, y0 - length / 2, 0 },
        { x0 + width / 2, y0 - length / 2, 0 },
        { x0 + width / 2, y0 + length / 2, 0 },
        { x0 - width / 2, y0 + length / 2, 0 });
    float tiling_factor = width / 2.0f;
    floor_mesh.uv = { {0,0}, {tiling_factor,0}, {tiling_factor,tiling_factor}, {0,tiling_factor} };
    floor_mesh.fill_empty_field(); // Fill normals and other attributes
    floor.initialize_data_on_gpu(floor_mesh, mesh_drawable::default_shader, floor_texture);
    floor.material.phong.ambient = 0.5f;
    floor.material.phong.diffuse = 0.6f;
    floor.material.phong.specular = 0.2f;
}

void Apartment::create_ceiling() {
    auto grid = load_layout_from_csv("assets/layout.csv");
    int min_i, max_i, min_j, max_j;
    compute_grid_bounds(grid, min_i, max_i, min_j, max_j);
    float cell_size = 1.0f;
    float width = (max_j - min_j + 1) * cell_size;
    float length = (max_i - min_i + 1) * cell_size;
    float x0 = (min_j + max_j + 1) * cell_size / 2.0f - (grid[0].size() * cell_size / 2.0f);
    float y0 = (min_i + max_i + 1) * cell_size / 2.0f - (grid.size() * cell_size / 2.0f);
    mesh ceiling_mesh = mesh_primitive_quadrangle({ x0 - width / 2, y0 - length / 2, room_height },
        { x0 + width / 2, y0 - length / 2, room_height },
        { x0 + width / 2, y0 + length / 2, room_height },
        { x0 - width / 2, y0 + length / 2, room_height });
    float tiling_factor = width / 2.5f;
    ceiling_mesh.uv = { {0,0}, {tiling_factor,0}, {tiling_factor,tiling_factor}, {0,tiling_factor} };
    ceiling_mesh.fill_empty_field(); // Fill normals and other attributes
    ceiling.initialize_data_on_gpu(ceiling_mesh, mesh_drawable::default_shader, ceiling_texture);
}

void Apartment::create_walls()
{
    // Define wall positions and dimensions for a 3-room apartment
    // Living room, bedroom, and bathroom
    walls.clear(); // Ensure walls container is empty before adding new walls
    wall_positions.clear(); // Clear collision data
    wall_dimensions.clear();

    // Wall texture tiling factors - adjusted for consistent look with ceiling
    float horizontal_tiling = 2.0f;
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

// Helper: Load grid layout from CSV
std::vector<std::vector<char>> Apartment::load_layout_from_csv(const std::string& filename) {
    std::vector<std::vector<char>> grid;
    std::ifstream file(filename);
    std::string line;
    while (std::getline(file, line)) {
        std::vector<char> row;
        std::stringstream ss(line);
        std::string cell;
        while (std::getline(ss, cell, ',')) {
            if (!cell.empty())
                row.push_back(cell[0]);
        }
        if (!row.empty())
            grid.push_back(row);
    }
    return grid;
}

// Grid-based wall/door generation - simplified approach
void Apartment::create_walls_from_grid(const std::vector<std::vector<char>>& grid) {
    walls.clear();
    wall_positions.clear();
    wall_dimensions.clear();
    
    int rows = grid.size();
    int cols = (rows > 0) ? grid[0].size() : 0;
    
    if (rows == 0 || cols == 0) return;
    
    float cell_size = 1.0f;
    float wall_thickness = 0.2f;
    float x0 = -cols * cell_size / 2.0f;
    float y0 = -rows * cell_size / 2.0f;
    
    // Directly create walls for each 'W' cell in the grid
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            if (grid[i][j] == 'W') {
                // Wall cell found - build 4 walls checking neighbors
                float x = x0 + j * cell_size;
                float y = y0 + i * cell_size;
                
                // Check all 4 directions for non-wall neighbors to create walls
                // North wall (if no wall to the north)
                if (i == 0 || grid[i-1][j] != 'W') {
                    create_wall_segment(
                        x, y, x + cell_size, y, 
                        0, room_height, wall_thickness, true);
                }
                
                // East wall (if no wall to the east)
                if (j == cols-1 || grid[i][j+1] != 'W') {
                    create_wall_segment(
                        x + cell_size, y, x + cell_size, y + cell_size, 
                        0, room_height, wall_thickness, false);
                }
                
                // South wall (if no wall to the south)
                if (i == rows-1 || grid[i+1][j] != 'W') {
                    create_wall_segment(
                        x, y + cell_size, x + cell_size, y + cell_size, 
                        0, room_height, wall_thickness, true);
                }
                
                // West wall (if no wall to the west)
                if (j == 0 || grid[i][j-1] != 'W') {
                    create_wall_segment(
                        x, y, x, y + cell_size, 
                        0, room_height, wall_thickness, false);
                }
            }
            else if (grid[i][j] == 'D') {
                // Door cell - create door frame with opening
                float x = x0 + j * cell_size;
                float y = y0 + i * cell_size;
                
                // Door is just a cell with partial walls (opening in the middle)
                // We'll create walls on all sides except where the door opening is
                
                // For now, always make the door opening on the south side
                // North wall
                create_wall_segment(
                    x, y, x + cell_size, y, 
                    0, room_height, wall_thickness, true);
                
                // East wall
                create_wall_segment(
                    x + cell_size, y, x + cell_size, y + cell_size, 
                    0, room_height, wall_thickness, false);
                
                // West wall
                create_wall_segment(
                    x, y, x, y + cell_size, 
                    0, room_height, wall_thickness, false);
                
                // South wall with door opening (create two segments)
                float door_width = 0.6f;
                float door_position = (cell_size - door_width) / 2;
                
                // Left segment of south wall
                create_wall_segment(
                    x, y + cell_size, x + door_position, y + cell_size, 
                    0, room_height, wall_thickness, true);
                
                // Right segment of south wall
                create_wall_segment(
                    x + door_position + door_width, y + cell_size, x + cell_size, y + cell_size, 
                    0, room_height, wall_thickness, true);
                
                // Top of door frame
                create_wall_segment(
                    x + door_position, y + cell_size, x + door_position + door_width, y + cell_size,
                    room_height * 0.7f, room_height, wall_thickness, true);
            }
        }
    }
    
    // Debug output
    std::cout << "Created " << walls.size() << " wall segments" << std::endl;
    std::cout << "Collision boxes: " << wall_positions.size() << std::endl;
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

// Helper method to create a door
void Apartment::create_door(float x1, float x2, float y, float z0, float z1, float wall_thickness, bool isHorizontal) {
    // Door dimensions
    const float door_height = room_height * 0.8f;
    const float door_width = 0.8f;
    const float cell_size = 1.0f; // Cell size for proper UV scaling
    
    // For horizontal doors (along x-axis)
    if (isHorizontal) {
        // Calculate center point of the door
        float door_center_x = (x1 + x2) / 2.0f;
        
        // Create left and right parts of the wall with a door-sized gap in the middle
        
        // 1. Left part of the wall (if needed)
        if (door_center_x - door_width/2.0f > x1 + 0.1f) {
            float left_x2 = door_center_x - door_width/2.0f;
            float mid_height = z0 + room_height/2.0f;
            
            // Bottom half of left wall segment
            mesh wall_bottom = mesh_primitive_quadrangle(
                {x1, y, z0}, {left_x2, y, z0}, 
                {left_x2, y + wall_thickness, mid_height}, {x1, y + wall_thickness, mid_height});
            float u_scale = (left_x2 - x1) / cell_size;
            float v_scale_half = room_height / (2.0f * cell_size);
            wall_bottom.uv = { {0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half} };
            wall_bottom.fill_empty_field();
            
            // Top half of left wall segment
            mesh wall_top = mesh_primitive_quadrangle(
                {x1, y, mid_height}, {left_x2, y, mid_height}, 
                {left_x2, y + wall_thickness, room_height}, {x1, y + wall_thickness, room_height});
            wall_top.uv = { {0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half} };
            wall_top.fill_empty_field();
            
            mesh_drawable wall_bottom_drawable, wall_top_drawable;
            wall_bottom_drawable.initialize_data_on_gpu(wall_bottom, mesh_drawable::default_shader, wall_texture);
            wall_top_drawable.initialize_data_on_gpu(wall_top, mesh_drawable::default_shader, wall_texture);
            walls.push_back(wall_bottom_drawable);
            walls.push_back(wall_top_drawable);
            
            wall_positions.push_back({(x1 + left_x2)/2, y + wall_thickness/2, room_height/2});
            wall_dimensions.push_back({left_x2 - x1, wall_thickness, room_height});
        }
        
        // 2. Right part of the wall (if needed)
        if (door_center_x + door_width/2.0f < x2 - 0.1f) {
            float right_x1 = door_center_x + door_width/2.0f;
            float mid_height = z0 + room_height/2.0f;
            
            // Bottom half of right wall segment
            mesh wall_bottom = mesh_primitive_quadrangle(
                {right_x1, y, z0}, {x2, y, z0}, 
                {x2, y + wall_thickness, mid_height}, {right_x1, y + wall_thickness, mid_height});
            float u_scale = (x2 - right_x1) / cell_size;
            float v_scale_half = room_height / (2.0f * cell_size);
            wall_bottom.uv = { {0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half} };
            wall_bottom.fill_empty_field();
            
            // Top half of right wall segment
            mesh wall_top = mesh_primitive_quadrangle(
                {right_x1, y, mid_height}, {x2, y, mid_height}, 
                {x2, y + wall_thickness, room_height}, {right_x1, y + wall_thickness, room_height});
            wall_top.uv = { {0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half} };
            wall_top.fill_empty_field();
            
            mesh_drawable wall_bottom_drawable, wall_top_drawable;
            wall_bottom_drawable.initialize_data_on_gpu(wall_bottom, mesh_drawable::default_shader, wall_texture);
            wall_top_drawable.initialize_data_on_gpu(wall_top, mesh_drawable::default_shader, wall_texture);
            walls.push_back(wall_bottom_drawable);
            walls.push_back(wall_top_drawable);
            
            wall_positions.push_back({(right_x1 + x2)/2, y + wall_thickness/2, room_height/2});
            wall_dimensions.push_back({x2 - right_x1, wall_thickness, room_height});
        }
        
        // 3. Top part of door frame
        mesh top_frame = mesh_primitive_quadrangle(
            {door_center_x - door_width/2.0f, y, z0 + door_height}, 
            {door_center_x + door_width/2.0f, y, z0 + door_height},
            {door_center_x + door_width/2.0f, y + wall_thickness, z1}, 
            {door_center_x - door_width/2.0f, y + wall_thickness, z1});
        float door_width_scale = door_width / cell_size;
        float door_height_scale = (z1 - (z0 + door_height)) / cell_size;
        top_frame.uv = { {0,0}, {door_width_scale,0}, {door_width_scale,door_height_scale}, {0,door_height_scale} };
        top_frame.fill_empty_field(); // Fill normals and other missing data
        mesh_drawable top;
        top.initialize_data_on_gpu(top_frame, mesh_drawable::default_shader, wall_texture);
        walls.push_back(top);
        wall_positions.push_back({door_center_x, y + wall_thickness/2, (z0 + door_height + z1)/2});
        wall_dimensions.push_back({door_width, wall_thickness, z1 - (z0 + door_height)});
    }
    // For vertical doors (along y-axis)
    else {
        // Calculate center point of the door
        float door_center_y = (x1 + x2) / 2.0f;
        
        // Create top and bottom parts of the wall with a door-sized gap in the middle
        
        // 1. Bottom part of the wall (if needed)
        if (door_center_y - door_width/2.0f > x1 + 0.1f) {
            float bottom_y2 = door_center_y - door_width/2.0f;
            float mid_height = z0 + room_height/2.0f;

            // Bottom half
            mesh front_bottom = mesh_primitive_quadrangle(
                {y - wall_thickness/2.0f, x1, z0},
                {y - wall_thickness/2.0f, bottom_y2, z0},
                {y - wall_thickness/2.0f, bottom_y2, mid_height},
                {y - wall_thickness/2.0f, x1, mid_height});
            
            // Top half
            mesh front_top = mesh_primitive_quadrangle(
                {y - wall_thickness/2.0f, x1, mid_height},
                {y - wall_thickness/2.0f, bottom_y2, mid_height},
                {y - wall_thickness/2.0f, bottom_y2, room_height},
                {y - wall_thickness/2.0f, x1, room_height});
                
            // Back side - Bottom half
            mesh back_bottom = mesh_primitive_quadrangle(
                {y + wall_thickness/2.0f, bottom_y2, z0},
                {y + wall_thickness/2.0f, x1, z0},
                {y + wall_thickness/2.0f, x1, mid_height},
                {y + wall_thickness/2.0f, bottom_y2, mid_height});
                
            // Back side - Top half
            mesh back_top = mesh_primitive_quadrangle(
                {y + wall_thickness/2.0f, bottom_y2, mid_height},
                {y + wall_thickness/2.0f, x1, mid_height},
                {y + wall_thickness/2.0f, x1, room_height},
                {y + wall_thickness/2.0f, bottom_y2, room_height});
                
            // Texture coordinates
            float u_scale = (bottom_y2 - x1);
            float v_scale_half = room_height/2.0f;
            
            front_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            front_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            back_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            back_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            
            // Fill normals
            front_bottom.fill_empty_field();
            front_top.fill_empty_field();
            back_bottom.fill_empty_field();
            back_top.fill_empty_field();
            
            // Create drawables
            mesh_drawable front_bottom_drawable, front_top_drawable, back_bottom_drawable, back_top_drawable;
            front_bottom_drawable.initialize_data_on_gpu(front_bottom, mesh_drawable::default_shader, wall_texture);
            front_top_drawable.initialize_data_on_gpu(front_top, mesh_drawable::default_shader, wall_texture);
            back_bottom_drawable.initialize_data_on_gpu(back_bottom, mesh_drawable::default_shader, wall_texture);
            back_top_drawable.initialize_data_on_gpu(back_top, mesh_drawable::default_shader, wall_texture);
            
            // Add to walls collection
            walls.push_back(front_bottom_drawable);
            walls.push_back(front_top_drawable);
            walls.push_back(back_bottom_drawable);
            walls.push_back(back_top_drawable);
            
            wall_positions.push_back({y, (x1 + bottom_y2)/2, room_height/2});
            wall_dimensions.push_back({wall_thickness, bottom_y2 - x1, room_height});
        }
        
        // 2. Top part of the wall (if needed)
        if (door_center_y + door_width/2.0f < x2 - 0.1f) {
            float top_y1 = door_center_y + door_width/2.0f;
            float mid_height = z0 + room_height/2.0f;
            
            // Front side (facing -X) - Bottom half
            mesh front_bottom = mesh_primitive_quadrangle(
                {y - wall_thickness/2.0f, top_y1, z0},
                {y - wall_thickness/2.0f, x2, z0},
                {y - wall_thickness/2.0f, x2, mid_height},
                {y - wall_thickness/2.0f, top_y1, mid_height});
                
            // Front side (facing -X) - Top half
            mesh front_top = mesh_primitive_quadrangle(
                {y - wall_thickness/2.0f, top_y1, mid_height},
                {y - wall_thickness/2.0f, x2, mid_height},
                {y - wall_thickness/2.0f, x2, room_height},
                {y - wall_thickness/2.0f, top_y1, room_height});
                
            // Back side (facing +X) - Bottom half
            mesh back_bottom = mesh_primitive_quadrangle(
                {y + wall_thickness/2.0f, x2, z0},
                {y + wall_thickness/2.0f, top_y1, z0},
                {y + wall_thickness/2.0f, top_y1, mid_height},
                {y + wall_thickness/2.0f, x2, mid_height});
                
            // Back side (facing +X) - Top half
            mesh back_top = mesh_primitive_quadrangle(
                {y + wall_thickness/2.0f, x2, mid_height},
                {y + wall_thickness/2.0f, top_y1, mid_height},
                {y + wall_thickness/2.0f, top_y1, room_height},
                {y + wall_thickness/2.0f, x2, room_height});
            
            // Texture coordinates
            float u_scale = (x2 - top_y1);
            float v_scale_half = room_height/2.0f;
            
            front_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            front_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            back_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            back_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
            
            // Fill normals
            front_bottom.fill_empty_field();
            front_top.fill_empty_field();
            back_bottom.fill_empty_field();
            back_top.fill_empty_field();
            
            // Create drawables
            mesh_drawable front_bottom_drawable, front_top_drawable, back_bottom_drawable, back_top_drawable;
            front_bottom_drawable.initialize_data_on_gpu(front_bottom, mesh_drawable::default_shader, wall_texture);
            front_top_drawable.initialize_data_on_gpu(front_top, mesh_drawable::default_shader, wall_texture);
            back_bottom_drawable.initialize_data_on_gpu(back_bottom, mesh_drawable::default_shader, wall_texture);
            back_top_drawable.initialize_data_on_gpu(back_top, mesh_drawable::default_shader, wall_texture);
            
            // Add to walls collection
            walls.push_back(front_bottom_drawable);
            walls.push_back(front_top_drawable);
            walls.push_back(back_bottom_drawable);
            walls.push_back(back_top_drawable);
            
            wall_positions.push_back({y, (top_y1 + x2)/2, room_height/2});
            wall_dimensions.push_back({wall_thickness, x2 - top_y1, room_height});
        }
        
        // 3. Top part of door frame (horizontal beam above door)
        mesh door_top_mesh;
        door_top_mesh.position = {
            // Front face (bottom side)
            {y - wall_thickness/2.0f, door_center_y - door_width/2.0f, z0 + door_height},
            {y + wall_thickness/2.0f, door_center_y - door_width/2.0f, z0 + door_height},
            {y + wall_thickness/2.0f, door_center_y + door_width/2.0f, z0 + door_height},
            {y - wall_thickness/2.0f, door_center_y + door_width/2.0f, z0 + door_height},
            // Back face (top side)
            {y - wall_thickness/2.0f, door_center_y - door_width/2.0f, room_height},
            {y + wall_thickness/2.0f, door_center_y - door_width/2.0f, room_height},
            {y + wall_thickness/2.0f, door_center_y + door_width/2.0f, room_height},
            {y - wall_thickness/2.0f, door_center_y + door_width/2.0f, room_height}
        };
        
        // Define the triangles for the door top
        door_top_mesh.connectivity = {
            // Bottom face
            {0, 1, 2}, {0, 2, 3},
            // Top face
            {4, 7, 6}, {4, 6, 5},
            // Left face
            {0, 3, 7}, {0, 7, 4},
            // Right face
            {1, 5, 6}, {1, 6, 2},
            // Near face
            {0, 4, 5}, {0, 5, 1},
            // Far face
            {3, 2, 6}, {3, 6, 7}
        };
        
        // UV coordinates for door top
        float door_width_scale = door_width / cell_size;
        float door_height_scale = (room_height - (z0 + door_height)) / cell_size;
        door_top_mesh.uv = {
            {0,0}, {wall_thickness,0}, {wall_thickness,door_width_scale}, {0,door_width_scale},
            {0,0}, {wall_thickness,0}, {wall_thickness,door_width_scale}, {0,door_width_scale}
        };
        
        // Fill empty fields like normals
        door_top_mesh.fill_empty_field();
        
        mesh_drawable door_top;
        door_top.initialize_data_on_gpu(door_top_mesh, mesh_drawable::default_shader, wall_texture);
        walls.push_back(door_top);
        wall_positions.push_back({y, door_center_y, (z0 + door_height + room_height)/2});
        wall_dimensions.push_back({wall_thickness, door_width, room_height - (z0 + door_height)});
    }
}

// Helper: Create a wall segment between two points
void Apartment::create_wall_segment(float x1, float y1, float x2, float y2, float z1, float z2, float thickness, bool isHorizontal) {
    // Create a wall between two points, with proper height and thickness
    // isHorizontal determines if this is a wall running along the X axis (true) or Y axis (false)
    
    // Calculate length of the wall for proper UV scaling
    float wall_length = isHorizontal ? (x2 - x1) : (y2 - y1);
    float wall_height = z2 - z1;
    float mid_height = z1 + wall_height/2.0f;
    
    // Divide wall into two parts (top and bottom) to avoid texture stretching
    // We'll create two separate quads for each visible face instead of a box mesh
    
    // Add collision data - Same for both approaches
    cgp::vec3 center;
    cgp::vec3 dimensions;
    
    if (isHorizontal) {
        center = {(x1 + x2)/2, y1, (z1 + z2)/2};
        dimensions = {x2 - x1, thickness, z2 - z1};
    } else {
        center = {x1, (y1 + y2)/2, (z1 + z2)/2};
        dimensions = {thickness, y2 - y1, z2 - z1};
    }
    wall_positions.push_back(center);
    wall_dimensions.push_back(dimensions);
    
    // Create front face
    if (isHorizontal) {
        // Horizontal wall - Front side (facing -Y)
        // Bottom half
        mesh front_bottom = mesh_primitive_quadrangle(
            {x1, y1 - thickness/2.0f, z1},             // Bottom-left
            {x2, y1 - thickness/2.0f, z1},             // Bottom-right
            {x2, y1 - thickness/2.0f, mid_height},     // Top-right
            {x1, y1 - thickness/2.0f, mid_height});    // Top-left
            
        // Top half
        mesh front_top = mesh_primitive_quadrangle(
            {x1, y1 - thickness/2.0f, mid_height},     // Bottom-left
            {x2, y1 - thickness/2.0f, mid_height},     // Bottom-right
            {x2, y1 - thickness/2.0f, z2},             // Top-right
            {x1, y1 - thickness/2.0f, z2});            // Top-left
            
        // Back side (facing +Y)
        // Bottom half
        mesh back_bottom = mesh_primitive_quadrangle(
            {x2, y1 + thickness/2.0f, z1},             // Bottom-left
            {x1, y1 + thickness/2.0f, z1},             // Bottom-right
            {x1, y1 + thickness/2.0f, mid_height},     // Top-right
            {x2, y1 + thickness/2.0f, mid_height});    // Top-left
            
        // Top half
        mesh back_top = mesh_primitive_quadrangle(
            {x2, y1 + thickness/2.0f, mid_height},     // Bottom-left
            {x1, y1 + thickness/2.0f, mid_height},     // Bottom-right
            {x1, y1 + thickness/2.0f, z2},             // Top-right
            {x2, y1 + thickness/2.0f, z2});            // Top-left
            
        // Texture coordinates - using proper aspect ratio
        float u_scale = wall_length;
        float v_scale_half = wall_height/2.0f;
        
        front_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        front_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        back_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        back_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        
        // Fill normals
        front_bottom.fill_empty_field();
        front_top.fill_empty_field();
        back_bottom.fill_empty_field();
        back_top.fill_empty_field();
        
        // Create drawables
        mesh_drawable front_bottom_drawable, front_top_drawable, back_bottom_drawable, back_top_drawable;
        front_bottom_drawable.initialize_data_on_gpu(front_bottom, mesh_drawable::default_shader, wall_texture);
        front_top_drawable.initialize_data_on_gpu(front_top, mesh_drawable::default_shader, wall_texture);
        back_bottom_drawable.initialize_data_on_gpu(back_bottom, mesh_drawable::default_shader, wall_texture);
        back_top_drawable.initialize_data_on_gpu(back_top, mesh_drawable::default_shader, wall_texture);
        
        // Add to walls collection
        walls.push_back(front_bottom_drawable);
        walls.push_back(front_top_drawable);
        walls.push_back(back_bottom_drawable);
        walls.push_back(back_top_drawable);
    } 
    else {
        // Vertical wall - Front side (facing -X)
        // Bottom half
        mesh front_bottom = mesh_primitive_quadrangle(
            {x1 - thickness/2.0f, y1, z1},             // Bottom-left
            {x1 - thickness/2.0f, y2, z1},             // Bottom-right
            {x1 - thickness/2.0f, y2, mid_height},     // Top-right
            {x1 - thickness/2.0f, y1, mid_height});    // Top-left
            
        // Top half
        mesh front_top = mesh_primitive_quadrangle(
            {x1 - thickness/2.0f, y1, mid_height},     // Bottom-left
            {x1 - thickness/2.0f, y2, mid_height},     // Bottom-right
            {x1 - thickness/2.0f, y2, z2},             // Top-right
            {x1 - thickness/2.0f, y1, z2});            // Top-left
            
        // Back side (facing +X)
        // Bottom half
        mesh back_bottom = mesh_primitive_quadrangle(
            {x1 + thickness/2.0f, y2, z1},             // Bottom-left
            {x1 + thickness/2.0f, y1, z1},             // Bottom-right
            {x1 + thickness/2.0f, y1, mid_height},     // Top-right
            {x1 + thickness/2.0f, y2, mid_height});    // Top-left
            
        // Top half
        mesh back_top = mesh_primitive_quadrangle(
            {x1 + thickness/2.0f, y2, mid_height},     // Bottom-left
            {x1 + thickness/2.0f, y1, mid_height},     // Bottom-right
            {x1 + thickness/2.0f, y1, z2},             // Top-right
            {x1 + thickness/2.0f, y2, z2});            // Top-left
            
        // Texture coordinates - using proper aspect ratio
        float u_scale = wall_length;
        float v_scale_half = wall_height/2.0f;
        
        front_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        front_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        back_bottom.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        back_top.uv = {{0,0}, {u_scale,0}, {u_scale,v_scale_half}, {0,v_scale_half}};
        
        // Fill normals
        front_bottom.fill_empty_field();
        front_top.fill_empty_field();
        back_bottom.fill_empty_field();
        back_top.fill_empty_field();
        
        // Create drawables
        mesh_drawable front_bottom_drawable, front_top_drawable, back_bottom_drawable, back_top_drawable;
        front_bottom_drawable.initialize_data_on_gpu(front_bottom, mesh_drawable::default_shader, wall_texture);
        front_top_drawable.initialize_data_on_gpu(front_top, mesh_drawable::default_shader, wall_texture);
        back_bottom_drawable.initialize_data_on_gpu(back_bottom, mesh_drawable::default_shader, wall_texture);
        back_top_drawable.initialize_data_on_gpu(back_top, mesh_drawable::default_shader, wall_texture);
        
        // Add to walls collection
        walls.push_back(front_bottom_drawable);
        walls.push_back(front_top_drawable);
        walls.push_back(back_bottom_drawable);
        walls.push_back(back_top_drawable);
    }
}
