# Discovery Answers

**Date:** 2025-11-22 14:50
**Phase:** Context Discovery - Complete

---

## Q1: Should calibration trigger automatically on every power-on/reset?
**Answer:** No

**Rationale:** User reconsidered initial request - automatic calibration on every power-on would be annoying/tiring. Calibration will only run when manually triggered via the Settings dialog button.

---

## Q2: Should the motor attempt to recover if it's physically positioned beyond the current stored limits at power-on?
**Answer:** No

**Rationale:** Encoder is not working properly yet and data cannot be trusted. The system will trust stored limits and rely on limit switch triggers to handle any position mismatches. Users can manually recalibrate if the range becomes problematic.

---

## Q3: Should calibration be interruptible by the user (e.g., emergency stop button)?
**Answer:** Yes

**Rationale:** Safety-first approach - user must always be able to stop motor movement during calibration via emergency stop (webapp or physical buttons).

---

## Q4: Should the webapp display real-time progress during calibration?
**Answer:** Yes

**Rationale:** Better UX - calibration could take 5-30 seconds. The Motor Status card's "State" field will show progress messages like "Calibrating: Finding lower limit", "Calibrating: Finding upper limit", "Calibration complete".

---

## Q5: Should existing manual movements be blocked while calibration is in progress?
**Answer:** Yes

**Rationale:** Safety-first - prevents conflicts between calibration routine and user commands. All manual movement commands (jog, position slider, WebSocket) will be blocked during calibration. Only emergency stop and limit switches can interrupt.

---

**Key Decisions Summary:**
- Manual trigger only (no auto-calibration on power-on)
- Emergency stop always available
- Real-time status feedback in webapp
- Movement commands blocked during calibration
- Rely on limit switches for safety boundaries
