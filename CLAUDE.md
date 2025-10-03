# Claude Code Session Notes

## Project Overview
LilyGo Motion Controller - Modular wireless stepper motor controller for LilyGo T-Motor hardware with TMC2209 driver and MT6816 encoder.

## Implementation Status: ✅ COMPLETE + HIGH PRIORITY FEATURES ADDED

### ✅ Completed Tasks (Core System)
1. **Modular Architecture Implementation** - Clean separation into Configuration, MotorController, LimitSwitch, and WebServer modules
2. **Factory-Accurate Hardware Initialization** - TMC2209 and MT6816 setup matching manufacturer's example exactly
3. **FreeRTOS Task Structure** - InputTask (Core 0) + WebServerTask (Core 1) with proper task separation
4. **Library Conflict Resolution** - Fixed ESPAsyncWebServer compatibility by switching to maintained `ESP32Async/ESPAsyncWebServer` repository
5. **WebServer Integration** - WiFiManager + WebSocket + REST API fully functional
6. **Build Success** - Final build: RAM 16.0%, Flash 83.1%

### ✅ High Priority Features Added (September 2025)
7. **mDNS Support** - Device accessible via `http://lilygo-motioncontroller.local/` with automatic service discovery
8. **Debug Serial WebSocket Stream** - Real-time debugging via `/debug` WebSocket endpoint with browser console integration
9. **Unified Logging System** - LOG_ERROR/WARN/INFO/DEBUG macros with consistent timestamp format: `[HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message`
10. **WebSocket Command Logging** - All incoming commands logged to serial/debug stream with backward compatibility for `"cmd"` and `"goto"` fields
11. **Unit Testing Framework** - Native cross-platform tests for MotorController calculation functions (20 tests passing)

### ✅ CI/CD Pipeline (October 2025)
12. **GitHub Actions Workflow** - Automated build and release pipeline in `.github/workflows/build.yml`
   - Triggers only on version tags (e.g., `v1.0.0`)
   - Builds firmware for `pico32` environment
   - Builds SPIFFS filesystem image
   - Creates GitHub Release with versioned artifacts:
     - `lilygo-motion-controller-v1.0.0.bin` (firmware)
     - `spiffs-v1.0.0.bin` (web interface)
     - `checksums.txt` (SHA256 hashes)
   - PlatformIO package caching for fast subsequent builds (~1-3 minutes)
   - Free tier: Unlimited minutes for public repositories
   - **Usage**: `git tag -a v1.0.0 -m "Release" && git push origin v1.0.0`

### ✅ WebApp Features (October 2025)
13. **Continuous Jog Controls** - Press-and-hold buttons for smooth continuous movement
   - Backend `jogStart`/`jogStop` commands for true continuous motion
   - Ref-based state tracking to prevent spurious stops
   - Mouse and touch event support with leave detection
   - `stopGently()` method that doesn't trigger emergency flag
14. **Motor Configuration UI** - Settings dialog for motor parameters
   - Max Speed: 100-100,000 steps/sec with real-time validation
   - Acceleration: 100-500,000 steps/sec² with real-time validation
   - StealthChop mode toggle (Quiet vs Powerful)
   - Read-only limit position display
   - Changes auto-saved to ESP32 NVRAM
   - Motor-agnostic ranges accommodate different hardware
15. **Speed Slider Enhancement** - Replaced percentage input with actual speed values
   - Direct steps/sec control (100 to maxSpeed)
   - Dynamic range based on current configuration
   - Touch-friendly slider interface
16. **Backend Validation System** - Safety limits enforced on all inputs
   - Constants: MIN/MAX_SPEED (100-100,000), MIN/MAX_ACCELERATION (100-500,000)
   - Clamping behavior (not rejection) for better UX
   - Warning logs when values are adjusted
   - Protects against WebSocket, REST API, and internal calls
17. **REST API Cleanup** - Simplified to read-only monitoring
   - Removed all POST control endpoints (move, stop, reset, config)
   - Kept GET endpoints for status and config monitoring
   - WebSocket established as single control interface
   - Saved 2KB flash memory

## Key Technical Details

### Library Dependencies (platformio.ini)
```ini
lib_deps =
    AccelStepper
    TMCStepper
    OneButton
    tzapu/WiFiManager
    ESP32Async/ESPAsyncWebServer  # CRITICAL: Use maintained repository
    ayushsharma82/ElegantOTA
    bblanchon/ArduinoJson

build_flags =
    -std=c++14
    -Wno-pragmas
    -Wno-format
    -DELEGANTOTA_USE_ASYNC_WEBSERVER=1

# Native test environment added
[env:native]
platform = native
test_ignore = test_embedded
build_src_filter = -<*>  # No ESP32 source files needed for tests
build_flags = -DUNIT_TEST -DUNITY_INCLUDE_DOUBLE
```

### High Priority Features Technical Implementation

#### mDNS Configuration
```cpp
// Device naming (configurable via defines)
#define DEVICE_NAME "LilyGo-MotionController"
#define DEVICE_HOSTNAME "lilygo-motioncontroller"

// Auto-initialized after WiFi connection
mdns_hostname_set(DEVICE_HOSTNAME);
mdns_service_add(DEVICE_HOSTNAME, "_http", "_tcp", 80, NULL, 0);
```

#### Debug WebSocket Architecture
```cpp
// Separate WebSocket endpoints
AsyncWebSocket ws("/ws");        // Main control interface
AsyncWebSocket debugWs("/debug"); // Debug output stream

// Real-time debug streaming (no circular buffer to prevent flooding)
void broadcastDebugMessage(const String& message) {
    if (debugWs.count() > 0) {
        debugWs.textAll(message);
    }
}
```

#### Unified Logging System
```cpp
// Available log levels
LOG_ERROR("Critical error: %s", errorMessage);
LOG_WARN("Warning: limit switch triggered at position %ld", position);
LOG_INFO("Motor started, moving to position: %ld", targetPos);
LOG_DEBUG("TMC mode switched to %s", useStealthChop ? "StealthChop" : "SpreadCycle");

// Output format: [HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message
// Dual output: Serial console + debug WebSocket stream
```

#### WebSocket Command Compatibility
```cpp
// Supports both new and legacy command formats
{"command": "move", "position": 1000, "speed": 50}  // New format
{"cmd": "goto", "position": 1000, "speed": 50}     // Legacy format (webapp)
```

### Critical Include Order (WebServer.h)
```cpp
#include <WiFiManager.h>  // MUST be included BEFORE ESPAsyncWebServer.h
#include <ESPAsyncWebServer.h>
```

### Hardware Pin Mapping
- **TMC2209**: EN=2, STEP=23, DIR=18, UART_RX=26, UART_TX=27
- **MT6816**: CS=15, CLK=14, MISO=12, MOSI=13
- **Limits**: Switch1=21, Switch2=22
- **Debug Buttons**: BTN1=36, BTN2=34, BTN3=35
- **LED Sequence**: iStep=25, iDIR=32, iEN=33 (diagnostic only)

### Factory Code Compliance
The `factory-example.cpp` provided by user was used as reference to ensure:
- Exact TMC2209 register initialization sequence
- Proper LED initialization sequence (ledcSetup/ledcAttachPin/ledcWrite pattern)
- MT6816 SPI communication protocol
- Pin configuration and timing

### Module Architecture

```
src/modules/
├── Configuration/     # ESP32 Preferences, motor settings, limit positions
├── DebugConsole/     # Serial logging is sent over websocket to a text field for display
├── MotorController/   # TMC2209 + MT6816, factory-accurate initialization
├── LimitSwitch/      # Debounced switches, position learning
└── WebServer/        # WiFiManager, WebSocket, REST API
```

### FreeRTOS Task Design
- **main loop()**: High-frequency motor control (motorController.update())
- **InputTask**: Button handling, limit switches, encoder speed calculation (100ms)
- **WebServerTask**: WebSocket cleanup, ElegantOTA handling (50ms)

## Known Issues & Solutions

### ❌ Library Compatibility Issue (RESOLVED)
**Problem**: Original `mathieucarbou/ESPAsyncWebServer` had const-correctness conflicts with newer AsyncTCP
**Solution**: User switched to maintained `ESP32Async/ESPAsyncWebServer` repository

### ArduinoJson API Updates
- Replaced deprecated `StaticJsonDocument<N>` with `JsonDocument`
- Replaced `doc.containsKey()` with `doc["key"].is<Type>()`

### Missing Methods Added
- **Configuration**: `getMinLimit()`, `getMaxLimit()` methods
- **LimitSwitch**: `isMinTriggered()`, `isMaxTriggered()` convenience methods
- **MotorController**: `isMoving()`, `isEmergencyStopActive()` for WebServer integration

## API Specification

### REST Endpoints
- `GET /api/status` - Full system status
- `POST /api/move` - Move to position (params: position, speed)
- `POST /api/emergency-stop` - Emergency stop
- `POST /api/stop` - Stop
- `POST /api/reset` - Clear emergency stop
- `GET /api/config` - Motor configuration

### WebSocket Protocol
- Connect to `/ws`
- Commands: `{"command": "move", "position": N, "speed": N}`
- Responses: Status broadcasts, position updates

## User Feedback & Requirements

### Original User Requirements (requirements.md)
- ✅ Modular/hierarchical architecture (explicitly requested: "don't put everything in main.cpp")
- ✅ WebSocket control interface
- ✅ WiFiManager integration (no hardcoded credentials)
- ✅ Limit switch position learning
- ✅ TMC2209 mode optimization
- ✅ Real-time position feedback
- ✅ OTA update capability
- ✅ SPIFFS web app serving

### User Comments During Development
- "please dont forget my requirement for a modular/hierarchical setup"
- "How can we ensure that the way the hardware is controlled is still the way inside the manufacturers example?" → Led to factory-example.cpp compliance
- Provided working ledflower project as reference for ESPAsyncWebServer integration patterns

## Testing Status

### ✅ Compilation Testing
- Minimal build (without WebServer): SUCCESS
- Full build (with WebServer): SUCCESS
- Memory usage within acceptable limits

## Development Notes

### Code Style Maintained
- Factory hardware initialization patterns preserved exactly
- Modern C++14 features where appropriate
- Consistent error handling and serial logging
- Real-time safety considerations for motor control

### Modular Design Benefits
- Easy to extend with new modules
- Clean dependency management
- Testable components
- Hardware abstraction maintained

## Future Session Preparation

### If Hardware Testing Reveals Issues
1. **Check TMC2209 communication**: UART diagnostic outputs available
2. **Verify encoder operation**: SPI transaction logging implemented
3. **Limit switch behavior**: Debouncing and trigger logic can be adjusted
4. **WiFi connectivity**: Full diagnostic logging available

### Potential Enhancements
1. **Web UI**: Static files served from SPIFFS (framework ready)
2. **Position Profiles**: Save/recall common positions
3. **Speed Ramping**: More sophisticated acceleration profiles
4. **Multi-motor Support**: Architecture supports expansion

### Library Management
- **CRITICAL**: Always use `ESP32Async/ESPAsyncWebServer` (not deprecated mathieucarbou version)
- ArduinoJson v7+ API patterns established
- WiFiManager include order is critical for compilation

## Session Success Metrics
✅ **Architecture**: Fully modular as requested
✅ **Hardware**: Factory-accurate initialization
✅ **Compilation**: Clean build, reasonable memory usage
✅ **Features**: All MoSCoW requirements implemented
✅ **User Satisfaction**: Requirements met, library conflicts resolved

**Status**: Ready for hardware deployment 🚀