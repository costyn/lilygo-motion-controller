# Detail Questions

**Generated:** 2025-11-22 14:50
**Phase:** Expert Requirements Questions

These questions address specific implementation details now that I understand the codebase architecture.

---

## Q1: Should calibration start by moving to the MIN limit first, then MAX limit?
**Default if unknown:** Yes (logical sequence: establish lower boundary first, then upper boundary)

**Context:** The calibration routine needs a specific order of operations. Starting with MIN limit (backward movement) then MAX limit (forward movement) creates a predictable sequence. This also means the motor ends at the MAX position after calibration completes.

**Alternative:** Start at MAX first, then MIN (motor would end at MIN position after calibration).

**Technical impact:** Affects the state machine sequence in `MotorController::startCalibration()`.

---

## Q2: Should calibration use a fixed safe speed or be based on current motor configuration?
**Default if unknown:** Based on current config (30% of current maxSpeed setting)

**Context:**
- Fixed speed option: Always use a conservative value like 2000 steps/sec
- Dynamic speed option: Use percentage of current `config.getMaxSpeed()` (e.g., 30%)

**Rationale for default:** Using 30% of maxSpeed adapts to user's motor capabilities while remaining conservative. If user has set maxSpeed=20000, calibration uses 6000 steps/sec. If maxSpeed=5000, calibration uses 1500 steps/sec.

**Technical impact:** Code in `handleCalibrateCommand()` will either hardcode speed or calculate it dynamically.

---

## Q3: When calibration is interrupted by emergency stop, should it automatically resume or require manual restart?
**Default if unknown:** Require manual restart (safer, gives user control)

**Context:** If user hits emergency stop during calibration (e.g., motor about to crash into obstacle), what should happen:
- **Option A (Default):** Calibration aborts completely. User must click "Recalibrate" again to restart.
- **Option B:** System remembers calibration was in progress, offers "Resume Calibration" option.

**Rationale for default:** Simpler implementation, clearer state machine, avoids ambiguity about whether partial calibration data is valid.

**Technical impact:** If manual restart: just clear calibration state on emergency stop. If auto-resume: need to persist calibration state and add resume logic.

---

## Q4: Should the webapp disable the "Recalibrate" button while calibration is in progress?
**Default if unknown:** Yes (prevents user from clicking it twice and confusing the state)

**Context:** During calibration, the "Recalibrate" button should either:
- Be disabled with text "Calibrating..." to show it's in progress
- Remain enabled and show "Cancel Calibration" to allow user to abort
- Be hidden entirely while calibration runs

**Rationale for default:** Disabling with "Calibrating..." text is clearest UX. User can still use emergency stop to abort if needed.

**Technical impact:** Button's `disabled` prop tied to `motorStatus.isCalibrating` (or similar status field).

---

## Q5: When calibration completes successfully, should the system report total travel distance to webapp?
**Default if unknown:** Yes (provides useful feedback and confirms calibration worked)

**Context:** After calibration finishes, webapp could show a success message like:
- Simple: "Calibration complete"
- Detailed: "Calibration complete. Travel range: 12,543 steps (Min: -5000, Max: 7543)"

The total steps calculation is simple: `abs(maxLimit - minLimit)`.

**Rationale for default:** More informative feedback helps user understand their motor's actual travel range. No significant implementation cost.

**Technical impact:**
- Add `totalSteps` field to MotorConfig type
- Calculate and include in `broadcastConfig()` after calibration
- Display in success message or MotorStatus card

---

**Next Steps:**
- Answer questions one at a time
- Answers will be recorded in 05-detail-answers.md
- Phase will transition to requirements specification
