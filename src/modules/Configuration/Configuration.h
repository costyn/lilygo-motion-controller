#pragma once

#include <Preferences.h>

class Configuration
{
private:
    // ESP NVRAM Preference API
    Preferences preferences;

public:
    // Motor configuration
    struct MotorConfig
    {
        long acceleration;
        long maxSpeed;
        long limitPos1;
        long limitPos2;
        bool useStealthChop;
    } motorConfig;

    // Constructor
    Configuration();

    // Initialize configuration system
    bool begin();

    // Load configuration from NVRAM
    void loadConfiguration();

    // Save configuration to NVRAM
    void saveConfiguration();

    // Save only limit positions (called when limits are triggered)
    void saveLimitPositions(long pos1, long pos2);

    // Get configuration values
    long getAcceleration() const { return motorConfig.acceleration; }
    long getMaxSpeed() const { return motorConfig.maxSpeed; }
    long getLimitPos1() const { return motorConfig.limitPos1; }
    long getLimitPos2() const { return motorConfig.limitPos2; }
    long getMinLimit() const { return min(motorConfig.limitPos1, motorConfig.limitPos2); }
    long getMaxLimit() const { return max(motorConfig.limitPos1, motorConfig.limitPos2); }
    bool getUseStealthChop() const { return motorConfig.useStealthChop; }

    // Set configuration values
    void setAcceleration(long accel);
    void setMaxSpeed(long speed);
    void setLimitPos1(long pos) { motorConfig.limitPos1 = pos; }
    void setLimitPos2(long pos) { motorConfig.limitPos2 = pos; }
    void setUseStealthChop(bool use) { motorConfig.useStealthChop = use; }
};

extern Configuration config;