#include "api_service.hpp"
#include <iostream>
#include <thread>

using json = nlohmann::json;

APIService& APIService::getInstance() {
    static APIService instance;
    return instance;
}

APIService::APIService() {
    // Register our message handler with the WebSocketService
    WebSocketService::getInstance().registerMessageHandler(
        [this](const std::string& message) {
            this->handleWebSocketMessage(message);
        }
    );
}

// WebSocket message handling
bool APIService::connectWebSocket(const std::string& roomId) {
    if (!isLoggedIn()) {
        std::cerr << "Cannot connect to WebSocket: not logged in" << std::endl;
        return false;
    }
    
    std::string ws_url = "ws://" + base_url.substr(7) + "/ws"; // Convert http:// to ws://
    return WebSocketService::getInstance().connect(ws_url, auth_token, roomId);
}

void APIService::disconnectWebSocket() {
    WebSocketService::getInstance().disconnect();
}

bool APIService::isWebSocketConnected() const {
    return WebSocketService::getInstance().isConnected();
}

void APIService::sendWebSocketMessage(const std::string& message) {
    if (!isWebSocketConnected()) {
        std::cerr << "Cannot send message: WebSocket not connected" << std::endl;
        return;
    }
    WebSocketService::getInstance().send(message);
}

void APIService::registerWebSocketHandler(WebSocketMessageType type, 
                                        std::function<void(const json&)> handler) {
    std::lock_guard<std::mutex> lock(handlers_mutex_);
    message_handlers_[type] = handler;
}

void APIService::handleWebSocketMessage(const std::string& message) {
    try {
        json data = json::parse(message);
        WebSocketMessageType type = getMessageType(data);

        std::function<void(const nlohmann::json&)> handler_to_call = nullptr;
        {
            std::lock_guard<std::mutex> lock(handlers_mutex_);
            auto it = message_handlers_.find(type);
            if (it != message_handlers_.end() && it->second) {
                handler_to_call = it->second;
            }
        }

        if (handler_to_call) {
            // If a specific handler is registered, call it
            if (type == WebSocketMessageType::CHAT && data.contains("content") && data.contains("username")) {
                // Optional: still log to console for debugging even if handler is called
                std::cout << "[Debug] Chat message from " << data["username"].get<std::string>() << ": " 
                          << data["content"].get<std::string>() << std::endl;
            }
            handler_to_call(data);
        } else {
            // No handler was registered for this message type
            if (type == WebSocketMessageType::CHAT && data.contains("content") && data.contains("username")) {
                // Specific handling for CHAT messages when no UI handler is present
                std::cout << "Chat message (console only) from " << data["username"].get<std::string>() << ": " 
                          << data["content"].get<std::string>() << std::endl;
                std::cout << "Note: No UI handler registered for WebSocketMessageType::CHAT. "
                          << "Register a handler using registerWebSocketHandler to display this in the chat box." << std::endl;
            } else {
                std::cout << "No handler registered for message type: " 
                          << static_cast<int>(type) << " (raw message: " << message << ")" << std::endl;
            }
        }
    } catch (const json::exception& e) {
        std::cerr << "Error parsing WebSocket message: " << e.what() << " (message: " << message << ")" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error processing WebSocket message: " << e.what() << std::endl;
    }
}

WebSocketMessageType APIService::getMessageType(const json& data) {
    // Determine the message type based on the "type" field
    try {
        if (data.contains("type")) {
            std::string type = data["type"];
            
            if (type == "ERROR") return WebSocketMessageType::ERROR;
            if (type == "UPDATE") return WebSocketMessageType::UPDATE;
            if (type == "SERVER") return WebSocketMessageType::SERVER;
            if (type == "CHAT") return WebSocketMessageType::CHAT;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error determining message type: " << e.what() << std::endl;
    }
    
    return WebSocketMessageType::UNKNOWN;
}


bool APIService::checkServerConnection() const{
    httplib::Client client(base_url);
    client.set_connection_timeout(5);
    
    std::cout << "Testing connection to: " << base_url << std::endl;
    auto res = client.Get("/");
    
    if (res) {
        std::cout << "Server responded with status: " << res->status << std::endl;
        return true;
    } else {
        std::cout << "Connection failed: " << httplib::to_string(res.error()) << std::endl;
        return false;
    }
}

bool APIService::getUserInfo(std::function<void(bool success, const std::string& userData)> callback){
    std::thread([=](){
        try{
            for(int attempt = 0; attempt < MAX_RETRIES; attempt++){
                if(attempt > 0){
                    std::cout << "Retry attempt " << attempt << " of " << MAX_RETRIES << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                }
                
                httplib::Client client(base_url);
                client.set_connection_timeout(10);
                client.set_read_timeout(10);

                std::cout << "Getting user info from: " << base_url << "api/auth/me" << std::endl;

                httplib::Headers headers = {
                     {"Authorization", "Bearer " + auth_token},
                     {"Content-Type", "application/json"}
                };

                auto res = client.Get("/api/auth/me", headers);

                if(res){
                    std::cout << "Response received: Status - " << res->status << std::endl;

                    if(res->status >= 200 && res->status < 300){
                        std::cout << "User data received: " << res->body << std::endl;
                        try {
                            json response = json::parse(res->body);
                            username = response["username"];
                            callback(true, res->body);
                        } catch(const std::exception& e) {
                            std::cout << "Error parsing user data: " << e.what() << std::endl;
                            callback(false, "Error parsing response");
                        }
                        return;
                    }
                    else if(res->status >= 500 && attempt < MAX_RETRIES - 1){
                        std::cout << "Server error: " << res->status << "; retrying..." << std::endl;
                        continue;
                    }
                    else{
                        std::string message = "Server error: " + std::to_string(res->status);
                        try{
                            json response = json::parse(res->body);
                            if(response.contains("message")){
                                message = response["message"];
                            }
                            std::cout << "Failed to parse error response" << std::endl;
                        }
                        catch(...){
                            std::cout << "Failed to parse error response" << std::endl;
                        }
                        callback(false, message);
                        return;
                    }
                }
                else{
                    auto err = res.error();
                    std::cout << "Connection error: " << httplib::to_string(err) << std::endl;

                    if(attempt < MAX_RETRIES - 1){
                        if (err == httplib::Error::Connection || 
                            err == httplib::Error::Read || 
                            err == httplib::Error::Write) {
                            continue;
                    }
                }
                callback(false, "Connection failed: " + httplib::to_string(err));
                return;
            }
        }
        callback(false, "Failed to get user info after" + std::to_string(MAX_RETRIES) + " attempts");
        }catch(const std::exception& e){
            std::cout << "Exception getting user info: " << e.what() << std::endl;
            callback(false, std::string("Exception: ") + e.what());
        }
    }).detach();
}

bool APIService::login(const std::string& email, const std::string& password, bool rememberMe,
                       std::function<void(bool success, const std::string& message, const std::string& token)> callback) {
    // Create a new thread to avoid blocking the UI
    std::thread([=]() {
        try {
            for (int attempt = 0; attempt < MAX_RETRIES; attempt++) {
                if (attempt > 0) {
                    std::cout << "Retry attempt " << attempt << " of " << MAX_RETRIES << std::endl;
                    std::this_thread::sleep_for(std::chrono::milliseconds(RETRY_DELAY_MS));
                }
                
                // Create HTTP client with proper timeouts
                httplib::Client client(base_url);
                client.set_connection_timeout(10); // Increased timeout
                client.set_read_timeout(10);       // Increased timeout
                
                std::cout << "Connecting to: " << base_url << "/auth/login" << std::endl;
                
                // Prepare JSON payload
                json payload = {
                    {"email", email},
                    {"passwd", password},
                    {"rememberMe", rememberMe}
                };
                
                std::string payload_str = payload.dump();
                std::cout << "Sending payload: " << payload_str << std::endl;
                
                // Create proper headers
                httplib::Headers headers = {
                    {"Content-Type", "application/json"}
                };
                
                // Make POST request with explicit headers
                auto res = client.Post("/api/auth/login", headers, payload_str, "application/json");

                if (res) {
                    std::cout << "Response received: Status " << res->status << std::endl;
                    
                    if (res->status >= 200 && res->status < 300) {
                        // Parse response
                        json response = json::parse(res->body);
                        std::cout << "Response body: " << res->body << std::endl;
                        
                        std::string token = response["token"];
                        
                        // Store token
                        auth_token = token;
                        
                        // Call callback
                        callback(true, "Login successful", token);
                        return true;
                    } else if (res->status >= 500 && attempt < MAX_RETRIES - 1) {
                        // Server error, will retry
                        std::cout << "Server error: " << res->status << ", will retry..." << std::endl;
                        continue;
                    } else {
                        // Handle error status
                        std::string message = "Server error: " + std::to_string(res->status);
                        try {
                            json response = json::parse(res->body);
                            if (response.contains("message")) {
                                message = response["message"];
                            }
                            std::cout << "Error response: " << res->body << std::endl;
                        } catch (...) {
                            std::cout << "Failed to parse error response" << std::endl;
                        }
                        
                        callback(false, message, "");
                        return true;
                    }
                } else {
                    // No response object - connection failed
                    auto err = res.error();
                    std::cout << "Connection error: " << httplib::to_string(err) << std::endl;
                    
                    // Only retry for certain types of errors
                    if (attempt < MAX_RETRIES - 1) {
                        if (err == httplib::Error::Connection || 
                            err == httplib::Error::Read || 
                            err == httplib::Error::Write) {
                            continue; // Retry for connection issues
                        }
                    }
                    
                    callback(false, "Connection failed: " + httplib::to_string(err), "");
                    return true;
                }
            }
            
            // If we reach here, all retries failed
            callback(false, "Failed after " + std::to_string(MAX_RETRIES) + " attempts", "");
            
        } catch (const std::exception& e) {
            // Catch any exceptions
            std::cout << "Exception occurred: " << e.what() << std::endl;
            callback(false, std::string("Exception: ") + e.what(), "");
        }
    }).detach();
    
    return true;
}

