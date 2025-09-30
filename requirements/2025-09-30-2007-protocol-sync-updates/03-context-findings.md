# Context Findings - Phase 3

**Session**: 2025-09-30-2007-protocol-sync-updates
**Date**: 2025-09-30T20:15:00Z

## Codebase Analysis

### Current Implementation

#### WebSocket Broadcasting
- **Location**: `src/modules/WebServer/WebServer.cpp`
- **Methods**:
  - `broadcastStatus()` - lines 416-433 (sends full status)
  - `broadcastPosition()` - lines 435-447 (sends position only)
- **Current Usage**:
  - `broadcastStatus()` called on: WebSocket connect (line 252), explicit "status" command (line 334)
  - `broadcastPosition()` exists but is **NEVER called** anywhere in the codebase
- **Update Task**: `WebServerTask` in `main.cpp` runs every 50ms (line 183)

#### Motor State Detection
- **Location**: `src/modules/MotorController/MotorController.h`
- **Key Method**: `isMoving()` - line 57
  ```cpp
  bool isMoving() const { return stepper->distanceToGo() != 0; }
  ```
- **Other Methods**: `getCurrentPosition()`, `isEmergencyStopActive()`, etc.
- **Update Loop**: `motorController.update()` called from `main loop()` at high frequency

#### Timing Infrastructure
- **Standard**: `millis()` used throughout codebase (Arduino standard)
- **Example Usage**: `src/modules/LimitSwitch/LimitSwitch.cpp` lines 95-99 (debouncing)
- **FreeRTOS Tasks**: Use `vTaskDelay(pdMS_TO_TICKS(ms))` for task delays

### Message Types (Already Defined)

#### webapp/src/types/index.ts

**MotorStatus** (lines 3-13):
```typescript
{
  type: 'status',
  position: number,
  isMoving: boolean,
  emergencyStop: boolean,
  limitSwitches: {
    min: boolean,
    max: boolean,
    any: boolean
  }
}
```

**PositionUpdate** (lines 15-18):
```typescript
{
  type: 'position',
  position: number
}
```

### Webapp Integration

#### useMotorController Hook
- **Location**: `webapp/src/hooks/useMotorController.tsx`
- **WebSocket Handler**: Lines 121-156
  - Handles `status` type → updates `motorStatus` state (line 128)
  - Handles `position` type → updates only position field (lines 130-135)
- **Reconnection**: Auto-reconnect with max 3 attempts, 2 second delay (lines 45-46)

### Architecture Patterns

#### Module Structure
```
WebServerClass (WebServer.h/cpp)
├── AsyncWebSocket ws           // Control WebSocket at /ws
├── AsyncWebSocket debugWs      // Debug WebSocket at /debug
├── void update()               // Called from WebServerTask every 50ms
├── void broadcastStatus()      // Full status broadcast
└── void broadcastPosition()    // Position-only broadcast (unused)
```

#### FreeRTOS Task Structure
```
Core 0 (InputTask):
├── 100ms intervals
├── Button handling
├── Encoder speed calculation
└── Limit switch monitoring (commented out)

Core 1 (WebServerTask):
├── 50ms intervals  ← PERFECT FOR BROADCAST TIMING
├── webServer.update()
└── WebSocket cleanup

Main Loop (motor control):
├── High frequency (microseconds)
└── motorController.update()
```

## Implementation Strategy

### Where to Add Logic

**Primary Location**: `WebServerClass::update()` in `WebServer.cpp`
- Already called every 50ms from WebServerTask
- Has access to all WebSocket broadcast methods
- Clean separation of concerns

**Required Additions to WebServerClass** (WebServer.h):
```cpp
private:
    // Status broadcast timing (add to private section)
    unsigned long lastPositionBroadcast;
    unsigned long lastStatusBroadcast;
    bool wasMovingLastUpdate;
```

### Timing Intervals (Based on Discovery Answers)

| Broadcast Type | Interval | Condition |
|----------------|----------|-----------|
| Position Update | 100ms | While moving |
| Full Status Update | 500ms | While moving |
| Final Status | Immediate | On movement completion |
| Idle Updates | None | When not moving |

### Movement State Detection

**Movement Completion Detection**:
```cpp
bool isCurrentlyMoving = motorController.isMoving();
bool movementJustCompleted = wasMovingLastUpdate && !isCurrentlyMoving;
```

### Update Logic Flow

```
WebServerClass::update() called every 50ms
│
├─→ Check: motorController.isMoving()
│   │
│   ├─→ TRUE (Motor is moving):
│   │   ├─→ Check: (millis() - lastPositionBroadcast >= 100)
│   │   │   └─→ TRUE: broadcastPosition() + update timestamp
│   │   │
│   │   └─→ Check: (millis() - lastStatusBroadcast >= 500)
│   │       └─→ TRUE: broadcastStatus() + update timestamp
│   │
│   └─→ FALSE (Motor is idle):
│       └─→ Check: wasMovingLastUpdate == TRUE (just stopped)
│           └─→ TRUE: broadcastStatus() (final update)
│
└─→ Update: wasMovingLastUpdate = isCurrentlyMoving
```

## Files Requiring Modification

### Primary Changes

1. **src/modules/WebServer/WebServer.h**
   - Add private member variables for timing state
   - No public API changes needed

2. **src/modules/WebServer/WebServer.cpp**
   - Modify `WebServerClass` constructor to initialize timing variables
   - Enhance `update()` method with broadcast timing logic

### No Changes Needed

- **main.cpp**: WebServerTask already calls update() at 50ms
- **MotorController**: Already provides `isMoving()` and `getCurrentPosition()`
- **webapp/src/types/index.ts**: Message types already defined
- **webapp/src/hooks/useMotorController.tsx**: Already handles both message types

## Technical Considerations

### Performance Impact
- **WebSocket bandwidth**: Position updates ~20 bytes, Status updates ~80 bytes
- **CPU overhead**: Minimal - just timestamp checks and JSON serialization
- **Memory**: 3 unsigned long variables (12 bytes) + existing JSON buffer reuse

### Edge Cases Handled
1. **Movement start**: First position update sent within 100ms
2. **Movement completion**: Immediate final status ensures sync
3. **Emergency stop**: Triggers movement completion → final status sent
4. **Limit switches**: Also stop movement → final status sent
5. **WebSocket disconnect**: No broadcasts queued (textAll() handles empty client list)
6. **No clients connected**: `ws.count() == 0` → broadcasts are no-op

### Compatibility
- **Backward compatible**: Existing "status" command still works
- **No breaking changes**: Only adds automatic broadcasts
- **Webapp ready**: Already handles both message types correctly

## Related Code Patterns

### Existing Timing Pattern (LimitSwitch debouncing)
```cpp
// From LimitSwitch.cpp lines 95-99
if (currentReading != lastState) {
    lastDebounceTime = millis();
}
if ((millis() - lastDebounceTime) > debounceDelay) {
    // Process stable reading
}
```

### Existing Broadcast Pattern
```cpp
// From WebServer.cpp lines 416-433
void WebServerClass::broadcastStatus() {
    if (!initialized) return;

    JsonDocument doc;
    doc["type"] = "status";
    doc["position"] = motorController.getCurrentPosition();
    // ... fill other fields

    String message;
    serializeJson(doc, message);
    ws.textAll(message);
}
```

## Best Practices to Follow

1. **Check initialized flag**: Don't broadcast before WiFi/WebSocket ready
2. **Use existing methods**: Call `broadcastStatus()` and `broadcastPosition()` as-is
3. **Minimal state**: Only track what's necessary for timing
4. **Non-blocking**: All operations remain non-blocking
5. **FreeRTOS safe**: No shared state modification, read-only access to motorController

## Next Steps for Requirements Phase

Need to clarify with user:
1. Exact timing intervals (100ms position / 500ms status confirmed?)
2. Should emergency stop trigger immediate status broadcast before the final one?
3. Any debug logging desired for broadcast timing?
4. Network bandwidth concerns for deployment environment?
5. Should broadcast timing be configurable or hard-coded constants?
