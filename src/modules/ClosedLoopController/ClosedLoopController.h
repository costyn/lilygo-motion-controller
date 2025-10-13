#pragma once

#include <Arduino.h>

/**
 * ClosedLoopController - Closed-loop position control using MT6816 encoder feedback
 *
 * CONTROL STRATEGY: Deadband-based Proportional Controller (P-only, NOT full PID)
 * ================================================================================
 *
 * Why P-only instead of full PID?
 * - Stepper motors have no steady-state error when stopped (no need for Integral term)
 * - Deadband prevents oscillation/hunting (no need for Derivative term)
 * - Prioritizes quiet operation over aggressive tracking
 * - Sufficient for detecting/correcting missed steps during movement
 *
 * Two Separate Correction Mechanisms:
 * ------------------------------------
 * 1. ACTIVE CORRECTION (during powered movement):
 *    - Runs continuously at 50Hz (every 20ms) while motor enabled
 *    - Detects position errors within 20ms of occurrence
 *    - Applies gentle proportional correction: correction = Kp * error
 *    - Only triggers when error exceeds deadband threshold (3°)
 *    - Prevents motor buzzing by ignoring small errors
 *
 * 2. SYNC-ON-ENABLE (after freewheel):
 *    - Runs ONCE when motor transitions from disabled → enabled
 *    - Syncs AccelStepper to encoder position to accept manual movement
 *    - Prevents unexpected "snap back" jerks after user rotates shaft by hand
 *    - NOT the main correction mechanism - just a safety feature
 *
 * Features:
 * ---------
 * - Multi-turn absolute position tracking (rotation counter for Z-axis applications)
 * - Graceful degradation to open-loop if encoder fails
 * - Tracks position during freewheel mode (motor disabled)
 * - Transparent to existing WebSocket API
 */
class ClosedLoopController
{
private:
    // Multi-turn position tracking
    int32_t rotationCount;           // Number of complete 360° rotations
    uint16_t lastEncoderRaw;         // Previous raw encoder reading (0-16383)

    // Position tracking
    long encoderPositionSteps;       // Current encoder position in motor steps
    long lastCommandedPosition;      // Previous commanded position for change detection

    // Control parameters (compile-time constants per requirements)
    static constexpr float DEADBAND_THRESHOLD_DEGREES = 3.0;  // No correction within ±3°
    static constexpr float Kp = 0.5;                          // Proportional gain for corrections

    // Encoder health monitoring
    bool encoderHealthy;                    // True if encoder responding correctly
    unsigned long lastEncoderChangeTime;    // Timestamp of last encoder value change
    uint16_t lastHealthCheckRaw;            // Raw value from last health check
    unsigned long lastHealthCheckTime;      // Timestamp of last health check
    static constexpr unsigned long ENCODER_STUCK_TIMEOUT_MS = 5000;  // 5 seconds
    static constexpr unsigned long HEALTH_CHECK_RETRY_MS = 10000;    // 10 seconds between retries

    // Motor enable state tracking
    bool motorWasEnabled;
    bool softLimitActive;  // Track if motor is actively holding a soft limit boundary

    // Coordinate conversion constants
    // Motor: 200 steps/rev × 16 microsteps = 3200 steps/rev
    // Encoder: 14-bit MT6816 = 16384 counts/rev
    static constexpr float STEPS_PER_REV = 3200.0f;
    static constexpr float ENCODER_COUNTS_PER_REV = 16384.0f;
    static constexpr float STEPS_PER_ENCODER_COUNT = STEPS_PER_REV / ENCODER_COUNTS_PER_REV;

    // Helper methods
    uint16_t readEncoderRaw();
    void updateRotationCounter(uint16_t currentRaw);
    long getMultiTurnEncoderPositionSteps();
    bool checkEncoderHealth();
    float stepsToDegrees(long steps);
    long degreesToSteps(float degrees);

public:
    // Constructor
    ClosedLoopController();

    // Initialize the controller
    bool begin();

    // Main update function (called from ClosedLoopTask at 20ms intervals)
    void update();

    // Status queries (for WebSocket reporting)
    long getEncoderPositionSteps() const { return encoderPositionSteps; }
    bool isEncoderHealthy() const { return encoderHealthy; }
    float getPositionErrorSteps() const;
    float getPositionErrorDegrees() const;
    int32_t getRotationCount() const { return rotationCount; }
};

extern ClosedLoopController closedLoopController;
