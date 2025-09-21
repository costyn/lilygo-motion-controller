# Codebase Analysis Findings

**Date:** 2025-09-21 14:55

## Current Architecture

### Project Structure
- **Platform:** PlatformIO with ESP32 (Pico32 board)
- **Single source file:** `src/main.cpp` (358 lines)
- **Build system:** Arduino framework

### Current Libraries (Already Installed)
- `AccelStepper` - Stepper motor control with acceleration
- `TMCStepper` - TMC2209 stepper driver communication
- `OneButton` - Button input handling

### Hardware Configuration (Current Implementation)
- **TMC2209 Driver Pins:**
  - EN_PIN: 2 (enable)
  - DIR_PIN: 18 (direction)
  - STEP_PIN: 23 (step)
  - SW_RX: 26, SW_TX: 27 (UART communication)
- **MT6816 Encoder Pins:**
  - SPI_CLK: 14, SPI_MISO: 12, SPI_MOSI: 13, SPI_MT_CS: 15
- **Button Pins:**
  - BTN1: 36, BTN2: 34, BTN3: 35
- **User's Required Limit Switch Pins:**
  - IO21 and IO22 (not yet implemented)

### Current FreeRTOS Implementation
- **Task1**: Button handling, encoder monitoring, speed calculation
- **Task2**: WiFi connectivity testing (connects and deletes itself)
- **Main Loop**: Continuous `stepper.runSpeed()` execution

### Existing Functionality
- ✅ TMC2209 driver configuration with SpreadCycle mode
- ✅ MT6816 encoder reading with speed calculation
- ✅ Button-based speed control (increase/decrease/stop)
- ✅ FreeRTOS multitasking structure
- ✅ WiFi connectivity (basic test implementation)
- ✅ Motor acceleration/deceleration support via AccelStepper
- ✅ Current/microstepping configuration (2000mA RMS, 16 microsteps)

### Missing Components for Requirements
- ❌ WebSocket server implementation
- ❌ State synchronization mechanisms
- ❌ Modular code organization (currently monolithic)
- ❌ Configuration persistence (SPIFFS/NVRAM)
- ❌ ElegantOTA integration
- ❌ WiFiManager integration
- ❌ Limit switch handling (IO21/IO22)
- ❌ Debug WebSocket endpoint
- ❌ Separate HTTP server paths

### Code Quality Observations
- **Pros:**
  - Working manufacturer example with comprehensive TMC2209 setup
  - Good hardware abstractions for encoder and stepper
  - FreeRTOS task structure already in place
- **Cons:**
  - Hardcoded WiFi credentials
  - No error handling or safety mechanisms
  - All functionality in single file
  - Limited configurability

### Technical Constraints Identified
- Current board: Pico32 (need to verify if this matches LilyGo T-Motor)
- Memory considerations for WebSocket + OTA + multiple tasks
- Pin conflicts need verification (limit switches vs current usage)
- Current WiFi task deletes itself after connection

### Integration Points for New Features
1. **WebSocket Integration:** Can extend Task2 or create Task3
2. **Limit Switch Safety:** Integrate with existing button handling in Task1
3. **Configuration System:** New task or integration with existing structure
4. **State Management:** Requires new shared data structures with mutexes

## Phase 3: Technical Research Findings

### ESPAsyncWebServer + WebSocket Integration
- **Library:** `ESP Async WebServer` by ESP32Async + `AsyncTCP`
- **FreeRTOS Integration:** Well-documented examples of running AsyncWebServer in dedicated FreeRTOS tasks
- **WebSocket Setup:** Uses `AsyncWebSocket ws("/ws")` with event handling
- **Core Pinning:** Recommendations to pin to Core 0 with `xTaskCreatePinnedToCore`
- **Memory Considerations:** Time-intensive operations should run in separate tasks to avoid blocking WebServer events

### ElegantOTA Integration
- **Library:** `ayushsharma82/ElegantOTA` (Version 3.X - successor to deprecated AsyncElegantOTA)
- **PlatformIO Config:** Simple lib_deps addition
- **FreeRTOS Integration:** Can run in dedicated WebServer task
- **Dependencies:** Requires ESPAsyncWebServer
- **Key Insight:** V3.X is not compatible with AsyncElegantOTA - must use newest ElegantOTA

### WiFiManager Integration
- **Library:** `tzapu/WiFiManager` or enhanced `khoih-prog/ESP_WiFiManager`
- **Captive Portal:** Automatic AP mode + DNS server (192.168.4.1)
- **Process:** Tries saved credentials → AP mode → captive portal → configuration
- **Custom Parameters:** Can collect additional parameters beyond WiFi credentials
- **Integration:** Works well with existing WiFi.begin() patterns, can replace hardcoded credentials

### Required PlatformIO Library Dependencies
```ini
lib_deps =
    AccelStepper                    # Already installed
    TMCStepper                      # Already installed
    OneButton                       # Already installed
    https://github.com/ESP32Async/ESPAsyncWebServer.git
    AsyncTCP-esphome @ ^2.0.0
    ayushsharma82/ElegantOTA @ ^3.0.0
    https://github.com/tzapu/WiFiManager.git
```

### Architecture Implications
- **Task Structure:** Need dedicated WebServer task + existing stepper control tasks
- **Memory Management:** AsyncWebServer + OTA + WebSocket requires careful memory allocation
- **Pin Conflicts:** Current limit switches (IO21/IO22) don't conflict with existing pins
- **WiFi Task Modification:** Current Task2 deletes itself after connection - needs to become persistent web server