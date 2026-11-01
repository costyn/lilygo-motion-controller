#include "LimitSwitch.h"
#include "../MotorController/MotorController.h"
#include "util.h"
#include "soc/gpio_struct.h"

// Global instances
LimitSwitch minLimitSwitch(21);
LimitSwitch maxLimitSwitch(22);

// Static member initialization
LimitSwitch *LimitSwitch::instances[2] = {nullptr, nullptr};
uint8_t LimitSwitch::instanceCount = 0;

LimitSwitch::LimitSwitch(uint8_t limitPin)
    : pin(limitPin), storedPosition(0), triggered(false), pending(false),
      pressPosition(0), pressCaptured(false), pressHandled(false),
      onLimitTriggered(nullptr), instanceIndex(instanceCount++)
{
    // Register this instance for ISR routing
    if (instanceIndex < 2)
    {
        instances[instanceIndex] = this;
    }
}

bool LimitSwitch::begin()
{
    // Configure pin as INPUT_PULLUP (switch is active LOW)
    pinMode(pin, INPUT_PULLUP);
    pressHandled = isPressed();

    // Attach hardware interrupt on CHANGE (both press and release)
    attachInterrupt(digitalPinToInterrupt(pin), onISR, CHANGE);

    LOG_INFO("Limit switch initialized with interrupt on pin %d", pin);
    return true;
}

void LimitSwitch::setLimitCallback(LimitSwitchCallback callback)
{
    onLimitTriggered = callback;
}

void LimitSwitch::update()
{
    // Process any pending limit switch event from ISR
    // This runs in InputTask and handles non-ISR-safe operations

    if (pending)
    {
        pending = false;

        // Check if switch is currently pressed (LOW) or released (HIGH)
        bool isCurrentlyPressed = isPressed();

        if (!isCurrentlyPressed)
        {
            pressCaptured = false;
        }

        // Only act on real changes: bounce and the other switch's interrupts also land here
        if (isCurrentlyPressed == pressHandled)
        {
            return;
        }

        // Switch bounce while calibration backs off MIN: at slow speed the switch stays
        // pressed for a while, and stopping the motor here would hang calibration
        if (isCurrentlyPressed && this == &minLimitSwitch &&
            motorController.getCalibrationState() == CAL_FINDING_MAX)
        {
            return;
        }

        const char *name = this == &minLimitSwitch ? "MIN" : "MAX";

        if (isCurrentlyPressed)
        {
            // PRESS event - stop motor and report where the switch was hit
            pressHandled = true;
            triggered = true;

            // Stop motor immediately (safe in task context)
            motorController.jogStop();

            long position = pressCaptured ? pressPosition : motorController.getCurrentPosition();
            storedPosition = position;
            LOG_WARN("%s limit switch PRESSED at position: %ld", name, position);

            // Call callback if set
            if (onLimitTriggered)
            {
                onLimitTriggered(position);
            }
        }
        else
        {
            // RELEASE event - just log it
            pressHandled = false;
            LOG_INFO("%s limit switch RELEASED", name);
        }

        // Broadcast status update to webapp on both press and release
        extern void broadcastStatusFromLimitSwitch();
        broadcastStatusFromLimitSwitch();
    }
}

// Static ISR handler (IRAM_ATTR ensures it's in RAM for fast execution)
// CRITICAL: ISR must be MINIMAL - only set flags
void IRAM_ATTR LimitSwitch::onISR()
{
    long position = MotorController::livePosition;

    // Both switches share this handler, so check each one
    for (uint8_t i = 0; i < instanceCount; i++)
    {
        LimitSwitch *instance = instances[i];
        if (instance)
        {
            // Set pending flag to process in update() (handles both press and release)
            instance->pending = true;

            // Switches are active LOW. Keep only the first edge of a bouncing press.
            if (!instance->pressCaptured && ((GPIO.in >> instance->pin) & 1) == 0)
            {
                instance->pressPosition = position;
                instance->pressCaptured = true;

                if (!instance->pressHandled)
                {
                    MotorController::stopRequest = instance == &minLimitSwitch
                                                       ? MotorController::STOP_MIN_SWITCH
                                                       : MotorController::STOP_MAX_SWITCH;
                }
            }
        }
    }
}

void LimitSwitch::clearTrigger()
{
    triggered = false;
    pending = false;
    pressHandled = isPressed();
    pressCaptured = false;
}