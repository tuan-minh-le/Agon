#include "player.hpp"
#include "environment.hpp" 
#include <algorithm> 
#include <cmath> 
#include <iostream> 
#include "environment.hpp" 
#include <algorithm> 
#include <cmath> 
#include <iostream> 

Player::Player()
    :movement_speed(0.f), height(0.f), position(0, 0, 0),is_dead(false),is_dead(false),
    velocity(0, 0, 0), acceleration(15.0f), deceleration(10.0f), max_velocity(6.0f),
    current_pitch(0.0f), max_pitch_up(85.0f), max_pitch_down(-85.0f), isGrounded(true), collision_radius(0.f),
    shooting_flag(false), moving_flag(false) { 
}

void Player::initialise(cgp::input_devices& inputs, cgp::window_structure& window, AudioSystem* audio_sys) {
    hp = 100;

    movement_speed = 6.0f;
    height = 1.95f;
    position = cgp::vec3(-3.f, -3.f, height);
    collision_radius = 0.5f;

    
    velocity = cgp::vec3(0, 0, 0);
    acceleration = 15.0f;    
    deceleration = 12.0f;    
    max_velocity = movement_speed;
    
    gravity = 11.f;

    jumpForce = 4.5f;
    isGrounded = true;

    
    current_pitch = 0.0f;
    max_pitch_up = 85.0f;     
    max_pitch_down = -85.0f;  

    camera.initialize(inputs, window);
    camera.set_rotation_axis_z(); 
    camera.look_at(position, position + cgp::vec3(0.2, 0, 0)); 
    
    camera.is_cursor_trapped = true;

    weapon.initialize(audio_sys);

    
    
    
    
    
    
    
    
    
}

void Player::set_initial_model_properties(const cgp::mesh& base_mesh_data, const cgp::rotation_transform& initial_rotation_transform) {
    initial_model_rotation = initial_rotation_transform; 
    player_visual_model.initialize_data_on_gpu(base_mesh_data);
    player_visual_model.model.set_scaling(0.9f);
    
    
    
    
}

void Player::update(float dt, const cgp::inputs_keyboard_parameters& keyboard, const cgp::inputs_mouse_parameters& mouse, cgp::mat4& camera_view_matrix) {
    if (is_dead) return; 

    static cgp::vec3 forward;
    static cgp::vec3 right;
    static cgp::vec3 desired_direction(0, 0, 0);
    static cgp::vec3 velocity_change;

    
    shooting_flag = false;
    moving_flag = false;
    running_flag = false;

    forward = camera.camera_model.front();
    right = camera.camera_model.right();

    forward.z = 0;
    right.z = 0;

    
    if (keyboard.is_pressed(GLFW_KEY_R)) {
        weapon.reload();
    }
    
    
    
    if (mouse.click.left) {
        shooting_flag = true; 
    }

    if (keyboard.is_pressed(GLFW_KEY_R)) {
        weapon.reload();
    }

    
    if (cgp::norm(forward) > 0.01f) forward = cgp::normalize(forward);
    if (cgp::norm(right) > 0.01f) right = cgp::normalize(right);

    
    desired_direction = { 0,0,0 };
    if (keyboard.is_pressed(GLFW_KEY_W)) {
        desired_direction += forward;
        moving_flag = true; 
    }
    if (keyboard.is_pressed(GLFW_KEY_S)) {
        desired_direction -= forward;
        moving_flag = true; 
    }
    if (keyboard.is_pressed(GLFW_KEY_D)) {
        desired_direction += right;
        moving_flag = true; 
    }
    if (keyboard.is_pressed(GLFW_KEY_A)) {
        desired_direction -= right;
        moving_flag = true; 
    }

    
    if (cgp::norm(desired_direction) > 0.01f) {
        desired_direction = cgp::normalize(desired_direction);
    }

    
    float target_speed = keyboard.shift ? max_velocity * 1.8f : max_velocity;
    cgp::vec3 target_velocity = desired_direction * target_speed;
    
    
    if (moving_flag && keyboard.shift) {
        running_flag = true;
    }

    
    if (cgp::norm(desired_direction) > 0.01f) {
        
        velocity_change = target_velocity - velocity;
        float change_rate = acceleration * dt;
        float max_change = cgp::norm(velocity_change);

        if (max_change > 0.01f) {
            velocity_change = velocity_change * std::min(change_rate / max_change, 1.0f);
            velocity += velocity_change;
        }
    }
    else {
        
        float current_speed = cgp::norm(velocity);
        if (current_speed > 0.01f) {
            float decel_amount = deceleration * dt;
            if (decel_amount >= current_speed) {
                velocity = { 0,0,0 };  
            }
            else {
                velocity *= (1.0f - decel_amount / current_speed);
            }
        }
    }




    

    
    if (position.z <= height) {
        position.z = height;  
        isGrounded = true;    
        verticalVelocity = 0.0f; 
    }

    if (isGrounded) {
        verticalVelocity = 0.0f; 
        if (keyboard.is_pressed(GLFW_KEY_SPACE)) {
            verticalVelocity = jumpForce; 
            isGrounded = false;          
        }
    }
    else {
        verticalVelocity -= gravity * dt; 
    }

    
    position.z += verticalVelocity * dt;

    
    cgp::vec3 intended_position = position;
    intended_position.x += velocity.x * dt;
    intended_position.y += velocity.y * dt;

    
    bool has_collision = (apartment != nullptr && apartment->check_collision(intended_position, collision_radius));

    if (!has_collision) {
        
        position = intended_position;
    }
    else {
        
        
        cgp::vec3 x_only_position = position;
        x_only_position.x += velocity.x * dt;
        bool x_collision = (apartment != nullptr && apartment->check_collision(x_only_position, collision_radius));

        
        cgp::vec3 y_only_position = position;
        y_only_position.y += velocity.y * dt;
        bool y_collision = (apartment != nullptr && apartment->check_collision(y_only_position, collision_radius));

        
        if (!x_collision) {
            position.x = x_only_position.x;
        }

        if (!y_collision) {
            position.y = y_only_position.y;
        }

        
        if (apartment != nullptr && apartment->check_collision(position, collision_radius)) {
            
            cgp::vec3 push_direction = compute_push_direction(position);

            
            float push_strength = 0.01f; 
            position += push_direction * push_strength;
        }
    }

    weapon.update(dt);

    
    float camera_forward_offset = 0.1f; 
    camera.camera_model.position_camera = position + camera.camera_model.front() * camera_forward_offset;
    camera_view_matrix = camera.camera_model.matrix_view();

    
    cgp::vec3 right_offset = camera.camera_model.right() * 0.2f; 
    player_visual_model.model.translation = position + right_offset;
    player_visual_model.model.translation.z -= 0.8f; 

    player_visual_model.model.rotation = cgp::rotation_transform::from_axis_angle({0,0,1}, camera.camera_model.yaw);

}


cgp::vec3 Player::compute_push_direction(const cgp::vec3& pos) {
    if (apartment == nullptr) return cgp::vec3(0, 0, 0);

    cgp::vec3 closest_point(0, 0, 0);
    float min_distance = 1000.0f;

    
    for (size_t i = 0; i < apartment->wall_positions.size(); ++i) {
        const cgp::vec3& wall_pos = apartment->wall_positions[i];
        const cgp::vec3& wall_dim = apartment->wall_dimensions[i];

        
        cgp::vec3 wall_min = wall_pos - wall_dim * 0.5f;
        cgp::vec3 wall_max = wall_pos + wall_dim * 0.5f;

        
        cgp::vec3 point;
        point.x = std::max(wall_min.x, std::min(pos.x, wall_max.x));
        point.y = std::max(wall_min.y, std::min(pos.y, wall_max.y));
        point.z = std::max(wall_min.z, std::min(pos.z, wall_max.z));

        float distance = cgp::norm(pos - point);
        if (distance < min_distance) {
            min_distance = distance;
            closest_point = point;
        }
    }

    
    cgp::vec3 direction = pos - closest_point;
    if (cgp::norm(direction) < 0.001f) {
        
        return cgp::vec3(0, 0, 1);
    }
    return cgp::normalize(direction);
}

cgp::vec3 Player::getPosition() const
{
    return position;
}


bool Player::isShooting() const {
    return shooting_flag;
}

bool Player::isMoving() const {
    return moving_flag;
}

bool Player::isRunning() const {
    return running_flag;
}

void Player::handle_mouse_move(cgp::vec2 const& mouse_position_current, cgp::vec2 const& mouse_position_previous, cgp::mat4& camera_view_matrix) {
    
    camera.action_mouse_move(camera_view_matrix); 

    
    float& current_cam_pitch_rad = camera.camera_model.pitch; 

    
    
    float local_max_pitch_up_rad = max_pitch_up * cgp::Pi / 180.0f;
    float local_max_pitch_down_rad = max_pitch_down * cgp::Pi / 180.0f; 

    current_cam_pitch_rad = cgp::clamp(current_cam_pitch_rad, local_max_pitch_down_rad, local_max_pitch_up_rad);

    
    camera_view_matrix = camera.camera_model.matrix_view();

    
}

void Player::set_apartment(Apartment* apartment_ptr)
{
    apartment = apartment_ptr;
}

const Weapon& Player::getWeapon() const {
    return weapon;
}

Weapon& Player::getWeaponMutable() {
    return weapon;
}

HitInfo Player::performShoot(const std::map<std::string, RemotePlayer>& remote_players) {
    shooting_flag = true; 
    return weapon.shootWithHitDetection(*this, remote_players);
}

void Player::draw_model(const cgp::environment_generic_structure& environment) {
    
    
    
    
    
    
    

    draw(player_visual_model, environment);
}

void Player::die() {
    is_dead = true;
    hp = 0;
    std::cout << "Player died." << std::endl;
}

void Player::respawn() {
    is_dead = false;
    hp = 100;
    position = cgp::vec3(-3.f, -3.f, height); // Position initiale
    velocity = cgp::vec3(0, 0, 0);
    verticalVelocity = 0.0f;
    std::cout << "Player respawned." << std::endl;
}

bool Player::isDead() const {
    return is_dead;
}

void Player::die() {
    is_dead = true;
    hp = 0;
    std::cout << "Player died." << std::endl;
}

void Player::respawn() {
    is_dead = false;
    hp = 100;
    position = cgp::vec3(-3.f, -3.f, height); // Position initiale
    velocity = cgp::vec3(0, 0, 0);
    verticalVelocity = 0.0f;
    std::cout << "Player respawned." << std::endl;
}

bool Player::isDead() const {
    return is_dead;
}


void Player::updateHealth(int healthChange) {
    hp += healthChange;
    
    
    if (hp > 100) {
        hp = 100;
    } else if (hp < 0) {
        hp = 0;
    }
    
    std::cout << "Player health updated: " << hp << " HP (change: " << healthChange << ")" << std::endl;
}

void Player::setHP(int newHP) {
    
    if (newHP > 100) {
        hp = 100;
    } else if (newHP < 0) {
        hp = 0;
    } else {
        hp = newHP;
    }
    
    std::cout << "Player health set to: " << hp << " HP" << std::endl;
}

void Player::die() {
    is_dead = true;
    hp = 0;
    std::cout << "Player died." << std::endl;
}

void Player::respawn() {
    is_dead = false;
    hp = 100;
    position = cgp::vec3(-3.f, -3.f, height); 
    velocity = cgp::vec3(0, 0, 0);
    verticalVelocity = 0.0f;
    std::cout << "Player respawned." << std::endl;
}

bool Player::isDead() const {
    return is_dead;
}

bool Player::getGrounded() const {
    return isGrounded;
}
