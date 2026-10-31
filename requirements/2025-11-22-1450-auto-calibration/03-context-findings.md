# Context Findings

**Date:** 2025-11-22 14:50
**Phase:** Targeted Context Gathering - Complete

## Architecture Overview

### Firmware (ESP32)
- **Language:** C++ with Arduino framework, PlatformIO build system
- **RTOS:** FreeRTOS with dual-core task architecture
  - Core 0: InputTask (limit switches, buttons) @ 100ms
  - Core 1: WebServerTask (WebSocket, WiFi) @ 50ms
  - Main loop: Motor control (high-frequency, no delay)

### Modules Structure
```
src/modules/
├── Configuration/      # ESP32 Preferences (NVRAM), motor settings
├── MotorController/    # TMC2209 + AccelStepper, movement control
├── LimitSwitch/        # Hardware interrupts, position learning
└── WebServer/          # WiFiManager, WebSocket, REST API, mDNS
```

### Webapp (React + TypeScript)
- **Framework:** React 18 + TypeScript + Vite
- **UI:** Tailwind CSS + shadcn/ui components
- **State:** Custom hooks (useMotorController)
- **Protocol:** WebSocket-based control interface

---

## Key Files for Calibration Feature

### Firmware Files
1. **[src/modules/MotorController/MotorController.h](../../../src/modules/MotorController/MotorController.h)** - Motor control interface
2. **[src/modules/MotorController/MotorController.cpp](../../../src/modules/MotorController/MotorController.cpp)** - Movement logic
3. **[src/modules/LimitSwitch/LimitSwitch.h](../../../src/modules/LimitSwitch/LimitSwitch.h)** - Limit switch interface
4. **[src/modules/LimitSwitch/LimitSwitch.cpp](../../../src/modules/LimitSwitch/LimitSwitch.cpp)** - Interrupt handlers
5. **[src/modules/Configuration/Configuration.h](../../../src/modules/Configuration/Configuration.h)** - NVRAM storage
6. **[src/modules/Configuration/Configuration.cpp](../../../src/modules/Configuration/Configuration.cpp)** - Config persistence
7. **[src/modules/WebServer/WebServer.h](../../../src/modules/WebServer/WebServer.h)** - WebSocket interface
8. **[src/modules/WebServer/WebServer.cpp](../../../src/modules/WebServer/WebServer.cpp)** - Command handlers

### Webapp Files
1. **[webapp/src/types/index.ts](../../../webapp/src/types/index.ts)** - TypeScript types for WebSocket protocol
2. **[webapp/src/hooks/useMotorController.tsx](../../../webapp/src/hooks/useMotorController.tsx)** - WebSocket communication hook
3. **[webapp/src/components/MotorConfig/MotorConfigDialog.tsx](../../../webapp/src/components/MotorConfig/MotorConfigDialog.tsx)** - Settings UI (where Recalibrate button goes)
4. **[webapp/src/App.tsx](../../../webapp/src/App.tsx)** - Main app component

---

## Existing Patterns to Follow

### 1. Limit Switch Position Learning Pattern
**File:** `src/modules/LimitSwitch/LimitSwitch.cpp:42-103`

Current behavior (lines 54-78):
```cpp
if (isCurrentlyPressed)
{
    triggered = true;
    motorController.jogStop();  // Stop motor immediately

    long currentPos = motorController.getCurrentPosition();
    storedPosition = currentPos;

    // Determine which limit and save position
    if (this == &minLimitSwitch) {
        config.setLimitPos1(currentPos);
        config.saveLimitPositions(currentPos, config.getLimitPos2());
        LOG_WARN("MIN limit switch PRESSED at position: %ld", currentPos);
    }

    // Broadcast status update to webapp
    extern void broadcastStatusFromLimitSwitch();
    broadcastStatusFromLimitSwitch();
}
```

**Key insight:** Limit switches already save their positions to NVRAM automatically when triggered. Calibration routine can leverage this existing mechanism.

---

### 2. Motor Movement Pattern
**File:** `src/modules/MotorController/MotorController.cpp:103-123`

Existing `moveTo()` function:
```cpp
void MotorController::moveTo(long position, int speed)
{
    if (emergencyStopActive) {
        LOG_WARN("Cannot move - emergency stop active");
        return;
    }

    digitalWrite(EN_PIN, LOW); // Enable motor

    targetPosition = position;
    stepper->setMaxSpeed(speed);
    stepper->moveTo(position);

    LOG_INFO("Moving to position: %ld at speed: %d steps/sec", position, speed);
}
```

**Key insight:** Simple interface, already checks emergency stop. Calibration can use this same method.

---

### 3. Emergency Stop Pattern
**File:** `src/modules/MotorController/MotorController.h:69`

Two stop methods:
```cpp
void jogStop();          // Gentle stop without emergency flag
void emergencyStop();    // Full emergency stop with flag
```

**Key insight:** During calibration, emergency stop should abort the calibration routine entirely.

---

### 4. WebSocket Command Pattern
**File:** `src/modules/WebServer/WebServer.cpp:421-425`

Existing command handlers:
```cpp
else if (command == "jogStart") {
    handleJogStartCommand(doc);
}
else if (command == "jogStop") {
    handleJogStopCommand(doc);
}
```

**Key insight:** Need to add `handleCalibrateCommand()` following same pattern.

---

### 5. WebSocket Status Broadcasting Pattern
**File:** `src/modules/WebServer/WebServer.cpp` (various locations)

Status updates sent to webapp:
```cpp
broadcastStatus();  // Sends motor position, state, emergency stop, limit switches
```

**Key insight:** Calibration progress can be communicated via existing status message by adding a new field (e.g., `calibrationState`).

---

### 6. Configuration Persistence Pattern
**File:** `src/modules/Configuration/Configuration.cpp`

NVRAM operations:
```cpp
void Configuration::saveLimitPositions(long pos1, long pos2)
{
    preferences.begin("motor-config", false);
    preferences.putLong("limitPos1", pos1);
    preferences.putLong("limitPos2", pos2);
    preferences.end();
    LOG_INFO("Saved limit positions: %ld, %ld", pos1, pos2);
}
```

**Key insight:** Limit positions automatically persisted when set. No additional NVRAM work needed for calibration.

---

### 7. Webapp Settings Dialog Pattern
**File:** `webapp/src/components/MotorConfig/MotorConfigDialog.tsx:313-326`

Existing footer with buttons:
```tsx
<DialogFooter>
  <Button variant="outline" onClick={handleRevert} disabled={!hasChanges}>
    Revert
  </Button>
  <Button onClick={handleApply}>
    Apply
  </Button>
</DialogFooter>
```

**Key insight:** Add "Recalibrate" button in the footer or as a separate section before the footer.

---

### 8. WebSocket TypeScript Types Pattern
**File:** `webapp/src/types/index.ts:78-96`

Existing command types:
```typescript
export interface JogStartCommand {
  command: 'jogStart';
  direction: 'forward' | 'backward';
  speed: number;
}

export type ControlCommand =
  | MoveCommand
  | JogStartCommand
  // ...
```

**Key insight:** Need to add `CalibrateCommand` interface and include in `ControlCommand` union.

---

## Technical Constraints & Considerations

### 1. Calibration Speed
- **Consideration:** Should calibration use a fixed safe speed or user-configurable speed?
- **Recommendation:** Use a moderate fixed speed (e.g., 30% of current maxSpeed) for safety and consistency

### 2. Calibration Direction
- **Current assumption:** Move to min limit first (backward), then to max limit (forward)
- **Question:** Which direction should calibration start? (Needs clarification in detail questions)

### 3. State Management
- **Need:** New calibration state variable in MotorController
- **Options:**
  - Add boolean flag `isCalibrating`
  - Add enum state: `IDLE`, `CALIBRATING_MIN`, `CALIBRATING_MAX`, `CALIBRATION_COMPLETE`
- **Recommendation:** Use enum for better state tracking and status reporting

### 4. Movement Blocking During Calibration
- **Pattern:** Similar to emergency stop check
- **Implementation:** Add `isCalibrating` check in all movement commands
  ```cpp
  if (isCalibrating) {
      LOG_WARN("Cannot move - calibration in progress");
      return;
  }
  ```

### 5. Limit Switch Callback Integration
- **Current:** Limit switches have callback support but not currently used
  ```cpp
  void setLimitCallback(LimitSwitchCallback callback);
  ```
- **Opportunity:** Calibration routine can register callback to know when limit is reached

### 6. Total Steps Calculation
- **Formula:** `totalSteps = abs(maxLimit - minLimit)`
- **Storage:** No need to store separately - can be calculated on-demand
- **Reporting:** Include in config broadcast message

---

## Similar Features in Codebase

### 1. Jog Controls (Continuous Movement)
**File:** `src/modules/WebServer/WebServer.cpp:248-296`
- Pattern: Move to limit position at specified speed
- Calibration is similar: automated movement to both limits in sequence

### 2. Limit Switch Position Learning
**File:** `src/modules/LimitSwitch/LimitSwitch.cpp:54-103`
- Pattern: Detect limit trigger → save position → broadcast update
- Calibration will trigger this same mechanism automatically

### 3. Emergency Stop Recovery
**File:** `src/modules/MotorController/MotorController.h:57`
- Pattern: Special stop mode that blocks normal operations until cleared
- Calibration needs similar blocking behavior while in progress

---

## Integration Points

### Firmware Integration Points
1. **MotorController:** Add calibration state machine and start/stop methods
2. **WebServer:** Add WebSocket command handler for "calibrate"
3. **LimitSwitch:** Use existing callback mechanism to detect calibration completion
4. **Configuration:** No changes needed - existing save methods work

### Webapp Integration Points
1. **Types:** Add `CalibrateCommand` and calibration status fields to `MotorStatus`
2. **useMotorController hook:** Add `startCalibration()` method
3. **MotorConfigDialog:** Add "Recalibrate" button UI
4. **MotorStatus component:** Display calibration progress messages

---

## Best Practices from Codebase

### Logging
- Use unified logging macros: `LOG_ERROR`, `LOG_WARN`, `LOG_INFO`, `LOG_DEBUG`
- Format: `[HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message`
- Always log state transitions and important events

### Safety Checks
- Always check `emergencyStopActive` before movement
- Clamp speed/acceleration values to MIN/MAX constants
- Validate inputs before acting on commands

### FreeRTOS Best Practices
- Keep ISR handlers minimal (LimitSwitch pattern)
- Do heavy work in task context (update() methods)
- Use `vTaskDelay()` for timing in tasks

### WebSocket Communication
- Always broadcast status after state changes
- Use immediate broadcasts on movement start
- Include error messages in responses

---

## Known Limitations

1. **Encoder not working:** Cannot use encoder for position verification during calibration
2. **No homing:** Current position tracking is relative, not absolute (relies on limit switches for boundaries)
3. **Manual movement while powered off:** If motor is moved manually when powered off, position tracking will be incorrect until re-calibration

---

## Next Steps

Proceed to Phase 4: Expert Requirements Questions to clarify:
1. Calibration movement direction and sequence
2. Calibration speed strategy
3. Behavior when calibration is interrupted
4. UI feedback details
5. Error handling scenarios
