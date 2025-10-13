# Detail Answers

**Date:** 2025-10-13
**Phase:** Detail Questions

## Q6: Should the ClosedLoopController run in a dedicated FreeRTOS task, or be called from main loop/existing task?

**Answer:** Dedicated FreeRTOS ClosedLoopTask (user suggestion adopted)

**Implications:**
- Create new FreeRTOS task: `ClosedLoopTask` running at 20ms interval (50Hz update rate)
- Task priority should be set appropriately (likely same or higher than InputTask)
- Clean architectural separation - closed-loop logic isolated from motor control
- Task can be pinned to Core 0 (same as InputTask) to avoid conflicts with WebServerTask on Core 1
- FreeRTOS mutex/semaphore may be needed if accessing shared MotorController state
- Follows existing pattern: InputTask (100ms), WebServerTask (50ms), ClosedLoopTask (20ms)
- Easy to enable/disable closed-loop by suspending task
- 20ms update rate provides good balance between responsiveness and CPU overhead

---

## Q7: Should the closed-loop system use a deadband (position error threshold) approach, or full PID control with gentle tuning?

**Answer:** Deadband approach (default)

**Implications:**
- Simple threshold-based correction: only act if `abs(error) > DEADBAND_THRESHOLD`
- Suggested deadband: 2-5° (roughly 90-230 encoder counts or 18-46 steps with 200 steps/rev motor)
- No PID library dependency needed - reduces complexity
- Clear behavior: within threshold = no action, outside threshold = correction
- May still want proportional correction speed based on error magnitude
- Example logic: `if (error > threshold) { correction = Kp * error; }`
- Can add simple rate limiting to avoid aggressive corrections
- Easier to tune: single threshold parameter vs three PID gains
- Completely eliminates buzzing when within tolerance

---

## Q8: When motor re-enables after freewheeling, should we sync AccelStepper position to encoder position, or keep AccelStepper position and move motor back?

**Answer:** Option A - sync AccelStepper to encoder position

**Implications:**
- On motor re-enable (EN_PIN LOW), read current encoder position
- Convert encoder multi-turn position to steps
- Call `stepper->setCurrentPosition(encoderPositionInSteps)`
- AccelStepper now matches physical reality
- No unexpected movement when motor powers up
- Manual adjustments during freewheel are respected
- Encoder is the source of truth for actual position
- Prevents dangerous "snap back" movements
- User can manually position axis while motor disabled, system accepts it

---

## Q9: Should encoder position be reported to the WebSocket clients in addition to (or instead of) the AccelStepper step position?

**Answer:** Report encoder position as primary, optionally add commanded position for debugging

**Implications:**
- WebSocket status: `"position": <encoderPositionInSteps>` (actual position)
- Optional debug field: `"commandedPosition": <accelStepperPosition>` (if different)
- End users see real physical position (most useful)
- When closed-loop working correctly, both values should match (within deadband)
- Position error visible when values diverge (useful for debugging)
- At movement completion, positions should converge to match
- Encoder position is what matters for external measurements
- `getCurrentPosition()` in MotorController should return encoder position when available

---

## Q10: Should the rotation counter be saved to NVRAM (Configuration), or reset to zero on every boot?

**Answer:** Reset on boot (default)

**Implications:**
- Rotation counter initialized to 0 on startup
- System requires homing/calibration after boot to establish position reference
- Avoids NVRAM wear from frequent writes during long Z-axis movements
- Matches typical 3D printer behavior (home after power-up)
- Simpler implementation - no NVRAM write logic needed
- Limit switches already present for homing functionality
- Future homing feature will establish encoder rotation count reference
- User expectation: power cycle = unknown position, homing required
- No position persistence across reboots (acceptable for this use case)
