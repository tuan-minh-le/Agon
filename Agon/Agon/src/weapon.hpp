#ifndef WEAPON_HPP
#define WEAPON_HPP

#include <iostream>
#include <chrono>
#include "cgp/cgp.hpp"

// Forward declarations
struct RemotePlayer;
class Player;

// Structure to hold hit information
struct HitInfo {
    bool hit = false;
    std::string target_player_id;
    cgp::vec3 hit_position;
    float distance;
    int damage;
};

class Weapon {
private:
    int currentMag;     // Current bullets in magazine
    int maxBullet;      // Maximum magazine capacity
    int totalAmmo;      // Total ammo available for reloads
    int bulletDamage;   // Damage per bullet

    bool reloading;     // Is the weapon currently reloading?

    // Time tracking for shooting and reloading
    int lastShotTime;
    int reloadStartTime;

public:
    Weapon();

    void initialize();

    float fireRate;     // Time between shots in seconds
    float reloadTime;   // Time to reload in seconds

    // Core functions
    void reload();
    void shoot();
    void update(float dt);
    bool canShoot() const;

    // New shooting system with hit detection
    HitInfo shootWithHitDetection(const Player& shooter, const std::map<std::string, RemotePlayer>& remote_players);

    // Getters
    int getBulletCount() const;
    int getTotalAmmo() const;
    bool isReloading() const;
    float getReloadProgress() const;
    int getDamage() const;

    // Ammo management
    void addAmmo(int amount);

private:
    // Helper function for raycasting hit detection
    bool checkPlayerHit(const cgp::vec3& ray_origin, const cgp::vec3& ray_direction, 
                       const RemotePlayer& target, float& hit_distance) const;
};

#endif // !WEAPON_HPP
