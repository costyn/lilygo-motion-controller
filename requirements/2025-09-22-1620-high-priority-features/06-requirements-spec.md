# High Priority Features - Requirements Specification

## Problem Statement

The LilyGo Motion Controller project has completed core functionality but needs key debugging and usability features before hardware deployment. The high priority features will enhance development workflow, improve debugging capabilities, and provide better user experience for network access.

## Solution Overview

Implement 5 high-priority features that extend the existing modular architecture without breaking changes:

1. **mDNS Network Discovery** - Easy device access via hostname
2. **Debug Serial WebSocket Stream** - Remote debugging capability
3. **WebSocket Command Logging** - Development troubleshooting
4. **Unified Logging System** - Consistent debug output with levels
5. **Unit Testing Framework** - Validate calculation functions

## Functional Requirements

### FR1: mDNS Support
- **Objective**: Enable device access via `lilygo-motioncontroller.local`
- **Behavior**:
  - Initialize mDNS service after successful WiFi connection
  - Register HTTP service on port 80
  - Use configurable hostname via #define for project customization
- **Success Criteria**: Device accessible via browser at hostname.local

### FR2: Debug Serial WebSocket Stream
- **Objective**: Provide `/debug` WebSocket endpoint for serial output streaming
- **Behavior**:
  - Separate WebSocket path (`/debug`) from main control WebSocket (`/ws`)
  - Stream all existing Serial.print/printf output to connected debug clients
  - Implement circular buffer (50-100 lines) with recent messages sent on client connect
  - Fallback to simple streaming if circular buffer proves too complex
  - Automatic connection for any debug client (no authentication)
- **Success Criteria**: Web client can connect to `/debug` and receive real-time serial output

### FR3: WebSocket Command Serial Logging
- **Objective**: Log incoming WebSocket commands to serial output for development debugging
- **Behavior**:
  - Log all received WebSocket commands to serial before processing
  - Use unified logging format with timestamps
  - Include command content and client identification
- **Success Criteria**: Serial monitor shows incoming WebSocket commands during operation

### FR4: Unified Logging System
- **Objective**: Standardize all serial output with consistent format and log levels
- **Behavior**:
  - Create logging macros: LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG
  - Format: `[TIMESTAMP] [LEVEL] [MODULE]: message`
  - Maintain existing `timeToString()` function for timestamp generation
  - Convert all existing Serial.printf calls to use new system
- **Success Criteria**: All modules use consistent logging format with appropriate levels

### FR5: Unit Testing Framework
- **Objective**: Test MotorController calculation functions (`calculateSpeed()`, `updateTMCMode()`)
- **Behavior**:
  - Native environment testing only (local dev machine)
  - Focus on calculation function validation
  - PlatformIO Unity framework integration
  - Test organization in separate directories (test_native/, test_embedded/)
- **Success Criteria**: `pio test -e native` successfully validates calculation functions

## Technical Requirements

### TR1: Memory Constraints
- Debug circular buffer: Maximum 100 lines × ~80 chars = ~8KB RAM impact
- Log level filtering to reduce production overhead
- Conditional compilation for debug features

### TR2: Performance Requirements
- Debug WebSocket streaming must not block main motor control loop
- mDNS initialization must not delay WiFi connection establishment
- Logging system overhead < 1ms per log call

### TR3: Compatibility Requirements
- Maintain existing WebSocket API at `/ws` unchanged
- Preserve existing WiFiManager configuration portal behavior
- No changes to existing motor control timing or accuracy

## Implementation Details

### File Modifications Required

#### src/modules/WebServer/WebServer.h
```cpp
// Add debug WebSocket support
AsyncWebSocket debugWs;
CircularBuffer<String> debugBuffer; // or simple streaming fallback
void setupDebugWebSocket();
void broadcastDebugMessage(const String& message);
```

#### src/modules/WebServer/WebServer.cpp
```cpp
// Add mDNS initialization in begin() after WiFi connect
// Add debug WebSocket setup and handlers
// Add WebSocket command logging
```

#### src/util.h
```cpp
// Device naming
#define DEVICE_NAME "LilyGo-MotionController"
#define DEVICE_HOSTNAME "lilygo-motioncontroller"

// Logging system
typedef enum {
    LOG_LEVEL_ERROR = 0,
    LOG_LEVEL_WARN = 1,
    LOG_LEVEL_INFO = 2,
    LOG_LEVEL_DEBUG = 3
} log_level_t;

#define LOG_ERROR(fmt, ...) logPrint(LOG_LEVEL_ERROR, __func__, fmt, ##__VA_ARGS__)
#define LOG_WARN(fmt, ...)  logPrint(LOG_LEVEL_WARN, __func__, fmt, ##__VA_ARGS__)
#define LOG_INFO(fmt, ...)  logPrint(LOG_LEVEL_INFO, __func__, fmt, ##__VA_ARGS__)
#define LOG_DEBUG(fmt, ...) logPrint(LOG_LEVEL_DEBUG, __func__, fmt, ##__VA_ARGS__)

void logPrint(log_level_t level, const char* function, const char* format, ...);
```

#### platformio.ini
```ini
# Add native test environment
[env:native]
platform = native
test_ignore = test_embedded
build_src_filter = +<*> -<main.cpp>
```

#### test/ Directory Structure
```
test/
├── test_native/
│   └── test_motor_calculations/
│       └── test_motor_controller.cpp
└── test_embedded/
    └── (reserved for future hardware tests)
```

### Integration Points

#### mDNS Integration
- **Location**: `WebServerClass::begin()` after `wm.autoConnect()` success
- **Dependencies**: ESP-IDF mDNS library (already available)
- **Pattern**: Follow user-provided mDNS code example

#### Debug WebSocket Integration
- **Location**: `WebServerClass::setupWebSocket()` add second WebSocket instance
- **Pattern**: Mirror existing `/ws` WebSocket setup for `/debug`
- **Buffer**: Circular buffer managed in WebServer class, populated by logging system

#### Logging System Integration
- **Approach**: Global function `logPrint()` that outputs to Serial AND debug WebSocket
- **Migration**: Replace existing `Serial.printf()` calls module by module
- **Conditional**: Use `#ifdef DEBUG` to disable debug level logging in production

## Acceptance Criteria

### AC1: mDNS Functionality
- [ ] Device accessible at `http://lilygo-motioncontroller.local`
- [ ] Service discovery shows HTTP service on port 80
- [ ] Works alongside existing WiFiManager portal

### AC2: Debug WebSocket Stream
- [ ] WebSocket client can connect to `/debug` path
- [ ] Receives real-time serial output messages
- [ ] On connection, receives recent message history (if circular buffer implemented)
- [ ] Multiple clients can connect simultaneously

### AC3: WebSocket Command Logging
- [ ] All incoming WebSocket commands logged to serial
- [ ] Commands logged before processing with timestamp and client info
- [ ] Visible in debug WebSocket stream

### AC4: Unified Logging
- [ ] All modules use LOG_* macros instead of Serial.printf
- [ ] Consistent timestamp format: `[HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message`
- [ ] Log levels configurable via #define
- [ ] Debug WebSocket receives all log output

### AC5: Unit Testing
- [ ] `pio test -e native` runs successfully
- [ ] Tests validate `MotorController::calculateSpeed()` output ranges
- [ ] Tests validate `MotorController::updateTMCMode()` logic
- [ ] No ESP32 hardware required for test execution

## Assumptions

### Technical Assumptions
- ESP-IDF mDNS library sufficient for requirements (no additional dependencies)
- Existing AsyncWebSocket library supports multiple WebSocket instances
- Current memory usage allows for 8KB debug buffer overhead
- ArduinoJson library handles debug message serialization efficiently

### Usage Assumptions
- Debug features primarily used during development, not production
- Network administrators allow mDNS traffic on target networks
- Developers have PlatformIO installed for unit testing
- Debug WebSocket clients will be modern browsers supporting WebSocket API

### Scope Assumptions
- No authentication/security required for debug WebSocket (development use)
- Existing motor control real-time requirements unchanged
- No breaking changes to public API or WebSocket protocol
- Unit tests focus on pure calculation functions, not hardware integration

## Future Considerations

### Potential Enhancements
- **Authentication**: Add optional debug WebSocket authentication for production use
- **Log Filtering**: Web UI for real-time log level adjustment
- **Metrics**: Integrate debug stream with system performance metrics
- **Mobile**: Debug WebSocket client mobile app

### Scalability
- **Multiple Controllers**: mDNS service discovery for multi-device setups
- **Log Aggregation**: Central logging server for multiple devices
- **Remote Testing**: Unit test execution on CI/CD pipeline

---

**Requirements Status**: Complete ✅
**Implementation Ready**: Yes
**Estimated Effort**: 2-3 days development + testing
**Risk Level**: Low (extensions to existing stable architecture)