#include "LimitSwitch.h"
#include "../MotorController/MotorController.h"
#include "../Configuration/Configuration.h"
#include "util.h"

// Global instance
LimitSwitch limitSwitch;

LimitSwitch::LimitSwitch(uint8_t limitPin1, uint8_t limitPin2)
{
    button1 = new OneButton(limitPin1, true); // true = INPUT_PULLUP, active LOW
    button2 = new OneButton(limitPin2, true);
    switch1Triggered = false;
    switch2Triggered = false;
    onLimitTriggered = nullptr;
}

bool LimitSwitch::begin()
{
    // Attach click handlers for limit switches
    button1->attachClick(onSwitch1Pressed);
    button2->attachClick(onSwitch2Pressed);

    LOG_INFO("Limit switches initialized on pins %d and %d", button1->pin(), button2->pin());
    return true;
}

void LimitSwitch::setLimitCallback(LimitSwitchCallback callback)
{
    onLimitTriggered = callback;
}

void LimitSwitch::update()
{
    // OneButton handles all the debouncing
    button1->tick();
    button2->tick();
}

void LimitSwitch::onSwitch1Pressed()
{
    limitSwitch.switch1Triggered = true;
    long currentPos = motorController.getCurrentPosition();

    LOG_WARN("Limit Switch 1 triggered at position: %ld", currentPos);

    // Stop motor with recovery to limit position
    motorController.emergencyStop();

    // Save limit position
    config.setLimitPos1(currentPos);
    config.saveLimitPositions(currentPos, config.getLimitPos2());

    // Broadcast status update to webapp
    extern void broadcastStatusFromLimitSwitch();
    broadcastStatusFromLimitSwitch();

    // Call callback if set
    if (limitSwitch.onLimitTriggered)
    {
        limitSwitch.onLimitTriggered(1, currentPos);
    }
}

void LimitSwitch::onSwitch2Pressed()
{
    limitSwitch.switch2Triggered = true;
    long currentPos = motorController.getCurrentPosition();

    LOG_WARN("Limit Switch 2 triggered at position: %ld", currentPos);

    // Stop motor with recovery to limit position
    motorController.emergencyStop();

    // Save limit position
    config.setLimitPos2(currentPos);
    config.saveLimitPositions(config.getLimitPos1(), currentPos);

    // Broadcast status update to webapp
    extern void broadcastStatusFromLimitSwitch();
    broadcastStatusFromLimitSwitch();

    // Call callback if set
    if (limitSwitch.onLimitTriggered)
    {
        limitSwitch.onLimitTriggered(2, currentPos);
    }
}

void LimitSwitch::clearTriggers()
{
    switch1Triggered = false;
    switch2Triggered = false;
    motorController.clearEmergencyStop(); // Also clear the emergency stop
    LOG_INFO("Limit switch triggers and emergency stop cleared");
}