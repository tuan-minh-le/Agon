#ifndef WEAPON_HPP
#define WEAPON_HPP

#include <iostream>
#include <chrono>

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

    // Getters
    int getBulletCount() const;
    int getTotalAmmo() const;
    bool isReloading() const;
    float getReloadProgress() const;
    int getDamage() const;

    // Ammo management
    void addAmmo(int amount);
};

#endif // !WEAPON_HPP
