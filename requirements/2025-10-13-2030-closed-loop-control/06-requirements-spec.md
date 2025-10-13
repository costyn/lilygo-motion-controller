# Requirements Specification: Closed-Loop Stepper Motor Control

**Date:** 2025-10-13
**Project:** LilyGo Motion Controller
**Feature:** Transition from Open-Loop to Closed-Loop Control

---

## Executive Summary

This specification defines the implementation of closed-loop position control for the LilyGo Motion Controller using the existing MT6816 magnetic encoder (14-bit, 0.1° accuracy). The system will detect and correct missed steps during powered movement, track position accurately during freewheel mode, and gracefully degrade to open-loop control if encoder failures occur.

**Target Application:** 3D printer Z-axis style linear actuator with threaded rod (many rotations travel range)

---

## Problem Statement

The current system uses open-loop stepper motor control via AccelStepper library. This approach:
- Cannot detect missed steps caused by excessive load, binding, or incorrect tuning
- Loses position awareness when motor is freewheeling (disabled)
- Has no mechanism to verify commanded position matches actual position
- Cannot recover from mechanical interference or manual shaft movement

The MT6816 encoder is already integrated and functional, but only used for speed monitoring (`calculateSpeed()`) which provides no control feedback.

---

## Solution Overview

Implement a modular **ClosedLoopController** that:

1. **Continuously tracks encoder position** - Monitor actual shaft position in both powered and unpowered states
2. **Multi-turn position tracking** - Count full rotations beyond 360° for Z-axis applications
3. **Deadband-based error correction** - Apply gentle corrections only when error exceeds threshold (no buzzing)
4. **Freewheel-aware operation** - Track position during freewheel, sync AccelStepper on re-enable
5. **Graceful degradation** - Fall back to open-loop if encoder fails
6. **Transparent integration** - No changes to existing WebSocket API or user controls

---

## Functional Requirements

### FR1: Multi-Turn Encoder Position Tracking
- **Description**: Track absolute position across multiple motor rotations
- **Rationale**: Z-axis applications require many turns from min to max limit
- **Implementation**:
  - Maintain rotation counter (signed integer) to track full 360° turns
  - Detect encoder wrap-around transitions (0↔16384)
  - Calculate combined position: `(rotationCount × 16384) + encoderRawValue`
  - Convert encoder position to steps for comparison with AccelStepper
- **Initial State**: Rotation counter resets to 0 on boot (homing required)

### FR2: Continuous Position Monitoring
- **Description**: Read encoder position continuously regardless of motor state
- **Rationale**: Support both powered correction and freewheel tracking
- **Update Rate**: 20ms (50Hz) via dedicated FreeRTOS task
- **Operating Modes**:
  - **Motor Enabled (EN_PIN LOW)**: Active position monitoring with correction
  - **Motor Disabled (EN_PIN HIGH)**: Passive position tracking only

### FR3: Deadband-Based Position Correction
- **Description**: Apply position corrections only when error exceeds threshold
- **Rationale**: Avoid constant buzzing and motor heating from micro-corrections
- **Algorithm**:
  ```
  encoderPosition = getMultiTurnEncoderPosition()
  commandedPosition = stepper->currentPosition()
  error = commandedPosition - encoderPosition

  if (motorEnabled AND abs(error) > DEADBAND_THRESHOLD):
      correction = Kp * error  // Proportional correction
      stepper->moveTo(commandedPosition + correction)
  ```
- **Parameters**:
  - `DEADBAND_THRESHOLD`: 2-5° (configurable, default ~3° or 100 steps)
  - `Kp`: Proportional gain (default 0.5, tuned during testing)
- **Behavior**: Within deadband, no correction applied (silent operation)

### FR4: Freewheel Mode Position Tracking
- **Description**: Track encoder position when motor is disabled
- **Rationale**: Know actual position after manual adjustments or external forces
- **Implementation**:
  - Continue reading encoder and updating rotation counter
  - Do NOT apply corrections (motor is disabled)
  - Log significant position changes (>10° movement while freewheeling)
  - WebSocket broadcasts show actual encoder position

### FR5: AccelStepper Synchronization on Motor Enable
- **Description**: When motor re-enables, sync AccelStepper position to encoder
- **Rationale**: Accept physical reality, prevent unexpected "snap back" movements
- **Trigger**: Transition from EN_PIN HIGH → LOW
- **Algorithm**:
  ```
  if (motorBecomingEnabled):
      encoderPositionInSteps = convertEncoderToSteps(getMultiTurnEncoderPosition())
      stepper->setCurrentPosition(encoderPositionInSteps)
      LOG_INFO("AccelStepper synced to encoder: %ld steps", encoderPositionInSteps)
  ```

### FR6: Encoder Failure Detection and Graceful Degradation
- **Description**: Detect encoder malfunctions and continue operating in open-loop mode
- **Failure Conditions**:
  - SPI communication errors (timeout, invalid response)
  - Stuck encoder value (no change over 5 seconds while motor moving)
  - Impossible transitions (>180° change in 20ms)
- **Response**:
  1. Log warning: `"Encoder fault detected - switching to open-loop mode"`
  2. Disable closed-loop corrections
  3. Continue using AccelStepper in pure open-loop mode
  4. WebSocket status: `"encoderStatus": "fault", "controlMode": "open-loop"`
  5. Retry encoder health check every 10 seconds
- **Recovery**: If encoder recovers, log info and re-enable closed-loop

### FR7: Position Reporting
- **Description**: Report encoder-based actual position as primary position
- **WebSocket Status Broadcast**:
  ```json
  {
    "position": 12345,           // Encoder position in steps (primary)
    "commandedPosition": 12350,  // AccelStepper position (for debugging)
    "positionError": 5,          // Difference in steps (optional)
    "encoderStatus": "ok",       // "ok" | "fault"
    "controlMode": "closed-loop" // "closed-loop" | "open-loop"
  }
  ```
- **Behavior**: When closed-loop working correctly, `position` and `commandedPosition` match (within deadband)

### FR8: No User-Facing Configuration Changes
- **Description**: Closed-loop operates transparently
- **Rationale**: End users control position/speed only, not PID parameters
- **User Interface**: No changes to existing WebSocket commands or web UI
- **Parameters**: PID/deadband values are compile-time constants

---

## Technical Requirements

### TR1: ClosedLoopController Module
- **Location**: `src/modules/ClosedLoopController/`
- **Files**:
  - `ClosedLoopController.h` - Class definition and public interface
  - `ClosedLoopController.cpp` - Implementation
- **Class Structure**:
  ```cpp
  class ClosedLoopController {
  private:
      // Multi-turn tracking
      int32_t rotationCount;
      uint16_t lastEncoderRaw;

      // Position tracking
      long encoderPositionSteps;
      long lastCommandedPosition;

      // Control parameters
      const float DEADBAND_THRESHOLD = 3.0; // degrees
      const float Kp = 0.5; // Proportional gain

      // State management
      bool encoderHealthy;
      unsigned long lastEncoderChangeTime;
      bool motorWasEnabled;

      // Helper methods
      uint16_t readEncoderRaw();
      void updateRotationCounter(uint16_t currentRaw);
      long getMultiTurnEncoderPositionSteps();
      bool checkEncoderHealth();
      float calculatePositionError();
      void applyCorrection(float error);

  public:
      ClosedLoopController();
      bool begin();
      void update(); // Called from ClosedLoopTask

      // Status queries
      long getEncoderPosition() const;
      bool isEncoderHealthy() const;
      float getPositionError() const;
  };

  extern ClosedLoopController closedLoopController;
  ```

### TR2: FreeRTOS Task Integration
- **Task Name**: `ClosedLoopTask`
- **Update Interval**: 20ms (50Hz)
- **Priority**: Same as InputTask (moderate priority)
- **Core Affinity**: Core 0 (same as InputTask, avoids WebServer conflicts)
- **Stack Size**: 4096 bytes (similar to InputTask)
- **Location**: `src/main.cpp`
- **Implementation**:
  ```cpp
  void ClosedLoopTask(void *pvParameters) {
      TickType_t xLastWakeTime = xTaskGetTickCount();
      const TickType_t xFrequency = pdMS_TO_TICKS(20); // 20ms = 50Hz

      while (true) {
          closedLoopController.update();
          vTaskDelayUntil(&xLastWakeTime, xFrequency);
      }
  }

  void setup() {
      // ... existing setup code ...

      closedLoopController.begin();

      xTaskCreatePinnedToCore(
          ClosedLoopTask,
          "ClosedLoop",
          4096,
          NULL,
          1,
          NULL,
          0  // Core 0
      );
  }
  ```

### TR3: MotorController Integration
- **File**: `src/modules/MotorController/MotorController.h` and `.cpp`
- **Changes Required**:
  1. **Remove obsolete method**: Delete or deprecate `calculateSpeed()` (encoder no longer used for speed monitoring)
  2. **Expose motor enable state**:
     ```cpp
     bool isMotorEnabled() const { return digitalRead(EN_PIN) == LOW; }
     ```
  3. **Allow position override** (for AccelStepper sync):
     ```cpp
     // Already exists: setCurrentPosition(long position)
     ```
  4. **Optional**: Expose encoder read for ClosedLoopController:
     ```cpp
     friend class ClosedLoopController; // Grant access to readEncoder()
     ```

### TR4: WebSocket Status Updates
- **File**: `src/modules/WebServer/WebServer.cpp`
- **Modification**: Update status broadcast JSON
- **Before**:
  ```cpp
  status["position"] = motorController.getCurrentPosition();
  ```
- **After**:
  ```cpp
  if (closedLoopController.isEncoderHealthy()) {
      status["position"] = closedLoopController.getEncoderPosition();
      status["commandedPosition"] = motorController.getCurrentPosition();
      status["positionError"] = closedLoopController.getPositionError();
      status["controlMode"] = "closed-loop";
  } else {
      status["position"] = motorController.getCurrentPosition();
      status["controlMode"] = "open-loop";
      status["encoderStatus"] = "fault";
  }
  ```

### TR5: Coordinate System Conversions
- **Encoder to Steps**:
  ```cpp
  // Assuming 200 steps/rev motor with 16 microsteps = 3200 steps/rev
  const float STEPS_PER_REV = 3200;
  const float ENCODER_COUNTS_PER_REV = 16384;
  const float STEPS_PER_ENCODER_COUNT = STEPS_PER_REV / ENCODER_COUNTS_PER_REV;

  long encoderToSteps(int32_t rotationCount, uint16_t encoderRaw) {
      int64_t totalEncoderCounts = (int64_t)rotationCount * 16384 + encoderRaw;
      return (long)(totalEncoderCounts * STEPS_PER_ENCODER_COUNT);
  }
  ```
- **Steps to Degrees**:
  ```cpp
  float stepsToDegrees(long steps) {
      return (steps / STEPS_PER_REV) * 360.0;
  }
  ```

### TR6: Thread Safety Considerations
- **Shared State**: MotorController position accessed by both main loop and ClosedLoopTask
- **Protection Options**:
  - **Option A**: FreeRTOS mutex around critical sections
  - **Option B**: Atomic operations for position reads (if supported by ESP32)
  - **Option C**: Accept brief race conditions (AccelStepper position reads are generally safe)
- **Recommendation**: Start with Option C (simple), add mutex only if issues observed

### TR7: Logging and Debugging
- **Use Existing Logging Macros**: LOG_ERROR, LOG_WARN, LOG_INFO, LOG_DEBUG
- **Key Events to Log**:
  - Encoder initialization success/failure
  - Rotation counter wrap-around detection
  - Position corrections applied (at INFO level)
  - Large freewheel movements detected
  - Encoder fault detection and recovery
  - AccelStepper sync operations
- **Debug WebSocket**: All logs automatically streamed to `/debug` WebSocket

---

## Implementation Hints and Patterns

### Pattern 1: Rotation Counter Update (Wrap Detection)
```cpp
void ClosedLoopController::updateRotationCounter(uint16_t currentRaw) {
    int16_t delta = currentRaw - lastEncoderRaw;

    // Detect wrap-around (forward or backward)
    if (delta > 8192) {
        // Wrapped backward (16384 → 0)
        rotationCount--;
        LOG_DEBUG("Rotation counter decremented: %d", rotationCount);
    } else if (delta < -8192) {
        // Wrapped forward (0 → 16384)
        rotationCount++;
        LOG_DEBUG("Rotation counter incremented: %d", rotationCount);
    }

    lastEncoderRaw = currentRaw;
}
```

### Pattern 2: Deadband Position Correction
```cpp
void ClosedLoopController::update() {
    // Read encoder and update rotation tracking
    uint16_t encoderRaw = readEncoderRaw();
    updateRotationCounter(encoderRaw);

    // Convert to steps
    encoderPositionSteps = encoderToSteps(rotationCount, encoderRaw);
    long commandedPosition = motorController.getCurrentPosition();

    // Calculate error in steps
    long errorSteps = commandedPosition - encoderPositionSteps;
    float errorDegrees = stepsToDegrees(abs(errorSteps));

    // Check if motor is enabled
    bool motorEnabled = motorController.isMotorEnabled();

    // Handle motor enable/disable transitions
    if (motorEnabled && !motorWasEnabled) {
        // Motor just became enabled - sync AccelStepper to encoder
        motorController.setCurrentPosition(encoderPositionSteps);
        LOG_INFO("Motor enabled - AccelStepper synced to encoder: %ld", encoderPositionSteps);
    }
    motorWasEnabled = motorEnabled;

    // Apply correction only if motor enabled and error exceeds deadband
    if (motorEnabled && errorDegrees > DEADBAND_THRESHOLD) {
        // Proportional correction
        long correction = (long)(errorSteps * Kp);
        long newTarget = commandedPosition + correction;

        motorController.moveTo(newTarget, config.getMaxSpeed());
        LOG_INFO("Position correction applied: error=%.1f° correction=%ld steps",
                 errorDegrees, correction);
    }
}
```

### Pattern 3: Encoder Health Monitoring
```cpp
bool ClosedLoopController::checkEncoderHealth() {
    uint16_t raw = readEncoderRaw();

    // Check for SPI errors (all 0s or all 1s)
    if (raw == 0 || raw == 0xFFFF) {
        LOG_WARN("Encoder SPI error detected");
        return false;
    }

    // Check for stuck encoder (no movement while motor moving)
    bool motorMoving = motorController.isMoving();
    if (motorMoving) {
        if (raw == lastEncoderRaw) {
            if (millis() - lastEncoderChangeTime > 5000) {
                LOG_WARN("Encoder stuck - no change in 5 seconds while motor moving");
                return false;
            }
        } else {
            lastEncoderChangeTime = millis();
        }
    }

    return true;
}
```

### Pattern 4: Similar to TMC Mode Switching
The closed-loop enable/disable logic can follow the pattern established in [MotorController.cpp:217-232](src/modules/MotorController/MotorController.cpp#L217-L232) for TMC mode switching:
```cpp
// Similar pattern to updateTMCMode()
void ClosedLoopController::updateControlMode() {
    bool encoderHealthy = checkEncoderHealth();

    if (encoderHealthy != this->encoderHealthy) {
        this->encoderHealthy = encoderHealthy;
        LOG_INFO("Control mode switched to %s",
                 encoderHealthy ? "closed-loop" : "open-loop");
    }
}
```

---

## Acceptance Criteria

### AC1: Position Tracking Accuracy
- ✅ Encoder position reported to WebSocket matches physical shaft position within ±0.5°
- ✅ Multi-turn tracking correctly counts rotations over 100+ full rotations
- ✅ Position maintained accurately during freewheel mode (manual movement tracked)

### AC2: Error Correction Behavior
- ✅ Position errors > deadband threshold trigger correction within 100ms
- ✅ Position errors within deadband do not cause motor buzzing or corrections
- ✅ Corrections are gentle and proportional (no sudden jerks)
- ✅ After correction, final position error is within deadband

### AC3: Freewheel Mode Integration
- ✅ Motor can still freewheel after movement (EN_PIN HIGH)
- ✅ Encoder continues tracking position while freewheeling
- ✅ When motor re-enables, no unexpected movement occurs (AccelStepper synced)
- ✅ WebSocket shows actual encoder position during freewheel

### AC4: Graceful Degradation
- ✅ If encoder disconnected, system continues operating in open-loop mode
- ✅ SPI errors detected and logged without crashing
- ✅ WebSocket indicates encoder fault status
- ✅ System recovers automatically when encoder reconnected

### AC5: Performance and Stability
- ✅ ClosedLoopTask runs consistently at 20ms intervals
- ✅ No impact on WebSocket responsiveness or WebServer performance
- ✅ Motor movements remain smooth with closed-loop enabled
- ✅ CPU usage remains reasonable (<80% peak)

### AC6: Existing Functionality Preserved
- ✅ All existing WebSocket commands work unchanged
- ✅ Limit switches still trigger emergency stop correctly
- ✅ Web UI shows accurate position without modifications
- ✅ Configuration system (NVRAM) unaffected

---

## Testing Strategy

### Unit Tests
- Rotation counter wrap-around logic (forward/backward)
- Encoder-to-steps coordinate conversion
- Deadband threshold calculation
- Position error calculation

### Integration Tests
1. **Basic Closed-Loop Operation**
   - Move motor to position, verify encoder matches
   - Introduce artificial error, verify correction applied
   - Verify no corrections within deadband

2. **Freewheel Mode**
   - Command movement, wait for freewheel
   - Manually rotate shaft, verify encoder tracks
   - Command new movement, verify AccelStepper sync

3. **Multi-Turn Tracking**
   - Command 50 full rotations
   - Verify rotation counter increments correctly
   - Reverse direction, verify counter decrements

4. **Encoder Failure**
   - Disconnect encoder SPI
   - Verify graceful degradation to open-loop
   - Reconnect encoder, verify recovery

5. **Limit Switch Integration**
   - Trigger limit switch during movement
   - Verify emergency stop works with closed-loop
   - Verify rotation counter reset (if implemented)

### Hardware Testing
- Test with actual Z-axis threaded rod setup
- Verify step loss detection under load
- Verify smooth operation at various speeds
- Confirm no motor buzzing or overheating

---

## Migration and Rollout Plan

### Phase 1: Module Creation (Week 1)
- Create ClosedLoopController module skeleton
- Implement basic encoder reading and rotation tracking
- Add unit tests for coordinate conversions

### Phase 2: Core Logic (Week 2)
- Implement deadband position correction algorithm
- Add FreeRTOS task integration
- Test multi-turn tracking on bench

### Phase 3: Integration (Week 3)
- Modify MotorController for encoder access
- Update WebSocket status reporting
- Implement encoder health monitoring

### Phase 4: Testing and Tuning (Week 4)
- Hardware testing with actual Z-axis setup
- Tune deadband threshold and Kp gain
- Verify freewheel behavior and AccelStepper sync

### Phase 5: Documentation and Release
- Update CLAUDE.md with closed-loop details
- Add user documentation for homing procedure
- Tag release version

---

## Future Enhancements (Out of Scope)

These features are NOT part of the initial closed-loop implementation but could be added later:

1. **Runtime PID Tuning** - WebSocket commands to adjust Kp, deadband threshold
2. **Automatic Homing** - Use encoder and limit switches for auto-calibration on boot
3. **Load Estimation** - Analyze position error trends to detect binding or excessive load
4. **Stall Detection** - Detect motor stall (commanded motion but no encoder movement)
5. **External Force Detection** - Alert when encoder moves without motor command
6. **Encoder Position Saving** - Optional NVRAM storage of rotation counter
7. **Full PID Controller** - Add integral and derivative terms if deadband insufficient
8. **Vibration Detection** - Analyze encoder oscillations to detect mechanical resonance

---

## Assumptions

1. **Motor Type**: 200 steps/rev stepper with 16 microsteps (3200 steps/rev)
2. **Encoder Mount**: MT6816 rigidly coupled to motor shaft (no slippage)
3. **Limit Switches**: Functional and will be used for future homing feature
4. **Network Latency**: WebSocket position updates acceptable at 100ms intervals
5. **CPU Headroom**: ESP32 has sufficient capacity for additional 50Hz task
6. **Encoder Reliability**: MT6816 provides consistent readings under normal operation
7. **User Workflow**: Users will home system after boot before critical operations

---

## Dependencies

### Hardware
- LilyGo T-Motor board with ESP32
- TMC2209 stepper driver
- MT6816 magnetic encoder (14-bit, SPI interface)
- Limit switches (existing)

### Software Libraries
- AccelStepper (existing)
- TMCStepper (existing)
- FreeRTOS (ESP32 native)
- No new external libraries required

### Internal Modules
- `MotorController` - Requires minor modifications for encoder access
- `Configuration` - No changes required
- `WebServer` - Minor changes to status JSON
- `LimitSwitch` - No changes required

---

## Related Documentation

- **Discovery Answers**: [02-discovery-answers.md](02-discovery-answers.md)
- **Detail Answers**: [05-detail-answers.md](05-detail-answers.md)
- **Research Findings**: [03-context-findings.md](03-context-findings.md)
- **Project Overview**: [CLAUDE.md](../../CLAUDE.md)
- **Current Architecture**: [src/modules/MotorController/](../../src/modules/MotorController/)

---

## Glossary

- **Open-Loop Control**: Stepper motor control without position feedback (step counting only)
- **Closed-Loop Control**: Motor control with encoder feedback for error detection and correction
- **Deadband**: Tolerance zone where no corrections are applied (reduces chattering)
- **Multi-Turn Tracking**: Tracking absolute position across multiple 360° rotations
- **Freewheel Mode**: Motor disabled (EN_PIN HIGH), shaft can rotate freely
- **AccelStepper**: Arduino library for stepper motor control with acceleration
- **MT6816**: 14-bit absolute magnetic rotary encoder (0-16383 counts per revolution)
- **Rotation Counter**: Integer tracking full 360° rotations for multi-turn positioning
- **Position Error**: Difference between commanded position and actual encoder position

---

**Document Version:** 1.0
**Last Updated:** 2025-10-13
**Status:** ✅ Complete - Ready for Implementation
