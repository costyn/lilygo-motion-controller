# Requirements Specification: Freewheel Config & Mode Switching Fixes

**Session ID**: 2025-10-05-1211-freewheel-and-mode-switching
**Date**: 2025-10-05
**Status**: Ready for Implementation

---

## Problem Statement

### Issue 1: Inconsistent Freewheel Behavior
The motor freewheel functionality is partially implemented but inconsistent:
- **Works**: After jogging (`jogStop()`) and emergency stop
- **Doesn't work**: After slider movements and quick position buttons
- **Root cause**: No detection of movement completion in `update()` loop

**User Impact**: Cannot configure whether motor should hold position or freewheel after movement completes. Inconsistent behavior confuses users.

---

### Issue 2: Motor Buzzing and Heating
When freewheel is disabled (motor should hold position), the motor buzzes audibly and both motor and TMC2209 driver get warm.

**Root Cause Discovery**:
- `stepper->run()` is called continuously in main loop (line 126, main.cpp → line 241, MotorController.cpp)
- AccelStepper's `run()` executes even when `distanceToGo() == 0`
- This causes TMC2209 to send continuous micro-step corrections to "maintain" a position it's already at
- Unnecessary PWM activity causes buzzing and heat

**User Impact**: Motor unusable in hold mode due to noise and heat concerns.

---

### Issue 3: StealthChop/SpreadCycle Mode Switching Never Happens
The `updateTMCMode()` method (lines 200-211, MotorController.cpp) is designed to automatically switch between StealthChop (quiet, low speed) and SpreadCycle (powerful, high speed) based on motor speed.

**Root Cause**:
- Logic uses `motorSpeed` variable (line 202)
- `motorSpeed` is declared as `static float motorSpeed = 0;` (line 25)
- **Variable is never assigned** - stays at 0 permanently
- Speed percentage is always 0%, mode never switches
- Logs never show mode switching messages

**User Impact**: Motor always runs in StealthChop mode, missing the performance benefits of SpreadCycle at higher speeds.

---

## Solution Overview

### Solution 1: Implement Configurable Freewheel
Add `freewheelAfterMove` boolean configuration setting:
- Backend: Add to Configuration module with NVRAM persistence
- Frontend: Add toggle switch to existing MotorConfigDialog
- Motor control: Detect movement completion, apply freewheel based on config
- Consistent behavior across all movement types (jog, slider, quick positions)

**Default**: Disabled (motor holds position) - safer for upgrades

---

### Solution 2: Fix Motor Buzzing
Stop calling `stepper->run()` when motor has reached target position:
- Check `if (stepper->distanceToGo() != 0)` before calling `run()`
- When at target: Apply freewheel if configured, otherwise leave motor enabled but idle
- Keep `ihold(1)` unchanged (matches factory example)

**Key Insight**: Problem is not hold current, it's unnecessary run() calls.

---

### Solution 3: Fix Mode Switching
Replace unusable `motorSpeed` variable with AccelStepper's commanded speed:
- Use `abs(stepper->speed())` instead of `motorSpeed`
- Returns actual commanded speed in steps/sec
- Works without encoder (encoder will be used for position tracking in future)

**Benefits**: Mode switching will work immediately, logs will show transitions

---

## Functional Requirements

### FR-1: Freewheel Configuration Setting
**Priority**: MUST HAVE

**Behavior**:
- New boolean config field: `freewheelAfterMove`
- Default value: `false` (disabled - motor holds position)
- Stored in ESP32 NVRAM (persists across reboots)
- Configurable via WebSocket `setConfig` command
- Exposed in MotorConfigDialog UI

**Acceptance Criteria**:
- [ ] Config field added to backend Configuration module
- [ ] Value persists across ESP32 reboots
- [ ] WebSocket API accepts and returns field
- [ ] UI shows toggle switch in Settings dialog
- [ ] Changes take effect immediately

---

### FR-2: Movement Completion Detection
**Priority**: MUST HAVE

**Behavior**:
- In `update()` loop, detect when movement completes (`distanceToGo() == 0`)
- Use simple state tracking: was motor moving last iteration?
- On transition from moving → stopped:
  - If `freewheelAfterMove` enabled: `digitalWrite(EN_PIN, HIGH)`
  - If disabled: Leave motor enabled (no freewheel)

**State Machine** (simple):
```cpp
static bool wasMoving = false;
bool isMoving = (stepper->distanceToGo() != 0);

if (isMoving) {
    stepper->run();
    wasMoving = true;
} else if (wasMoving) {
    // Just stopped moving
    if (config.getFreewheelAfterMove()) {
        digitalWrite(EN_PIN, HIGH);
    }
    wasMoving = false;
}
```

**Acceptance Criteria**:
- [ ] Movement completion detected reliably
- [ ] Freewheel applied when configured
- [ ] Motor holds position when freewheel disabled
- [ ] No buzzing when holding position
- [ ] Works for all movement types (jog, slider, quick positions)

---

### FR-3: Stop Unnecessary Run Calls
**Priority**: MUST HAVE

**Behavior**:
- Only call `stepper->run()` when motor is actually moving
- Check `distanceToGo() != 0` before calling `run()`
- When at target position, motor remains enabled but idle (no PWM activity)

**Impact**: Eliminates buzzing and heating when holding position

**Acceptance Criteria**:
- [ ] `run()` only called when motor moving
- [ ] No buzzing when motor at target position
- [ ] No excessive heating of motor or driver
- [ ] Motor still responds immediately to new movement commands

---

### FR-4: JogStop Respects Freewheel Config
**Priority**: MUST HAVE

**Current Behavior**: `jogStop()` always freewheels (line 129, MotorController.cpp)

**New Behavior**:
```cpp
void MotorController::jogStop()
{
    stepper->setCurrentPosition(stepper->currentPosition());
    stepper->setSpeed(0);

    if (config.getFreewheelAfterMove()) {
        digitalWrite(EN_PIN, HIGH);
    }

    LOG_INFO("Motor jog stopped");
}
```

**Acceptance Criteria**:
- [ ] Jog stop freewheels when config enabled
- [ ] Jog stop holds position when config disabled
- [ ] Consistent with other movement types

---

### FR-5: Emergency Stop Always Freewheels
**Priority**: MUST HAVE

**Behavior**: Emergency stop should **always** freewheel regardless of configuration setting (safety requirement)

**Current Implementation**: Already correct (lines 138, 237)

**Acceptance Criteria**:
- [ ] Emergency stop always freewheels
- [ ] Config setting does not affect emergency stop behavior
- [ ] Documentation notes this exception

---

### FR-6: Fix Mode Switching Logic
**Priority**: MUST HAVE

**Current Code** (broken):
```cpp
float currentSpeedPercent = abs(motorSpeed) / (float)config.getMaxSpeed();
```

**New Code**:
```cpp
float currentSpeedPercent = abs(stepper->speed()) / (float)config.getMaxSpeed();
```

**Behavior**:
- Use AccelStepper's commanded speed instead of encoder speed
- Calculate percentage of max speed
- Switch to SpreadCycle when speed > 50% of max
- Switch to StealthChop when speed < 50% of max
- Log mode changes with `LOG_DEBUG()`

**Acceptance Criteria**:
- [ ] Mode switching logic executes
- [ ] Logs show "TMC mode switched to StealthChop/SpreadCycle"
- [ ] Switching happens at appropriate speeds
- [ ] No dependency on encoder (works with broken encoder)

---

### FR-7: Freewheel UI in MotorConfigDialog
**Priority**: MUST HAVE

**Location**: webapp/src/components/MotorConfig/MotorConfigDialog.tsx

**UI Element**: Switch component (similar to useStealthChop toggle)

**Label**: "Freewheel After Movement"

**Help Text**: "Enable to let motor spin freely after movement completes. Disable to hold position (uses power)."

**Layout** (add after StealthChop switch):
```tsx
{/* Freewheel Mode */}
<div className="grid gap-2">
  <Label htmlFor="freewheel">Freewheel After Movement</Label>
  <div className="flex items-center gap-2">
    <Switch
      id="freewheel"
      checked={freewheelAfterMove}
      onCheckedChange={setFreewheelAfterMove}
    />
    <span className="text-sm">
      {freewheelAfterMove ? "Enabled (Motor spins freely)" : "Disabled (Holds position)"}
    </span>
  </div>
  <p className="text-xs text-muted-foreground">
    Enable to let motor spin freely after movement. Disable to hold position (uses power).
  </p>
</div>
```

**Acceptance Criteria**:
- [ ] Switch appears in Settings dialog
- [ ] Toggle changes state immediately
- [ ] Help text explains behavior clearly
- [ ] Apply button saves changes
- [ ] Changes persist across page reloads

---

## Technical Requirements

### TR-1: Backend - Configuration Module Changes

**File**: src/modules/Configuration/Configuration.h

**Add to MotorConfig struct** (after line 19):
```cpp
struct MotorConfig
{
    long acceleration;
    long maxSpeed;
    long limitPos1;
    long limitPos2;
    bool useStealthChop;
    bool freewheelAfterMove;  // NEW
} motorConfig;
```

**Add getter** (after line 44):
```cpp
bool getFreewheelAfterMove() const { return motorConfig.freewheelAfterMove; }
```

**Add setter** (after line 51):
```cpp
void setFreewheelAfterMove(bool value) { motorConfig.freewheelAfterMove = value; }
```

---

**File**: src/modules/Configuration/Configuration.cpp

**Update constructor** (line 14):
```cpp
Configuration::Configuration() {
    // Default values
    motorConfig.acceleration = 1000 * 80;
    motorConfig.maxSpeed = 180 * 80;
    motorConfig.limitPos1 = 0;
    motorConfig.limitPos2 = 2000;
    motorConfig.useStealthChop = true;
    motorConfig.freewheelAfterMove = false;  // NEW - disabled by default
}
```

**Update loadConfiguration** (line 33):
```cpp
void Configuration::loadConfiguration() {
    motorConfig.acceleration = preferences.getLong("acceleration", motorConfig.acceleration);
    motorConfig.maxSpeed = preferences.getLong("maxSpeed", motorConfig.maxSpeed);
    motorConfig.limitPos1 = preferences.getLong("limitPos1", motorConfig.limitPos1);
    motorConfig.limitPos2 = preferences.getLong("limitPos2", motorConfig.limitPos2);
    motorConfig.useStealthChop = preferences.getBool("stealthChop", motorConfig.useStealthChop);
    motorConfig.freewheelAfterMove = preferences.getBool("freewheel", motorConfig.freewheelAfterMove);  // NEW

    LOG_INFO("Configuration loaded - Accel: %ld, MaxSpeed: %ld, Limit1: %ld, Limit2: %ld, Freewheel: %d",
             motorConfig.acceleration, motorConfig.maxSpeed, motorConfig.limitPos1,
             motorConfig.limitPos2, motorConfig.freewheelAfterMove);
}
```

**Update saveConfiguration** (line 44):
```cpp
void Configuration::saveConfiguration() {
    preferences.putLong("acceleration", motorConfig.acceleration);
    preferences.putLong("maxSpeed", motorConfig.maxSpeed);
    preferences.putLong("limitPos1", motorConfig.limitPos1);
    preferences.putLong("limitPos2", motorConfig.limitPos2);
    preferences.putBool("stealthChop", motorConfig.useStealthChop);
    preferences.putBool("freewheel", motorConfig.freewheelAfterMove);  // NEW
    LOG_INFO("Configuration saved");
}
```

**Add setter implementation** (after line 64):
```cpp
void Configuration::setFreewheelAfterMove(bool value) {
    motorConfig.freewheelAfterMove = value;
    preferences.putBool("freewheel", value);
}
```

---

### TR-2: Backend - MotorController Update Logic

**File**: src/modules/MotorController/MotorController.cpp

**Replace update() method** (lines 225-243):
```cpp
void MotorController::update()
{
    // Track movement state for completion detection
    static bool wasMoving = false;
    bool isMoving = (stepper->distanceToGo() != 0);

    // Update TMC mode based on current commanded speed
    updateTMCMode();

    // Handle movement
    if (emergencyStopActive)
    {
        stepper->setSpeed(0);
        digitalWrite(EN_PIN, HIGH); // Always freewheel during emergency stop
    }
    else if (isMoving)
    {
        // Motor is moving - call run() to step motor
        stepper->run();
        wasMoving = true;
    }
    else if (wasMoving)
    {
        // Motor just stopped moving
        if (config.getFreewheelAfterMove())
        {
            digitalWrite(EN_PIN, HIGH); // Freewheel
            LOG_INFO("Movement complete - freewheeling");
        }
        else
        {
            // Motor enabled but idle - holding position
            LOG_INFO("Movement complete - holding position");
        }
        wasMoving = false;
    }
    // else: motor is stopped and we've already logged it
}
```

**Key Changes**:
- Added `wasMoving` static variable (simple state tracking)
- Check `distanceToGo() != 0` before calling `run()`
- Detect transition from moving → stopped
- Apply freewheel based on config
- Clear, single-purpose logic flow

---

### TR-3: Backend - Fix Mode Switching

**File**: src/modules/MotorController/MotorController.cpp

**Replace updateTMCMode() method** (lines 200-211):
```cpp
void MotorController::updateTMCMode()
{
    // Use commanded speed from AccelStepper (not encoder)
    float currentSpeedPercent = abs(stepper->speed()) / (float)config.getMaxSpeed();
    bool shouldUseStealthChop = currentSpeedPercent < STEALTH_CHOP_THRESHOLD;

    if (shouldUseStealthChop != useStealthChop)
    {
        useStealthChop = shouldUseStealthChop;
        driver->en_spreadCycle(!useStealthChop);
        LOG_DEBUG("TMC mode switched to %s (speed: %.0f steps/sec, %.0f%% of max)",
                  useStealthChop ? "StealthChop" : "SpreadCycle",
                  abs(stepper->speed()),
                  currentSpeedPercent * 100);
    }
}
```

**Key Changes**:
- Use `abs(stepper->speed())` instead of `motorSpeed`
- Enhanced log message shows actual speed and percentage
- No dependency on encoder

---

### TR-4: Backend - Fix JogStop

**File**: src/modules/MotorController/MotorController.cpp

**Replace jogStop() method** (lines 123-131):
```cpp
void MotorController::jogStop()
{
    // Stop motor movement without triggering emergency stop flag
    stepper->setCurrentPosition(stepper->currentPosition());
    stepper->setSpeed(0);

    // Respect freewheel configuration
    if (config.getFreewheelAfterMove())
    {
        digitalWrite(EN_PIN, HIGH); // Freewheel
    }

    LOG_INFO("Motor jog stopped");
}
```

**Key Changes**:
- Check `config.getFreewheelAfterMove()` before freewheeling
- Consistent with other movement completion logic

---

### TR-5: Backend - WebSocket API Updates

**File**: src/modules/WebServer/WebServer.cpp

**Update GET /api/config response** (lines 136-144):
```cpp
server.on("/api/config", HTTP_GET, [this](AsyncWebServerRequest *request)
{
    JsonDocument doc;
    doc["maxSpeed"] = config.getMaxSpeed();
    doc["acceleration"] = config.getAcceleration();
    doc["minLimit"] = config.getMinLimit();
    doc["maxLimit"] = config.getMaxLimit();
    doc["useStealthChop"] = config.getUseStealthChop();
    doc["freewheelAfterMove"] = config.getFreewheelAfterMove();  // NEW

    String response;
    serializeJson(doc, response);
    request->send(200, "application/json", response);
});
```

**Update setConfig handler** (lines 347-362, add after line 349):
```cpp
void WebServerClass::handleSetConfigCommand(JsonDocument& doc)
{
    bool updated = false;

    if (doc["maxSpeed"].is<long>())
    {
        config.setMaxSpeed(doc["maxSpeed"]);
        motorController.setMaxSpeed(doc["maxSpeed"]);
        updated = true;
    }

    if (doc["acceleration"].is<long>())
    {
        config.setAcceleration(doc["acceleration"]);
        motorController.setAcceleration(doc["acceleration"]);
        updated = true;
    }

    if (doc["useStealthChop"].is<bool>())
    {
        config.setUseStealthChop(doc["useStealthChop"]);
        motorController.setTMCMode(doc["useStealthChop"]);
        updated = true;
    }

    // NEW: Handle freewheelAfterMove
    if (doc["freewheelAfterMove"].is<bool>())
    {
        config.setFreewheelAfterMove(doc["freewheelAfterMove"]);
        updated = true;
    }

    if (updated)
    {
        config.saveConfiguration();
        broadcastConfig();
        ws.textAll("{\"type\":\"configUpdated\",\"status\":\"success\"}");
        LOG_INFO("Configuration updated via WebSocket");
    }
}
```

---

### TR-6: Frontend - Type Definitions

**File**: webapp/src/types/index.ts

**Update MotorConfig interface** (lines 20-27):
```typescript
export interface MotorConfig {
  type: 'config';
  maxSpeed: number;
  acceleration: number;
  minLimit: number;
  maxLimit: number;
  useStealthChop: boolean;
  freewheelAfterMove: boolean;  // NEW
}
```

**Update SetConfigCommand interface** (lines 70-75):
```typescript
export interface SetConfigCommand {
  command: 'setConfig';
  maxSpeed?: number;
  acceleration?: number;
  useStealthChop?: boolean;
  freewheelAfterMove?: boolean;  // NEW
}
```

---

### TR-7: Frontend - MotorConfigDialog Updates

**File**: webapp/src/components/MotorConfig/MotorConfigDialog.tsx

**Add state** (after line 27):
```typescript
const [freewheelAfterMove, setFreewheelAfterMove] = useState(currentConfig.freewheelAfterMove)
```

**Update useEffect** (add to line 43):
```typescript
useEffect(() => {
  if (open) {
    setMaxSpeed(currentConfig.maxSpeed)
    setAcceleration(currentConfig.acceleration)
    setUseStealthChop(currentConfig.useStealthChop)
    setFreewheelAfterMove(currentConfig.freewheelAfterMove)  // NEW
    setMinLimit(currentConfig.minLimit)
    setMaxLimit(currentConfig.maxLimit)
    setMaxSpeedError(null)
    setAccelerationError(null)
    setMinLimitError(null)
    setMaxLimitError(null)
  }
}, [open, currentConfig])
```

**Update hasChanges** (add to line 134):
```typescript
const hasChanges =
  maxSpeed !== currentConfig.maxSpeed ||
  acceleration !== currentConfig.acceleration ||
  useStealthChop !== currentConfig.useStealthChop ||
  freewheelAfterMove !== currentConfig.freewheelAfterMove ||  // NEW
  minLimit !== currentConfig.minLimit ||
  maxLimit !== currentConfig.maxLimit
```

**Update handleRevert** (add to line 143):
```typescript
const handleRevert = () => {
  setMaxSpeed(currentConfig.maxSpeed)
  setAcceleration(currentConfig.acceleration)
  setUseStealthChop(currentConfig.useStealthChop)
  setFreewheelAfterMove(currentConfig.freewheelAfterMove)  // NEW
  setMinLimit(currentConfig.minLimit)
  setMaxLimit(currentConfig.maxLimit)
  setMaxSpeedError(null)
  setAccelerationError(null)
  setMinLimitError(null)
  setMaxLimitError(null)
}
```

**Update handleApply** (add to line 161):
```typescript
const handleApply = () => {
  if (!isFormValid) return

  const changes: Partial<Omit<MotorConfig, 'type'>> = {}
  if (maxSpeed !== currentConfig.maxSpeed) changes.maxSpeed = maxSpeed
  if (acceleration !== currentConfig.acceleration) changes.acceleration = acceleration
  if (useStealthChop !== currentConfig.useStealthChop) changes.useStealthChop = useStealthChop
  if (freewheelAfterMove !== currentConfig.freewheelAfterMove) changes.freewheelAfterMove = freewheelAfterMove  // NEW
  if (minLimit !== currentConfig.minLimit) changes.minLimit = minLimit
  if (maxLimit !== currentConfig.maxLimit) changes.maxLimit = maxLimit

  onApply(changes)
  onOpenChange(false)
}
```

**Add UI element** (after StealthChop switch section, around line 250):
```tsx
{/* Freewheel Mode */}
<div className="grid gap-2">
  <Label htmlFor="freewheel">Freewheel After Movement</Label>
  <div className="flex items-center gap-2">
    <Switch
      id="freewheel"
      checked={freewheelAfterMove}
      onCheckedChange={setFreewheelAfterMove}
    />
    <span className="text-sm">
      {freewheelAfterMove ? "Enabled (Motor spins freely)" : "Disabled (Holds position)"}
    </span>
  </div>
  <p className="text-xs text-muted-foreground">
    Enable to let motor spin freely after movement completes. Disable to hold position (uses power).
  </p>
</div>
```

---

## Edge Cases and Error Handling

### Edge Case 1: Emergency Stop Always Freewheels
**Scenario**: User has freewheel disabled, triggers emergency stop

**Handling**: Emergency stop always freewheels regardless of config (safety requirement)

**Code**: Already correct in `update()` method - emergency stop path doesn't check config

---

### Edge Case 2: Movement Interrupted by New Command
**Scenario**: Motor moving to position A, new `moveTo()` command for position B arrives

**Current Behavior**: New command overrides old (AccelStepper behavior)

**Handling**:
- `wasMoving` stays true (motor still moving)
- Completion detection only fires when final position reached
- No premature freewheel

**No special handling needed** - state machine handles this naturally

---

### Edge Case 3: Rapid Position Changes
**Scenario**: User rapidly sends multiple position commands via slider

**Risk**: Could `wasMoving` flag get out of sync?

**Handling**: No - flag is updated every loop iteration based on `distanceToGo()`

**Test**: Verify rapid slider movements don't cause unexpected freewheel

---

### Edge Case 4: Power Loss During Movement
**Scenario**: ESP32 loses power while motor moving

**Behavior**: Motor freewheels (EN_PIN defaults to HIGH on reset)

**On Reboot**: Config loaded from NVRAM, motor starts in correct state

**No special handling needed**

---

### Edge Case 5: Config Changed While Motor Moving
**Scenario**: User changes freewheel setting while motor is moving

**Behavior**: Setting takes effect for **next** movement completion

**Reasoning**: Don't want to interrupt current movement

**Acceptable** - user confirmed this behavior is fine

---

## Testing Strategy

### Unit Testing
Not applicable - requires hardware (TMC2209 driver state, AccelStepper timing)

### Integration Testing (Hardware Required)

#### Test 1: Freewheel After Slider Movement
1. Enable freewheel in Settings dialog
2. Use position slider to move motor
3. Wait for movement to complete
4. **Verify**: Motor freewheels (can turn shaft by hand)
5. **Verify**: No buzzing or heating

#### Test 2: Hold After Slider Movement
1. Disable freewheel in Settings dialog
2. Use position slider to move motor
3. Wait for movement to complete
4. **Verify**: Motor holds position (cannot turn shaft easily)
5. **Verify**: No buzzing
6. **Verify**: No excessive heating

#### Test 3: Freewheel After Quick Position
1. Enable freewheel
2. Click "Center" quick position button
3. Wait for movement to complete
4. **Verify**: Motor freewheels

#### Test 4: Hold After Quick Position
1. Disable freewheel
2. Click "Min" quick position button
3. Wait for movement to complete
4. **Verify**: Motor holds position
5. **Verify**: No buzzing

#### Test 5: Jog with Freewheel Enabled
1. Enable freewheel
2. Hold jog forward button
3. Release button
4. **Verify**: Motor freewheels immediately

#### Test 6: Jog with Freewheel Disabled
1. Disable freewheel
2. Hold jog forward button
3. Release button
4. **Verify**: Motor holds position
5. **Verify**: No buzzing

#### Test 7: Emergency Stop Always Freewheels
1. Disable freewheel (motor should hold)
2. Start movement
3. Click emergency stop
4. **Verify**: Motor freewheels (despite config being disabled)

#### Test 8: Mode Switching at Low Speed
1. Configure max speed to 10,000 steps/sec
2. Move motor at 3,000 steps/sec (30% of max, below 50% threshold)
3. **Verify**: Serial logs show "TMC mode switched to StealthChop"
4. **Verify**: Log shows speed and percentage

#### Test 9: Mode Switching at High Speed
1. Move motor at 7,000 steps/sec (70% of max, above 50% threshold)
2. **Verify**: Serial logs show "TMC mode switched to SpreadCycle"
3. **Verify**: Motor runs (may be slightly louder)

#### Test 10: Mode Switching Back to StealthChop
1. After high-speed movement, move at 2,000 steps/sec
2. **Verify**: Logs show switch back to StealthChop

#### Test 11: Config Persistence
1. Enable freewheel in Settings dialog
2. Apply changes
3. Power cycle ESP32
4. Reconnect webapp
5. Open Settings dialog
6. **Verify**: Freewheel setting still enabled

#### Test 12: Rapid Slider Movements
1. Enable freewheel
2. Rapidly drag slider back and forth multiple times
3. Let movement complete
4. **Verify**: Motor freewheels
5. **Verify**: No unexpected behavior or crashes

---

## Implementation Checklist

### Backend Changes

- [ ] **Configuration.h**: Add `freewheelAfterMove` field to struct
- [ ] **Configuration.h**: Add getter method
- [ ] **Configuration.h**: Add setter method declaration
- [ ] **Configuration.cpp**: Set default value in constructor
- [ ] **Configuration.cpp**: Add load logic in `loadConfiguration()`
- [ ] **Configuration.cpp**: Add save logic in `saveConfiguration()`
- [ ] **Configuration.cpp**: Implement setter method
- [ ] **MotorController.cpp**: Replace `update()` with new logic
- [ ] **MotorController.cpp**: Fix `updateTMCMode()` to use `stepper->speed()`
- [ ] **MotorController.cpp**: Update `jogStop()` to respect config
- [ ] **WebServer.cpp**: Add field to `/api/config` GET response
- [ ] **WebServer.cpp**: Add field handling in `setConfig` command

### Frontend Changes

- [ ] **types/index.ts**: Add `freewheelAfterMove` to `MotorConfig` interface
- [ ] **types/index.ts**: Add field to `SetConfigCommand` interface
- [ ] **MotorConfigDialog.tsx**: Add state variable
- [ ] **MotorConfigDialog.tsx**: Add to useEffect reset
- [ ] **MotorConfigDialog.tsx**: Add to hasChanges check
- [ ] **MotorConfigDialog.tsx**: Add to handleRevert
- [ ] **MotorConfigDialog.tsx**: Add to handleApply
- [ ] **MotorConfigDialog.tsx**: Add Switch UI element

### Testing

- [ ] Build firmware: `pio run -e pico32`
- [ ] Flash to hardware: `pio run -e pico32 -t upload`
- [ ] Build webapp: `cd webapp && npm run build`
- [ ] Upload filesystem: `pio run -e pico32 -t uploadfs`
- [ ] Run all hardware tests (12 scenarios above)
- [ ] Verify serial logs show mode switching
- [ ] Verify no buzzing in hold mode
- [ ] Verify config persistence across reboots

### Documentation

- [ ] Update CLAUDE.md with completed features
- [ ] Update TODO.md to mark items complete
- [ ] Add notes about mode switching using commanded speed
- [ ] Document freewheel config setting in README

---

## Assumptions

1. **Default freewheel disabled** - Motor holds position by default (safer for upgrades)
2. **Emergency stop always freewheels** - Safety overrides config
3. **Simple state machine** - Single `wasMoving` flag sufficient, no complex state tracking
4. **Keep ihold at 1** - Factory setting is correct, buzzing caused by unnecessary run() calls
5. **Use commanded speed permanently** - Encoder will be used for position tracking in future, not speed
6. **No debounce needed** - AccelStepper's distanceToGo is reliable enough
7. **Config changes take effect on next movement** - Don't interrupt current movement

---

## Dependencies

### Backend Libraries (No Changes)
All existing:
- AccelStepper (motor control)
- TMCStepper (driver communication)
- Preferences (NVRAM storage)
- ArduinoJson (WebSocket protocol)

### Frontend Libraries (No Changes)
All existing:
- React 18.3.1
- Radix UI Switch component (already used for useStealthChop)
- TypeScript type system

### Hardware Requirements
- LilyGo T-Motor with TMC2209
- Encoder not required (mode switching uses commanded speed)
- Serial connection for monitoring logs (optional but recommended)

---

## Key Insights

### Discovery 1: Root Cause of Buzzing
The buzzing is NOT caused by low hold current. It's caused by **continuously calling `stepper->run()` when the motor has already reached its target**. AccelStepper keeps sending micro-step corrections, causing unnecessary PWM activity and buzzing.

**Solution**: Only call `run()` when `distanceToGo() != 0`.

### Discovery 2: Mode Switching Variable Never Used
The `motorSpeed` variable (line 25, MotorController.h) is declared but **never assigned**. It stays at 0 forever. This is why mode switching never happens.

**Solution**: Use `stepper->speed()` which returns the actual commanded speed.

### Discovery 3: Simple State Machine is Sufficient
No need for complex state tracking. A single `wasMoving` boolean flag is enough to detect the transition from moving → stopped and apply freewheel logic.

### Discovery 4: Freewheel Pattern Already Exists
The `jogStop()` and `emergencyStop()` methods already implement freewheel with `digitalWrite(EN_PIN, HIGH)`. We just need to:
1. Make it configurable
2. Apply it at movement completion
3. Make jogStop respect the config

---

## Implementation Priority

### Phase 1: Core Fixes (Required for basic functionality)
1. Fix `update()` method - stop calling run() when stationary
2. Fix `updateTMCMode()` - use stepper->speed() instead of motorSpeed
3. Test: Verify no buzzing and mode switching works

### Phase 2: Freewheel Config (Feature completion)
4. Add Configuration field and persistence
5. Add movement completion detection
6. Update jogStop() to respect config
7. Test: Verify freewheel applies correctly

### Phase 3: WebSocket & UI (User-facing)
8. Update WebSocket API
9. Update frontend types
10. Add UI toggle to MotorConfigDialog
11. Test: End-to-end config changes via UI

---

## Success Criteria

### Must Have (Required for completion)
- [ ] Motor does not buzz when holding position
- [ ] Serial logs show mode switching between StealthChop and SpreadCycle
- [ ] Freewheel configuration setting exists and persists
- [ ] Freewheel applies to all movement types (jog, slider, quick positions)
- [ ] Settings dialog shows freewheel toggle
- [ ] Changes take effect immediately
- [ ] Emergency stop always freewheels

### Should Have (Expected quality)
- [ ] No excessive heating of motor or driver
- [ ] Mode switching logs show speed and percentage
- [ ] Config survives ESP32 reboot
- [ ] All 12 test scenarios pass

### Nice to Have (Polish)
- [ ] Clear help text in UI explaining freewheel behavior
- [ ] Movement completion logs indicate freewheel vs hold
- [ ] Documentation updated

---

## Sign-off

**Requirements Gathered By**: Claude Code
**Approved By**: User (Costyn)
**Date**: 2025-10-05
**Status**: ✅ Ready for Implementation

All discovery and detail questions answered. Root causes identified through code analysis. Implementation strategy defined with specific code examples. No blocking issues identified.

**Estimated Implementation Time**:
- Backend fixes (update, mode switching): 15-20 minutes
- Backend config (Configuration module): 15 minutes
- Backend WebSocket (API updates): 10 minutes
- Frontend types and UI: 20-25 minutes
- Testing and validation: 30-45 minutes
- **Total**: ~1.5-2 hours

**Risk Level**: LOW
- Changes are well-isolated to specific methods
- Follow existing patterns (useStealthChop config)
- No breaking changes to API
- Simple state machine (one flag)
- Fixes are based on clear root cause analysis

**Critical Discovery**: The buzzing issue root cause was identified during requirements gathering - calling `stepper->run()` unnecessarily when motor is already at target. This makes the fix straightforward and reliable.
