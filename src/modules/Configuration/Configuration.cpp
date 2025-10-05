#include "Configuration.h"
#include "util.h"
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
    motorConfig.freewheelAfterMove = false; // Disabled by default - motor holds position
}

bool Configuration::begin() {
    bool success = preferences.begin("motor-config", false);
    if (success) {
        loadConfiguration();
        LOG_INFO("Configuration module initialized");
    } else {
        LOG_ERROR("Failed to initialize configuration module");
    }
    return success;
}

void Configuration::loadConfiguration() {
    motorConfig.acceleration = preferences.getLong("acceleration", motorConfig.acceleration);
    motorConfig.maxSpeed = preferences.getLong("maxSpeed", motorConfig.maxSpeed);
    motorConfig.limitPos1 = preferences.getLong("limitPos1", motorConfig.limitPos1);
    motorConfig.limitPos2 = preferences.getLong("limitPos2", motorConfig.limitPos2);
    motorConfig.useStealthChop = preferences.getBool("stealthChop", motorConfig.useStealthChop);
    motorConfig.freewheelAfterMove = preferences.getBool("freewheel", motorConfig.freewheelAfterMove);

    LOG_INFO("Configuration loaded - Accel: %ld, MaxSpeed: %ld, Limit1: %ld, Limit2: %ld, Freewheel: %d",
             motorConfig.acceleration, motorConfig.maxSpeed, motorConfig.limitPos1, motorConfig.limitPos2,
             motorConfig.freewheelAfterMove);
}

void Configuration::saveConfiguration() {
    preferences.putLong("acceleration", motorConfig.acceleration);
    preferences.putLong("maxSpeed", motorConfig.maxSpeed);
    preferences.putLong("limitPos1", motorConfig.limitPos1);
    preferences.putLong("limitPos2", motorConfig.limitPos2);
    preferences.putBool("stealthChop", motorConfig.useStealthChop);
    preferences.putBool("freewheel", motorConfig.freewheelAfterMove);
    LOG_INFO("Configuration saved");
}

void Configuration::saveLimitPositions(long pos1, long pos2) {
    motorConfig.limitPos1 = pos1;
    motorConfig.limitPos2 = pos2;
    preferences.putLong("limitPos1", pos1);
    preferences.putLong("limitPos2", pos2);
    LOG_INFO("Limit positions saved: %ld, %ld", pos1, pos2);
}

void Configuration::setAcceleration(long accel) {
    motorConfig.acceleration = accel;
    preferences.putLong("acceleration", accel);
}

void Configuration::setMaxSpeed(long speed) {
    motorConfig.maxSpeed = speed;
    preferences.putLong("maxSpeed", speed);
}

void Configuration::setFreewheelAfterMove(bool value) {
    motorConfig.freewheelAfterMove = value;
    preferences.putBool("freewheel", value);
}