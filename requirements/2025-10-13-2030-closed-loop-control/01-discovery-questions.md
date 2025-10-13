# Discovery Questions

**Date:** 2025-10-13
**Phase:** Discovery

These questions help understand the problem space and use cases for closed-loop control.

## Q1: Is the primary goal to detect and correct missed steps during powered movement?

**Context**: Closed-loop can be used to either (A) detect/correct errors while motor is actively moving, or (B) track position when motor is freewheeling/disabled.

**Default if unknown:** Yes (active correction during movement is the standard use case for closed-loop control and provides the most benefit for precision applications)

---

## Q2: Should the closed-loop system automatically correct position errors without user intervention?

**Context**: Some systems alert the user when errors are detected, while others automatically adjust the motor position to match the encoder. Auto-correction provides smoother operation but could mask underlying mechanical issues.

**Default if unknown:** Yes (automatic correction is the standard behavior for closed-loop systems and provides transparent error correction without requiring user interaction)

---

## Q3: Will users need to tune PID parameters through the web interface?

**Context**: PID tuning can be exposed via WebSocket/web UI (allowing runtime adjustment) or hardcoded as compile-time constants (simpler but less flexible). Different motors/loads may require different tuning.

**Default if unknown:** No (for initial implementation, reasonable default PID values can be determined through testing and hardcoded, avoiding the complexity of a tuning interface. Advanced users can modify source code if needed)

---

## Q4: Does the system need to track absolute position across multiple motor rotations (multi-turn tracking)?

**Context**: The MT6816 encoder wraps at 360°, so tracking multiple full rotations requires additional logic to count rotations. This is essential for applications with >360° range of motion.

**Default if unknown:** Yes (most practical applications require multi-turn tracking, and the system already has limit switches suggesting >360° travel range)

---

## Q5: Should the system fall back to open-loop control if the encoder fails or produces invalid readings?

**Context**: Sensor failures can occur (loose connection, electrical noise, etc.). Having a fallback to open-loop AccelStepper control ensures the system remains operational, though less accurate.

**Default if unknown:** Yes (graceful degradation is a best practice for embedded systems - continuing to operate in open-loop mode is safer than completely failing)

---

## Summary of Defaults

If all questions are answered with defaults:
- ✅ Active error correction during powered movement
- ✅ Automatic position correction without user intervention
- ❌ PID parameters hardcoded (not configurable via web UI)
- ✅ Multi-turn absolute position tracking
- ✅ Fallback to open-loop if encoder fails
