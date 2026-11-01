#pragma once

#include <AccelStepper.h>
#include <TMCStepper.h>
#include <SPI.h>

// Calibration state machine states
enum CalibrationState {
    CAL_IDLE,           // Not calibrating
    CAL_FINDING_MIN,    // Moving toward MIN limit switch
    CAL_FINDING_MAX,    // Moving toward MAX limit switch
    CAL_COMPLETE        // Calibration finished (brief state before returning to IDLE)
};

class MotorController
{
private:
    // TMC2209 and stepper objects
    HardwareSerial *serialDriver;
    TMC2209Stepper *driver;
    AccelStepper *stepper;
    SPIClass *mt6816;

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

    // Step timing diagnostics (written in loop(), read from ClosedLoopTask)
    unsigned long lastUpdateMicros;
    volatile uint32_t maxStepGapUs;
    volatile uint32_t tmcSwitchCount;
    volatile uint32_t maxTmcSwitchUs;

    // Step counter shift requested from ClosedLoopTask, applied in update()
    volatile long pendingPositionShift;

    // TMC2209 UART address that answered at startup, -1 if none
    int8_t tmcUartAddress;
    uint8_t tmcVersion;

    // Limit switch recovery
    volatile bool needsLimitRecovery;
    volatile long limitRecoveryPosition;

    // Calibration state
    CalibrationState calibrationState;
    long calibrationSpeed;
    bool calibrated;        // Position is referenced to the switches. Never true after boot.
    long maxSwitchPosition; // Position of the MAX switch, with the MIN switch at 0

    // Speed threshold for TMC mode switching (percentage)
    const float STEALTH_CHOP_THRESHOLD = 0.5;

    void referencePosition(long measuredPosition, long actualPosition);

    // Safety limits for motor configuration (based on TMC2209 capabilities)
    static constexpr long MIN_SPEED = 100;           // steps/sec
    static constexpr long MAX_SPEED = 100000;        // steps/sec (TMC2209 practical limit)
    static constexpr long MIN_ACCELERATION = 100;    // steps/sec²
    static constexpr long MAX_ACCELERATION = 500000; // steps/sec² (TMC2209 practical limit)

public:
    // Step position mirrored from update(), for limit switch ISRs to capture
    static volatile long livePosition;

    // Constructor
    MotorController();

    // Initialize motor system
    bool begin();

    // Initialize encoder (call from InputTask)
    bool initEncoder();

    // Motor control methods
    void moveTo(long position, int speed);

    // Stop methods: We have TWO distinct stop variants (no generic "stop" to avoid confusion)
    void jogStop(); // Gentle stop without emergency flag (for ending jog operations)
    void emergencyStop(); // Full emergency stop with flag (requires manual reset via clearEmergencyStop)
    void emergencyStopWithRecovery(long limitPosition); // Emergency stop with return to limit position
    void clearEmergencyStop();

    // Calibration methods
    void startCalibration();
    void stopCalibration();
    bool isCalibrating() const { return calibrationState != CAL_IDLE; }
    bool isCalibrated() const { return calibrated; }
    CalibrationState getCalibrationState() const { return calibrationState; }
    void onMinLimitReached(long position);
    void onMaxLimitReached(long position);

    // Position and status
    long getCurrentPosition() const;
    long getTargetPosition() const { return targetPosition; }
    float getCommandedSpeed() const { return stepper->speed(); }
    bool isMotorEnabled() const;
    void requestPositionShift(long steps) { pendingPositionShift += steps; }
    int8_t getTmcUartAddress() const { return tmcUartAddress; }
    uint8_t getTmcVersion() const { return tmcVersion; }
    uint32_t getMaxStepGapUs() const { return maxStepGapUs; }
    uint32_t getTmcSwitchCount() const { return tmcSwitchCount; }
    uint32_t getMaxTmcSwitchUs() const { return maxTmcSwitchUs; }
    void resetTimingStats();
    double getMonitorSpeed() const { return monitorSpeed; }
    float getMotorSpeed() const { return motorSpeed; }
    int8_t getDirection() const { return direction; }
    bool isEmergencyStopped() const { return emergencyStopActive; }
    bool isStealthChopActive() const { return useStealthChop; }
    bool isMoving() const { return !emergencyStopActive && stepper->distanceToGo() != 0; }
    bool isEmergencyStopActive() const { return emergencyStopActive; }

    // Encoder operations
    uint16_t readEncoderFrame();
    int readEncoder();
    double calculateSpeed(float ms);

    // TMC2209 operations
    void probeTMCAddress();
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