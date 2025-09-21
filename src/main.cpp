#include <Arduino.h>
#include <OneButton.h>

// Import our modules
#include "modules/Configuration/Configuration.h"
#include "modules/MotorController/MotorController.h"
#include "modules/LimitSwitch/LimitSwitch.h"
#include "modules/WebServer/WebServer.h"

/*
 * LilyGo Motion Controller
 *
 * A modular wireless stepper motor controller for LilyGo T-Motor hardware
 * with TMC2209 driver and MT6816 encoder.
 *
 * Features:
 * - WebSocket-based control interface
 * - Smart limit switch handling with position learning
 * - Automatic TMC2209 mode optimization (StealthChop/SpreadCycle)
 * - Configuration persistence via ESP32 Preferences
 * - Over-the-air firmware updates
 * - Real-time position feedback via encoder
 *
 * Hardware:
 * - LilyGo T-Motor with ESP32 Pico
 * - TMC2209 stepper driver
 * - MT6816 magnetic encoder (16384 pulses/rotation)
 * - Stepper: 17HS19-2004S1 (1.8°, 59Ncm, 2.0A/phase)
 * - Limit switches on IO21 and IO22
 */

// Button definitions (for debugging/testing)
#define BTN1 36
#define BTN2 34
#define BTN3 35

OneButton button1(BTN1, true);
OneButton button2(BTN2, true);
OneButton button3(BTN3, true);

// Task handles
TaskHandle_t inputTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;

// Task function declarations
void InputTask(void *pvParameters);
void WebServerTask(void *pvParameters);

// Button callback functions (for debugging)
void onButton1Click();
void onButton2Click();
void onButton3Click();

void setup() {
    Serial.begin(115200);
    Serial.println("========================================");
    Serial.println("LilyGo Motion Controller Starting...");
    Serial.println("========================================");

    // Initialize all modules in order
    Serial.println("Initializing modules...");

    // 1. Configuration first (needed by other modules)
    if (!config.begin()) {
        Serial.println("FATAL: Failed to initialize Configuration module");
        while(1) delay(1000);
    }

    // 2. Motor controller
    if (!motorController.begin()) {
        Serial.println("FATAL: Failed to initialize Motor Controller");
        while(1) delay(1000);
    }

    // 3. Limit switches
    if (!limitSwitch.begin()) {
        Serial.println("FATAL: Failed to initialize Limit Switches");
        while(1) delay(1000);
    }

    // 4. Web server
    if (!webServer.begin()) {
        Serial.println("FATAL: Failed to initialize Web Server");
        while(1) delay(1000);
    }

    Serial.println("All modules initialized successfully");

    // Create FreeRTOS tasks
    Serial.println("Creating FreeRTOS tasks...");

    xTaskCreatePinnedToCore(
        InputTask,           // Task function
        "InputTask",         // Task name
        8192,               // Stack size
        NULL,               // Parameters
        2,                  // Priority
        &inputTaskHandle,   // Task handle
        0                   // Core (0 for input monitoring)
    );

    xTaskCreatePinnedToCore(
        WebServerTask,       // Task function
        "WebServerTask",     // Task name
        16384,              // Stack size (larger for web operations)
        NULL,               // Parameters
        1,                  // Priority
        &webServerTaskHandle, // Task handle
        1                   // Core (1 for web server)
    );

    Serial.println("FreeRTOS tasks created");
    Serial.println("========================================");
    Serial.println("System ready!");
    Serial.println("========================================");
}

void loop() {
    // Main loop handles motor control (time-critical)
    motorController.update();

    // Small delay to prevent watchdog issues
    delayMicroseconds(100);
}

void InputTask(void *pvParameters) {
    Serial.println("Input Task started");

    // Initialize LED sequence exactly like factory code
    motorController.initLEDSequence();

    // Initialize buttons for debugging
    button1.attachClick(onButton1Click);
    button2.attachClick(onButton2Click);
    button3.attachClick(onButton3Click);

    // Initialize encoder after LED sequence (like factory code)
    motorController.initEncoder();

    // Task main loop
    while (1) {
        // Update button states
        button1.tick();
        button2.tick();
        button3.tick();

        // Update limit switches
        limitSwitch.update();

        // Calculate speed from encoder (like factory code)
        motorController.calculateSpeed(100);

        // 100ms update rate for input monitoring
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void WebServerTask(void *pvParameters) {
    Serial.println("Web Server Task started");

    // Task main loop
    while (1) {
        // Update web server (handles WebSocket, WiFi reconnection, etc.)
        webServer.update();

        // 50ms update rate for web operations
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

// Button callback functions (for debugging/testing)
void onButton1Click() {
    Serial.println("Button 1 pressed - Move forward");
    if (!limitSwitch.isAnyTriggered()) {
        long currentPos = motorController.getCurrentPosition();
        motorController.moveTo(currentPos + 100, 30); // Move 100 steps forward at 30% speed
    }
}

void onButton2Click() {
    Serial.println("Button 2 pressed - Emergency stop");
    motorController.emergencyStop();
}

void onButton3Click() {
    Serial.println("Button 3 pressed - Move backward");
    if (!limitSwitch.isAnyTriggered()) {
        long currentPos = motorController.getCurrentPosition();
        motorController.moveTo(currentPos - 100, 30); // Move 100 steps backward at 30% speed
    }
}