#include "scene.hpp"


using namespace cgp;


void scene_structure::initialize()
{

    current_state = GameState::LOGIN;
    login_ui.initialize();

    camera_control.initialize(inputs, window); // Give access to the inputs and window global state to the camera controler
    camera_control.set_rotation_axis_z();
    camera_control.look_at({ 3.0f, 2.0f, 2.0f }, { 0,0,0 }, { 0,0,1 });
    global_frame.initialize_data_on_gpu(mesh_primitive_frame());

    showChat = false;

    fps_mode = true;

    // Setup WebSocket message handlers
    setupWebSocketHandlers();

    apartment.initialize();

    player.initialise(inputs, window);
    player.set_apartment(&apartment);

    // Initialize player model data
    cgp::mesh player_mesh_data = cgp::mesh_load_file_obj("assets/man.obj");
    player_mesh_data.centered();
    player_mesh_data.scale(0.16f); // Set a base scale for the mesh data
    // Rotate the mesh to be upright. Assuming model is oriented along Y and needs to be pitched up.
    player_mesh_data.rotate({1, 0, 0}, cgp::Pi / 2.0f); 
    player_mesh_data.rotate({0, 0, 1}, cgp::Pi);

    // The second argument (initial_rotation_transform) is stored by Player
    // but currently not used by Player::update's rotation logic.
    // Pass identity, as player_mesh_data is now in its correct base orientation.
    cgp::rotation_transform player_initial_base_rotation; 
    player.set_initial_model_properties(player_mesh_data, player_initial_base_rotation);

    // The mesh_obj and obj_man below are for a separate model, possibly for debugging or other scene elements.
    mesh_obj = mesh_load_file_obj("assets/man.obj");


    mesh_obj.centered();
    mesh_obj.scale(0.16f);
	mesh_obj.rotate({ 1, 0, 0 }, 90.0f * cgp::Pi / 180.0f);

    obj_man.initialize_data_on_gpu(mesh_obj);

}

void scene_structure::setupWebSocketHandlers() {
    // Handler for CHAT messages
    APIService::getInstance().registerWebSocketHandler(
        WebSocketMessageType::CHAT,
        [this](const nlohmann::json& msg_json) {
            try {
                if (msg_json.contains("content") && msg_json.contains("username")) {
                    std::string sender = msg_json["username"].get<std::string>();
                    std::string messageContent = msg_json["content"].get<std::string>();
                    
                    ChatMessage new_message;
                    new_message.username = sender;
                    new_message.message = messageContent;
                    new_message.timestamp = static_cast<float>(glfwGetTime());
                    
                    {
                        std::lock_guard<std::mutex> lock(chat_mutex);
                        chat_messages.push_back(new_message);
                        while (chat_messages.size() > MAX_CHAT_MESSAGES) {
                            chat_messages.pop_front();
                        }
                    }
                    // Log that the scene processed it, APIService already logs receipt
                    std::cout << "Chat message processed by scene: [" << sender << "]: " << messageContent << std::endl;
                } else {
                    std::cerr << "Malformed CHAT message received by scene's CHAT handler: " << msg_json.dump() << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error in scene's CHAT handler: " << e.what() << std::endl;
            }
        });

    // Handler for SERVER messages
    APIService::getInstance().registerWebSocketHandler(
        WebSocketMessageType::SERVER,
        [this](const nlohmann::json& msg_json) {
            try {
                if (msg_json.contains("content")) {
                    std::string content = msg_json["content"].get<std::string>();
                    
                    ChatMessage new_message;
                    new_message.username = "System"; // SERVER messages are from "System"
                    new_message.message = content;
                    new_message.timestamp = static_cast<float>(glfwGetTime());

                    {
                        std::lock_guard<std::mutex> lock(chat_mutex);
                        chat_messages.push_back(new_message);
                        while (chat_messages.size() > MAX_CHAT_MESSAGES) {
                            chat_messages.pop_front();
                        }
                    }
                    std::cout << "Server message processed by scene: " << content << std::endl;
                } else {
                     std::cerr << "Malformed SERVER message received by scene's SERVER handler: " << msg_json.dump() << std::endl;
                }
            } catch (const std::exception& e) {
                std::cerr << "Error in scene's SERVER handler: " << e.what() << std::endl;
            }
        });
    
    // Handler for ERROR messages
    APIService::getInstance().registerWebSocketHandler(
        WebSocketMessageType::ERROR,
        [this](const nlohmann::json& msg_json) {
            try {
                std::string error_content = "Unknown error";
                if (msg_json.contains("message")) {
                    error_content = msg_json["message"].get<std::string>();
                } else if (msg_json.contains("content")) {
                     error_content = msg_json["content"].get<std::string>();
                } else {
                    error_content = msg_json.dump(); // Fallback
                }
                
                ChatMessage new_message;
                new_message.username = "Error"; // Special username for errors
                new_message.message = error_content;
                new_message.timestamp = static_cast<float>(glfwGetTime());

                {
                    std::lock_guard<std::mutex> lock(chat_mutex);
                    chat_messages.push_back(new_message);
                    while (chat_messages.size() > MAX_CHAT_MESSAGES) {
                        chat_messages.pop_front();
                    }
                }
                std::cerr << "Error message processed by scene and added to chat: " << error_content << std::endl;

            } catch (const std::exception& e) {
                std::cerr << "Error in scene's ERROR handler: " << e.what() << std::endl;
            }
        });
    
    // You can also register a handler for WebSocketMessageType::UPDATE if needed
    // APIService::getInstance().registerWebSocketHandler(WebSocketMessageType::UPDATE, [this](const nlohmann::json& msg_json){ ... });

    // Handler for UPDATE messages (player states from server)
    APIService::getInstance().registerWebSocketHandler(
        WebSocketMessageType::UPDATE,
        [this](const nlohmann::json& msg_json) {
            try {
                // Validate JSON structure first
                if (!msg_json.is_object()) {
                    std::cerr << "UPDATE message is not a JSON object" << std::endl;
                    return;
                }

                // Check if the message has the expected structure
                std::string remote_username;
                const nlohmann::json* content_ptr = nullptr;
                
                if (msg_json.contains("username") && msg_json.contains("content")) {
                    // Standard format: {"username": "...", "content": {...}}
                    try {
                        remote_username = msg_json["username"].get<std::string>();
                        content_ptr = &msg_json["content"];
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to extract username from UPDATE: " << e.what() << std::endl;
                        return;
                    }
                } else if (msg_json.contains("content") && msg_json["content"].contains("username")) {
                    // Alternative format: {"content": {"username": "...", ...}}
                    try {
                        remote_username = msg_json["content"]["username"].get<std::string>();
                        content_ptr = &msg_json["content"];
                    } catch (const std::exception& e) {
                        std::cerr << "Failed to extract username from UPDATE content: " << e.what() << std::endl;
                        return;
                    }
                } else {
                    std::cerr << "UPDATE message missing required fields (username and content)" << std::endl;
                    std::cerr << "Message structure: " << msg_json.dump() << std::endl;
                    return;
                }
                
                // Ignore updates for the local player
                if (remote_username.empty() || remote_username == this->username) {
                    return;
                }

                const auto& content = *content_ptr;
                if (!content.is_object()) {
                    std::cerr << "UPDATE content is not a JSON object" << std::endl;
                    return;
                }

                cgp::vec3 remote_position(0.0f, 0.0f, 0.0f);
                cgp::mat4 remote_aim_matrix = cgp::mat4::build_identity();

                // Safely extract position with more robust validation
                if (content.contains("position") && content["position"].is_object()) {
                    const auto& pos_json = content["position"];
                    try {
                        remote_position.x = pos_json.value("x", 0.0f);
                        remote_position.y = pos_json.value("y", 0.0f);
                        remote_position.z = pos_json.value("z", 0.0f);
                    } catch (const std::exception& e) {
                        std::cerr << "Error extracting position values: " << e.what() << std::endl;
                        return;
                    }
                } else {
                    std::cerr << "UPDATE: position missing or not an object" << std::endl;
                    return;
                }

                // Safely extract aimDirection matrix with bounds checking
                if (content.contains("aimDirection") && content["aimDirection"].is_array() && content["aimDirection"].size() == 4) {
                    try {
                        // Log the matrix values for debugging
                        std::cout << "Processing aimDirection matrix for " << remote_username << std::endl;
                        
                        for (int i = 0; i < 4; ++i) {
                            if (!content["aimDirection"][i].is_array() || content["aimDirection"][i].size() != 4) {
                                std::cerr << "UPDATE: aimDirection matrix row " << i << " invalid" << std::endl;
                                return;
                            }
                            for (int j = 0; j < 4; ++j) {
                                if (content["aimDirection"][i][j].is_number()) {
                                    float value = content["aimDirection"][i][j].get<float>();
                                    if (!std::isfinite(value)) {
                                        std::cerr << "UPDATE: aimDirection matrix element [" << i << "][" << j << "] is not finite: " << value << std::endl;
                                        return;
                                    }
                                    remote_aim_matrix(i, j) = value;
                                } else {
                                    std::cerr << "UPDATE: aimDirection matrix element [" << i << "][" << j << "] is not a number" << std::endl;
                                    return;
                                }
                            }
                        }
                        
                        // Check for very small matrix values that might indicate problems
                        float matrix_magnitude = 0.0f;
                        for (int i = 0; i < 4; ++i) {
                            for (int j = 0; j < 4; ++j) {
                                matrix_magnitude += std::abs(remote_aim_matrix(i, j));
                            }
                        }
                        
                        if (matrix_magnitude < 1e-3f) {
                            std::cerr << "UPDATE: aimDirection matrix has very small magnitude: " << matrix_magnitude << ", using identity" << std::endl;
                            remote_aim_matrix = cgp::mat4::build_identity();
                        }
                        
                    } catch (const std::exception& e) {
                        std::cerr << "Error extracting aimDirection matrix: " << e.what() << std::endl;
                        return;
                    }
                } else {
                    std::cerr << "UPDATE: aimDirection missing or not a 4x4 array" << std::endl;
                    return;
                }

                // Lock before accessing remote_players map - use try_lock to avoid deadlocks
                std::unique_lock<std::mutex> lock(remote_players_mutex, std::try_to_lock);
                if (!lock.owns_lock()) {
                    std::cerr << "Could not acquire lock for remote_players - skipping update" << std::endl;
                    return;
                }

                // Check if player exists, if not, create and initialize
                auto it = remote_players.find(remote_username);
                if (it == remote_players.end()) {
                    try {
                        // Create new remote player
                        RemotePlayer new_player;
                        
                        // Initialize with the same pre-rotated mesh as the local player
                        cgp::mesh remote_player_mesh_data = cgp::mesh_load_file_obj("assets/man.obj");
                        remote_player_mesh_data.centered();
                        remote_player_mesh_data.scale(0.16f);
                        remote_player_mesh_data.rotate({1, 0, 0}, cgp::Pi / 2.0f); 
                        remote_player_mesh_data.rotate({0, 0, 1}, cgp::Pi); // Face forward
                        
                        new_player.initialize_data_on_gpu(remote_player_mesh_data);
                        new_player.model_drawable.model.set_scaling(0.6f); // Same scaling as local player
                        
                        // Insert the new player
                        remote_players[remote_username] = std::move(new_player);
                        std::cout << "Created new remote player: " << remote_username << std::endl;
                    } catch (const std::exception& e) {
                        std::cerr << "Error creating remote player " << remote_username << ": " << e.what() << std::endl;
                        return;
                    }
                }
                
                // Update player state safely
                try {
                    remote_players[remote_username].update_state(remote_position, remote_aim_matrix);
                } catch (const std::exception& e) {
                    std::cerr << "Error updating state for remote player " << remote_username << ": " << e.what() << std::endl;
                }

            } catch (const std::exception& e) {
                std::cerr << "Critical error in scene's UPDATE handler: " << e.what() << std::endl;
            }
        });


    std::cout << "Scene WebSocket handlers registered with APIService." << std::endl;
}

void scene_structure::sendChatMessage(const std::string& message) {
    if (!WebSocketService::getInstance().isConnected()) {
        std::cerr << "Cannot send message: WebSocket not connected" << std::endl;
        return;
    }
    
    // Create JSON message according to the API documentation
    nlohmann::json chat_json;
    chat_json["type"] = "CHAT";
    chat_json["content"] = message;
    std::string formatted_message = chat_json.dump();
    
    // Send the formatted message
    WebSocketService::getInstance().send(formatted_message);
    
    // Also add our own message to the chat display immediately
    ChatMessage own_message;
    own_message.username = username;
    own_message.message = message;
    own_message.timestamp = static_cast<float>(glfwGetTime());
    
    {
        std::lock_guard<std::mutex> lock(chat_mutex);
        chat_messages.push_back(own_message);
        while (chat_messages.size() > MAX_CHAT_MESSAGES) {
            chat_messages.pop_front();
        }
    }
}

void scene_structure::display_frame()
{
    if(current_state == GameState::LOGIN){
        login_ui.render(environment);

        if(login_ui.is_login_button_clicked()){
            std::string auth_token = APIService::getInstance().getAuthToken();
            std::cout << "Auth token: " << auth_token << std::endl;
            
            // Connect to WebSocket server using WebSocketService directly
            std::string ws_url = "ws://10.42.229.253:4500/ws";
            if (WebSocketService::getInstance().connect(ws_url, auth_token, login_ui.get_roomid())) {
                std::cout << "Connected to WebSocket server successfully" << std::endl;
                roomID = login_ui.get_roomid();
            } else {
                std::cout << "Failed to connect to WebSocket server" << std::endl;
            }
        }

        if(WebSocketService::getInstance().isConnected() || login_ui.get_email() == "admin"){
            current_state = GameState::MAIN_GAME;
            login_ui.reset_login_clicked();
            username = login_ui.get_username();
            
            // We don't need to send a join notification, the server will broadcast it
            // automatically when we connect to the WebSocket
        }
    }
    
    else{        // Only recreate model data when needed
        static bool model_needs_update = false;
        

        if (gui.x_rotation != previous_x_rotation) {
            mesh_obj.rotate({ 1, 0, 0 }, gui.x_rotation - previous_x_rotation);
            previous_x_rotation = gui.x_rotation;
            model_needs_update = true;
        }

        if (gui.y_rotation != previous_y_rotation) {
            mesh_obj.rotate({ 0, 1, 0 }, gui.y_rotation - previous_y_rotation);
            previous_y_rotation = gui.y_rotation;
            model_needs_update = true;
        }

        // Only update GPU data when needed
        if (model_needs_update) {
            obj_man.initialize_data_on_gpu(mesh_obj);
            model_needs_update = false;
        }


        // Update the camera view based on the current mode
        if (fps_mode) {
            // Use player's camera view
                glfwSetInputMode(window.glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
            environment.camera_view = player.camera.camera_model.matrix_view();
        }
        else {
            // Use orbit camera view
            environment.camera_view = camera_control.camera_model.matrix_view();
        }

        // Set the light to the current position of the camera
        environment.light = camera_control.camera_model.position();

        apartment.draw(environment);

        // Draw the local player's model
        // Model is now always drawn, visible in both FPS and free-camera/orbit mode.
        player.draw_model(environment);

        // Draw remote players
        {
            std::lock_guard<std::mutex> lock(remote_players_mutex);
            for (auto const& remote_pair : remote_players) {
                const std::string& name = remote_pair.first;
                const RemotePlayer& remote_player_data = remote_pair.second;
                if (name != username) { // Don't draw local player again
                    remote_player_data.draw(environment);
                }
            }
        }

        // Draw the frame if enabled
        if (gui.display_frame)
            draw(global_frame, environment);

        // Draw other scene elements here
        if (gui.display_wireframe) {
            draw_wireframe(obj_man, environment);
        }
        else {
            draw(obj_man, environment);
        }}
}

void scene_structure::display_gui()
{
    ImGui::Text("HP: %d", player.getHP());

    ImGui::Text("x: ");
    ImGui::SameLine();
    ImGui::Text("%.2f", player.getPosition().x);

    ImGui::Text("y: ");
    ImGui::SameLine();
    ImGui::Text("%.2f", player.getPosition().y);

    ImGui::Text("z: ");
    ImGui::SameLine();
    ImGui::Text("%.2f", player.getPosition().z);

    ImGui::Text("Username: ");
    ImGui::SameLine();
    ImGui::Text("%s",username.c_str());
}

void scene_structure::display_weapon_info() {
    ImGui::Text("Ammo: ");
    ImGui::SameLine();
    ImGui::Text("%d", player.getWeapon().getBulletCount());
    ImGui::SameLine();
    ImGui::Text(" / ");
    ImGui::SameLine();
    ImGui::Text("%d", player.getWeapon().getTotalAmmo());

    if (player.getWeapon().isReloading()) {
        ImGui::Text("Reloading...");
        ImGui::SameLine();
        ImGui::Text("(%.1f s)", player.getWeapon().getReloadProgress());
    }
}



void scene_structure::display_chat(){
    // Create a fixed size chat window
    const float CHAT_WINDOW_HEIGHT = 90.0f;  // Height in pixels
    const float CHAT_WINDOW_WIDTH = 400.0f;   // Width in pixels
    
    // Begin a child window for the chat messages with a fixed size
    ImGui::BeginChild("ChatMessages", ImVec2(CHAT_WINDOW_WIDTH, CHAT_WINDOW_HEIGHT), true);
    
    // Display only the last 3 chat messages from the queue
    {
        std::lock_guard<std::mutex> lock(chat_mutex);
        if (!chat_messages.empty()) {
            // Calculate starting index to show only the last 3 messages
            size_t start_idx = (chat_messages.size() > 3) ? chat_messages.size() - 3 : 0;
            
            // Iterate through the last 3 messages (or fewer if there aren't 3 yet)
            for (size_t i = start_idx; i < chat_messages.size(); ++i) {
                const auto& msg = chat_messages[i];
                // Different colors for different message types
                ImVec4 usernameColor;
                if (msg.username == "System") {
                    usernameColor = ImVec4(0.8f, 0.8f, 0.2f, 1.0f); // Yellow for system
                } else if (msg.username == "Error") {
                    usernameColor = ImVec4(1.0f, 0.3f, 0.3f, 1.0f); // Red for errors
                } else if (msg.username == username) {
                    usernameColor = ImVec4(0.3f, 1.0f, 0.3f, 1.0f); // Green for own messages
                } else {
                    usernameColor = ImVec4(0.4f, 0.8f, 1.0f, 1.0f); // Blue for others
                }
                
                ImGui::TextColored(usernameColor, "%s:", msg.username.c_str());
                ImGui::SameLine();
                ImGui::Text("%s", msg.message.c_str());
            }
        }
    }
    
    // Auto-scroll to bottom - this ensures newest messages are always visible
    ImGui::SetScrollHereY(1.0f);
    ImGui::EndChild();
    
    // Chat input box
    ImGui::Text("Chat: ");
    ImGui::SameLine();
    
    // Set keyboard focus on the input text field
    ImGui::SetKeyboardFocusHere();
    
    // Process input text and handle Enter key for sending
    if (ImGui::InputText("##Chat", chat_buffer, IM_ARRAYSIZE(chat_buffer), 
                         ImGuiInputTextFlags_EnterReturnsTrue)) {
        
        // Make sure the message isn't empty
        if (chat_buffer[0] != '\0') {
            // Send the message
            sendChatMessage(chat_buffer);
            std::cout << "Message sent: " << chat_buffer << std::endl;
        }
        
        // Hide the chat window
        showChat = false;
        
        // Clear the input field after sending
        chat_buffer[0] = '\0';
    }
}

void scene_structure::mouse_move_event()
{
    if (fps_mode) {
        // Only process mouse movement if not on GUI
        if (!inputs.mouse.on_gui) {
            // Send the mouse positions to the player for handling
            player.handle_mouse_move(inputs.mouse.position.current, inputs.mouse.position.previous, environment.camera_view);
        }
    }
    else if (!inputs.keyboard.shift) {
        // Standard orbit camera control for non-FPS mode
        camera_control.action_mouse_move(environment.camera_view);
    }
}

void scene_structure::mouse_click_event()
{
	camera_control.action_mouse_click(environment.camera_view);
}

void scene_structure::keyboard_event()
{
    // Toggle chat with T key regardless of camera mode
    if (inputs.keyboard.last_action.key == GLFW_KEY_T && inputs.keyboard.last_action.action == GLFW_PRESS) {
        std::cout << "T key detected - Toggling chat" << std::endl;
        showChat = !showChat;
        if (showChat) {
            // Clear the chat buffer when opening
            chat_buffer[0] = '\0';
        }
    }
    
    // Different input handling based on camera mode
    if (fps_mode) {
        // Exit FPS mode with Escape key
        if (inputs.keyboard.is_pressed(GLFW_KEY_ESCAPE)) {
            std::cout << "Player camera deactivated" << std::endl;
            toggle_fps_mode();
        }
    }
    else {
        // Standard orbit camera control
        camera_control.action_keyboard(environment.camera_view);

        // Enter FPS mode with ` key (backtick)
        if (inputs.keyboard.is_pressed('`')) {
            std::cout << "Debug Camera deactivated, Player camera activated" << std::endl;
            toggle_fps_mode();
        }
    }
}


void scene_structure::idle_frame() {
    if (fps_mode) {
        // Limit update frequency for player movement
        static float update_timer = 0;
        update_timer += inputs.time_interval;

        if (update_timer >= 0.016f) { // ~60 fps
            player.update(update_timer, inputs.keyboard, inputs.mouse, environment.camera_view);
            update_timer = 0;

            // Send player state update - only if connected and username is set
            if (WebSocketService::getInstance().isConnected() && !username.empty()) {
                try {
                    nlohmann::json update_payload;
                    update_payload["type"] = "UPDATE";

                    nlohmann::json content_data;
                    
                    // Player Position - validate position values
                    cgp::vec3 player_pos = player.getPosition();
                    
                    // Check for NaN or infinite values
                    if (std::isfinite(player_pos.x) && std::isfinite(player_pos.y) && std::isfinite(player_pos.z)) {
                        content_data["position"]["x"] = player_pos.x;
                        content_data["position"]["y"] = player_pos.y;
                        content_data["position"]["z"] = player_pos.z;
                    } else {
                        std::cerr << "Invalid player position detected, skipping update" << std::endl;
                        return;
                    }

                    // Aim Direction (4x4 matrix from camera view)
                    cgp::mat4 aim_matrix = environment.camera_view;
                    nlohmann::json aim_json_matrix = nlohmann::json::array();
                    
                    bool matrix_valid = true;
                    for (int r = 0; r < 4; ++r) {
                        nlohmann::json row_array = nlohmann::json::array();
                        for (int c = 0; c < 4; ++c) {
                            float value = aim_matrix(r, c);
                            if (!std::isfinite(value)) {
                                matrix_valid = false;
                                break;
                            }
                            row_array.push_back(value);
                        }
                        if (!matrix_valid) break;
                        aim_json_matrix.push_back(row_array);
                    }
                    
                    if (!matrix_valid) {
                        std::cerr << "Invalid aim matrix detected, skipping update" << std::endl;
                        return;
                    }
                    
                    content_data["aimDirection"] = aim_json_matrix;

                    // Player states (with safe method calls)
                    try {
                        content_data["isShooting"] = player.isShooting(); 
                        content_data["isMoving"] = player.isMoving();   
                    } catch (const std::exception& e) {
                        std::cerr << "Error getting player states: " << e.what() << std::endl;
                        // Use default values
                        content_data["isShooting"] = false;
                        content_data["isMoving"] = false;
                    }
                    
                    update_payload["content"] = content_data;

                    // Send the message via WebSocket if still connected
                    if (WebSocketService::getInstance().isConnected()) {
                        WebSocketService::getInstance().send(update_payload.dump());
                    }
                } catch (const std::exception& e) {
                    std::cerr << "Error creating or sending player update: " << e.what() << std::endl;
                }
            }
        }
    }
    else {
        camera_control.idle_frame(environment.camera_view);
    }
}

void scene_structure::toggle_fps_mode()
{
    fps_mode = !fps_mode;

    if (fps_mode) {
        // Hide cursor for FPS mode
        glfwSetInputMode(window.glfw_window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
    }
    else {
        // Show cursor for normal mode
        glfwSetInputMode(window.glfw_window, GLFW_CURSOR, GLFW_CURSOR_NORMAL);
    }
}

// Clean up resources when closing the game
void scene_structure::cleanup() {
    // We don't need to send a leave message, the server will detect the
    // disconnection and broadcast the notification automatically
    
    // Disconnect from WebSocket server
    WebSocketService::getInstance().disconnect();
    
    std::cout << "Scene cleanup complete" << std::endl;
}