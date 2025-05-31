#include <httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <iostream>
#include <mutex>
#include "websocket_service.hpp"  // Add WebSocketService header

const int MAX_RETRIES = 3;
const int RETRY_DELAY_MS = 2000;

enum class LoginStatus {
    IDLE,
    PENDING,
    SUCCESS,
    ERROR
};

// WebSocket message types
enum class WebSocketMessageType {
    UNKNOWN = 0,
    ERROR = 1,
    UPDATE = 2,
    SERVER = 3,
    CHAT = 4
};

class APIService {
public:
    static APIService& getInstance();

    std::string username;
    
    bool login(const std::string& email, const std::string& password, bool rememberMe, 
              std::function<void(bool success, const std::string& message, const std::string& token)> callback);
              
    bool refreshToken(std::function<void(bool success, const std::string& newToken)> callback);
    
    bool getUserInfo(std::function<void(bool success, const std::string& userData)> callback);
    
    // WebSocket message handling methods
    bool connectWebSocket(const std::string& roomId);
    void disconnectWebSocket();
    bool isWebSocketConnected() const;
    void sendWebSocketMessage(const std::string& message);
    
    // Register message handlers for specific message types
    void registerWebSocketHandler(WebSocketMessageType type, 
                                 std::function<void(const nlohmann::json&)> handler);
    
    void setAuthToken(const std::string& token) { auth_token = token; }
    bool checkServerConnection() const;
    std::string getAuthToken() const { return auth_token; }
    bool isLoggedIn() const { return !auth_token.empty(); }

private:
    APIService();
    // Make sure there's no trailing slash in base_url
    std::string base_url = "http://10.42.229.253:4500";
    std::string auth_token;

    LoginStatus status = LoginStatus::IDLE;
    std::mutex status_mutex; // For thread safety
    
    // WebSocket message handling
    void handleWebSocketMessage(const std::string& message);
    WebSocketMessageType getMessageType(const nlohmann::json& json);
    
    // Message handlers for different types of messages
    std::map<WebSocketMessageType, std::function<void(const nlohmann::json&)>> message_handlers_;
    std::mutex handlers_mutex_; // For thread safety of the handlers map
};