#include "weapon.hpp"
#include <chrono>
#include <thread>

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
    bulletDamage = 10; // Default damage per bullet

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
