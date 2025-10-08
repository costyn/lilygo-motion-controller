#pragma once

#include <Arduino.h>

class LimitSwitch {
private:
    uint8_t pin;
    long storedPosition;
    volatile bool triggered;
    volatile bool pending;

    // Callback function type for limit switch events
    typedef void (*LimitSwitchCallback)(long position);
    LimitSwitchCallback onLimitTriggered;

    // ISR handler (must be static for attachInterrupt)
    static void IRAM_ATTR onISR();

    // Instance tracking for ISR routing
    static LimitSwitch* instances[2];
    static uint8_t instanceCount;
    uint8_t instanceIndex;

public:
    // Constructor
    LimitSwitch(uint8_t limitPin);

    // Initialize limit switch
    bool begin();

    // Set callback for limit switch events
    void setLimitCallback(LimitSwitchCallback callback);

    // Update function (processes pending interrupts from main loop)
    void update();

    // Status getters
    bool isTriggered() const { return triggered; }
    long getStoredPosition() const { return storedPosition; }

    // Manual reset (for clearing after safe movement)
    void clearTrigger();

    // Save position when triggered
    void setStoredPosition(long pos) { storedPosition = pos; }
};

// Global instances
extern LimitSwitch minLimitSwitch;
extern LimitSwitch maxLimitSwitch;