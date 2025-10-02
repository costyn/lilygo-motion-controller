#pragma once

#include <AccelStepper.h>
#include <TMCStepper.h>
#include <SPI.h>

class MotorController {
private:
    // TMC2209 and stepper objects
    HardwareSerial* serialDriver;
    TMC2209Stepper* driver;
    AccelStepper* stepper;
    SPIClass* mt6816;

    // Position and speed tracking
    static double lastLocation;
    static double currentLocation;
    static double monitorSpeed;
    static float motorSpeed;
    static int8_t direction;

    // State management
    volatile long targetPosition;
    volatile bool emergencyStopActive;
    bool useStealthChop;

    // Speed threshold for TMC mode switching (percentage)
    const float STEALTH_CHOP_THRESHOLD = 0.5;

public:
    // Constructor
    MotorController();

    // Initialize motor system
    bool begin();

    // Initialize encoder (call from InputTask after LED sequence)
    bool initEncoder();

    // Initialize LED sequence (call from InputTask)
    void initLEDSequence();

    // Motor control methods
    void moveTo(long position, int speedPercent = 50);
    void stop();
    void stopGently();  // Stop without triggering emergency stop (for jogging)
    void emergencyStop();
    void clearEmergencyStop();

    // Position and status
    long getCurrentPosition() const;
    long getTargetPosition() const { return targetPosition; }
    double getMonitorSpeed() const { return monitorSpeed; }
    float getMotorSpeed() const { return motorSpeed; }
    int8_t getDirection() const { return direction; }
    bool isEmergencyStopped() const { return emergencyStopActive; }
    bool isStealthChopActive() const { return useStealthChop; }
    bool isMoving() const { return stepper->distanceToGo() != 0; }
    bool isEmergencyStopActive() const { return emergencyStopActive; }

    // Encoder operations
    int readEncoder();
    double calculateSpeed(float ms);

    // TMC2209 operations
    void updateTMCMode();
    void setTMCMode(bool stealthChop);
    uint32_t getTMCStatus();

    // Main update function (call from main loop)
    void update();

    // Configuration
    void setAcceleration(long accel);
    void setMaxSpeed(long speed);
    void setCurrentPosition(long position);
};

extern MotorController motorController;