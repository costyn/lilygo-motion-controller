# Context Findings

## Existing Code Architecture Analysis

### Current WebSocket Implementation
- **Location**: `src/modules/WebServer/WebServer.cpp` and `WebServer.h`
- **Pattern**: AsyncWebSocket at `/ws` path with JSON command handling
- **Integration**: Already broadcasts status and position updates to all connected clients
- **Extension Point**: Can easily add new WebSocket path for debug stream

### Serial Output Patterns
- **Current Usage**: 6 files use Serial.print/printf for debugging
- **Timestamp Function**: `timeToString()` already implemented in `util.cpp/util.h`
- **Format**: Returns "HH:MM:SS.mmm" format (24-hour wrap)
- **Example Pattern**: `Serial.printf("%s: %s: message\n", timeToString().c_str(), functionName, data)`

### mDNS Implementation Context
- **ESP32 Native Support**: Built-in ESP-IDF mDNS library available
- **User Provided Code**: Working implementation from another project:
```cpp
void start_mdns_service() {
    esp_err_t err = mdns_init();
    mdns_hostname_set("Lumifera");
    mdns_instance_name_set("Lumifera");
    mdns_service_add("Lumifera", "_http", "_tcp", 80, NULL, 0);
}
```
- **Integration Point**: WebServer::begin() after WiFi connection established

### Unit Testing Infrastructure
- **Existing Directory**: `/test/` directory already exists
- **PlatformIO Support**: Native cross-platform testing framework available
- **Target Functions**: `MotorController::calculateSpeed()` and `updateTMCMode()` identified in TODO
- **Strategy**: Dual environment setup (native for dev, esp32 for validation)

## Specific Files Requiring Modification

### 1. Debug WebSocket Stream (`/debug` endpoint)
**Files to modify:**
- `src/modules/WebServer/WebServer.h` - Add debug WebSocket member and handlers
- `src/modules/WebServer/WebServer.cpp` - Implement debug stream functionality
- `src/util.h` - Add serial redirection utilities
- `src/util.cpp` - Implement serial capture for WebSocket streaming

### 2. mDNS Support
**Files to modify:**
- `src/modules/WebServer/WebServer.cpp` - Add mDNS initialization in begin()
- `platformio.ini` - No changes needed (ESP-IDF includes mDNS)

### 3. Unified Serial Output
**Files to standardize:**
- `src/modules/Configuration/Configuration.cpp`
- `src/modules/MotorController/MotorController.cpp`
- `src/modules/LimitSwitch/LimitSwitch.cpp`
- `src/modules/WebServer/WebServer.cpp`
- `src/main.cpp`
- `src/util.h` - Add standardized logging macros

### 4. WebSocket Command Serial Logging
**Files to modify:**
- `src/modules/WebServer/WebServer.cpp` - Add logging to handleWebSocketMessage()

### 5. Unit Tests
**Files to create:**
- `platformio.ini` - Add native test environment
- `test/test_motor_controller/test_motor_calculations.cpp`
- `test/test_desktop/` directory structure
- `test/test_embedded/` directory structure

## Implementation Patterns Found

### WebSocket Broadcasting Pattern
```cpp
void WebServerClass::broadcastStatus() {
    JsonDocument doc;
    doc["type"] = "status";
    // ... populate data
    String jsonString;
    serializeJson(doc, jsonString);
    ws.textAll(jsonString);
}
```

### Serial Logging Pattern (Current)
```cpp
constexpr const char *SGN = "ClassName::methodName()";
Serial.printf("%s: %s: message\n", timeToString().c_str(), SGN, data);
```

### JSON Command Handling Pattern
```cpp
void WebServerClass::handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
    JsonDocument doc;
    deserializeJson(doc, (char*)data);
    String command = doc["command"];
    if (command == "move") {
        // handle command
    }
}
```

## Library Dependencies Analysis

### Current Dependencies (No additions needed)
- `ESP32Async/ESPAsyncWebServer` - Already supports multiple WebSocket paths
- `bblanchon/ArduinoJson` - JSON serialization for debug messages
- ESP-IDF - Built-in mDNS support
- PlatformIO Unity - Built-in unit testing framework

### Architecture Benefits
- **Modular Design**: Each feature can be implemented independently
- **Existing Patterns**: WebSocket, JSON, and serial logging patterns already established
- **No Breaking Changes**: All additions are extensions to existing functionality
- **Memory Efficient**: Debug features can be conditionally compiled

## Technical Constraints Identified

### WebSocket Debug Stream
- **Memory Usage**: Need to buffer serial output for WebSocket transmission
- **Performance**: Avoid blocking main loop with debug stream processing
- **Connection Management**: Handle multiple debug clients efficiently

### mDNS Integration
- **WiFi Dependency**: Must initialize after successful WiFi connection
- **Hostname Conflicts**: Need unique hostname (suggest "lilygo-motioncontroller")
- **Service Discovery**: Register HTTP and possibly WebSocket services

### Unit Testing
- **Code Isolation**: Need to extract calculation functions for testability
- **Mock Dependencies**: Serial, hardware interfaces need mocking for native tests
- **Build Configuration**: Separate test builds from production firmware