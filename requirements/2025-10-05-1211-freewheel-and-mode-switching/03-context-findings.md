# Context Findings

## Current Implementation Analysis

### 1. Freewheel Implementation Status

#### Current Locations Where Freewheel is Applied
Based on grep results for `digitalWrite(EN_PIN, HIGH)`:

1. **Line 56** (`MotorController.cpp`): `digitalWrite(EN_PIN, HIGH); // Disable driver until movement`
   - Location: `begin()` initialization
   - Purpose: Motor starts disabled

2. **Line 108** (`MotorController.cpp`): `digitalWrite(EN_PIN, LOW); // Enable motor`
   - Location: `moveTo()` method
   - Purpose: Enable motor when starting movement

3. **Line 129** (`MotorController.cpp`): `digitalWrite(EN_PIN, HIGH); // Disable motor => freewheel`
   - Location: `jogStop()` method
   - Purpose: **WORKS** - Freewheel after jog stop

4. **Line 138** (`MotorController.cpp`): `digitalWrite(EN_PIN, HIGH); // Disable motor => freewheel`
   - Location: `emergencyStop()` method
   - Purpose: **WORKS** - Freewheel after emergency stop

5. **Line 237** (`MotorController.cpp`): `digitalWrite(EN_PIN, HIGH); // Disable motor = freewheel`
   - Location: `update()` method (inside emergencyStopActive check)
   - Purpose: Keep motor disabled during emergency stop

#### Missing Locations (User's Complaint)
The user reports freewheel does NOT work after:
- **Slider movements** (position slider in webapp)
- **Quick positions** (preset position buttons)

Both of these use the `moveTo()` command, which:
1. Enables the motor: `digitalWrite(EN_PIN, LOW)` (line 108)
2. Sets target position and speed
3. Returns immediately (movement happens in `update()` loop via `stepper->run()`)
4. **NEVER disables motor when movement completes**

#### Root Cause
There is **no code that checks when movement completes** and applies freewheel. The `update()` method (lines 225-243) only:
- Calculates encoder speed
- Updates TMC mode
- Calls `stepper->run()` (which handles movement)
- Checks emergency stop

**Missing**: Check if `stepper->distanceToGo() == 0` (movement complete) and apply freewheel.

---

### 2. Motor Buzzing/Heating Issue

#### TMC2209 Hold Current Configuration
Current setting (line 71, `MotorController.cpp`):
```cpp
driver->ihold(1);
```

**Analysis**:
- `ihold(1)` sets hold current to 1/32 of RMS current
- RMS current is set to 2000mA (line 69)
- Hold current = 2000 / 32 = **62.5mA**

**Factory example** (line 263, `factory-example.cpp`):
```cpp
driver.ihold(1);
```
Same setting, so this is intentional.

#### Why Motor Still Buzzes/Heats
Even with low hold current (1/32), the motor is:
1. **Enabled** (EN_PIN = LOW)
2. **Actively microstepping** to maintain position
3. **TMC2209 is energized** and generating small PWM pulses

**Hypothesis**: The hold current is too low to smoothly maintain position, causing micro-vibrations (buzzing). The driver is working hard to maintain position with insufficient current, causing heat.

**Possible Solutions**:
1. **Increase ihold**: From 1 to higher value (2-10) for smoother holding
2. **Add iholddelay**: Configure how long to wait before reducing to hold current
3. **Implement freewheel**: When freewheel enabled, disable motor entirely (`digitalWrite(EN_PIN, HIGH)`)

---

### 3. StealthChop/SpreadCycle Mode Switching Bug

#### Current Implementation (lines 200-211, `MotorController.cpp`)
```cpp
void MotorController::updateTMCMode()
{
    float currentSpeedPercent = abs(motorSpeed) / (float)config.getMaxSpeed();
    bool shouldUseStealthChop = currentSpeedPercent < STEALTH_CHOP_THRESHOLD;

    if (shouldUseStealthChop != useStealthChop)
    {
        useStealthChop = shouldUseStealthChop;
        driver->en_spreadCycle(!useStealthChop);
        LOG_DEBUG("TMC mode switched to %s", useStealthChop ? "StealthChop" : "SpreadCycle");
    }
}
```

**Variables Used**:
- `motorSpeed` (static float, line 25) - Should be set somewhere but **never is**
- `config.getMaxSpeed()` - Returns configured max speed
- `STEALTH_CHOP_THRESHOLD = 0.5` (line 33) - Switch at 50% speed

**Root Cause**:
`motorSpeed` is declared as `static float motorSpeed = 0;` but **is never assigned a value anywhere in the codebase**. It stays at 0 permanently.

**Consequence**:
```
currentSpeedPercent = abs(0) / maxSpeed = 0
shouldUseStealthChop = 0 < 0.5 = true (always)
```
Mode never switches because speed is always calculated as 0%.

#### User's Discovery Answer
> "Encoder currently does not report any useful data due to hardware mounting issues"

This confirms the encoder-based speed (`monitorSpeed` from `calculateSpeed()`) is not reliable.

**Solution**: Use AccelStepper's internal speed tracking instead of encoder:
- `stepper->speed()` returns current commanded speed in steps/sec
- This represents the actual speed the motor is being commanded to move at
- Works even without encoder feedback

---

### 4. Configuration System Analysis

#### Current MotorConfig Structure (Configuration.h, lines 13-20)
```cpp
struct MotorConfig
{
    long acceleration;
    long maxSpeed;
    long limitPos1;
    long limitPos2;
    bool useStealthChop;
} motorConfig;
```

**Missing**: No `freewheelAfterMove` or similar boolean field.

#### NVRAM Persistence (Configuration.cpp, lines 39-46)
```cpp
void Configuration::saveConfiguration() {
    preferences.putLong("acceleration", motorConfig.acceleration);
    preferences.putLong("maxSpeed", motorConfig.maxSpeed);
    preferences.putLong("limitPos1", motorConfig.limitPos1);
    preferences.putLong("limitPos2", motorConfig.limitPos2);
    preferences.putBool("stealthChop", motorConfig.useStealthChop);
    LOG_INFO("Configuration saved");
}
```

**Pattern**: All config fields saved to ESP32 Preferences (NVRAM).

**Required Changes**:
1. Add `bool freewheelAfterMove` field to struct
2. Add load/save logic in `loadConfiguration()` and `saveConfiguration()`
3. Add getter: `getFreewheelAfterMove()`
4. Add setter: `setFreewheelAfterMove(bool value)`

---

### 5. WebApp Configuration UI

#### MotorConfigDialog Component (MotorConfigDialog.tsx)
Current form fields (lines 24-29):
- `maxSpeed` - editable number input
- `acceleration` - editable number input
- `useStealthChop` - switch toggle
- `minLimit` - editable number input
- `maxLimit` - editable number input

**Pattern to Follow** (lines 27, 42, 134):
```typescript
const [useStealthChop, setUseStealthChop] = useState(currentConfig.useStealthChop)

// In useEffect reset:
setUseStealthChop(currentConfig.useStealthChop)

// In hasChanges check:
useStealthChop !== currentConfig.useStealthChop
```

**Required Changes**:
1. Add `freewheelAfterMove` boolean field to `MotorConfig` type (types/index.ts)
2. Add state: `const [freewheelAfterMove, setFreewheelAfterMove] = useState(...)`
3. Add Switch component in dialog form
4. Add to reset logic in useEffect
5. Add to hasChanges comparison
6. Include in onApply changes object

---

## Files Requiring Modification

### Backend (ESP32)
1. **src/modules/Configuration/Configuration.h**
   - Add `bool freewheelAfterMove` to `MotorConfig` struct
   - Add getter/setter methods

2. **src/modules/Configuration/Configuration.cpp**
   - Add default value in constructor
   - Add load/save in `loadConfiguration()` and `saveConfiguration()`
   - Implement setter method

3. **src/modules/MotorController/MotorController.h**
   - Possibly add `isMotionComplete()` helper method (optional)

4. **src/modules/MotorController/MotorController.cpp**
   - Fix `updateTMCMode()` to use `stepper->speed()` instead of `motorSpeed`
   - Add movement completion detection in `update()`
   - Apply freewheel when motion complete (based on config setting)
   - Possibly increase `ihold()` value to reduce buzzing

5. **src/modules/WebServer/WebServer.cpp**
   - Add `freewheelAfterMove` to `/api/config` GET response
   - Handle `freewheelAfterMove` in `setConfig` command

### Frontend (WebApp)
1. **webapp/src/types/index.ts**
   - Add `freewheelAfterMove: boolean` to `MotorConfig` interface

2. **webapp/src/components/MotorConfig/MotorConfigDialog.tsx**
   - Add state for `freewheelAfterMove`
   - Add Switch component in form
   - Add to validation/reset/apply logic

---

## Similar Patterns in Codebase

### Pattern 1: Boolean Config Setting with Switch UI
**useStealthChop** follows this exact pattern:
- Backend: `bool useStealthChop` in Configuration
- Frontend: Switch component in MotorConfigDialog
- Persistence: Saved to NVRAM with `preferences.putBool()`
- WebSocket: Sent/received in config messages

**Freewheel should follow identical pattern**.

### Pattern 2: Movement Completion Detection
**No existing pattern** - This is new functionality needed.

Closest example is `isMoving()` method (MotorController.h:68):
```cpp
bool isMoving() const { return stepper->distanceToGo() != 0; }
```

Can use this in `update()` to detect completion:
```cpp
static bool wasMoving = false;
bool isMoving = stepper->distanceToGo() != 0;

if (wasMoving && !isMoving) {
    // Movement just completed
    if (config.getFreewheelAfterMove()) {
        digitalWrite(EN_PIN, HIGH);
    }
}
wasMoving = isMoving;
```

---

## Technical Constraints

### 1. TMC2209 ihold Range
- Valid range: 0-31
- Current setting: 1 (lowest non-zero)
- Factory example: 1 (same)
- **Recommendation**: Try 2-5 for testing (still low but may reduce buzzing)

### 2. AccelStepper Speed Tracking
- `stepper->speed()` returns **signed float** in steps/sec
- Returns current instantaneous speed (not target)
- During acceleration: returns intermediate values
- At constant speed: returns target speed
- During deceleration: returns decreasing values
- **Use `abs(stepper->speed())` for mode switching comparison**

### 3. WebSocket Protocol Compatibility
Current `setConfig` command format (WebServer.cpp, lines 317-362):
```json
{
  "command": "setConfig",
  "maxSpeed": 14400,
  "acceleration": 80000,
  "useStealthChop": true
}
```

**Adding freewheelAfterMove**:
```json
{
  "command": "setConfig",
  "freewheelAfterMove": true
}
```

Pattern already handles optional fields (lines 321-349), so adding new field is straightforward.

---

## Edge Cases to Consider

### Edge Case 1: Freewheel During Emergency Stop
**Current**: Emergency stop always freewheels (line 138, 237)
**Desired**: Emergency stop should always freewheel regardless of config
**Solution**: Keep current behavior, only apply config-based freewheel for normal movement completion

### Edge Case 2: Jog Stop Behavior
**Current**: `jogStop()` always freewheels (line 129)
**Question**: Should jog respect freewheel config or always freewheel?
**Recommendation**: Jog should respect config for consistency

### Edge Case 3: Movement Interrupted by New Command
**Scenario**: Motor moving to position A, new `moveTo()` command for position B
**Current**: New command overrides old (AccelStepper behavior)
**Question**: Should interrupted movement trigger freewheel?
**Recommendation**: No, only trigger freewheel when motion naturally completes (distanceToGo reaches 0)

### Edge Case 4: Rapid Position Changes
**Scenario**: User rapidly sends multiple position commands
**Risk**: Movement completion detection fires multiple times
**Solution**: Use state machine pattern (wasMoving flag) to detect transitions only

---

## Testing Strategy

### Unit Testing (Not Applicable)
- Encoder speed calculation already has unit tests (test/test_native/test_motor_calculations/)
- New functionality requires hardware (TMC2209 driver state)

### Integration Testing (Hardware Required)

**Test 1: Freewheel After Slider Movement**
1. Enable freewheel in config dialog
2. Use position slider to move motor
3. Wait for movement to complete
4. Verify motor freewheels (can turn shaft by hand)

**Test 2: Hold After Slider Movement**
1. Disable freewheel in config dialog
2. Use position slider to move motor
3. Wait for movement to complete
4. Verify motor holds position (cannot turn shaft by hand)
5. Verify no buzzing or excessive heating

**Test 3: Mode Switching During Movement**
1. Configure max speed to 10,000 steps/sec
2. Move motor at 3,000 steps/sec (below 50% threshold)
3. Verify logs show StealthChop active
4. Move motor at 7,000 steps/sec (above 50% threshold)
5. Verify logs show switch to SpreadCycle
6. Move motor at 3,000 steps/sec again
7. Verify logs show switch back to StealthChop

**Test 4: Jog with Freewheel Enabled**
1. Enable freewheel
2. Hold jog forward button
3. Release button
4. Verify motor freewheels immediately

**Test 5: Jog with Freewheel Disabled**
1. Disable freewheel
2. Hold jog forward button
3. Release button
4. Verify motor holds position after jog

---

## Related Features

### Similar Config Settings
- `useStealthChop` - Boolean toggle for TMC mode (similar to freewheelAfterMove)
- `maxSpeed` / `acceleration` - Numeric config with validation (different from freewheel)

### Similar Movement Logic
- `jogStop()` - Already implements freewheel logic
- `emergencyStop()` - Already implements freewheel logic
- `moveTo()` - Needs freewheel logic added for completion

---

## Key Insights for Implementation

1. **Freewheel is trivial to implement** - Just `digitalWrite(EN_PIN, HIGH)` at the right time
2. **Challenge is detecting movement completion** - Need state machine in `update()`
3. **Mode switching is already implemented** - Just using wrong speed variable
4. **Configuration pattern is well-established** - Follow useStealthChop example exactly
5. **UI already has Switch component** - Easy to add freewheel toggle
6. **Hold current tuning may help buzzing** - Test ihold values 2-5

---

## Summary

### Bug 1: Freewheel Not Working (Slider/Quick Positions)
**Root Cause**: No code checks for movement completion
**Fix**: Add state machine in `update()` to detect when `distanceToGo() == 0`

### Bug 2: Motor Buzzing When Not Freewheeling
**Root Cause**: Low hold current (ihold=1) may cause micro-vibrations
**Fix**: Experiment with ihold values 2-5 for smoother holding

### Bug 3: Mode Switching Not Logging
**Root Cause**: `motorSpeed` variable never assigned, always 0
**Fix**: Use `stepper->speed()` instead of `motorSpeed` for speed percentage calculation

### Feature: Configurable Freewheel
**Implementation**: Follow exact pattern of `useStealthChop` boolean config setting
**Backend**: Add to Configuration struct + NVRAM persistence
**Frontend**: Add Switch to MotorConfigDialog
