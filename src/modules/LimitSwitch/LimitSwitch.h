#pragma once

#include <Arduino.h>

class LimitSwitch {
private:
    uint8_t pin1;
    uint8_t pin2;
    volatile bool switch1Triggered;
    volatile bool switch2Triggered;

    // Pending interrupt flags (set by ISR, cleared by update())
    volatile bool switch1Pending;
    volatile bool switch2Pending;

    // Callback function type for limit switch events
    typedef void (*LimitSwitchCallback)(int switchNumber, long position);
    LimitSwitchCallback onLimitTriggered;

    // Unified handler (called by static ISRs)
    void handleSwitchPressed(int switchNumber);

    // Static ISR handlers (must be static for attachInterrupt)
    static void IRAM_ATTR onSwitch1ISR();
    static void IRAM_ATTR onSwitch2ISR();

public:
    // Constructor
    LimitSwitch(uint8_t limitPin1 = 21, uint8_t limitPin2 = 22);

    // Initialize limit switches
    bool begin();

    // Set callback for limit switch events
    void setLimitCallback(LimitSwitchCallback callback);

    // Update function (processes pending interrupts from main loop)
    void update();

    // Status getters
    bool isSwitch1Triggered() const { return switch1Triggered; }
    bool isSwitch2Triggered() const { return switch2Triggered; }
    bool isMinTriggered() const { return switch1Triggered; }  // Assume switch1 is min limit
    bool isMaxTriggered() const { return switch2Triggered; }  // Assume switch2 is max limit
    bool isAnyTriggered() const { return switch1Triggered || switch2Triggered; }

    // Manual reset (for clearing after safe movement)
    void clearTriggers();

    // Allow static ISRs to access private members
    friend void IRAM_ATTR onSwitch1ISR();
    friend void IRAM_ATTR onSwitch2ISR();
};

extern LimitSwitch limitSwitch;