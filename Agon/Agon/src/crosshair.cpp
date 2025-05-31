#include "crosshair.hpp"
#include <iostream>
#include "cgp/09_geometric_transformation/projection/projection.hpp"

using namespace cgp;

Crosshair::Crosshair() 
    : type(CrosshairType::CROSS), color(1.0f, 1.0f, 1.0f), size(20.0f), 
      thickness(2.0f), enabled(true) {
}

Crosshair::~Crosshair() {
}

void Crosshair::initialize() {
    // Create initial geometry
    update_geometry();
    
    // Initialize the drawable with the geometry
    crosshair_mesh.initialize_data_on_gpu(crosshair_geometry);
    
    std::cout << "Crosshair: Successfully initialized" << std::endl;
}

void Crosshair::set_type(CrosshairType new_type) {
    if (type != new_type) {
        type = new_type;
        update_geometry();
        if (crosshair_mesh.vbo_position.id != 0) {
            crosshair_mesh.initialize_data_on_gpu(crosshair_geometry);
        }
    }
}

void Crosshair::set_color(const vec3& new_color) {
    color = new_color;
}

void Crosshair::set_size(float new_size) {
    if (size != new_size) {
        size = new_size;
        update_geometry();
        if (crosshair_mesh.vbo_position.id != 0) {
            crosshair_mesh.initialize_data_on_gpu(crosshair_geometry);
        }
    }
}

void Crosshair::set_thickness(float new_thickness) {
    if (thickness != new_thickness) {
        thickness = new_thickness;
        update_geometry();
        if (crosshair_mesh.vbo_position.id != 0) {
            crosshair_mesh.initialize_data_on_gpu(crosshair_geometry);
        }
    }
}

void Crosshair::set_enabled(bool new_enabled) {
    enabled = new_enabled;
}

void Crosshair::update_geometry() {
    switch (type) {
        case CrosshairType::CROSS:
            create_cross_geometry();
            break;
        case CrosshairType::DOT:
            create_dot_geometry();
            break;
        case CrosshairType::CIRCLE:
            create_circle_geometry();
            break;
        case CrosshairType::CROSSHAIR:
            create_crosshair_geometry();
            break;
    }
}

void Crosshair::create_cross_geometry() {
    crosshair_geometry = cgp::mesh(); // Create new empty mesh
    
    float half_size = size * 0.5f;
    float half_thickness = thickness * 0.5f;
    
    // Horizontal bar (left to right)
    crosshair_geometry.position.push_back({-half_size, -half_thickness, 0.0f});
    crosshair_geometry.position.push_back({half_size, -half_thickness, 0.0f});
    crosshair_geometry.position.push_back({half_size, half_thickness, 0.0f});
    crosshair_geometry.position.push_back({-half_size, half_thickness, 0.0f});
    
    // Vertical bar (bottom to top)
    crosshair_geometry.position.push_back({-half_thickness, -half_size, 0.0f});
    crosshair_geometry.position.push_back({half_thickness, -half_size, 0.0f});
    crosshair_geometry.position.push_back({half_thickness, half_size, 0.0f});
    crosshair_geometry.position.push_back({-half_thickness, half_size, 0.0f});
    
    // Connectivity for horizontal bar
    crosshair_geometry.connectivity.push_back({0, 1, 2});
    crosshair_geometry.connectivity.push_back({0, 2, 3});
    
    // Connectivity for vertical bar
    crosshair_geometry.connectivity.push_back({4, 5, 6});
    crosshair_geometry.connectivity.push_back({4, 6, 7});
    
    // Set all vertices to white (will be overridden by shader color)
    for (size_t i = 0; i < crosshair_geometry.position.size(); ++i) {
        crosshair_geometry.color.push_back({1.0f, 1.0f, 1.0f});
    }
    
    // Fill missing fields (normals, UVs)
    crosshair_geometry.fill_empty_field();
}

void Crosshair::create_dot_geometry() {
    crosshair_geometry = cgp::mesh();
    
    float radius = thickness;
    int segments = 8;
    
    // Center point
    crosshair_geometry.position.push_back({0.0f, 0.0f, 0.0f});
    crosshair_geometry.color.push_back({1.0f, 1.0f, 1.0f});
    
    // Circle points
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = radius * cos(angle);
        float y = radius * sin(angle);
        
        crosshair_geometry.position.push_back({x, y, 0.0f});
        crosshair_geometry.color.push_back({1.0f, 1.0f, 1.0f});
    }
    
    // Create triangles from center to circle points
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        crosshair_geometry.connectivity.push_back({0, static_cast<unsigned int>(i + 1), static_cast<unsigned int>(next + 1)});
    }
    
    // Fill missing fields (normals, UVs)
    crosshair_geometry.fill_empty_field();
}

void Crosshair::create_circle_geometry() {
    crosshair_geometry = cgp::mesh();
    
    float outer_radius = size * 0.5f;
    float inner_radius = outer_radius - thickness;
    int segments = 16;
    
    // Outer circle points
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x_outer = outer_radius * cos(angle);
        float y_outer = outer_radius * sin(angle);
        float x_inner = inner_radius * cos(angle);
        float y_inner = inner_radius * sin(angle);
        
        crosshair_geometry.position.push_back({x_outer, y_outer, 0.0f});
        crosshair_geometry.position.push_back({x_inner, y_inner, 0.0f});
        crosshair_geometry.color.push_back({1.0f, 1.0f, 1.0f});
        crosshair_geometry.color.push_back({1.0f, 1.0f, 1.0f});
    }
    
    // Create quad strips for the circle
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        
        unsigned int outer_current = i * 2;
        unsigned int inner_current = i * 2 + 1;
        unsigned int outer_next = next * 2;
        unsigned int inner_next = next * 2 + 1;
        
        // First triangle
        crosshair_geometry.connectivity.push_back({outer_current, outer_next, inner_current});
        // Second triangle
        crosshair_geometry.connectivity.push_back({inner_current, outer_next, inner_next});
    }
    
    // Fill missing fields (normals, UVs)
    crosshair_geometry.fill_empty_field();
}

void Crosshair::create_crosshair_geometry() {
    crosshair_geometry = cgp::mesh();
    
    // Create a cross
    create_cross_geometry();
    
    // Add center dot
    size_t base_vertices = crosshair_geometry.position.size();
    
    float dot_radius = thickness * 0.7f;
    int segments = 6;
    
    // Center point for dot
    crosshair_geometry.position.push_back({0.0f, 0.0f, 0.0f});
    crosshair_geometry.color.push_back({1.0f, 1.0f, 1.0f});
    
    // Circle points for dot
    for (int i = 0; i < segments; ++i) {
        float angle = 2.0f * M_PI * i / segments;
        float x = dot_radius * cos(angle);
        float y = dot_radius * sin(angle);
        
        crosshair_geometry.position.push_back({x, y, 0.0f});
        crosshair_geometry.color.push_back({1.0f, 1.0f, 1.0f});
    }
    
    // Create triangles for center dot
    for (int i = 0; i < segments; ++i) {
        int next = (i + 1) % segments;
        crosshair_geometry.connectivity.push_back({
            static_cast<unsigned int>(base_vertices), 
            static_cast<unsigned int>(base_vertices + i + 1), 
            static_cast<unsigned int>(base_vertices + next + 1)
        });
    }
    
    // Fill missing fields (normals, UVs)
    crosshair_geometry.fill_empty_field();
}

void Crosshair::draw_opengl(const environment_structure& environment, int window_width, int window_height) {
    if (!enabled) return;
    
    // Save current OpenGL state
    GLboolean depth_test_enabled = glIsEnabled(GL_DEPTH_TEST);
    GLboolean blend_enabled = glIsEnabled(GL_BLEND);
    
    // Disable depth testing for overlay
    glDisable(GL_DEPTH_TEST);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    
    // Create orthographic projection for 2D overlay
    mat4 projection = projection_orthographic(-window_width/2.0f, window_width/2.0f, 
                                             -window_height/2.0f, window_height/2.0f, 
                                             -1.0f, 1.0f);
    
    // Identity view matrix (no camera transformation for overlay)
    mat4 view = mat4::build_identity();
    
    
    // Set the crosshair color
    crosshair_mesh.material.color = color;
    crosshair_mesh.material.phong.specular = 0.0f; // Make it non-shiny
    
    // Create a temporary environment for 2D rendering
    environment_structure overlay_env = environment;
    overlay_env.camera_projection = projection;
    overlay_env.camera_view = view;
    
    // Draw the crosshair
    draw(crosshair_mesh, overlay_env);
    
    // Restore OpenGL state
    if (depth_test_enabled) glEnable(GL_DEPTH_TEST);
    if (!blend_enabled) glDisable(GL_BLEND);
}

void Crosshair::draw_imgui(int window_width, int window_height) {
    if (!enabled) return;
    
    ImDrawList* draw_list = ImGui::GetOverlayDrawList();
    ImVec2 center(window_width * 0.5f, window_height * 0.5f);
    ImU32 col = IM_COL32(static_cast<int>(color.x * 255), 
                         static_cast<int>(color.y * 255), 
                         static_cast<int>(color.z * 255), 255);
    
    switch (type) {
        case CrosshairType::CROSS: {
            float half_size = size * 0.5f;
            // Horizontal line
            draw_list->AddLine(ImVec2(center.x - half_size, center.y), 
                              ImVec2(center.x + half_size, center.y), col, thickness);
            // Vertical line
            draw_list->AddLine(ImVec2(center.x, center.y - half_size), 
                              ImVec2(center.x, center.y + half_size), col, thickness);
            break;
        }
        case CrosshairType::DOT: {
            draw_list->AddCircleFilled(center, thickness, col);
            break;
        }
        case CrosshairType::CIRCLE: {
            draw_list->AddCircle(center, size * 0.5f, col, 16, thickness);
            break;
        }
        case CrosshairType::CROSSHAIR: {
            float half_size = size * 0.5f;
            // Cross
            draw_list->AddLine(ImVec2(center.x - half_size, center.y), 
                              ImVec2(center.x + half_size, center.y), col, thickness);
            draw_list->AddLine(ImVec2(center.x, center.y - half_size), 
                              ImVec2(center.x, center.y + half_size), col, thickness);
            // Center dot
            draw_list->AddCircleFilled(center, thickness * 0.7f, col);
            break;
        }
    }
}

void Crosshair::display_gui() {
        ImGui::Indent();
        
        ImGui::Checkbox("Enable Crosshair", &enabled);
        
        // Crosshair type selection
        const char* type_names[] = { "Cross", "Dot", "Circle", "Crosshair" };
        int current_type = static_cast<int>(type);
        if (ImGui::Combo("Type", &current_type, type_names, 4)) {
            set_type(static_cast<CrosshairType>(current_type));
        }
        
        // Color picker
        float color_array[3] = { color.x, color.y, color.z };
        if (ImGui::ColorEdit3("Color", color_array)) {
            set_color(vec3(color_array[0], color_array[1], color_array[2]));
        }
        
        // Size slider
        float temp_size = size;
        if (ImGui::SliderFloat("Size", &temp_size, 5.0f, 50.0f)) {
            set_size(temp_size);
        }
        
        // Thickness slider
        float temp_thickness = thickness;
        if (ImGui::SliderFloat("Thickness", &temp_thickness, 1.0f, 10.0f)) {
            set_thickness(temp_thickness);
        }
        
        ImGui::Unindent();
}
