# Expert Technical Questions

**Date:** 2025-10-13
**Phase:** Detail Questions

These questions clarify specific implementation details based on deep understanding of the codebase.

## Q6: Should the ClosedLoopController run in a dedicated FreeRTOS task, or be called from main loop/existing task?

**Context**: Currently the system uses FreeRTOS with:
- main loop(): High-frequency motor control (motorController.update())
- InputTask: 100ms interval (limit switches, buttons)
- WebServerTask: 50ms interval (WebSocket cleanup)

Closed-loop control typically needs 10-50ms update rate for stability.

**User suggestion:** Create a dedicated ClosedLoopTask for clean architecture.

**Default if unknown:** Dedicated FreeRTOS ClosedLoopTask at 20ms intervals (clean separation of concerns, matches existing architecture pattern, optimal update frequency for PID control, doesn't interfere with other tasks)

---

## Q7: Should the closed-loop system use a deadband (position error threshold) approach, or full PID control with gentle tuning?

**Context**: Two approaches for "lenient" correction:
- **Deadband**: Only correct if error exceeds threshold (e.g., 2°), otherwise do nothing
- **Gentle PID**: Always active but with low P gain and minimal I/D terms

Deadband is simpler and avoids buzzing completely. PID provides smoother continuous correction but requires careful tuning.

**Default if unknown:** Deadband approach (simpler to implement and test, completely eliminates buzzing within threshold, easier to reason about behavior, and user specifically mentioned avoiding constant corrections)

---

## Q8: When motor re-enables after freewheeling, should we sync AccelStepper position to encoder position, or keep AccelStepper position and move motor back?

**Context**: If motor shaft is manually moved while freewheeling, encoder tracks the new position but AccelStepper still has old position. When re-enabling:
- **Option A**: Update AccelStepper position to match encoder (accepts manual movement as new reference)
- **Option B**: Command motor to return to original AccelStepper position (rejects manual movement)

**Default if unknown:** Option A - sync AccelStepper to encoder (respects physical reality, prevents unexpected motor movement on enable, treats encoder as source of truth per Q1/Q2 discussion)

---

## Q9: Should encoder position be reported to the WebSocket clients in addition to (or instead of) the AccelStepper step position?

**Context**: Currently WebSocket status broadcasts include `getCurrentPosition()` which returns AccelStepper step count. With closed-loop, we'll have both:
- AccelStepper position (commanded/intended steps)
- Encoder position (actual physical position)

Users may want to see actual position, or both for debugging.

**Default if unknown:** Report encoder position as primary position field, add separate "commandedPosition" field for debugging (encoder reflects physical reality and is more accurate, matches the principle of treating encoder as source of truth)

---

## Q10: Should the rotation counter be saved to NVRAM (Configuration), or reset to zero on every boot?

**Context**: The rotation counter tracks full 360° turns. Options:
- **Save to NVRAM**: Preserves position across reboots (requires write on every rotation)
- **Reset on boot**: Simple but loses position reference, requires re-homing

Current system uses limit switch position learning stored in Configuration/NVRAM.

**Default if unknown:** Reset on boot (simpler, avoids NVRAM wear from frequent writes during movement, aligns with expectation that system should home after power cycle for Z-axis applications, matches typical 3D printer behavior)

---

## Summary of Defaults

If all questions answered with defaults:
- ✅ Update closed-loop in main loop (high frequency)
- ✅ Use deadband approach (not continuous PID)
- ✅ Sync AccelStepper to encoder on re-enable
- ✅ Report encoder position as primary position
- ✅ Reset rotation counter on boot (require homing)
