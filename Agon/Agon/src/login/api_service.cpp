#include "api_service.hpp"
#include <iostream>
#include <thread>

using json = nlohmann::json;

APIService& APIService::getInstance() {
    static APIService instance;
    return instance;
}

APIService::APIService() {}

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

