# Requirements Specification: Auto-Calibration Feature

**Project:** LilyGo Motion Controller
**Feature:** Manual Auto-Calibration
**Date:** 2025-11-22
**Status:** Ready for Implementation

---

## 1. Problem Statement

The LilyGo Motion Controller currently requires manual positioning to learn limit switch positions. Users must manually move the motor to each limit switch and trigger them individually. This process is time-consuming and error-prone.

## 2. Solution Overview

Implement a **manual auto-calibration feature** that automatically moves the motor to both limit switches in sequence, learns their positions, and stores them in NVRAM. The calibration is triggered by a "Recalibrate" button in the webapp's Settings dialog.

### Key Characteristics
- **Manual trigger only:** No automatic calibration on power-on (user can initiate when needed)
- **Two-step sequence:** Move to MIN limit → Move to MAX limit
- **Safety-first:** Emergency stop always available, all manual commands blocked during calibration
- **Real-time feedback:** Status updates shown in webapp during calibration process
- **Persistent storage:** Limit positions saved to ESP32 NVRAM automatically via existing mechanism

---

## 3. Functional Requirements

### FR-1: Calibration Initiation
- User clicks "Recalibrate" button in Settings dialog (webapp)
- System sends `calibrate` command via WebSocket
- Firmware validates request and starts calibration routine
- **Precondition:** Motor must not be in emergency stop state
- **Success criteria:** Calibration state machine starts, button becomes disabled

### FR-2: Calibration Sequence
1. **Phase 1 - Find MIN Limit:**
   - Motor moves backward toward MIN limit at calibration speed (30% of maxSpeed)
   - Status broadcast: "Calibrating: Finding lower limit"
   - When MIN limit switch triggers:
     - Motor stops immediately (via existing `jogStop()`)
     - Current position saved as MIN limit (existing limit switch mechanism)
     - Transition to Phase 2

2. **Phase 2 - Find MAX Limit:**
   - Motor moves forward toward MAX limit at calibration speed
   - Status broadcast: "Calibrating: Finding upper limit"
   - When MAX limit switch triggers:
     - Motor stops immediately
     - Current position saved as MAX limit
     - Transition to completion

3. **Phase 3 - Completion:**
   - Calibration state cleared
   - Status broadcast: "Calibration complete"
   - Configuration broadcast with updated min/max limits
   - Motor remains at MAX position (end of sequence)

### FR-3: Calibration Speed
- Speed calculated as: `calibrationSpeed = config.getMaxSpeed() * 0.3`
- Adaptive to user's motor configuration
- Conservative 30% factor ensures safe operation across different motor capabilities

### FR-4: Movement Blocking During Calibration
- All manual movement commands blocked while calibration in progress:
  - Jog controls (forward/backward)
  - Position slider movements
  - WebSocket `move` commands
- Blocked commands return error: "Cannot move - calibration in progress"
- **Exception:** Emergency stop always functional

### FR-5: Emergency Stop Behavior
- Emergency stop interrupts calibration at any phase
- Calibration state cleared completely (abort, no resume)
- User must manually restart calibration via "Recalibrate" button
- Emergency stop flag must be cleared before new calibration can start

### FR-6: Real-Time Status Feedback
- Webapp displays calibration progress in Motor Status card's "State" field:
  - "Calibrating: Finding lower limit" (Phase 1)
  - "Calibrating: Finding upper limit" (Phase 2)
  - "Calibration complete" (Phase 3)
  - "Idle" (normal operation)
- Status updates broadcast via existing WebSocket status messages

### FR-7: Settings Dialog UI
- Add "Recalibrate" button to MotorConfigDialog component
- Button placement: Below limit position inputs, before footer buttons
- Button states:
  - **Enabled:** When connected and not calibrating
  - **Disabled with "Calibrating...":** During calibration
  - **Disabled when disconnected:** No WebSocket connection
- Visual styling: Consistent with existing button patterns

### FR-8: Configuration Persistence
- Limit positions automatically saved to NVRAM when limit switches trigger (existing mechanism)
- No additional persistence logic needed for calibration
- Configuration broadcast after calibration includes updated limits

---

## 4. Technical Requirements

### TR-1: Firmware Architecture

#### MotorController Module Changes
**File:** `src/modules/MotorController/MotorController.h` and `.cpp`

**New Members:**
```cpp
enum CalibrationState {
    CAL_IDLE,
    CAL_FINDING_MIN,
    CAL_FINDING_MAX,
    CAL_COMPLETE
};

private:
    CalibrationState calibrationState;
    long calibrationSpeed;

public:
    void startCalibration();
    void stopCalibration();  // Called on emergency stop
    bool isCalibrating() const { return calibrationState != CAL_IDLE; }
    CalibrationState getCalibrationState() const { return calibrationState; }
```

**Calibration Logic:**
```cpp
void MotorController::startCalibration()
{
    if (emergencyStopActive) {
        LOG_WARN("Cannot start calibration - emergency stop active");
        return;
    }

    calibrationState = CAL_FINDING_MIN;
    calibrationSpeed = config.getMaxSpeed() * 0.3;

    // Move toward MIN limit (large negative position)
    moveTo(-999999, calibrationSpeed);

    LOG_INFO("Calibration started - finding MIN limit at speed %ld", calibrationSpeed);
}
```

**State Machine in update():**
- Monitor `calibrationState`
- Use limit switch callbacks to detect when each limit is reached
- Transition between states automatically
- Broadcast status on each state change

**Movement Blocking:**
```cpp
void MotorController::moveTo(long position, int speed)
{
    if (isCalibrating()) {
        LOG_WARN("Cannot move - calibration in progress");
        return;
    }
    // ... existing logic
}
```

#### LimitSwitch Callback Integration
**File:** `src/modules/LimitSwitch/LimitSwitch.cpp`

- Existing callback mechanism: `setLimitCallback(LimitSwitchCallback callback)`
- MotorController registers callback during `begin()`
- Callback triggers calibration state transitions:
  - MIN limit pressed while `CAL_FINDING_MIN` → transition to `CAL_FINDING_MAX`
  - MAX limit pressed while `CAL_FINDING_MAX` → transition to `CAL_COMPLETE`

#### WebServer Command Handler
**File:** `src/modules/WebServer/WebServer.cpp`

**New Command Handler:**
```cpp
void WebServerClass::handleCalibrateCommand(JsonDocument& doc)
{
    if (!motorController.isEmergencyStopActive())
    {
        motorController.startCalibration();
        LOG_INFO("Calibration initiated via WebSocket");
        broadcastStatus();
    }
    else
    {
        ws.textAll("{\"type\":\"error\",\"message\":\"Cannot calibrate: emergency stop active\"}");
    }
}
```

**Command Routing:**
```cpp
else if (command == "calibrate") {
    handleCalibrateCommand(doc);
}
```

**Status Broadcast Enhancement:**
```cpp
doc["isCalibrating"] = motorController.isCalibrating();
doc["calibrationState"] = getCalibrationStateString(motorController.getCalibrationState());
```

Helper function for state string:
```cpp
const char* getCalibrationStateString(CalibrationState state) {
    switch(state) {
        case CAL_FINDING_MIN: return "Calibrating: Finding lower limit";
        case CAL_FINDING_MAX: return "Calibrating: Finding upper limit";
        case CAL_COMPLETE: return "Calibration complete";
        default: return "Idle";
    }
}
```

### TR-2: Webapp Architecture

#### TypeScript Types
**File:** `webapp/src/types/index.ts`

**Add to MotorStatus interface:**
```typescript
export interface MotorStatus {
  // ... existing fields
  isCalibrating?: boolean;
  calibrationState?: string;
}
```

**New command type:**
```typescript
export interface CalibrateCommand {
  command: 'calibrate';
}

export type ControlCommand =
  | MoveCommand
  | EmergencyStopCommand
  | CalibrateCommand  // Add here
  // ... other commands
```

#### useMotorController Hook
**File:** `webapp/src/hooks/useMotorController.tsx`

**New method:**
```typescript
const startCalibration = useCallback(() => {
  return sendCommand({ command: 'calibrate' })
}, [sendCommand])

// Add to return object:
return {
  // ... existing
  startCalibration
}
```

#### MotorConfigDialog Component
**File:** `webapp/src/components/MotorConfig/MotorConfigDialog.tsx`

**Props update:**
```typescript
interface MotorConfigDialogProps {
  // ... existing props
  onCalibrate: () => void;
  isCalibrating: boolean;
}
```

**UI Addition (before DialogFooter):**
```tsx
{/* Calibration Section */}
<div className="grid gap-2 pt-4 border-t">
  <Label>Calibration</Label>
  <Button
    variant="outline"
    onClick={onCalibrate}
    disabled={!isConnected || isCalibrating}
    className="w-full"
  >
    {isCalibrating ? 'Calibrating...' : 'Recalibrate'}
  </Button>
  <p className="text-xs text-muted-foreground">
    Automatically move motor to both limits and update positions. Motor will move to minimum first, then maximum.
  </p>
</div>
```

#### App.tsx Integration
**File:** `webapp/src/App.tsx`

**Pass new props to dialog:**
```tsx
<MotorConfigDialog
  // ... existing props
  onCalibrate={startCalibration}
  isCalibrating={motorStatus.isCalibrating || false}
/>
```

#### MotorStatus Display
**File:** `webapp/src/components/MotorControl/MotorStatus.tsx`

**State field logic:**
```typescript
const getStateText = () => {
  if (motorStatus.calibrationState && motorStatus.calibrationState !== 'Idle') {
    return motorStatus.calibrationState;  // Show calibration status
  }
  if (motorStatus.emergencyStop) return 'Emergency Stop';
  if (motorStatus.isMoving) return 'Moving';
  return 'Idle';
}
```

---

## 5. Implementation Hints

### Firmware Implementation Order
1. Add calibration state enum and members to MotorController.h
2. Implement `startCalibration()` and `stopCalibration()` in MotorController.cpp
3. Add state machine logic to `MotorController::update()`
4. Register limit switch callbacks in `MotorController::begin()`
5. Add movement blocking check to `moveTo()`, `jogStart()` commands
6. Add `handleCalibrateCommand()` to WebServer.cpp
7. Enhance status broadcast to include calibration state
8. Test with serial logging before webapp integration

### Webapp Implementation Order
1. Add types to `webapp/src/types/index.ts`
2. Add `startCalibration()` to useMotorController hook
3. Update MotorConfigDialog props and add Recalibrate button
4. Update App.tsx to pass calibration props
5. Update MotorStatus to display calibration state
6. Test with browser console logging

### Testing Strategy
1. **Unit Test:** Calibration state machine transitions
2. **Integration Test:** Limit switch callbacks triggering state changes
3. **Manual Test:** Full calibration sequence via webapp
4. **Safety Test:** Emergency stop during each calibration phase
5. **Edge Case Test:** Starting calibration with emergency stop active

---

## 6. Acceptance Criteria

### AC-1: Calibration Initiation
- [ ] "Recalibrate" button visible in Settings dialog
- [ ] Button disabled when not connected or already calibrating
- [ ] Clicking button sends `calibrate` command via WebSocket
- [ ] Firmware starts calibration and broadcasts status

### AC-2: Calibration Sequence Execution
- [ ] Motor moves to MIN limit first at 30% of maxSpeed
- [ ] MIN limit position saved to NVRAM when switch triggers
- [ ] Motor automatically transitions to MAX limit search
- [ ] MAX limit position saved to NVRAM when switch triggers
- [ ] Calibration completes and state clears

### AC-3: Status Feedback
- [ ] Webapp shows "Calibrating: Finding lower limit" during Phase 1
- [ ] Webapp shows "Calibrating: Finding upper limit" during Phase 2
- [ ] Webapp shows "Calibration complete" briefly after completion
- [ ] Status card returns to normal "Idle"/"Moving" display

### AC-4: Movement Blocking
- [ ] Jog controls disabled during calibration
- [ ] Position slider movements rejected during calibration
- [ ] WebSocket `move` commands return error during calibration
- [ ] Normal operation resumes after calibration completes

### AC-5: Emergency Stop Handling
- [ ] Emergency stop button remains functional during calibration
- [ ] Clicking emergency stop aborts calibration immediately
- [ ] Calibration state cleared on emergency stop
- [ ] User can restart calibration after clearing emergency stop

### AC-6: Configuration Persistence
- [ ] Limit positions saved to NVRAM during calibration
- [ ] Positions persist across power cycles
- [ ] Webapp receives updated config after calibration
- [ ] Settings dialog shows new limit values

### AC-7: Edge Cases
- [ ] Cannot start calibration when emergency stop active
- [ ] Cannot start calibration when already calibrating
- [ ] Calibration aborts if limit switch fails to trigger (timeout needed?)
- [ ] System recovers gracefully from unexpected interruptions

---

## 7. Assumptions

### A-1: Hardware Behavior
- Limit switches are properly installed and functional
- Limit switches will trigger before motor reaches mechanical end stops
- Motor can safely travel full range at 30% of configured maxSpeed

### A-2: Existing Functionality
- Limit switch position saving mechanism works correctly
- WebSocket communication is reliable
- Emergency stop functionality is tested and working
- Configuration persistence (NVRAM) is reliable

### A-3: User Behavior
- User initiates calibration only when motor is in safe position
- User does not physically obstruct motor during calibration
- User understands emergency stop will abort calibration

### A-4: System State
- WiFi connection stable during calibration (WebSocket remains connected)
- Power supply stable during calibration (no brownouts)
- No concurrent firmware operations that could interfere

---

## 8. Out of Scope

The following are explicitly NOT part of this feature:

### OS-1: Automatic Calibration on Power-On
- Initially requested but user decided against it
- Calibration is manual-trigger only

### OS-2: Calibration Resume After Emergency Stop
- Considered but rejected in favor of manual restart
- Simpler state machine, clearer user intent

### OS-3: Calibration Timeout Handling
- Not explicitly requested, may be added later
- Could add timeout if limit switch never triggers

### OS-4: Configurable Calibration Speed
- Fixed at 30% of maxSpeed
- Could be made configurable later if needed

### OS-5: Mid-Calibration Position Setting
- No ability to set current position as limit during calibration
- Only automatic limit switch detection

### OS-6: Calibration Direction Selection
- Always MIN first, then MAX
- Could add option to reverse if needed later

---

## 9. Success Metrics

### Usability Metrics
- Time to complete calibration: < 30 seconds for typical motor range
- User errors during calibration: 0 (process is fully automated)
- Calibration accuracy: Matches manual limit switch triggering

### Technical Metrics
- Code complexity: < 100 lines of new firmware code
- WebSocket message overhead: < 5 additional status broadcasts per calibration
- NVRAM writes: Exactly 2 (one per limit switch)
- Memory footprint increase: < 1KB

### Reliability Metrics
- Calibration success rate: 100% under normal conditions
- Emergency stop response time: < 100ms (existing mechanism)
- Configuration persistence reliability: 100% (existing mechanism)

---

## 10. Future Enhancements

Potential features to consider after initial implementation:

### FE-1: Calibration Timeout
- Abort calibration if limit switch not reached within X seconds
- Prevents infinite movement if limit switch fails

### FE-2: Calibration Speed Configuration
- Allow user to adjust calibration speed percentage
- UI: Slider in Settings dialog (10% - 50% range)

### FE-3: Reverse Calibration Direction
- Option to start with MAX limit instead of MIN
- UI: Toggle switch in Settings dialog

### FE-4: Calibration History
- Log calibration timestamps and results
- Display last calibration date in Settings dialog

### FE-5: Pre-Calibration Position Check
- Warn user if current position is near limit
- Suggest starting from middle of range

### FE-6: Calibration Verification
- Optionally move to center position after calibration
- Verify motor can move freely across full range

---

## 11. File Modification Checklist

### Firmware Files to Modify
- [x] `src/modules/MotorController/MotorController.h` - Add state enum, members, methods
- [x] `src/modules/MotorController/MotorController.cpp` - Implement calibration logic
- [x] `src/modules/WebServer/WebServer.h` - Add calibrate command handler declaration
- [x] `src/modules/WebServer/WebServer.cpp` - Implement command handler, update status broadcast
- [ ] No changes needed: `src/modules/LimitSwitch/*` (use existing callback mechanism)
- [ ] No changes needed: `src/modules/Configuration/*` (use existing save mechanism)

### Webapp Files to Modify
- [x] `webapp/src/types/index.ts` - Add CalibrateCommand, update MotorStatus
- [x] `webapp/src/hooks/useMotorController.tsx` - Add startCalibration method
- [x] `webapp/src/components/MotorConfig/MotorConfigDialog.tsx` - Add Recalibrate button
- [x] `webapp/src/App.tsx` - Pass calibration props to dialog
- [x] `webapp/src/components/MotorControl/MotorStatus.tsx` - Display calibration state

### Files to Create
- [ ] None (all changes are additions to existing files)

### Files to Review (No Changes Expected)
- [x] `src/main.cpp` - Verify no init changes needed
- [x] `src/util.h` - Verify logging macros sufficient
- [x] `platformio.ini` - No library changes needed

---

## 12. Risk Assessment

### High Risk (Likelihood: Low, Impact: High)
**R-1: Motor Crashes During Calibration**
- **Mitigation:** Use conservative 30% speed, rely on limit switches
- **Fallback:** Emergency stop always available

**R-2: Limit Switch Fails to Trigger**
- **Mitigation:** Future enhancement - add timeout
- **Fallback:** Emergency stop manually

### Medium Risk (Likelihood: Medium, Impact: Medium)
**R-3: WebSocket Disconnects During Calibration**
- **Mitigation:** Calibration continues on firmware, status updates lost
- **Impact:** User loses visibility but motor still stops at limits
- **Recovery:** Refresh webapp reconnects and shows final result

**R-4: User Starts Calibration from Wrong Position**
- **Mitigation:** Documentation, conservative speed
- **Impact:** Extra travel time, no damage expected
- **Recovery:** Emergency stop if needed

### Low Risk (Likelihood: Low, Impact: Low)
**R-5: Calibration Button Double-Click**
- **Mitigation:** Button disabled during calibration
- **Impact:** None (prevented by UI)

---

## 13. Dependencies

### Internal Dependencies
- MotorController emergency stop mechanism (existing)
- LimitSwitch interrupt and callback system (existing)
- Configuration NVRAM persistence (existing)
- WebSocket status broadcasting (existing)

### External Dependencies
- AccelStepper library (existing dependency)
- ESPAsyncWebServer library (existing dependency)
- React + TypeScript webapp (existing)

### No New Dependencies Required
- All functionality uses existing libraries and patterns

---

## 14. Documentation Updates Needed

### Code Documentation
- Add docstring comments to new calibration methods
- Update MotorController class header comment
- Document calibration state machine in comments

### User Documentation
- Update README.md with calibration feature description
- Add calibration instructions to user guide (if exists)
- Document emergency stop behavior during calibration

### Developer Documentation
- Update CLAUDE.md with calibration implementation details
- Document calibration state machine diagram
- Add calibration to architecture overview

---

**END OF REQUIREMENTS SPECIFICATION**

This document is ready for implementation. All questions have been answered, all patterns identified, and all technical details specified. The implementation can proceed following the order outlined in Section 5.
