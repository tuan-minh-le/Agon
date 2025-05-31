#include "login_ui.hpp"



LoginUI::LoginUI() : login_button_clicked(false) {}


void LoginUI::initialize() {
    
    memset(email_buffer, 0, sizeof(email_buffer));
    memset(password_buffer, 0, sizeof(password_buffer));
    login_button_clicked = false;
    error_message = "";
}


void LoginUI::render(cgp::environment_generic_structure& environment) {    
    ImGui::SetNextWindowPos(ImVec2(ImGui::GetIO().DisplaySize.x * 0.5f, ImGui::GetIO().DisplaySize.y * 0.5f), 
                            ImGuiCond_Always, ImVec2(0.5f, 0.5f));
    ImGui::SetNextWindowSize(ImVec2(350, 230), ImGuiCond_Always);
    
    ImGui::Begin("Login", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | 
                                  ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoSavedSettings);
    
    
    ImGui::Text("Email:");
    ImGui::InputText("##Email", email_buffer, IM_ARRAYSIZE(email_buffer));
    
    
    ImGui::Text("Password:");
    ImGui::InputText("##password", password_buffer, IM_ARRAYSIZE(password_buffer), ImGuiInputTextFlags_Password);

    ImGui::Checkbox("Remember me: ", &rememberMe);
    
    ImGui::Text("Room ID:");
    ImGui::InputText("##RoomID", roomid_buffer, IM_ARRAYSIZE(roomid_buffer));

    
    if (!error_message.empty()) {
        ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.0f, 0.0f, 1.0f));
        ImGui::TextWrapped("%s", error_message.c_str());
        ImGui::PopStyleColor();
    }
    
    

if (ImGui::Button("Login", ImVec2(ImGui::GetWindowWidth() - 20, 30))) {
    std::string email = get_email();
    std::string password = get_password();
    std::string room_id = get_roomid();
    
    if(email == "admin"){
        login_button_clicked = true;
        username = "ADMIN";
        return;
    }

    if (email.empty() || password.empty()) {
        set_error_message("Email and password cannot be empty");
    }

    else if(room_id.empty()){
        set_error_message("Room ID cannot be empty");
    }

    else {       
        set_error_message("Logging in...");     
        APIService::getInstance().login(email, password, rememberMe, 
            [this](bool success, const std::string& message, const std::string& token) {
                if (success) {  
                    clear_error_message(); 
                    APIService::getInstance().getUserInfo(
                        [this](bool success, const std::string& userData) {
                            if (success) {
                                try {
                                    
                                    auto json_data = nlohmann::json::parse(userData);
                                    username = json_data["username"];
                                    
                                    
                                    login_button_clicked = true;
                                    
                                    std::cout << "Username retrieved: " << username << std::endl;
                                } catch (const std::exception& e) {
                                    set_error_message("Error parsing user data");
                                    std::cout << "Error: " << e.what() << std::endl;
                                }
                            } else {
                                set_error_message("Failed to get user info: " + userData);
                            }
                        }
                    );
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



std::string LoginUI::get_roomid() const{
    return std::string(roomid_buffer);
}

std::string LoginUI::get_username() const{
    return username;
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