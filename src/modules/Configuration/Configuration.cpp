#include "Configuration.h"
#include <Arduino.h>

// Global instance
Configuration config;

Configuration::Configuration() {
    // Default values
    motorConfig.acceleration = 1000 * 80; // 80 steps per mm
    motorConfig.maxSpeed = 180 * 80; // 180 * steps_per_mm
    motorConfig.limitPos1 = 0;
    motorConfig.limitPos2 = 2500;
    motorConfig.useStealthChop = true;
}

bool Configuration::begin() {
    bool success = preferences.begin("motor-config", false);
    if (success) {
        loadConfiguration();
        Serial.println("Configuration module initialized");
    } else {
        Serial.println("Failed to initialize configuration module");
    }
    return success;
}

void Configuration::loadConfiguration() {
    motorConfig.acceleration = preferences.getLong("acceleration", motorConfig.acceleration);
    motorConfig.maxSpeed = preferences.getLong("maxSpeed", motorConfig.maxSpeed);
    motorConfig.limitPos1 = preferences.getLong("limitPos1", motorConfig.limitPos1);
    motorConfig.limitPos2 = preferences.getLong("limitPos2", motorConfig.limitPos2);
    motorConfig.useStealthChop = preferences.getBool("stealthChop", motorConfig.useStealthChop);

    Serial.printf("Configuration loaded - Accel: %ld, MaxSpeed: %ld, Limit1: %ld, Limit2: %ld\n",
                  motorConfig.acceleration, motorConfig.maxSpeed, motorConfig.limitPos1, motorConfig.limitPos2);
}

void Configuration::saveConfiguration() {
    preferences.putLong("acceleration", motorConfig.acceleration);
    preferences.putLong("maxSpeed", motorConfig.maxSpeed);
    preferences.putLong("limitPos1", motorConfig.limitPos1);
    preferences.putLong("limitPos2", motorConfig.limitPos2);
    preferences.putBool("stealthChop", motorConfig.useStealthChop);
    Serial.println("Configuration saved");
}

void Configuration::saveLimitPositions(long pos1, long pos2) {
    motorConfig.limitPos1 = pos1;
    motorConfig.limitPos2 = pos2;
    preferences.putLong("limitPos1", pos1);
    preferences.putLong("limitPos2", pos2);
    Serial.printf("Limit positions saved: %ld, %ld\n", pos1, pos2);
}

void Configuration::setAcceleration(long accel) {
    motorConfig.acceleration = accel;
    preferences.putLong("acceleration", accel);
}

void Configuration::setMaxSpeed(long speed) {
    motorConfig.maxSpeed = speed;
    preferences.putLong("maxSpeed", speed);
}