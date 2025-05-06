#include <httplib/httplib.h>
#include <nlohmann/json.hpp>
#include <string>
#include <functional>
#include <memory>
#include <iostream>

const int MAX_RETRIES = 3;
const int RETRY_DELAY_MS = 2000;

enum class LoginStatus {
    IDLE,
    PENDING,
    SUCCESS,
    ERROR
};


class APIService {
public:
    static APIService& getInstance();

    std::string username;
    
    bool login(const std::string& email, const std::string& password, bool rememberMe, 
              std::function<void(bool success, const std::string& message, const std::string& token)> callback);
              
    bool refreshToken(std::function<void(bool success, const std::string& newToken)> callback);
    
    bool getUserInfo(std::function<void(bool success, const std::string& userData)> callback);
    
    void setAuthToken(const std::string& token) { auth_token = token; }
    bool checkServerConnection() const;
    std::string getAuthToken() const { return auth_token; }
    bool isLoggedIn() const { return !auth_token.empty(); }

private:
    APIService();
    // Make sure there's no trailing slash in base_url
    std::string base_url = "http://10.42.234.85:4500";
    std::string auth_token;

    LoginStatus status = LoginStatus::IDLE;
    std::mutex status_mutex; // For thread safety
};