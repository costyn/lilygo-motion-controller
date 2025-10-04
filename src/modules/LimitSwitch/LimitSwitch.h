#pragma once

#include <Arduino.h>
#include <OneButton.h>

class LimitSwitch {
private:
    OneButton* button1;
    OneButton* button2;
    volatile bool switch1Triggered;
    volatile bool switch2Triggered;

    // Callback function type for limit switch events
    typedef void (*LimitSwitchCallback)(int switchNumber, long position);
    LimitSwitchCallback onLimitTriggered;

    // Unified handler (called by static callbacks)
    void handleSwitchPressed(int switchNumber);

    // Static callback handlers (required for OneButton)
    static void onSwitch1Pressed();
    static void onSwitch2Pressed();

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

    // Allow static callbacks to access private members
    friend void onSwitch1Pressed();
    friend void onSwitch2Pressed();
};

extern LimitSwitch limitSwitch;