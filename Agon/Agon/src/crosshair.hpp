#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"

enum class CrosshairType {
    CROSS,      // Simple cross (+)
    DOT,        // Single dot
    CIRCLE,     // Empty circle
    CROSSHAIR   // Cross with center dot
};

class Crosshair {
private:
    CrosshairType type;
    cgp::vec3 color;
    float size;
    float thickness;
    bool enabled;
    
    // For OpenGL rendering
    cgp::mesh_drawable crosshair_mesh;
    cgp::mesh crosshair_geometry;
    
    // Create different crosshair geometries
    void create_cross_geometry();
    void create_dot_geometry();
    void create_circle_geometry();
    void create_crosshair_geometry();
    void update_geometry();

public:
    Crosshair();
    ~Crosshair();
    
    // Initialization
    void initialize();
    
    // Configuration
    void set_type(CrosshairType new_type);
    void set_color(const cgp::vec3& new_color);
    void set_size(float new_size);
    void set_thickness(float new_thickness);
    void set_enabled(bool enabled);
    
    // Getters
    CrosshairType get_type() const { return type; }
    cgp::vec3 get_color() const { return color; }
    float get_size() const { return size; }
    float get_thickness() const { return thickness; }
    bool is_enabled() const { return enabled; }
    
    // Rendering
    void draw_opengl(const environment_structure& environment, int window_width, int window_height);
    void draw_imgui(int window_width, int window_height);
    
    // GUI for crosshair settings
    void display_gui();
};
