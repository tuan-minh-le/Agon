#pragma once
#include "cgp/cgp.hpp"
#include "api_service.hpp"
#include <string>

class LoginUI {
public:
    LoginUI();
    ~LoginUI();
    
    void initialize();
    void render(cgp::environment_generic_structure& environment);
    
    bool is_login_button_clicked() const;
    std::string get_email() const;
    std::string get_password() const;
    std::string get_username() const;
    std::string get_roomid() const;
    std::string get_token() const;
    void set_error_message(const std::string& message);
    void clear_error_message();
    void reset_login_clicked();
    
private:
    char email_buffer[128] = "";
    char password_buffer[128] = "";
    char roomid_buffer[128] = "";
    std::string username;
    std::string auth_token;
    bool rememberMe;
    bool login_button_clicked;
    std::string error_message;
    cgp::mesh_drawable background;
    cgp::environment_generic_structure background_environment;

};