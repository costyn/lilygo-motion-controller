#include "LimitSwitch.h"
#include "../MotorController/MotorController.h"
#include "../Configuration/Configuration.h"
#include "util.h"

// Global instance
LimitSwitch limitSwitch;

LimitSwitch::LimitSwitch(uint8_t limitPin1, uint8_t limitPin2)
    : pin1(limitPin1), pin2(limitPin2)
{
    switch1Triggered = false;
    switch2Triggered = false;
    switch1Pending = false;
    switch2Pending = false;
    onLimitTriggered = nullptr;
}

bool LimitSwitch::begin()
{
    // Configure pins as INPUT_PULLUP (switches are active LOW)
    pinMode(pin1, INPUT_PULLUP);
    pinMode(pin2, INPUT_PULLUP);

    // Attach hardware interrupts on FALLING edge (switch closes to ground)
    attachInterrupt(digitalPinToInterrupt(pin1), onSwitch1ISR, FALLING);
    attachInterrupt(digitalPinToInterrupt(pin2), onSwitch2ISR, FALLING);

    LOG_INFO("Limit switches initialized with interrupts on pins %d and %d", pin1, pin2);
    return true;
}

void LimitSwitch::setLimitCallback(LimitSwitchCallback callback)
{
    onLimitTriggered = callback;
}

void LimitSwitch::update()
{
    // Process any pending limit switch triggers from ISR
    // This runs in InputTask and handles non-ISR-safe operations

    if (switch1Pending)
    {
        switch1Pending = false;
        // Stop motor immediately (safe in task context)
        motorController.emergencyStop();
        handleSwitchPressed(1);
    }

    if (switch2Pending)
    {
        switch2Pending = false;
        // Stop motor immediately (safe in task context)
        motorController.emergencyStop();
        handleSwitchPressed(2);
    }
}

// Unified handler for both limit switches (called from update(), NOT ISR)
void LimitSwitch::handleSwitchPressed(int switchNumber)
{
    long currentPos = motorController.getCurrentPosition();

    LOG_WARN("Limit Switch %d triggered at position: %ld", switchNumber, currentPos);

    // Set appropriate trigger flag
    if (switchNumber == 1)
    {
        switch1Triggered = true;
    }
    else
    {
        switch2Triggered = true;
    }

    // Save limit position (NVRAM write - NOT ISR-safe)
    if (switchNumber == 1)
    {
        config.setLimitPos1(currentPos);
        config.saveLimitPositions(currentPos, config.getLimitPos2());
    }
    else
    {
        config.setLimitPos2(currentPos);
        config.saveLimitPositions(config.getLimitPos1(), currentPos);
    }

    // Broadcast status update to webapp (WebSocket - NOT ISR-safe)
    extern void broadcastStatusFromLimitSwitch();
    broadcastStatusFromLimitSwitch();

    // Call callback if set
    if (onLimitTriggered)
    {
        onLimitTriggered(switchNumber, currentPos);
    }
}

// Static ISR handlers (IRAM_ATTR ensures they're in RAM for fast execution)
// CRITICAL: ISRs must be MINIMAL - only set flags
// Motor stop happens in update() to avoid ISR conflicts
void IRAM_ATTR LimitSwitch::onSwitch1ISR()
{
    // Only trigger once - ignore subsequent bounces until cleared
    if (!limitSwitch.switch1Pending)
    {
        limitSwitch.switch1Pending = true;
    }
}

void IRAM_ATTR LimitSwitch::onSwitch2ISR()
{
    // Only trigger once - ignore subsequent bounces until cleared
    if (!limitSwitch.switch2Pending)
    {
        limitSwitch.switch2Pending = true;
    }
}

void LimitSwitch::clearTriggers()
{
    switch1Triggered = false;
    switch2Triggered = false;
    switch1Pending = false;
    switch2Pending = false;
    motorController.clearEmergencyStop(); // Also clear the emergency stop
    LOG_INFO("Limit switch triggers and emergency stop cleared");
}