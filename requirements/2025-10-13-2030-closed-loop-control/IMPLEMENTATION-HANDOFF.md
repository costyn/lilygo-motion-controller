# Implementation Handoff Note

**Date:** 2025-10-13
**Next Session:** Implementation of Closed-Loop Control

## Quick Start for Implementation Session

### 1. Read This First
Start by reading the **requirements specification**:
```
requirements/2025-10-13-2030-closed-loop-control/06-requirements-spec.md
```

This 400+ line document contains everything needed:
- Complete class structure with code examples
- All algorithms (rotation tracking, deadband correction, encoder health)
- Integration points with existing code
- Testing strategy and acceptance criteria

### 2. Key Implementation Facts

**What to Build:**
- New module: `src/modules/ClosedLoopController/` (ClosedLoopController.h/cpp)
- Dedicated FreeRTOS task running at 20ms intervals
- Multi-turn encoder position tracking with rotation counter
- Deadband-based position correction (NOT full PID)

**Control Algorithm Summary:**
```cpp
// Read encoder, track rotations, convert to steps
encoderSteps = getMultiTurnEncoderPositionSteps();
commandedSteps = motorController.getCurrentPosition();
error = commandedSteps - encoderSteps;

// Only correct if motor enabled AND error > threshold
if (motorEnabled && abs(error) > DEADBAND_THRESHOLD) {
    correction = Kp * error;
    motorController.moveTo(commandedSteps + correction);
}

// On motor enable transition: sync AccelStepper to encoder
if (motorEnabled && !wasEnabled) {
    motorController.setCurrentPosition(encoderSteps);
}
```

**Key Parameters:**
- Update rate: 20ms (50Hz)
- Deadband threshold: 2-5° (~100 steps)
- Proportional gain: Kp = 0.5
- Reset rotation counter on boot (no NVRAM)

### 3. Implementation Order (from spec)

**Phase 1 - Module Skeleton:**
1. Create `src/modules/ClosedLoopController/ClosedLoopController.h`
2. Create `src/modules/ClosedLoopController/ClosedLoopController.cpp`
3. Copy class structure from requirements spec TR1
4. Add basic constructor and `begin()` method

**Phase 2 - Core Logic:**
1. Implement `readEncoderRaw()` - call `motorController.readEncoder()`
2. Implement `updateRotationCounter()` - detect wrap-around (see Pattern 1 in spec)
3. Implement `getMultiTurnEncoderPositionSteps()` - coordinate conversion (see TR5)
4. Implement `update()` method - main control loop (see Pattern 2)

**Phase 3 - FreeRTOS Integration:**
1. Add `#include "modules/ClosedLoopController/ClosedLoopController.h"` to main.cpp
2. Call `closedLoopController.begin()` in setup()
3. Create `ClosedLoopTask` function (see TR2 in spec)
4. Call `xTaskCreatePinnedToCore()` in setup()

**Phase 4 - MotorController Changes:**
1. Add `isMotorEnabled()` method to MotorController.h
2. Optionally make ClosedLoopController a friend class OR add public `readEncoder()` wrapper
3. Remove/deprecate `calculateSpeed()` method (no longer needed)

**Phase 5 - WebSocket Updates:**
1. Modify WebServer.cpp status broadcast
2. Add encoder position, commanded position, control mode fields (see TR4)

**Phase 6 - Encoder Health Monitoring:**
1. Implement `checkEncoderHealth()` (see Pattern 3)
2. Add graceful degradation logic
3. Add periodic retry on encoder recovery

### 4. Files to Reference

**Existing code patterns to follow:**
- `src/modules/MotorController/MotorController.cpp:217-232` - TMC mode switching pattern
- `src/modules/LimitSwitch/LimitSwitch.cpp` - ISR and state management patterns
- `src/main.cpp` - FreeRTOS task creation examples (InputTask, WebServerTask)

**Configuration values:**
- Motor: 200 steps/rev × 16 microsteps = 3200 steps/rev
- Encoder: 14-bit MT6816 = 16384 counts/rev
- Conversion: `STEPS_PER_ENCODER_COUNT = 3200 / 16384 = 0.1953125`

### 5. Testing Checkpoints

After each phase, verify:
- ✅ Code compiles without errors
- ✅ No conflicts with existing functionality
- ✅ LOG_INFO messages show expected behavior
- ✅ WebSocket still responsive

**Final integration test:**
1. Move motor to position
2. Check WebSocket shows encoder position matches commanded
3. Manually rotate shaft while freewheeling
4. Verify encoder tracks movement
5. Command new movement, verify AccelStepper sync (no unexpected motion)

### 6. Important Constraints

- ⚠️ Preserve freewheel-after-movement feature (no buzzing when motor stopped)
- ⚠️ Only correct errors during powered movement (EN_PIN LOW)
- ⚠️ No changes to existing WebSocket commands (transparent to users)
- ⚠️ Must gracefully degrade if encoder fails
- ⚠️ Deadband prevents constant corrections (stability over precision)

### 7. Code Style Notes

- Use existing LOG_ERROR/WARN/INFO/DEBUG macros
- Follow modular architecture (separate class/module)
- Match existing patterns (see MotorController, LimitSwitch modules)
- Thread safety: Start simple, add mutex only if needed
- Comment complex algorithms (rotation wrap-around, coordinate conversions)

---

## If You Need Context

**Full requirements spec:**
`requirements/2025-10-13-2030-closed-loop-control/06-requirements-spec.md`

**Research and decisions:**
- `02-discovery-answers.md` - High-level design decisions
- `05-detail-answers.md` - Technical implementation choices
- `03-context-findings.md` - Background research on closed-loop control

**Quick summary:**
`requirements/2025-10-13-2030-closed-loop-control/README.md`

---

## Session Goal

Implement working closed-loop control with:
1. Multi-turn position tracking
2. Deadband-based error correction
3. Freewheel mode support
4. Graceful encoder failure handling
5. Clean FreeRTOS task integration

**Success Criteria:** Motor moves to commanded position, encoder confirms actual position matches, system tolerates small errors without buzzing, and gracefully handles encoder disconnection.

---

**Ready to code!** 🚀
