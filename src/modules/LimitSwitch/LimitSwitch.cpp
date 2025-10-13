#include "LimitSwitch.h"
#include "../MotorController/MotorController.h"
#include "../Configuration/Configuration.h"
#include "util.h"

// Global instances
LimitSwitch minLimitSwitch(21);
LimitSwitch maxLimitSwitch(22);

// Static member initialization
LimitSwitch *LimitSwitch::instances[2] = {nullptr, nullptr};
uint8_t LimitSwitch::instanceCount = 0;

LimitSwitch::LimitSwitch(uint8_t limitPin)
    : pin(limitPin), storedPosition(0), triggered(false), pending(false),
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

    // Attach hardware interrupt on FALLING edge (switch closes to ground)
    attachInterrupt(digitalPinToInterrupt(pin), onISR, FALLING);

    LOG_INFO("Limit switch initialized with interrupt on pin %d", pin);
    return true;
}

void LimitSwitch::setLimitCallback(LimitSwitchCallback callback)
{
    onLimitTriggered = callback;
}

void LimitSwitch::update()
{
    // Process any pending limit switch trigger from ISR
    // This runs in InputTask and handles non-ISR-safe operations

    if (pending)
    {
        pending = false;
        triggered = true;

        // Stop motor immediately (safe in task context)
        motorController.jogStop();

        // Get current position
        long currentPos = motorController.getCurrentPosition();
        storedPosition = currentPos;

        // Determine which limit switch this is and save position
        if (this == &minLimitSwitch)
        {
            config.setLimitPos1(currentPos);
            config.saveLimitPositions(currentPos, config.getLimitPos2());
            LOG_WARN("MIN limit switch triggered at position: %ld", currentPos);
        }
        else if (this == &maxLimitSwitch)
        {
            config.setLimitPos2(currentPos);
            config.saveLimitPositions(config.getLimitPos1(), currentPos);
            LOG_WARN("MAX limit switch triggered at position: %ld", currentPos);
        }

        // Broadcast status update to webapp (WebSocket - NOT ISR-safe)
        extern void broadcastStatusFromLimitSwitch();
        broadcastStatusFromLimitSwitch();

        // Call callback if set
        if (onLimitTriggered)
        {
            onLimitTriggered(currentPos);
        }
    }
}

// Static ISR handler (IRAM_ATTR ensures it's in RAM for fast execution)
// CRITICAL: ISR must be MINIMAL - only set flags
void IRAM_ATTR LimitSwitch::onISR()
{
    // Check which instance triggered by reading pin states
    for (uint8_t i = 0; i < instanceCount; i++)
    {
        if (instances[i] && digitalRead(instances[i]->pin) == LOW)
        {
            // Only trigger once - ignore subsequent bounces until cleared
            if (!instances[i]->pending)
            {
                instances[i]->pending = true;
            }
        }
    }
}

void LimitSwitch::clearTrigger()
{
    triggered = false;
    pending = false;
}