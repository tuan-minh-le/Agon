#include "weapon.hpp"
#include <chrono>
#include <thread>
#include "player.hpp"
#include "remote_player.hpp"
#include "audio_system.hpp"
#include <map>

Weapon::Weapon()
    : currentMag(0), maxBullet(0), fireRate(0.f), reloadTime(0.f), reloading(false),
    lastShotTime(0), totalAmmo(0), bulletDamage(0), audio_system(nullptr)
{
}

void Weapon::initialize(AudioSystem* audio_sys)
{
    currentMag = 30;
    maxBullet = 30;
    totalAmmo = 120;  // Total ammo available for reloads
    fireRate = 0.1f;  // Time between shots in seconds
    reloadTime = 2.0f; // Reload time in seconds
    reloading = false;
    lastShotTime = 0;
    bulletDamage = 20; // Default damage per bullet
    audio_system = audio_sys;

    // Initialize timer
    lastShotTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    // Load gunshot and reload sounds if audio system is available
    if (audio_system) {
        if (!audio_system->load_audio_clip("gunshot", "assets/gunshot.wav")) {
            std::cerr << "Failed to load gunshot sound" << std::endl;
        }
        if (!audio_system->load_audio_clip("reload", "assets/reload.wav")) {
            std::cerr << "Failed to load reload sound" << std::endl;
        }
    }
}

void Weapon::reload()
{
    // Check if we need to reload and if we have ammo
    if (!reloading && currentMag < maxBullet && totalAmmo > 0)
    {
        reloading = true;
        reloadStartTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        // Play reload sound
        if (audio_system) {
            audio_system->play_sound_2d("reload", 0.7f);
        }
    }
}

void Weapon::update(float dt)
{
    if(reloading)
    {
        // Get current time
        auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();
        
        std::cout << "Reloading: " << (reloadTime * 1000 - (currentTime - reloadStartTime))/1000 << "s left" << std::endl;

        // Check if reload is complete
        if (currentTime - reloadStartTime >= reloadTime * 1000)
        {
            // Calculate how many bullets we can reload
            int bulletsNeeded = maxBullet - currentMag;
            int bulletsToReload = std::min(bulletsNeeded, totalAmmo);

            // Update ammo counts
            totalAmmo -= bulletsToReload;
            currentMag += bulletsToReload;

            reloading = false;
            std::cout << "Reload complete. Magazine: " << currentMag
                << ", Total ammo: " << totalAmmo << std::endl;
        }
    }
}

bool Weapon::canShoot() const
{
    // Check if we can shoot (not reloading, have ammo, and fire rate elapsed)
    if (reloading || currentMag <= 0)
        return false;

    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    return (currentTime - lastShotTime >= fireRate * 1000);
}

void Weapon::shoot()
{
    if (canShoot())
    {
        // Decrease ammo
        currentMag--;

        // Update last shot time
        lastShotTime = std::chrono::duration_cast<std::chrono::milliseconds>(
            std::chrono::steady_clock::now().time_since_epoch()
        ).count();

        // Play gunshot sound
        if (audio_system) {
            audio_system->play_sound_2d("gunshot", 0.8f, false);
        }

        // Shooting logic 
        std::cout << "Shot! Ammo remaining: " << currentMag << std::endl;

        // Auto-reload if magazine is empty
        if (currentMag == 0 && totalAmmo > 0) {
            reload();
        }
    }
    else if (currentMag == 0) {
        std::cout << "Click! Out of ammo." << std::endl;

        // Auto-reload if we have ammo
        if (totalAmmo > 0 && !reloading) {
            reload();
        }
    }
}

int Weapon::getBulletCount() const {
    return currentMag;
}

int Weapon::getTotalAmmo() const {
    return totalAmmo;
}

bool Weapon::isReloading() const {
    return reloading;
}

float Weapon::getReloadProgress() const {
    if (!reloading)
        return 0.f; 

    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();

    float elapsed = (currentTime - reloadStartTime) / 1000.0f;
    return reloadTime - elapsed;
}

void Weapon::addAmmo(int amount) {
    totalAmmo += amount;
}

int Weapon::getDamage() const {
    return bulletDamage;
}

HitInfo Weapon::shootWithHitDetection(const Player& shooter, const std::map<std::string, RemotePlayer>& remote_players) {
    HitInfo hit_info;
    
    // Check if we can shoot
    if (!canShoot()) {
        return hit_info; // Return empty hit info if can't shoot
    }
    
    // Consume ammo and update shot time (same as regular shoot)
    currentMag--;
    lastShotTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
    
    // Play gunshot sound
    if (audio_system) {
        audio_system->play_sound_2d("gunshot", 0.8f, false);
    }
    
    // Get shooter's camera information for raycasting
    cgp::vec3 ray_origin = shooter.camera.camera_model.position();
    cgp::vec3 ray_direction = shooter.camera.camera_model.front();
    
    std::cout << "Shot fired! Ray origin: (" << ray_origin.x << ", " << ray_origin.y << ", " << ray_origin.z << ")" << std::endl;
    std::cout << "Ray direction: (" << ray_direction.x << ", " << ray_direction.y << ", " << ray_direction.z << ")" << std::endl;
    
    // Find the closest hit among all remote players
    float closest_distance = std::numeric_limits<float>::max();
    std::string hit_player_id;
    cgp::vec3 hit_position;
    int hit_damage = 0;
    
    for (const auto& player_pair : remote_players) {
        const std::string& player_id = player_pair.first;
        const RemotePlayer& remote_player = player_pair.second;
        
        float hit_distance;
        int calculated_damage;
        if (checkPlayerHit(ray_origin, ray_direction, remote_player, hit_distance, calculated_damage)) {
            if (hit_distance < closest_distance) {
                closest_distance = hit_distance;
                hit_player_id = player_id;
                hit_position = ray_origin + ray_direction * hit_distance;
                hit_damage = calculated_damage;
                hit_info.hit = true;
            }
        }
    }
    
    // Fill in hit information if we hit someone
    if (hit_info.hit) {
        hit_info.target_player_id = hit_player_id;
        hit_info.hit_position = hit_position;
        hit_info.distance = closest_distance;
        hit_info.damage = hit_damage; // Use calculated damage based on hit height
        
        std::cout << "HIT! Player: " << hit_player_id << " at distance: " << closest_distance << std::endl;
        std::cout << "Hit position: (" << hit_position.x << ", " << hit_position.y << ", " << hit_position.z << ")" << std::endl;
        std::cout << "Damage dealt: " << hit_damage << " (based on hit height: " << hit_position.z << ")" << std::endl;
    } else {
        std::cout << "MISS! No players hit." << std::endl;
    }
    
    // Auto-reload if magazine is empty
    if (currentMag == 0 && totalAmmo > 0) {
        reload();
    }
    
    return hit_info;
}

bool Weapon::checkPlayerHit(const cgp::vec3& ray_origin, const cgp::vec3& ray_direction, 
                           const RemotePlayer& target, float& hit_distance, int& damage) const {
    
    // Player's position represents the camera/eye level (top of player)
    cgp::vec3 player_eye_position = target.position;
    
    // Calculate feet position - player height is 1.9f, so feet are 1.9f below eye level
    cgp::vec3 player_feet_position = player_eye_position;
    player_feet_position.z -= 1.9f;
    
    std::cout << "DEBUG: Target player eye position: (" << player_eye_position.x << ", " << player_eye_position.y << ", " << player_eye_position.z << ")" << std::endl;
    std::cout << "DEBUG: Target player feet position: (" << player_feet_position.x << ", " << player_feet_position.y << ", " << player_feet_position.z << ")" << std::endl;
    
    // Player hitbox dimensions
    float player_radius = 0.5f; // Horizontal radius
    float player_height = 1.9f; // Total player height (matching player.hpp)
    
    // Define hitbox zones for different damage levels - these are RELATIVE heights from player's feet
    float legs_top = 1.0f;      // Top of legs zone
    float body_top = 1.75f;     // Top of body zone  
    float head_top = 1.9f;      // Top of head zone (player height)
    
    // Check intersection with a cylinder representing the player
    // We'll approximate this by checking intersection with multiple spheres at different heights
    
    bool hit_found = false;
    float closest_hit_distance = std::numeric_limits<float>::max();
    cgp::vec3 closest_hit_position;
    int hit_zone_damage = 5; // Default to legs damage
    
    // Sample multiple points along the player's height to create a better hitbox
    const int num_samples = 20; // Number of sample points along height
    for (int i = 0; i < num_samples; ++i) {
        float height_ratio = static_cast<float>(i) / (num_samples - 1);
        float sample_height = height_ratio * player_height;
        
        cgp::vec3 sample_center = player_feet_position;
        sample_center.z = player_feet_position.z + sample_height;
        
        // Use smaller radius for more accurate hit detection
        float sample_radius = player_radius * 0.8f;
        
        // Check intersection with this sphere
        cgp::intersection_structure intersection = cgp::intersection_ray_sphere(
            ray_origin, ray_direction, sample_center, sample_radius
        );
        
        if (intersection.valid) {
            float distance = cgp::norm(intersection.position - ray_origin);
            
            if (distance < closest_hit_distance) {
                closest_hit_distance = distance;
                closest_hit_position = intersection.position;
                hit_found = true;
                
                // Determine damage based on hit height
                float hit_height = intersection.position.z - player_feet_position.z;
                
                std::cout << "DEBUG: Hit at world Z: " << intersection.position.z << ", player feet Z: " << player_feet_position.z << std::endl;
                std::cout << "DEBUG: Calculated hit height (relative): " << hit_height << std::endl;
                
                if (hit_height < legs_top) {
                    hit_zone_damage = 5;  // Legs
                    std::cout << "DEBUG: Hit classified as LEGS (damage: 5)" << std::endl;
                } else if (hit_height < body_top) {
                    hit_zone_damage = 15; // Body  
                    std::cout << "DEBUG: Hit classified as BODY (damage: 10)" << std::endl;
                } else if (hit_height <= head_top) {
                    hit_zone_damage = 50; // Head
                    std::cout << "DEBUG: Hit classified as HEAD (damage: 20)" << std::endl;
                } else {
                    hit_zone_damage = 5;  // Default to legs if somehow above head
                    std::cout << "DEBUG: Hit above head, defaulting to LEGS (damage: 5)" << std::endl;
                }
            }
        }
    }
    
    if (hit_found) {
        hit_distance = closest_hit_distance;
        damage = hit_zone_damage;
        
        float hit_height = closest_hit_position.z - player_feet_position.z;
        std::string zone_name = (hit_height < legs_top) ? "LEGS" : 
                               (hit_height < body_top) ? "BODY" : "HEAD";
        
        std::cout << "Hit detected in " << zone_name << " zone (height: " << hit_height << ", damage: " << damage << ")" << std::endl;
        
        return true;
    }
    
    return false;
}
