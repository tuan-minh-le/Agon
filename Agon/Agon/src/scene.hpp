#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"
#include "player.hpp"
#include "apartment.hpp"
#include "login/login_ui.hpp"
#include "login/websocket_service.hpp"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <string>

using cgp::mesh_drawable;

enum class GameState{
    LOGIN,
    CONNECTING,
    MAIN_GAME
};



struct gui_parameters {
    bool display_frame = true;
    bool display_wireframe = false;
    float x_rotation = 0;
    float y_rotation = 0;
};



// The structure of the custom scene
struct scene_structure : cgp::scene_inputs_generic {

    // Core elements and shapes
    camera_controller_orbit_euler camera_control;
    camera_projection_perspective camera_projection;
    window_structure window;
    mesh_drawable global_frame;          // The standard global frame
    environment_structure environment;   // Standard environment controller
    input_devices inputs;                // Storage for inputs status
    gui_parameters gui;                  // Standard GUI element storage

    // Player for FPS movement
    Player player;
    bool fps_mode;

    std::string username;

    // Add login state and UI
    GameState current_state = GameState::LOGIN;
    LoginUI login_ui;
    bool showChat;

    // Apartment
    Apartment apartment;

    // Store previous rotation for model
    float previous_x_rotation;
    float previous_y_rotation;

    // OBJ Models
    cgp::mesh mesh_obj;
    cgp::mesh_drawable obj_man;
    

    // Core functions
    void initialize();    // Standard initialization to be called before the animation loop
    void display_frame(); // The frame display to be called within the animation loop

    // UI
    void display_gui();   // The display of the GUI, also called within the animation loop
    void display_weapon_info();
    void display_chat();

    void mouse_move_event();
    void mouse_click_event();
    void keyboard_event();
    void idle_frame();

    // FPS mode toggle
    void toggle_fps_mode();

    char chat_buffer[128];
};