#include "Configuration.h"
#include "util.h"
#include <Arduino.h>
#include <string>

// Global instance
Configuration config;

Configuration::Configuration() {
    // Default values
    motorConfig.acceleration = 15000; // steps/sec², reaches 14,400 steps/sec in about a second
    motorConfig.maxSpeed = 180 * 80; // 180 * steps_per_mm
    motorConfig.limitPos1 = 0;
    motorConfig.limitPos2 = 2000; // Sane default to allow motor movement even without calibration
    motorConfig.freewheelAfterMove = false; // Disabled by default - motor holds position
    motorConfig.calibrationSpeed = 500; // steps/sec, used when calibrationDirectDrive is enabled
    motorConfig.calibrationDirectDrive = true; // Enabled by default - safest for rigs without gearing
    motorConfig.motorCurrent = 400; // Low on purpose: too much current can burn out a small motor
    motorConfig.encoderFitted = true;
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
    motorConfig.freewheelAfterMove = preferences.getBool("freewheel", motorConfig.freewheelAfterMove);
    motorConfig.calibrationSpeed = preferences.getLong("calSpeed", motorConfig.calibrationSpeed);
    motorConfig.calibrationDirectDrive = preferences.getBool("calDirectDrive", motorConfig.calibrationDirectDrive);
    motorConfig.motorCurrent = preferences.getLong("motorCurrent", motorConfig.motorCurrent);
    motorConfig.encoderFitted = preferences.getBool("encFitted", motorConfig.encoderFitted);

    LOG_INFO("Configuration loaded - Accel: %ld, MaxSpeed: %ld, Limit1: %ld, Limit2: %ld, Freewheel: %d, CalSpeed: %ld, CalDirectDrive: %d",
             motorConfig.acceleration, motorConfig.maxSpeed, motorConfig.limitPos1, motorConfig.limitPos2,
             motorConfig.freewheelAfterMove, motorConfig.calibrationSpeed, motorConfig.calibrationDirectDrive);
}

void Configuration::saveConfiguration() {
    preferences.putLong("acceleration", motorConfig.acceleration);
    preferences.putLong("maxSpeed", motorConfig.maxSpeed);
    preferences.putLong("limitPos1", motorConfig.limitPos1);
    preferences.putLong("limitPos2", motorConfig.limitPos2);
    preferences.putBool("freewheel", motorConfig.freewheelAfterMove);
    preferences.putLong("calSpeed", motorConfig.calibrationSpeed);
    preferences.putBool("calDirectDrive", motorConfig.calibrationDirectDrive);
    preferences.putLong("motorCurrent", motorConfig.motorCurrent);
    preferences.putBool("encFitted", motorConfig.encoderFitted);
    LOG_INFO("Configuration saved");
}

void Configuration::saveLimitPositions(long pos1, long pos2) {
    motorConfig.limitPos1 = pos1;
    motorConfig.limitPos2 = pos2;
    preferences.putLong("limitPos1", pos1);
    preferences.putLong("limitPos2", pos2);
    LOG_INFO("Limit positions saved: %ld, %ld", pos1, pos2);
}

String Configuration::getProgramJson() {
    return preferences.getString("program", "");
}

void Configuration::saveProgramJson(const String &json) {
    if (json.length() == 0)
        preferences.remove("program");
    else
        preferences.putString("program", json);
    LOG_INFO("Patterns saved (%u bytes)", json.length());
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

void Configuration::setCalibrationSpeed(long speed) {
    motorConfig.calibrationSpeed = speed;
    preferences.putLong("calSpeed", speed);
}

void Configuration::setMotorCurrent(long current) {
    motorConfig.motorCurrent = current;
    preferences.putLong("motorCurrent", current);
}

void Configuration::setEncoderFitted(bool value) {
    motorConfig.encoderFitted = value;
    preferences.putBool("encFitted", value);
}

void Configuration::setCalibrationDirectDrive(bool value) {
    motorConfig.calibrationDirectDrive = value;
    preferences.putBool("calDirectDrive", value);
}