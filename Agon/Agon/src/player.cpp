#include "player.hpp"
#include "environment.hpp" // Keep this include
#include <algorithm> // For std::clamp
#include <cmath> // For M_PI/cgp::Pi if not available directly
#include <iostream> // For std::cout

Player::Player()
    :movement_speed(0.f), height(0.f), position(0, 0, 0),is_dead(false),
    velocity(0, 0, 0), acceleration(15.0f), deceleration(10.0f), max_velocity(6.0f),
    current_pitch(0.0f), max_pitch_up(85.0f), max_pitch_down(-85.0f), isGrounded(true), collision_radius(0.f),
    shooting_flag(false), moving_flag(false) { // Initialize new flags
}

void Player::initialise(cgp::input_devices& inputs, cgp::window_structure& window) {
    hp = 100;

    movement_speed = 6.0f;
    height = 1.9f;
    position = cgp::vec3(-3.f, -3.f, height);
    collision_radius = 0.5f;

    // Initialize smooth movement variables
    velocity = cgp::vec3(0, 0, 0);
    acceleration = 15.0f;    // Accelerate to full speed in ~0.4 seconds
    deceleration = 12.0f;    // Decelerate to stop in ~0.6 seconds
    max_velocity = movement_speed;
    // Initialize jumping attributes
    gravity = 11.f;

    jumpForce = 4.5f;
    isGrounded = true;

    // Initialize camera rotation limits
    current_pitch = 0.0f;
    max_pitch_up = 85.0f;     // Can look up to 85 degrees
    max_pitch_down = -85.0f;  // Can look down to 85 degrees

    camera.initialize(inputs, window);
    camera.set_rotation_axis_z(); // Sets z axis as the height for camera rotation and (x,y) for ground
    camera.look_at(position, position + cgp::vec3(1, 0, 0)); // Sets camera at position and looks at 1 unit along x axis

    // Enable cursor trapping for no-click rotation
    camera.is_cursor_trapped = true;

    weapon.initialize();

    // The player_visual_model will be initialized by set_initial_model_properties
    // using the base_player_mesh and initial_player_model_rotation from the scene.
    // So, remove direct loading and initialization here.
    // cgp::mesh model_mesh = cgp::mesh_load_file_obj("assets/man.obj");
    // model_mesh.centered(); 
    // model_mesh.scale(0.16f); 
    // initial_model_rotation = cgp::rotation_transform::from_axis_angle({1, 0, 0}, 90.0f * cgp::Pi / 180.0f);
    // model_mesh.apply_transform(initial_model_rotation.matrix()); 
    // player_visual_model.initialize_data_on_gpu(model_mesh);
}

void Player::set_initial_model_properties(const cgp::mesh& base_mesh_data, const cgp::rotation_transform& initial_rotation_transform) {
    initial_model_rotation = initial_rotation_transform; // Store the initial rotation
    player_visual_model.initialize_data_on_gpu(base_mesh_data);
    player_visual_model.model.set_scaling(0.6f);
    // The base_mesh_data is already centered, scaled, and rotated by initial_player_model_rotation in scene.cpp
    // So, we don't need to apply initial_model_rotation.matrix() to player_visual_model.model.rotation here.
    // The player_visual_model.model.rotation will be updated in Player::update based on camera orientation.
}

void Player::update(float dt, const cgp::inputs_keyboard_parameters& keyboard, const cgp::inputs_mouse_parameters& mouse, cgp::mat4& camera_view_matrix) {
    if (is_dead) return; // Bloque toute mise à jour si le joueur est mort

    static cgp::vec3 forward;
    static cgp::vec3 right;
    static cgp::vec3 desired_direction(0, 0, 0);
    static cgp::vec3 velocity_change;

    // Reset flags at the beginning of each update
    shooting_flag = false;
    moving_flag = false;
    running_flag = false;

    forward = camera.camera_model.front();
    right = camera.camera_model.right();

    forward.z = 0;
    right.z = 0;

    // Handle reload input
    if (keyboard.is_pressed(GLFW_KEY_R)) {
        weapon.reload();
    }
    
    // Shooting is now handled in scene.cpp with hit detection
    // Just update the shooting flag based on mouse input for networking
    if (mouse.click.left) {
        shooting_flag = true; // Set shooting flag for networking
    }

    if (keyboard.is_pressed(GLFW_KEY_R)) {
        weapon.reload();
    }

    // Normalize directions
    if (cgp::norm(forward) > 0.01f) forward = cgp::normalize(forward);
    if (cgp::norm(right) > 0.01f) right = cgp::normalize(right);

    // Calculate desired movement direction based on keyboard input
    desired_direction = { 0,0,0 };
    if (keyboard.is_pressed(GLFW_KEY_W)) {
        desired_direction += forward;
        moving_flag = true; // Set moving flag
    }
    if (keyboard.is_pressed(GLFW_KEY_S)) {
        desired_direction -= forward;
        moving_flag = true; // Set moving flag
    }
    if (keyboard.is_pressed(GLFW_KEY_D)) {
        desired_direction += right;
        moving_flag = true; // Set moving flag
    }
    if (keyboard.is_pressed(GLFW_KEY_A)) {
        desired_direction -= right;
        moving_flag = true; // Set moving flag
    }

    // Normalize the desired direction if it's not zero
    if (cgp::norm(desired_direction) > 0.01f) {
        desired_direction = cgp::normalize(desired_direction);
    }

    // Calculate target velocity
    float target_speed = keyboard.shift ? max_velocity * 1.8f : max_velocity;
    cgp::vec3 target_velocity = desired_direction * target_speed;
    
    // Set running flag if moving and shift is pressed
    if (moving_flag && keyboard.shift) {
        running_flag = true;
    }

    // Smoothly interpolate between current velocity and target velocity
    if (cgp::norm(desired_direction) > 0.01f) {
        // Acceleration
        velocity_change = target_velocity - velocity;
        float change_rate = acceleration * dt;
        float max_change = cgp::norm(velocity_change);

        if (max_change > 0.01f) {
            velocity_change = velocity_change * std::min(change_rate / max_change, 1.0f);
            velocity += velocity_change;
        }
    }
    else {
        // Deceleration (no input)
        float current_speed = cgp::norm(velocity);
        if (current_speed > 0.01f) {
            float decel_amount = deceleration * dt;
            if (decel_amount >= current_speed) {
                velocity = { 0,0,0 };  // Stop completely
            }
            else {
                velocity *= (1.0f - decel_amount / current_speed);
            }
        }
    }




    // Jumping and Gravity Logic

    // Check if the player is grounded
    if (position.z <= height) {
        position.z = height;  // Reset to ground level
        isGrounded = true;    // Player is now grounded
        verticalVelocity = 0.0f; // Reset vertical velocity
    }

    if (isGrounded) {
        verticalVelocity = 0.0f; // Reset vertical velocity when grounded
        if (keyboard.is_pressed(GLFW_KEY_SPACE)) {
            verticalVelocity = jumpForce; // Apply jump force
            isGrounded = false;          // Player is now mid-air
        }
    }
    else {
        verticalVelocity -= gravity * dt; // Apply gravity
    }

    // Update vertical position first (jumping/falling)
    position.z += verticalVelocity * dt;

    // Step 1: Try full movement
    cgp::vec3 intended_position = position;
    intended_position.x += velocity.x * dt;
    intended_position.y += velocity.y * dt;

    // Check if full movement would cause a collision
    bool has_collision = (apartment != nullptr && apartment->check_collision(intended_position, collision_radius));

    if (!has_collision) {
        // No collision, apply full movement
        position = intended_position;
    }
    else {
        // Step 2: Try sliding along walls
        // Try moving only on X axis
        cgp::vec3 x_only_position = position;
        x_only_position.x += velocity.x * dt;
        bool x_collision = (apartment != nullptr && apartment->check_collision(x_only_position, collision_radius));

        // Try moving only on Y axis
        cgp::vec3 y_only_position = position;
        y_only_position.y += velocity.y * dt;
        bool y_collision = (apartment != nullptr && apartment->check_collision(y_only_position, collision_radius));

        // Apply movement on non-colliding axes
        if (!x_collision) {
            position.x = x_only_position.x;
        }

        if (!y_collision) {
            position.y = y_only_position.y;
        }

        // Step 3: If still colliding, implement "push away" behavior
        if (apartment != nullptr && apartment->check_collision(position, collision_radius)) {
            // Find nearest wall and compute push-back direction
            cgp::vec3 push_direction = compute_push_direction(position);

            // Push player away from the wall slightly
            float push_strength = 0.01f; // Small push to avoid sticking
            position += push_direction * push_strength;
        }
    }

    weapon.update(dt);

    // Update camera position
    // camera.camera_model.position_camera = position; // Original line
    float camera_forward_offset = 0.5f; // Adjust this distance as needed
    camera.camera_model.position_camera = position + camera.camera_model.front() * camera_forward_offset;
    camera_view_matrix = camera.camera_model.matrix_view();

    // Update player model's visual properties
    player_visual_model.model.translation = position;
    player_visual_model.model.translation.z -= 0.8f; // Adjusted to lower player more

    // Model should rotate with camera's yaw, around the Z-axis.
    // Assumes camera.camera_model.axis_of_rotation is {0,0,1} due to set_rotation_axis_z()
    // and camera.camera_model.yaw is the yaw angle.
    player_visual_model.model.rotation = cgp::rotation_transform::from_axis_angle({0,0,1}, camera.camera_model.yaw);


}


cgp::vec3 Player::compute_push_direction(const cgp::vec3& pos) {
    if (apartment == nullptr) return cgp::vec3(0, 0, 0);

    cgp::vec3 closest_point(0, 0, 0);
    float min_distance = 1000.0f;

    // For each wall, find the closest one
    for (size_t i = 0; i < apartment->wall_positions.size(); ++i) {
        const cgp::vec3& wall_pos = apartment->wall_positions[i];
        const cgp::vec3& wall_dim = apartment->wall_dimensions[i];

        // Calculate minimum and maximum points of wall
        cgp::vec3 wall_min = wall_pos - wall_dim * 0.5f;
        cgp::vec3 wall_max = wall_pos + wall_dim * 0.5f;

        // Calculate closest point on wall to position
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

    // Compute push direction away from closest wall point
    cgp::vec3 direction = pos - closest_point;
    if (cgp::norm(direction) < 0.001f) {
        // If too close, use a default direction (up)
        return cgp::vec3(0, 0, 1);
    }
    return cgp::normalize(direction);
}

cgp::vec3 Player::getPosition() const
{
    return position;
}

// Implementations for new methods
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
    // Standard FPS camera behavior: mouse movement controls camera orientation
    camera.action_mouse_move(camera_view_matrix); // This updates camera_model.pitch, yaw and camera_view_matrix

    // Clamp pitch
    float& current_cam_pitch_rad = camera.camera_model.pitch; // Pitch is in radians

    // Convert player's degree-based limits to radians
    // Note: cgp::Pi should be available. If not, use M_PI from <cmath> or define Pi.
    float local_max_pitch_up_rad = max_pitch_up * cgp::Pi / 180.0f;
    float local_max_pitch_down_rad = max_pitch_down * cgp::Pi / 180.0f; // This is a negative value

    current_cam_pitch_rad = cgp::clamp(current_cam_pitch_rad, local_max_pitch_down_rad, local_max_pitch_up_rad);

    // Recompute camera_view_matrix as pitch might have been clamped
    camera_view_matrix = camera.camera_model.matrix_view();

    // The player model's rotation will be updated in Player::update based on camera.camera_model.yaw
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
    shooting_flag = true; // Set shooting flag for networking
    return weapon.shootWithHitDetection(*this, remote_players);
}

void Player::draw_model(const cgp::environment_generic_structure& environment) {
    // Ensure the model is updated with the latest position and rotation before drawing
    // This might be redundant if update() is always called before draw_model()
    // player_visual_model.model.translation = position;
    // cgp::rotation_transform camera_orientation = camera.camera_model.orientation();
    // cgp::mat4 view_matrix_no_pitch = cgp::look_at(cgp::vec3{0,0,0}, camera.camera_model.front_no_pitch(), camera.camera_model.up());
    // cgp::rotation_transform yaw_rotation = cgp::rotation_transform(cgp::mat3(view_matrix_no_pitch).inverse());
    // player_visual_model.model.rotation = yaw_rotation * initial_model_rotation;

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

// Health management methods
void Player::updateHealth(int healthChange) {
    hp += healthChange;
    
    // Clamp health to valid range (0 to 100)
    if (hp > 100) {
        hp = 100;
    } else if (hp < 0) {
        hp = 0;
    }
    
    std::cout << "Player health updated: " << hp << " HP (change: " << healthChange << ")" << std::endl;
}

void Player::setHP(int newHP) {
    // Clamp to valid range
    if (newHP > 100) {
        hp = 100;
    } else if (newHP < 0) {
        hp = 0;
    } else {
        hp = newHP;
    }
    
    std::cout << "Player health set to: " << hp << " HP" << std::endl;
}
