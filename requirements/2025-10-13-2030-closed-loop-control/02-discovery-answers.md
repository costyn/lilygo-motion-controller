# Discovery Answers

**Date:** 2025-10-13
**Phase:** Discovery

## Q1: Is the primary goal to detect and correct missed steps during powered movement?

**Answer:** Both use cases are needed - (A) detect/correct errors during powered movement AND (B) track position when motor is freewheeling/disabled.

**Implications:**
- Closed-loop controller must operate in both powered and unpowered states
- Need to track encoder position continuously, not just during active movement
- System should detect when motor is disabled and switch to tracking-only mode
- Position reporting should always reflect encoder (actual) position
- When motor re-enables, sync AccelStepper position with encoder position

---

## Q2: Should the closed-loop system automatically correct position errors without user intervention?

**Answer:** Yes, but with lenient/forgiving error tolerance to avoid constant corrections.

**Implications:**
- Implement "deadband" threshold - only correct if error exceeds reasonable tolerance (e.g., 2-5°)
- Must preserve existing freewheel-after-movement feature (motor disables when stopped)
- Closed-loop correction ONLY active during powered movement
- When motor is freewheeling (EN_PIN HIGH), PID loop should be disabled/dormant
- Track encoder position continuously, but don't attempt corrections when motor disabled
- Alternative approach: Use encoder position as source of truth and accept small errors rather than constant buzzing
- PID tuning should prioritize stability over aggressive correction

**Key constraint:** The freewheel feature must remain functional - closed-loop should NOT prevent the motor from disabling after movement completion

---

## Q3: Will users need to tune PID parameters through the web interface?

**Answer:** No - use hardcoded default values.

**Implications:**
- PID parameters (Kp, Ki, Kd) will be compile-time constants in the ClosedLoopController module
- Values will be tuned during development/testing for typical use cases
- End users only control position, direction, and speed (existing interface)
- Reduces WebSocket API complexity
- Advanced users can modify values in source code if needed for special cases
- No additional UI elements needed in web interface

---

## Q4: Does the system need to track absolute position across multiple motor rotations (multi-turn tracking)?

**Answer:** Yes - essential for 3D printer Z-axis style application with many rotations.

**Implications:**
- Must implement rotation counter to track full turns beyond 360°
- Algorithm to detect encoder wrap-around (0→16384 or 16384→0 transitions)
- Combined position = (rotation_count × 16384) + encoder_raw_value
- This multi-turn position can be converted to steps for comparison with AccelStepper
- Limit switch triggers should validate/reset rotation counter
- Storage: Track rotation count as signed integer (supports bidirectional movement)
- Initial homing routine critical to establish rotation count reference point

---

## Q5: Should the system fall back to open-loop control if the encoder fails or produces invalid readings?

**Answer:** Yes - graceful degradation preferred, keep working.

**Implications:**
- Detect encoder failures: SPI communication errors, stuck values, impossible transitions
- When failure detected, log warning and disable closed-loop corrections
- Continue operating with AccelStepper in pure open-loop mode
- System remains functional for non-critical applications
- WebSocket status should indicate "encoder fault - open loop mode"
- Periodic retry attempts to re-enable closed-loop if encoder recovers
- User can still control motor via existing interface without interruption
