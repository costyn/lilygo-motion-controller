# Detail Answers

**Date:** 2025-11-22 14:50
**Phase:** Expert Requirements Questions - Complete

---

## Q1: Should calibration start by moving to the MIN limit first, then MAX limit?
**Answer:** Yes (default)

**Rationale:** User indicated no preference, so using the logical default sequence: MIN limit first, then MAX limit. Motor will end at MAX position after calibration completes.

---

## Q2: Should calibration use a fixed safe speed or be based on current motor configuration?
**Answer:** Based on current config (30% of current maxSpeed setting)

**Rationale:** User had no preference, so using adaptive approach. Calibration speed = `config.getMaxSpeed() * 0.3`, which adapts to motor capabilities while remaining conservative.

---

## Q3: When calibration is interrupted by emergency stop, should it automatically resume or require manual restart?
**Answer:** Require manual restart (Option A)

**Rationale:** User explicitly chose Option A. Calibration will abort completely on emergency stop. User must click "Recalibrate" again to restart the entire calibration process from the beginning. Simpler state machine, clearer user intent.

---

## Q4: Should the webapp disable the "Recalibrate" button while calibration is in progress?
**Answer:** Yes

**Rationale:** User confirmed. Button will be disabled with "Calibrating..." text during calibration to prevent double-clicks and state confusion. Emergency stop remains available for aborting.

---

## Q5: When calibration completes successfully, should the system report total travel distance to webapp?
**Answer:** No

**Rationale:** User noted that webapp already displays range and available steps elsewhere in the UI. Simple "Calibration complete" message is sufficient without duplicating information.

---

**Key Decisions Summary:**
- Calibration sequence: MIN first, then MAX
- Calibration speed: 30% of current maxSpeed (adaptive)
- Emergency stop behavior: Abort completely, require manual restart
- Button state during calibration: Disabled with "Calibrating..." text
- Success message: Simple completion notice (no duplicate range info)
