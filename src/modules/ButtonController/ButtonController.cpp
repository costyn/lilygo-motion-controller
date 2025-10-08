#include "ButtonController.h"
#include "../MotorController/MotorController.h"
#include "../LimitSwitch/LimitSwitch.h"
#include "../Configuration/Configuration.h"
#include "util.h"

// Global instance
ButtonController buttonController;

ButtonController::ButtonController(uint8_t btn1Pin, uint8_t btn2Pin, uint8_t btn3Pin)
{
    button1 = new OneButton(btn1Pin, true); // true = INPUT_PULLUP, active LOW
    button2 = new OneButton(btn2Pin, true);
    button3 = new OneButton(btn3Pin, true);
}

bool ButtonController::begin()
{
    LOG_INFO("Initializing Button Controller...");

    // Button 1: Jog backward (press and hold)
    button1->attachLongPressStart(onButton1Press);
    button1->attachLongPressStop(onButton1Release);
    button1->setPressMs(100); // Start jogging after 100ms

    // Button 2: Emergency stop (click)
    button2->attachClick(onButton2Click);

    // Button 3: Jog forward (press and hold)
    button3->attachLongPressStart(onButton3Press);
    button3->attachLongPressStop(onButton3Release);
    button3->setPressMs(100); // Start jogging after 100ms

    LOG_INFO("Button Controller initialized (pins: %d, %d, %d)",
             button1->pin(), button2->pin(), button3->pin());
    return true;
}

void ButtonController::update()
{
    // Update button states (OneButton handles debouncing)
    button1->tick();
    button2->tick();
    button3->tick();
}

// Static callback implementations
void ButtonController::onButton1Press()
{
    // Button 1: Jog backward (to min limit)
    LOG_INFO("Button 1 press - Jog backward");
    if (!motorController.isEmergencyStopActive())
    {
        int jogSpeed = config.getMaxSpeed() * 0.3; // 30% of max speed
        long targetPosition = config.getMinLimit();
        motorController.moveTo(targetPosition, jogSpeed);
        LOG_INFO("Jog backward started to %ld at speed %d", targetPosition, jogSpeed);
    }
}

void ButtonController::onButton1Release()
{
    LOG_INFO("Button 1 release - Stop jog");
    motorController.jogStop();
}

void ButtonController::onButton2Click()
{
    LOG_INFO("Button 2 pressed - Emergency stop");
    motorController.emergencyStop();
}

void ButtonController::onButton3Press()
{
    // Button 3: Jog forward (to max limit)
    LOG_INFO("Button 3 press - Jog forward");
    if (!motorController.isEmergencyStopActive())
    {
        int jogSpeed = config.getMaxSpeed() * 0.3; // 30% of max speed
        long targetPosition = config.getMaxLimit();
        motorController.moveTo(targetPosition, jogSpeed);
        LOG_INFO("Jog forward started to %ld at speed %d", targetPosition, jogSpeed);
    }
}

void ButtonController::onButton3Release()
{
    LOG_INFO("Button 3 release - Stop jog");
    motorController.jogStop();
}
