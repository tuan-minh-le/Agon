#pragma once

#include "cgp/cgp.hpp"
#include "environment.hpp"
#include "player.hpp"
#include "apartment.hpp"
#include "login/login_ui.hpp"
#include "login/websocket_service.hpp"
#include "spectator.hpp"
#include <string>
#include <vector>
#include <deque>
#include <mutex>
#include <nlohmann/json.hpp>
#include <map> // Required for std::map
#include "remote_player.hpp" // Include the new RemotePlayer header

using cgp::mesh_drawable;

enum class GameState{
    LOGIN,
    CONNECTING,
    MAIN_GAME
};

// Structure for chat messages
struct ChatMessage {
    std::string username;
    std::string message;
    float timestamp;  // When the message was received (for potential expiration)
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

    std::string roomID;

    // Player for FPS movement
    Player player;
    Spectator spectator;
    bool fps_mode = true; 
    bool spectator_mode = false;
    bool follow_player_mode = false;

    bool death_pause = false;
    float death_timer = 0.0f;

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

    // Base mesh and initial rotation for all player models
    cgp::mesh base_player_mesh;
    cgp::rotation_transform initial_player_model_rotation;

    // OBJ Models
    cgp::mesh mesh_obj;
    cgp::mesh_drawable obj_man;
    
    // WebSocket and chat functionality
    void setupWebSocketHandlers();
    void sendChatMessage(const std::string& message);
    
    // Chat message storage
    std::deque<ChatMessage> chat_messages;
    const size_t MAX_CHAT_MESSAGES = 10;  // Maximum number of messages to display
    std::mutex chat_mutex;  // For thread safety when adding messages

    // Storage for remote players
    std::map<std::string, RemotePlayer> remote_players;
    std::mutex remote_players_mutex; // For thread safety when accessing remote_players
    
    std::vector<std::string> remote_player_usernames;
    int current_followed_index = -1;

    // Shooting system
    void handlePlayerShooting();
    void sendHitInfoToServer(const HitInfo& hit_info);

    // Core functions
    void initialize();    // Standard initialization to be called before the animation loop
    void display_frame(); // The frame display to be called within the animation loop
    void cleanup();       // Clean up resources when the game is closing

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