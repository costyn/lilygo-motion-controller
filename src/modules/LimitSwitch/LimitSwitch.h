#pragma once

#include <Arduino.h>

class LimitSwitch {
private:
    uint8_t pin1, pin2;
    volatile bool switch1Triggered;
    volatile bool switch2Triggered;
    bool lastState1, lastState2;
    unsigned long debounceDelay;
    unsigned long lastDebounceTime1, lastDebounceTime2;

    // Callback function type for limit switch events
    typedef void (*LimitSwitchCallback)(int switchNumber, long position);
    LimitSwitchCallback onLimitTriggered;

public:
    // Constructor
    LimitSwitch(uint8_t limitPin1 = 21, uint8_t limitPin2 = 22);

    // Initialize limit switches
    bool begin();

    // Set callback for limit switch events
    void setLimitCallback(LimitSwitchCallback callback);

    // Update function (call from task loop)
    void update();

    // Status getters
    bool isSwitch1Triggered() const { return switch1Triggered; }
    bool isSwitch2Triggered() const { return switch2Triggered; }
    bool isMinTriggered() const { return switch1Triggered; }  // Assume switch1 is min limit
    bool isMaxTriggered() const { return switch2Triggered; }  // Assume switch2 is max limit
    bool isAnyTriggered() const { return switch1Triggered || switch2Triggered; }

    // Manual reset (for clearing after safe movement)
    void clearTriggers();

private:
    // Debounced digital read
    bool debouncedRead(uint8_t pin, bool& lastState, unsigned long& lastDebounceTime);
};

extern LimitSwitch limitSwitch;