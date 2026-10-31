# Discovery Questions

**Generated:** 2025-11-22 14:50
**Phase:** Context Discovery

These questions help understand the problem space and user workflows for the auto-calibration feature.

---

## Q1: Should calibration trigger automatically on every power-on/reset?
**Default if unknown:** Yes (safety-first: ensures accuracy after power loss, meets user requirement)

**Rationale:** User explicitly requested "on power-on, the calibration should be performed". This suggests automatic behavior is desired, but we should confirm if there are cases where skipping calibration is acceptable (e.g., quick restart after firmware update).

---

## Q2: Should the motor attempt to recover if it's physically positioned beyond the current stored limits at power-on?
**Default if unknown:** Yes (safety-first: prevents confusion if motor was manually moved while powered off)

**Rationale:** If the device loses power while at position 5000, but the stored limits are 0-4000, the system needs a strategy to handle this mismatch. Either re-calibrate automatically or move to a known position first.

---

## Q3: Should calibration be interruptible by the user (e.g., emergency stop button)?
**Default if unknown:** Yes (safety-first: user must always be able to stop motor movement)

**Rationale:** Calibration involves automated motor movement that could take several seconds. User should have ability to abort if something goes wrong or if they need to stop for safety reasons.

---

## Q4: Should the webapp display real-time progress during calibration (e.g., "Finding lower limit...", "Finding upper limit...")?
**Default if unknown:** Yes (better UX: users understand what's happening, especially during initial power-on)

**Rationale:** Calibration could take 5-30 seconds depending on motor speed and travel distance. Showing progress prevents user confusion about whether the system is working or frozen.

---

## Q5: Should existing manual movements be blocked while calibration is in progress?
**Default if unknown:** Yes (safety-first: prevents conflicts between calibration routine and user commands)

**Rationale:** If calibration is running and user sends a "move to position" command via WebSocket, the system could enter an inconsistent state. Better to block all movement commands until calibration completes or fails.

---

**Next Steps:**
- Answer questions one at a time
- Answers will be recorded in 02-discovery-answers.md
- Phase will transition to targeted context gathering
