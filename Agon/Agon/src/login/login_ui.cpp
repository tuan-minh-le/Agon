#include "login_ui.hpp"



LoginUI::LoginUI() : login_button_clicked(false) {
    // Constructor implementation
}

LoginUI::~LoginUI() {
    // Destructor implementation
}

void LoginUI::initialize() {
    
    // Clear buffers
    memset(email_buffer, 0, sizeof(email_buffer));
    memset(password_buffer, 0, sizeof(password_buffer));
    login_button_clicked = false;
    error_message = "";
}

void LoginUI::render() {
    
    // Setup ImGui window
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), 
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(300, 200), ImGuiCond_Always);
    
    ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    
    // email field
    ImGui::Text("Email:");
    ImGui::InputText("##Email", email_buffer, IM_ARRAYSIZE(email_buffer));
    
    // Password field
    ImGui::Text("Password:");
    ImGui::InputText("##password", password_buffer, IM_ARRAYSIZE(password_buffer), ImGuiInputTextFlags_Password);

    ImGui::Checkbox("Remember me: ", &rememberMe);
    
    // Error message if any
    if (!error_message.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextWrapped("%s", error_message.c_str());
        ImGui::PopStyleColor();
    }
    
    // Login button
    if (ImGui::Button("Login", ImVec2(ImGui::GetWindowWidth() - 20, 30))) {
        std::string email = get_email();
        std::string password = get_password();
        
        if (email.empty() || password.empty()) {
            set_error_message("Username and password cannot be empty");
        } else {
            // Show loading indicator or disable button
            
            // Call API service
            APIService::getInstance().login(email, password, rememberMe, 
                [this](bool success, const std::string& message, const std::string& token) {
                    if (success) {
                        login_button_clicked = true;
                        clear_error_message();
                    } else {
                        set_error_message(message);
                    }
                }
            );
        }
    }

    
    
    ImGui::End();
}

bool LoginUI::is_login_button_clicked() const {
    return login_button_clicked;
}

std::string LoginUI::get_email() const {
    return std::string(email_buffer);
}

std::string LoginUI::get_password() const {
    return std::string(password_buffer);
}

void LoginUI::set_error_message(const std::string& message) {
    error_message = message;
}

void LoginUI::clear_error_message() {
    error_message.clear();
}

void LoginUI::reset_login_clicked() {
    login_button_clicked = false;
}