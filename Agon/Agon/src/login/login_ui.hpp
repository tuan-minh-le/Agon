#pragma once
#include "cgp/cgp.hpp"
#include "api_service.hpp"
#include <string>

class LoginUI {
public:
    LoginUI();
    ~LoginUI();
    
    void initialize();
    void render();
    
    bool is_login_button_clicked() const;
    std::string get_email() const;
    std::string get_password() const;
    void set_error_message(const std::string& message);
    void clear_error_message();

    void reset_login_clicked();
    
private:
    char email_buffer[128] = "";
    char password_buffer[128] = "";
    bool rememberMe;
    bool login_button_clicked;
    std::string error_message;
    cgp::mesh_drawable background;
    

};