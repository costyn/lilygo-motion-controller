#include "ClosedLoopController.h"
#include "../MotorController/MotorController.h"
#include "../Configuration/Configuration.h"
#include "../../util.h"

// Global instance
ClosedLoopController closedLoopController;

ClosedLoopController::ClosedLoopController()
    : rotationCount(0),
      lastEncoderRaw(0),
      encoderPositionSteps(0),
      lastCommandedPosition(0),
      encoderHealthy(true),
      lastEncoderChangeTime(0),
      lastHealthCheckRaw(0),
      lastHealthCheckTime(0),
      motorWasEnabled(false),
      softLimitActive(false)
{
}

bool ClosedLoopController::begin()
{
    LOG_INFO("Initializing ClosedLoopController...");

    // Read initial encoder position
    uint16_t initialRaw = readEncoderRaw();
    if (initialRaw == 0 || initialRaw == 0xFFFF)
    {
        LOG_WARN("Encoder may not be connected (read 0x%04X) - will operate in open-loop mode", initialRaw);
        encoderHealthy = false;
    }
    else
    {
        LOG_INFO("Initial encoder reading: %u (0x%04X)", initialRaw, initialRaw);
        encoderHealthy = true;
    }

    lastEncoderRaw = initialRaw;
    lastHealthCheckRaw = initialRaw;
    lastEncoderChangeTime = millis();
    lastHealthCheckTime = millis();

    // Initialize rotation counter to 0 (requires homing after boot)
    rotationCount = 0;

    LOG_INFO("ClosedLoopController initialized successfully");
    LOG_INFO("Control mode: %s", encoderHealthy ? "closed-loop" : "open-loop (encoder fault)");
    LOG_INFO("Deadband threshold: %.1f degrees", DEADBAND_THRESHOLD_DEGREES);
    LOG_INFO("Proportional gain (Kp): %.2f", Kp);

    return true;
}

/**
 * Read raw encoder value (0-16383)
 * Calls MotorController::readEncoder() which handles MT6816 SPI communication
 */
uint16_t ClosedLoopController::readEncoderRaw()
{
    int rawValue = motorController.readEncoder();
    return (uint16_t)(rawValue & 0x3FFF); // 14-bit mask (0-16383)
}

/**
 * Update rotation counter based on encoder wrap-around detection
 * Detects transitions across 0/16384 boundary
 */
void ClosedLoopController::updateRotationCounter(uint16_t currentRaw)
{
    int16_t delta = currentRaw - lastEncoderRaw;

    // Detect wrap-around using half-scale threshold (8192)
    if (delta > 8192)
    {
        // Large positive delta means we wrapped backward (16384 → 0)
        rotationCount--;
        LOG_DEBUG("Rotation counter decremented: %ld (encoder wrapped backward: %u → %u)",
                  rotationCount, lastEncoderRaw, currentRaw);
    }
    else if (delta < -8192)
    {
        // Large negative delta means we wrapped forward (0 → 16384)
        rotationCount++;
        LOG_DEBUG("Rotation counter incremented: %ld (encoder wrapped forward: %u → %u)",
                  rotationCount, lastEncoderRaw, currentRaw);
    }

    lastEncoderRaw = currentRaw;
}

/**
 * Calculate absolute multi-turn position in motor steps
 * Combines rotation counter with raw encoder value
 */
long ClosedLoopController::getMultiTurnEncoderPositionSteps()
{
    // Calculate total encoder counts including full rotations
    int64_t totalEncoderCounts = (int64_t)rotationCount * (int64_t)ENCODER_COUNTS_PER_REV + (int64_t)lastEncoderRaw;

    // Convert to motor steps
    long steps = (long)(totalEncoderCounts * STEPS_PER_ENCODER_COUNT);

    return steps;
}

/**
 * Check encoder health by detecting communication errors and stuck values
 */
bool ClosedLoopController::checkEncoderHealth()
{
    uint16_t raw = readEncoderRaw();

    // Check for SPI errors (all 0s or all 1s indicates communication failure)
    if (raw == 0 || raw == 0xFFFF)
    {
        LOG_WARN("Encoder SPI error detected (read 0x%04X)", raw);
        return false;
    }

    // Check for stuck encoder (no movement while motor moving)
    bool motorMoving = motorController.isMoving();
    if (motorMoving)
    {
        if (raw == lastHealthCheckRaw)
        {
            if (millis() - lastHealthCheckTime > ENCODER_STUCK_TIMEOUT_MS)
            {
                LOG_WARN("Encoder stuck - no change in %lu ms while motor moving (value: %u)",
                         ENCODER_STUCK_TIMEOUT_MS, raw);
                return false;
            }
        }
        else
        {
            // Encoder changed, update tracking
            lastHealthCheckRaw = raw;
            lastHealthCheckTime = millis();
        }
    }
    else
    {
        // Motor not moving, reset stuck detection
        lastHealthCheckRaw = raw;
        lastHealthCheckTime = millis();
    }

    return true;
}

/**
 * Convert steps to degrees for deadband comparison
 */
float ClosedLoopController::stepsToDegrees(long steps)
{
    return (abs(steps) / STEPS_PER_REV) * 360.0f;
}

/**
 * Convert degrees to steps for deadband threshold
 */
long ClosedLoopController::degreesToSteps(float degrees)
{
    return (long)((degrees / 360.0f) * STEPS_PER_REV);
}

/**
 * Get current position error in steps (commanded - encoder)
 */
float ClosedLoopController::getPositionErrorSteps() const
{
    long commandedPosition = motorController.getCurrentPosition();
    return (float)(commandedPosition - encoderPositionSteps);
}

/**
 * Get current position error in degrees
 */
float ClosedLoopController::getPositionErrorDegrees() const
{
    long commandedPosition = motorController.getCurrentPosition();
    long errorSteps = commandedPosition - encoderPositionSteps;
    return (abs(errorSteps) / STEPS_PER_REV) * 360.0f;
}

/**
 * Main update function - Called from ClosedLoopTask at 20ms intervals (50Hz)
 *
 * CONTROL STRATEGY EXPLAINED:
 * ===========================
 * This is a DEADBAND-BASED P-CONTROLLER, not full PID. Here's why:
 *
 * 1. P-only (Proportional) is sufficient because:
 *    - Stepper motors don't drift (no steady-state error when stopped)
 *    - We only need to correct missed steps during active movement
 *    - Integral term would fight the deadband (unnecessary corrections)
 *
 * 2. Deadband prevents motor buzzing:
 *    - Small errors (<3°) are ignored completely
 *    - Motor only corrects when error is significant
 *    - Prioritizes quiet operation over perfect tracking
 *
 * 3. Active correction happens DURING movement:
 *    - This function runs 50 times/second (every 20ms)
 *    - If encoder shows missed steps, correction applied within 20ms
 *    - NOT just on motor enable - that's a separate safety mechanism
 *
 * Implementation Steps:
 * 1. Read encoder and update rotation tracking
 * 2. Check encoder health (graceful degradation)
 * 3. Handle motor enable transitions (sync AccelStepper to encoder)
 * 4. Apply position corrections if error exceeds deadband threshold
 */
void ClosedLoopController::update()
{
    // ============================================================================
    // STEP 1: Read encoder and update multi-turn position tracking
    // ============================================================================

    uint16_t encoderRaw = readEncoderRaw();
    updateRotationCounter(encoderRaw);
    encoderPositionSteps = getMultiTurnEncoderPositionSteps();

    // ============================================================================
    // STEP 2: Check encoder health and handle graceful degradation
    // ============================================================================

    bool currentHealth = checkEncoderHealth();

    // Detect health state changes
    if (currentHealth != encoderHealthy)
    {
        encoderHealthy = currentHealth;
        if (encoderHealthy)
        {
            LOG_INFO("Encoder recovered - switching to closed-loop mode");
        }
        else
        {
            LOG_ERROR("Encoder fault detected - switching to open-loop mode");
            LOG_ERROR("System will continue operating without position feedback");
        }
    }

    // If encoder unhealthy, retry periodically but don't process corrections
    if (!encoderHealthy)
    {
        // Retry health check every 10 seconds
        if (millis() - lastHealthCheckTime > HEALTH_CHECK_RETRY_MS)
        {
            lastHealthCheckTime = millis();
            // checkEncoderHealth() will be called on next update cycle
        }
        return; // Don't apply corrections in open-loop mode
    }

    // ============================================================================
    // STEP 3: Handle motor enable/disable transitions (SYNC-ON-ENABLE mechanism)
    // ============================================================================
    // This is NOT the main correction loop - this is a one-time safety sync
    // that prevents jerks after manual movement during freewheel mode.
    //
    // Scenario:
    // 1. Motor completes movement and freewheels (EN_PIN HIGH)
    // 2. User manually rotates shaft by hand
    // 3. Motor re-enables for next movement
    // 4. Without sync: AccelStepper tries to return to old position (JERK!)
    // 5. With sync: AccelStepper accepts new encoder position as reality (SMOOTH!)

    bool motorEnabled = motorController.isMotorEnabled();

    // Detect motor becoming enabled (transition from freewheel to powered)
    // BUT: Only sync if there's no active movement command (distanceToGo == 0)
    // This prevents overwriting a freshly-set target position
    if (motorEnabled && !motorWasEnabled && motorController.getDistanceToGo() == 0)
    {
        // Sync AccelStepper position to actual encoder position
        // This prevents unexpected "snap back" movements after freewheel
        motorController.setCurrentPosition(encoderPositionSteps);
        LOG_INFO("Motor enabled - AccelStepper synced to encoder position: %ld steps", encoderPositionSteps);
    }

    motorWasEnabled = motorEnabled;

    // ============================================================================
    // STEP 4: Apply deadband-based position correction (ACTIVE CORRECTION mechanism)
    // ============================================================================
    // This is the MAIN closed-loop correction that runs continuously during movement.
    // It detects and corrects missed steps in real-time (within 20ms).
    //
    // How it works:
    // 1. Compare encoder position (actual) vs AccelStepper position (commanded)
    // 2. If error > deadband (3°), apply proportional correction
    // 3. Correction = Kp * error (P-only control, no I or D terms needed)
    // 4. Update AccelStepper target to gradually fix the error
    //
    // Why this works without causing jerks:
    // - Kp = 0.5 means gentle corrections (50% of error per cycle)
    // - Deadband prevents buzzing on small errors
    // - AccelStepper handles acceleration/deceleration smoothly
    // - Motor is already moving, so small target adjustments are imperceptible

    // Soft limits work in RAW encoder coordinates (not negated)
    // This avoids confusion with coordinate system conversions
    long minLimit = -config.getMaxLimit(); // Negate: user maxLimit → motor minLimit
    long maxLimit = -config.getMinLimit(); // Negate: user minLimit → motor maxLimit

    // Soft limit boundary offset: move to this many steps inside the limit, then release
    // This prevents the motor from constantly fighting at exactly the boundary
    static constexpr long SOFT_LIMIT_INSET = 200; // 200 steps inside the limit

    // Check if encoder is outside configured limits (using raw encoder coordinates)
    bool outsideLimits = (encoderPositionSteps < minLimit || encoderPositionSteps > maxLimit);

    if (!motorEnabled)
    {
        // ============================================================================
        // SOFT LIMITS: Enforce position boundaries during freewheel
        // ============================================================================
        // When motor is freewheeling, user can manually rotate the shaft.
        // If shaft is rotated beyond configured limits, engage motor to push back
        // slightly inside the boundary, then release.

        if (outsideLimits)
        {
            if (encoderPositionSteps < minLimit)
            {
                // Encoder went below minimum - push back inside lower boundary
                long recoveryPosition = minLimit + SOFT_LIMIT_INSET;
                LOG_INFO("Soft limit: encoder below min (raw: %ld < %ld), pushing to recovery position %ld",
                         encoderPositionSteps, minLimit, recoveryPosition);
                motorController.moveTo(recoveryPosition, config.getMaxSpeed());
                softLimitActive = true;
            }
            else if (encoderPositionSteps > maxLimit)
            {
                // Encoder went above maximum - push back inside upper boundary
                long recoveryPosition = maxLimit - SOFT_LIMIT_INSET;
                LOG_INFO("Soft limit: encoder above max (raw: %ld > %ld), pushing to recovery position %ld",
                         encoderPositionSteps, maxLimit, recoveryPosition);
                motorController.moveTo(recoveryPosition, config.getMaxSpeed());
                softLimitActive = true;
            }
        }

        // Otherwise: Motor freewheeling within limits - just track position, no corrections
        return;
    }

    // ============================================================================
    // SOFT LIMIT ACTIVE: Wait for recovery movement to complete, then release
    // ============================================================================
    if (softLimitActive)
    {
        // Check if motor has completed the recovery movement AND encoder is back within limits
        bool motorStopped = !motorController.isMoving();
        bool backWithinLimits = (encoderPositionSteps >= minLimit && encoderPositionSteps <= maxLimit);

        if (motorStopped && backWithinLimits)
        {
            // Movement complete AND encoder confirmed within limits - safe to release
            LOG_INFO("Soft limit: recovery complete at position %ld (within limits), releasing", currentEncoderPosition);
            softLimitActive = false;

            // If freewheeling is configured, disable motor now
            if (config.getFreewheelAfterMove())
            {
                // Disable motor by setting EN_PIN HIGH directly (bypass normal freewheel logic)
                pinMode(2, OUTPUT); // EN_PIN = 2
                digitalWrite(2, HIGH);
                LOG_INFO("Soft limit: freewheeling re-enabled");
            }
        }
        else if (motorStopped && !backWithinLimits)
        {
            // Motor stopped but encoder STILL outside limits - this shouldn't happen normally
            // Log error and re-trigger recovery attempt
            LOG_WARN("Soft limit: motor stopped but encoder still outside limits (%ld), retriggering recovery",
                     currentEncoderPosition);
            softLimitActive = false; // Reset flag so next cycle will re-trigger
        }
        // Otherwise: Still moving to recovery position, let it continue
        return;
    }

    // ============================================================================
    // ACTIVE CORRECTION DISABLED
    // ============================================================================
    // The active correction loop has been temporarily disabled because:
    // 1. It was using getCurrentPosition() instead of getTargetPosition()
    // 2. This caused it to continuously chase the encoder with corrections
    // 3. The motor would never reach its target, just continuously adjust
    //
    // TODO: Implement proper closed-loop control that:
    // - Monitors encoder position vs commanded steps (not vs target)
    // - Detects missed steps by comparing step count to encoder change
    // - Adds corrective steps without changing the target position
    // - Uses AccelStepper::setCurrentPosition() to adjust for errors
    //
    // For now, the motor runs in open-loop mode with sync-on-enable only.
    // ============================================================================
}
