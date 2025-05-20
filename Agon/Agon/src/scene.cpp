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
            std::string ws_url = "ws://10.42.235.34:4500/ws";
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