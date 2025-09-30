# Requirements Specification: Protocol Sync Updates

**Session ID**: 2025-09-30-2007-protocol-sync-updates
**Date**: 2025-09-30T20:25:00Z
**Status**: Ready for Implementation

---

## Problem Statement

The current WebSocket protocol between the ESP32 controller and webapp only sends status updates:
1. When a client first connects
2. When explicitly requested via "status" command

This creates a synchronization problem during motor movement:
- Webapp has no real-time position feedback during movement
- No way to detect if motor stalls or has issues
- Poor user experience with static/stale position display
- No confirmation when movement completes

**Impact**: Users cannot see smooth progress updates and may be uncertain about motor state.

---

## Solution Overview

Implement automatic, periodic WebSocket broadcasts during motor movement to maintain continuous state synchronization between controller and webapp.

**Key Features**:
- Dual-frequency updates (fast position, slower full status)
- Intelligent broadcast timing (only during movement)
- Immediate feedback on state changes
- Heartbeat mechanism to detect connection/motor issues
- Zero breaking changes to existing API

---

## Functional Requirements

### FR-1: Periodic Position Updates During Movement
**Priority**: MUST HAVE

When motor is moving (`motorController.isMoving() == true`):
- Broadcast position-only updates every 100ms
- Use existing `PositionUpdate` message type: `{"type": "position", "position": N}`
- Provides smooth UI updates (10 updates/second)

**Acceptance Criteria**:
- Position updates sent at 100ms ±10ms intervals while moving
- No position updates sent when motor is idle
- Updates contain current encoder position

---

### FR-2: Periodic Status Updates During Movement
**Priority**: MUST HAVE

When motor is moving:
- Broadcast full status updates every 500ms
- Use existing `MotorStatus` message type with all fields
- Provides comprehensive state sync (2 updates/second)

**Acceptance Criteria**:
- Status updates sent at 500ms ±20ms intervals while moving
- No status updates sent when motor is idle
- Status includes: position, isMoving, emergencyStop, limitSwitches

---

### FR-3: Immediate Update on Movement Start
**Priority**: MUST HAVE

When `moveTo()` command is received and processed:
- Immediately broadcast full status showing `isMoving: true`
- Do not wait for first scheduled interval

**Acceptance Criteria**:
- Status broadcast occurs within 50ms of movement start
- Webapp receives instant feedback that motor started

---

### FR-4: Immediate Update on Movement Completion
**Priority**: MUST HAVE

When motor stops moving (reaches target, limit switch, or emergency stop):
- Immediately broadcast full status with final position and `isMoving: false`
- Ensures webapp has exact final state

**Acceptance Criteria**:
- Final status broadcast within 50ms of movement completion
- Includes accurate final position
- Shows correct stopped state

---

### FR-5: Immediate Update on Emergency Stop
**Priority**: MUST HAVE

When emergency stop is triggered (WebSocket command or button):
- Immediately broadcast full status showing `emergencyStop: true`
- In addition to final status when motor actually stops

**Acceptance Criteria**:
- Emergency stop state broadcast within 50ms of trigger
- Webapp can instantly display emergency stop indicator

---

### FR-6: No Updates During Idle
**Priority**: MUST HAVE

When motor is not moving:
- No periodic broadcasts
- Only respond to explicit "status" commands
- Reduces bandwidth and CPU usage

**Acceptance Criteria**:
- Zero periodic broadcasts when `isMoving() == false`
- Manual "status" command still works

---

### FR-7: Debug Logging
**Priority**: SHOULD HAVE

Add LOG_DEBUG messages for broadcast events:
- Position broadcasts
- Status broadcasts
- Movement start/completion events

**Acceptance Criteria**:
- Logs visible at LOG_DEBUG level
- Can be compiled out for production
- Available via serial and `/debug` WebSocket

---

## Technical Requirements

### TR-1: File Modifications

#### src/modules/WebServer/WebServer.h
**Location**: Private member section (after line 35)

Add state tracking variables:
```cpp
// Broadcast timing state (add to private section)
unsigned long lastPositionBroadcast;
unsigned long lastStatusBroadcast;
bool wasMovingLastUpdate;
```

Add timing constants (before class definition):
```cpp
// Broadcast timing intervals (milliseconds)
#define POSITION_BROADCAST_INTERVAL_MS 100
#define STATUS_BROADCAST_INTERVAL_MS 500
```

**No public API changes needed** - all existing methods remain unchanged.

---

#### src/modules/WebServer/WebServer.cpp

**Location 1**: Constructor initialization (around line 57)
```cpp
WebServerClass::WebServerClass() : server(80), ws("/ws"), debugWs("/debug"), initialized(false)
{
    // Add initialization for new members
    lastPositionBroadcast = 0;
    lastStatusBroadcast = 0;
    wasMovingLastUpdate = false;
}
```

**Location 2**: Enhanced `update()` method (replace lines 486-497)

Current code:
```cpp
void WebServerClass::update()
{
    if (!initialized)
        return;

    // Handle ElegantOTA
    ElegantOTA.loop();

    // Cleanup disconnected WebSocket clients
    ws.cleanupClients();
    debugWs.cleanupClients();
}
```

Replace with:
```cpp
void WebServerClass::update()
{
    if (!initialized)
        return;

    // Handle ElegantOTA
    ElegantOTA.loop();

    // Cleanup disconnected WebSocket clients
    ws.cleanupClients();
    debugWs.cleanupClients();

    // Automatic status broadcasting during movement
    bool isCurrentlyMoving = motorController.isMoving();
    unsigned long currentMillis = millis();

    if (isCurrentlyMoving)
    {
        // Send position updates every 100ms during movement
        if (currentMillis - lastPositionBroadcast >= POSITION_BROADCAST_INTERVAL_MS)
        {
            LOG_DEBUG("Broadcasting position update: %ld", motorController.getCurrentPosition());
            broadcastPosition(motorController.getCurrentPosition());
            lastPositionBroadcast = currentMillis;
        }

        // Send full status every 500ms during movement
        if (currentMillis - lastStatusBroadcast >= STATUS_BROADCAST_INTERVAL_MS)
        {
            LOG_DEBUG("Broadcasting full status (movement active)");
            broadcastStatus();
            lastStatusBroadcast = currentMillis;
        }

        // Update movement tracking for next iteration
        wasMovingLastUpdate = true;
    }
    else if (wasMovingLastUpdate)
    {
        // Movement just completed - send final status
        LOG_INFO("Movement completed - sending final status");
        broadcastStatus();
        wasMovingLastUpdate = false;
    }
}
```

**Location 3**: Immediate broadcast on movement start (line 316, inside move command handler)

Current code (line 314-322):
```cpp
if (!limitSwitch.isAnyTriggered())
{
    motorController.moveTo(position, speed);
}
else
{
    ws.textAll("{\"error\":\"limit switch triggered\"}");
}
```

Add after `motorController.moveTo()`:
```cpp
if (!limitSwitch.isAnyTriggered())
{
    motorController.moveTo(position, speed);

    // Immediate status broadcast on movement start
    LOG_INFO("Movement started to position %ld - broadcasting initial status", position);
    broadcastStatus();
    lastPositionBroadcast = millis();
    lastStatusBroadcast = millis();
}
else
{
    ws.textAll("{\"error\":\"limit switch triggered\"}");
}
```

**Location 4**: Immediate broadcast on emergency stop (line 326, inside stop command handler)

Current code:
```cpp
else if (command == "stop")
{
    motorController.emergencyStop();
}
```

Add broadcast after emergency stop:
```cpp
else if (command == "stop")
{
    motorController.emergencyStop();

    // Immediate broadcast of emergency stop state
    LOG_WARN("Emergency stop triggered - broadcasting status");
    broadcastStatus();
}
```

---

### TR-2: Existing Code Reuse

**No modifications needed** for:
- `broadcastStatus()` method (lines 416-433) - use as-is
- `broadcastPosition()` method (lines 435-447) - use as-is
- `motorController.isMoving()` - already available
- `motorController.getCurrentPosition()` - already available
- WebSocket client management - already handled

---

### TR-3: Timing Architecture

**Execution Context**: WebServerTask (FreeRTOS Core 1)
- Called every 50ms from `main.cpp` line 183
- Allows precise timing checks (±10ms accuracy)
- Non-blocking operations only

**Timing Logic**:
```
50ms task cycle
├─→ Check if moving: YES
│   ├─→ 100ms elapsed since last position? → broadcastPosition()
│   └─→ 500ms elapsed since last status? → broadcastStatus()
│
└─→ Check if just stopped: YES
    └─→ broadcastStatus() (final update)
```

---

### TR-4: Performance Considerations

**Memory Impact**:
- 3 × unsigned long (12 bytes)
- 1 × bool (1 byte)
- **Total**: 13 bytes additional RAM usage

**CPU Impact**:
- 2-3 timestamp comparisons per 50ms cycle
- JSON serialization only when broadcasting
- **Negligible** - < 1ms per update cycle

**Network Bandwidth** (worst case, continuous movement):
- Position updates: 10/sec × ~20 bytes = 200 bytes/sec
- Status updates: 2/sec × ~80 bytes = 160 bytes/sec
- **Total**: ~360 bytes/sec = 2.8 kbps (minimal)

**WebSocket Overhead**: textAll() is no-op when no clients connected

---

## Implementation Patterns to Follow

### Pattern 1: Timing with millis()
Follow existing pattern from `LimitSwitch.cpp` lines 95-99:
```cpp
unsigned long currentMillis = millis();
if (currentMillis - lastTimestamp >= INTERVAL_MS) {
    // Do action
    lastTimestamp = currentMillis;
}
```

### Pattern 2: State Change Detection
```cpp
bool isCurrentState = getState();
bool stateChanged = wasInState && !isCurrentState;
if (stateChanged) {
    // Handle transition
}
wasInState = isCurrentState;
```

### Pattern 3: Logging
Follow existing LOG_* macro patterns:
```cpp
LOG_DEBUG("Variable value: %ld", longValue);
LOG_INFO("Action completed: %s", description);
LOG_WARN("Warning condition detected");
```

### Pattern 4: Early Returns
Follow existing pattern from `update()` and `broadcastStatus()`:
```cpp
if (!initialized) return;
```

---

## Edge Cases and Error Handling

### Edge Case 1: Movement Shorter Than Broadcast Interval
**Scenario**: Motor moves for only 50ms (shorter than 100ms position interval)

**Handling**:
- Immediate broadcast on movement start: ✓ Covered
- Final broadcast on completion: ✓ Covered
- Result: Webapp gets at least 2 updates (start + end)

---

### Edge Case 2: millis() Overflow
**Scenario**: millis() wraps around to 0 after ~49 days uptime

**Handling**:
- Subtraction `(currentMillis - lastMillis)` handles overflow correctly
- Arduino standard pattern - no special handling needed
- Worst case: One missed broadcast interval after 49 days

---

### Edge Case 3: No WebSocket Clients
**Scenario**: Broadcasting when no clients connected

**Handling**:
- `ws.textAll()` checks `ws.count() == 0` internally
- JSON serialization still occurs (minimal overhead)
- No network transmission
- Could optimize with `if (ws.count() > 0)` check before serialization

---

### Edge Case 4: Emergency Stop While Moving
**Scenario**: Emergency stop triggered during movement

**Handling**:
1. Immediate emergency stop broadcast (TR-2, Location 4)
2. Motor stops (motorController.emergencyStop())
3. Next update() cycle detects movement completion
4. Final status broadcast sent
5. Result: 2 broadcasts ensure state sync

---

### Edge Case 5: Limit Switch Triggered During Movement
**Scenario**: Limit switch stops motor mid-movement

**Handling**:
- Motor stops automatically (motorController handles this)
- `isMoving()` returns false
- Next update() cycle detects completion
- Final status broadcast includes limit switch state
- Result: Webapp correctly shows why motor stopped

---

### Edge Case 6: Multiple Rapid move() Commands
**Scenario**: New move() command while already moving

**Handling**:
- Each move() call broadcasts immediate status (FR-3)
- Continuous movement prevents "just stopped" condition
- Periodic updates continue throughout
- Result: Webapp stays synchronized with latest target

---

## Testing Strategy

### Unit Testing (Native Environment)
Not applicable - requires ESP32 hardware and WebSocket infrastructure.

### Integration Testing (Hardware)
**Test Scenario 1**: Long movement (> 1 second)
```
1. Send move command for 2000 steps at 50% speed
2. Monitor WebSocket messages
3. Verify:
   - Immediate status on start
   - Position updates every ~100ms
   - Status updates every ~500ms
   - Final status on completion
```

**Test Scenario 2**: Short movement (< 100ms)
```
1. Send move command for 50 steps at 100% speed
2. Monitor WebSocket messages
3. Verify:
   - At least 2 broadcasts (start + end)
   - Final position is accurate
```

**Test Scenario 3**: Emergency stop during movement
```
1. Start long movement
2. Send emergency stop command
3. Verify:
   - Immediate emergency stop status broadcast
   - Final status when motor stops
   - emergencyStop: true in both
```

**Test Scenario 4**: Idle state
```
1. Wait 5 seconds with motor idle
2. Monitor WebSocket messages
3. Verify: Zero broadcasts during idle period
```

**Test Scenario 5**: Manual status request while moving
```
1. Start movement
2. Send "status" command
3. Verify: Immediate response + periodic updates continue
```

### Debug Verification
Use debug WebSocket at `ws://lilygo-motioncontroller.local/debug`:
```javascript
const debugWs = new WebSocket('ws://lilygo-motioncontroller.local/debug');
debugWs.onmessage = (e) => console.log('Debug:', e.data);
```

Expected log output during movement:
```
[INFO] Movement started to position 2000 - broadcasting initial status
[DEBUG] Broadcasting position update: 245
[DEBUG] Broadcasting position update: 512
[DEBUG] Broadcasting full status (movement active)
[DEBUG] Broadcasting position update: 798
[INFO] Movement completed - sending final status
```

---

## Acceptance Criteria

### AC-1: Position Updates During Movement
- [ ] Position broadcasts sent every 100ms ±10ms while moving
- [ ] No position broadcasts when idle
- [ ] Position values match `motorController.getCurrentPosition()`

### AC-2: Status Updates During Movement
- [ ] Status broadcasts sent every 500ms ±20ms while moving
- [ ] No status broadcasts when idle
- [ ] Status includes all required fields

### AC-3: Movement Start
- [ ] Immediate status broadcast within 50ms of move command
- [ ] isMoving field is true
- [ ] Webapp shows movement started instantly

### AC-4: Movement Completion
- [ ] Final status broadcast within 50ms of motor stopping
- [ ] isMoving field is false
- [ ] Position matches final motor position

### AC-5: Emergency Stop
- [ ] Immediate status broadcast when emergency stop triggered
- [ ] emergencyStop field is true
- [ ] Final status sent when motor actually stops

### AC-6: Idle Behavior
- [ ] Zero periodic broadcasts when motor idle
- [ ] Manual "status" command still works
- [ ] Existing API functionality unchanged

### AC-7: Debug Logging
- [ ] LOG_DEBUG messages for position/status broadcasts
- [ ] LOG_INFO for movement start/completion
- [ ] Logs visible via serial and /debug WebSocket

### AC-8: Backward Compatibility
- [ ] Existing WebSocket commands work unchanged
- [ ] REST API endpoints unaffected
- [ ] Webapp with old code still functions (just missing new updates)

---

## Assumptions

1. **Hardware Availability**: Controller available for testing (user confirmed "lying on desk")
2. **No Motor/Switches Connected**: User noted not connected, so `isMoving()` simulation via commands is sufficient
3. **Network Environment**: Local WiFi network allows WebSocket connections
4. **Browser Testing**: Modern browser with WebSocket and DevTools available
5. **Serial Access**: User can paste serial output if needed (direct serial connection not working)
6. **Webapp Update**: Webapp already handles both message types (no webapp changes needed)

---

## Dependencies

**Existing Code** (no changes needed):
- `MotorController::isMoving()` - src/modules/MotorController/MotorController.h:57
- `MotorController::getCurrentPosition()` - src/modules/MotorController/MotorController.h:50
- `WebServerClass::broadcastStatus()` - src/modules/WebServer/WebServer.cpp:416-433
- `WebServerClass::broadcastPosition()` - src/modules/WebServer/WebServer.cpp:435-447
- WebServerTask timing - src/main.cpp:183

**Webapp** (already compatible):
- `MotorStatus` type - webapp/src/types/index.ts:3-13
- `PositionUpdate` type - webapp/src/types/index.ts:15-18
- Message handler - webapp/src/hooks/useMotorController.tsx:126-135

**External Libraries**:
- ArduinoJson (already used)
- ESPAsyncWebServer (already used)
- FreeRTOS (already used)

---

## Implementation Checklist

### Step 1: Add Defines and Members to WebServer.h
- [ ] Add `#define POSITION_BROADCAST_INTERVAL_MS 100` before class
- [ ] Add `#define STATUS_BROADCAST_INTERVAL_MS 500` before class
- [ ] Add `unsigned long lastPositionBroadcast;` to private section
- [ ] Add `unsigned long lastStatusBroadcast;` to private section
- [ ] Add `bool wasMovingLastUpdate;` to private section

### Step 2: Update Constructor in WebServer.cpp
- [ ] Initialize `lastPositionBroadcast = 0`
- [ ] Initialize `lastStatusBroadcast = 0`
- [ ] Initialize `wasMovingLastUpdate = false`

### Step 3: Enhance update() Method
- [ ] Add movement detection logic
- [ ] Add position broadcast timing check
- [ ] Add status broadcast timing check
- [ ] Add movement completion detection
- [ ] Add debug logging

### Step 4: Add Movement Start Broadcast
- [ ] Find move command handler (line ~316)
- [ ] Add `broadcastStatus()` after `moveTo()`
- [ ] Reset timing variables
- [ ] Add LOG_INFO message

### Step 5: Add Emergency Stop Broadcast
- [ ] Find stop command handler (line ~326)
- [ ] Add `broadcastStatus()` after `emergencyStop()`
- [ ] Add LOG_WARN message

### Step 6: Testing
- [ ] Build and flash firmware
- [ ] Open debug WebSocket in browser
- [ ] Test long movement
- [ ] Test short movement
- [ ] Test emergency stop
- [ ] Test idle behavior
- [ ] Verify timing intervals

### Step 7: Documentation
- [ ] Update CLAUDE.md with new feature
- [ ] Update README.md WebSocket protocol section
- [ ] Mark requirement session as complete

---

## Future Enhancements (Out of Scope)

These are explicitly NOT part of this requirement:

1. **Configurable Intervals**: Runtime configuration of broadcast intervals via API
2. **Adaptive Timing**: Adjust intervals based on movement speed or network conditions
3. **Bandwidth Optimization**: Only serialize JSON when clients connected
4. **Target Position Updates**: Add target position to status messages
5. **Speed/Velocity Updates**: Include calculated motor speed in broadcasts
6. **Prediction/ETA**: Calculate estimated time to completion
7. **Multiple Motor Support**: Broadcast updates for multiple motors
8. **History/Replay**: Store position history for analysis

---

## References

**Codebase Files**:
- src/modules/WebServer/WebServer.h (lines 1-69)
- src/modules/WebServer/WebServer.cpp (lines 1-497)
- src/modules/MotorController/MotorController.h (lines 1-78)
- src/main.cpp (lines 172-185 - WebServerTask)
- webapp/src/types/index.ts (lines 3-18 - Message types)
- webapp/src/hooks/useMotorController.tsx (lines 121-156 - Message handler)

**Requirements Documents**:
- 00-initial-request.md
- 01-discovery-questions.md
- 02-discovery-answers.md
- 03-context-findings.md
- 04-detail-questions.md
- 05-detail-answers.md

**External Documentation**:
- README.md - WebSocket Commands section
- CLAUDE.md - High Priority Features section
- Arduino millis() documentation (timing overflow behavior)

---

## Sign-off

**Requirements Gathered By**: Claude Code
**Approved By**: User (Costyn)
**Date**: 2025-09-30
**Status**: ✅ Ready for Implementation

All discovery and detail questions answered. Codebase analyzed. Implementation strategy defined. No blocking issues identified.

**Estimated Implementation Time**: 30-45 minutes
**Estimated Testing Time**: 15-30 minutes
**Total**: ~1 hour

**Risk Level**: LOW
- Minimal code changes
- No breaking changes to existing functionality
- Well-understood timing patterns
- Existing infrastructure (WebSocket, broadcasts) proven stable
