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

    // Step timing diagnostics (written in loop(), read from ClosedLoopTask)
    unsigned long lastUpdateMicros;
    volatile uint32_t maxStepGapUs;

    // Requests from other tasks, applied in update()
    volatile long pendingPositionShift;

    // Mirrored in update(): reading distanceToGo() from Core 0 can catch setCurrentPosition()
    // half done, which looks like a move starting
    volatile bool moving;
    volatile bool correctionRequested;
    volatile long correctionShift;
    volatile long correctionTarget;
    volatile int correctionSpeed;

    volatile uint32_t referenceCount; // Increases each time a switch re-references position

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
    bool switchesReferenced; // Calibrated from the switches, so a press gives the true position
    bool manualMinSet;       // Min set by hand since boot, waiting for Max

    void referencePosition(long measuredPosition, long actualPosition);
    void startMove(long position, int speed);

    // The chopper mode is chosen per move, never during one: handing over mid-move bumps
    // audibly, and speeds near a handover grind. Applied from ClosedLoopTask, the only
    // task using the UART after startup.
    static constexpr long QUIET_MAX_SPEED = 10000; // steps/sec, StealthChop stalled from 18,000
    volatile bool wantSpreadCycle;
    bool spreadCycleActive;

    // Share of run current kept while holding position
    static constexpr float HOLD_CURRENT_SHARE = 0.33f;
    volatile long wantMotorCurrent;
    long activeMotorCurrent;

    static constexpr long MIN_MOTOR_CURRENT = 100;  // mA RMS
    static constexpr long MAX_MOTOR_CURRENT = 1800; // mA RMS, the most the driver gives with this sense resistor

    // Enough for several turns, while keeping a jog bounded if its stop command is lost
    static constexpr long UNCALIBRATED_JOG_STEPS = 32000;

    // Safety limits for motor configuration (based on TMC2209 capabilities)
    static constexpr long MIN_SPEED = 100;           // steps/sec
    static constexpr long MAX_SPEED = 50000;         // steps/sec (48,000 ran clean on the rig, 64,000 stalled)
    static constexpr long MIN_ACCELERATION = 100;    // steps/sec²
    static constexpr long MAX_ACCELERATION = 200000; // steps/sec² (240,000 ran clean on the unloaded rig)

public:
    // Step position mirrored from update(), for limit switch ISRs to capture
    static volatile long livePosition;

    // Set from ISRs and other tasks, handled at the top of update() so it stops in microseconds
    enum StopRequest : uint8_t { STOP_NONE, STOP_MIN_SWITCH, STOP_MAX_SWITCH, STOP_STALL };
    static volatile StopRequest stopRequest;

    // Constructor
    MotorController();

    // Initialize motor system
    bool begin();

    // Initialize encoder (call from InputTask)
    bool initEncoder();

    // Motor control methods
    void moveTo(long position, int speed);

    // Hold-to-move jog at calibration speed, so limits can be set with no position known.
    // Returns an error message, or nullptr.
    const char *jogUncalibrated(bool forward, int speed);

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
    bool isManualMinSet() const { return manualMinSet; }

    // Limits set by hand at the current position. Return an error message, or nullptr.
    const char *setMinHere();
    const char *setMaxHere();
    CalibrationState getCalibrationState() const { return calibrationState; }
    void onMinLimitReached(long position);
    void onMaxLimitReached(long position);

    // Position and status
    long getCurrentPosition() const;
    long getTargetPosition() const { return targetPosition; }
    float getCommandedSpeed() const { return stepper->speed(); }
    void requestPositionShift(long steps) { pendingPositionShift += steps; }
    void requestStop() { stopRequest = STOP_STALL; }
    void requestCorrection(long shift, long target, int speed);
    uint32_t getReferenceCount() const { return referenceCount; }
    int8_t getTmcUartAddress() const { return tmcUartAddress; }
    uint8_t getTmcVersion() const { return tmcVersion; }
    uint32_t getMaxStepGapUs() const { return maxStepGapUs; }
    void resetTimingStats();
    void updateDriverSettings();
    void setMotorCurrent(long current);
    void applyFreewheelSetting();
    uint32_t readDriverStatus() { return driver->DRV_STATUS(); }
    double getMonitorSpeed() const { return monitorSpeed; }
    float getMotorSpeed() const { return motorSpeed; }
    int8_t getDirection() const { return direction; }
    bool isEmergencyStopped() const { return emergencyStopActive; }
    bool isMoving() const { return moving; }
    bool isEmergencyStopActive() const { return emergencyStopActive; }

    // Encoder operations
    uint16_t readEncoderFrame();
    int readEncoder();
    double calculateSpeed(float ms);

    // TMC2209 operations
    void probeTMCAddress();
    uint32_t getTMCStatus();

    // Main update function (call from main loop)
    void update();

    // Configuration
    void setAcceleration(long accel);
    void setMaxSpeed(long speed);
    void setCurrentPosition(long position);
};

extern MotorController motorController;