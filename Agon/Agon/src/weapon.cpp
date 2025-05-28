#include "weapon.hpp"
#include <chrono>
#include <thread>
#include "player.hpp"
#include "remote_player.hpp"
#include <map>

Weapon::Weapon()
    : currentMag(0), maxBullet(0), fireRate(0.f), reloadTime(0.f), reloading(false),
    lastShotTime(0), totalAmmo(0), bulletDamage(0)
{
}

void Weapon::initialize()
{
    currentMag = 30;
    maxBullet = 30;
    totalAmmo = 120;  // Total ammo available for reloads
    fireRate = 0.1f;  // Time between shots in seconds
    reloadTime = 2.0f; // Reload time in seconds
    reloading = false;
    lastShotTime = 0;
    bulletDamage = 20; // Default damage per bullet

    // Initialize timer
    lastShotTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::steady_clock::now().time_since_epoch()
    ).count();
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
    
    // Get shooter's camera information for raycasting
    cgp::vec3 ray_origin = shooter.camera.camera_model.position();
    cgp::vec3 ray_direction = shooter.camera.camera_model.front();
    
    std::cout << "Shot fired! Ray origin: (" << ray_origin.x << ", " << ray_origin.y << ", " << ray_origin.z << ")" << std::endl;
    std::cout << "Ray direction: (" << ray_direction.x << ", " << ray_direction.y << ", " << ray_direction.z << ")" << std::endl;
    
    // Find the closest hit among all remote players
    float closest_distance = std::numeric_limits<float>::max();
    std::string hit_player_id;
    cgp::vec3 hit_position;
    
    for (const auto& player_pair : remote_players) {
        const std::string& player_id = player_pair.first;
        const RemotePlayer& remote_player = player_pair.second;
        
        float hit_distance;
        if (checkPlayerHit(ray_origin, ray_direction, remote_player, hit_distance)) {
            if (hit_distance < closest_distance) {
                closest_distance = hit_distance;
                hit_player_id = player_id;
                hit_position = ray_origin + ray_direction * hit_distance;
                hit_info.hit = true;
            }
        }
    }
    
    // Fill in hit information if we hit someone
    if (hit_info.hit) {
        hit_info.target_player_id = hit_player_id;
        hit_info.hit_position = hit_position;
        hit_info.distance = closest_distance;
        hit_info.damage = bulletDamage;
        
        std::cout << "HIT! Player: " << hit_player_id << " at distance: " << closest_distance << std::endl;
        std::cout << "Hit position: (" << hit_position.x << ", " << hit_position.y << ", " << hit_position.z << ")" << std::endl;
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
                           const RemotePlayer& target, float& hit_distance) const {
    
    // Simple sphere-based hit detection around player position
    // This is a very basic implementation - you could make it more sophisticated
    
    cgp::vec3 player_position = target.position;
    float player_radius = 0.8f; // Approximate player hitbox radius
    
    // Use the intersection_ray_sphere function from cgp library
    cgp::intersection_structure intersection = cgp::intersection_ray_sphere(
        ray_origin, ray_direction, player_position, player_radius
    );
    
    if (intersection.valid) {
        hit_distance = cgp::norm(intersection.position - ray_origin);
        return true;
    }
    
    return false;
}
