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

// Unified handler for both limit switches (DRY principle)
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

    // Stop motor with emergency stop
    motorController.emergencyStop();

    // Save limit position
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

    // Broadcast status update to webapp
    extern void broadcastStatusFromLimitSwitch();
    broadcastStatusFromLimitSwitch();

    // Call callback if set
    if (onLimitTriggered)
    {
        onLimitTriggered(switchNumber, currentPos);
    }
}

// Static callbacks (required by OneButton library)
void LimitSwitch::onSwitch1Pressed()
{
    limitSwitch.handleSwitchPressed(1);
}

void LimitSwitch::onSwitch2Pressed()
{
    limitSwitch.handleSwitchPressed(2);
}

void LimitSwitch::clearTriggers()
{
    switch1Triggered = false;
    switch2Triggered = false;
    motorController.clearEmergencyStop(); // Also clear the emergency stop
    LOG_INFO("Limit switch triggers and emergency stop cleared");
}