#pragma once

#include <Arduino.h>
#include <OneButton.h>

class ButtonController
{
private:
    OneButton *button1;
    OneButton *button2;
    OneButton *button3;

    // Button callback functions
    static void onButton1Press();
    static void onButton1Release();
    static void onButton2Click();
    static void onButton3Press();
    static void onButton3Release();

public:
    // Constructor
    ButtonController(uint8_t btn1Pin = 36, uint8_t btn2Pin = 34, uint8_t btn3Pin = 35);

    // Initialize button controller
    bool begin();

    // Update function (call from InputTask loop)
    void update();

    // Allow static callbacks to access private members
    friend void onButton1Press();
    friend void onButton1Release();
    friend void onButton2Click();
    friend void onButton3Press();
    friend void onButton3Release();
};

extern ButtonController buttonController;
