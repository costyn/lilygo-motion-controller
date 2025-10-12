#include <Arduino.h>

// Import our modules
#include "util.h"
#include "modules/Configuration/Configuration.h"
#include "modules/MotorController/MotorController.h"
#include "modules/LimitSwitch/LimitSwitch.h"
#include "modules/ButtonController/ButtonController.h"
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

// Task handles
TaskHandle_t inputTaskHandle = NULL;
TaskHandle_t webServerTaskHandle = NULL;

// Task function declarations
void InputTask(void *pvParameters);
void WebServerTask(void *pvParameters);

void setup()
{
    Serial.begin(115200);
    delay(1000); // Allow serial to initialize
    LOG_INFO("========================================");
    LOG_INFO("LilyGo Motion Controller Starting...");
    LOG_INFO("========================================");

    // Initialize all modules in order
    LOG_INFO("Initializing modules...");

    // 1. Configuration first (needed by other modules)
    if (!config.begin())
    {
        LOG_ERROR("FATAL: Failed to initialize Configuration module");
        while (1)
            delay(1000);
    }

    // 2. Motor controller
    if (!motorController.begin())
    {
        LOG_ERROR("FATAL: Failed to initialize Motor Controller");
        while (1)
            delay(1000);
    }

    // 3. Limit switches
    if (!minLimitSwitch.begin() || !maxLimitSwitch.begin())
    {
        LOG_ERROR("FATAL: Failed to initialize Limit Switches");
        while (1)
            delay(1000);
    }

    // 4. Button controller
    if (!buttonController.begin())
    {
        LOG_ERROR("FATAL: Failed to initialize Button Controller");
        while (1)
            delay(1000);
    }

    // 5. Web server
    if (!webServer.begin())
    {
        LOG_WARN("WARN: Failed to initialize Web Server. Only PCB buttons work!");
    }

    LOG_INFO("All modules initialized successfully");

    // Create FreeRTOS tasks
    LOG_INFO("Creating FreeRTOS tasks...");

    xTaskCreatePinnedToCore(
        InputTask,        // Task function
        "InputTask",      // Task name
        8192,             // Stack size
        NULL,             // Parameters
        2,                // Priority
        &inputTaskHandle, // Task handle
        0                 // Core (0 for input monitoring)
    );

    xTaskCreatePinnedToCore(
        WebServerTask,        // Task function
        "WebServerTask",      // Task name
        16384,                // Stack size (larger for web operations)
        NULL,                 // Parameters
        1,                    // Priority
        &webServerTaskHandle, // Task handle
        1                     // Core (1 for web server)
    );

    LOG_INFO("FreeRTOS tasks created");
    LOG_INFO("========================================");
    LOG_INFO("System ready!");
    LOG_INFO("========================================");
}

void loop()
{
    // Main loop handles motor control (time-critical)
    motorController.update();

    // No delay - AccelStepper needs maximum call frequency for high speeds
    // Watchdog is automatically fed by FreeRTOS idle task
}

void InputTask(void *pvParameters)
{
    LOG_INFO("Input Task started");

    // Initialize encoder
    motorController.initEncoder();

    // Task main loop
    while (1)
    {
        // Update button controller
        buttonController.update();

        // Update limit switches
        minLimitSwitch.update();
        maxLimitSwitch.update();

        // Calculate speed from encoder
        motorController.calculateSpeed(100);

        // 100ms update rate for input monitoring
        vTaskDelay(pdMS_TO_TICKS(100));
    }
}

void WebServerTask(void *pvParameters)
{
    LOG_INFO("Web Server Task started");

    // Task main loop
    while (1)
    {
        // Update web server (handles WebSocket, WiFi reconnection, etc.)
        webServer.update();

        // 50ms update rate for web operations
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}