#include "LimitSwitch.h"
#include "../MotorController/MotorController.h"
#include "../Configuration/Configuration.h"

// Global instance
LimitSwitch limitSwitch;

LimitSwitch::LimitSwitch(uint8_t limitPin1, uint8_t limitPin2) {
    pin1 = limitPin1;
    pin2 = limitPin2;
    switch1Triggered = false;
    switch2Triggered = false;
    lastState1 = false;
    lastState2 = false;
    debounceDelay = 50; // 50ms debounce
    lastDebounceTime1 = 0;
    lastDebounceTime2 = 0;
    onLimitTriggered = nullptr;
}

bool LimitSwitch::begin() {
    pinMode(pin1, INPUT_PULLUP);
    pinMode(pin2, INPUT_PULLUP);

    Serial.printf("Limit switches initialized on pins %d and %d\n", pin1, pin2);
    return true;
}

void LimitSwitch::setLimitCallback(LimitSwitchCallback callback) {
    onLimitTriggered = callback;
}

void LimitSwitch::update() {
    // Read limit switch states with debouncing (active low)
    bool limit1Active = !debouncedRead(pin1, lastState1, lastDebounceTime1);
    bool limit2Active = !debouncedRead(pin2, lastState2, lastDebounceTime2);

    // Handle limit switch 1
    if (limit1Active && !switch1Triggered) {
        switch1Triggered = true;
        long currentPos = motorController.getCurrentPosition();

        Serial.printf("Limit Switch 1 triggered at position: %ld\n", currentPos);

        // Stop motor immediately
        motorController.emergencyStop();

        // Save limit position
        config.setLimitPos1(currentPos);
        config.saveLimitPositions(currentPos, config.getLimitPos2());

        // Call callback if set
        if (onLimitTriggered) {
            onLimitTriggered(1, currentPos);
        }
    } else if (!limit1Active && switch1Triggered) {
        // Switch released - don't clear trigger automatically
        // This requires manual clearTriggers() call
    }

    // Handle limit switch 2
    if (limit2Active && !switch2Triggered) {
        switch2Triggered = true;
        long currentPos = motorController.getCurrentPosition();

        Serial.printf("Limit Switch 2 triggered at position: %ld\n", currentPos);

        // Stop motor immediately
        motorController.emergencyStop();

        // Save limit position
        config.setLimitPos2(currentPos);
        config.saveLimitPositions(config.getLimitPos1(), currentPos);

        // Call callback if set
        if (onLimitTriggered) {
            onLimitTriggered(2, currentPos);
        }
    } else if (!limit2Active && switch2Triggered) {
        // Switch released - don't clear trigger automatically
    }
}

void LimitSwitch::clearTriggers() {
    switch1Triggered = false;
    switch2Triggered = false;
    Serial.println("Limit switch triggers cleared");
}

bool LimitSwitch::debouncedRead(uint8_t pin, bool& lastState, unsigned long& lastDebounceTime) {
    bool currentReading = digitalRead(pin);

    if (currentReading != lastState) {
        lastDebounceTime = millis();
    }

    bool stableReading = lastState;
    if ((millis() - lastDebounceTime) > debounceDelay) {
        stableReading = currentReading;
    }

    lastState = currentReading;
    return stableReading;
}