# Discovery Questions

**Date:** 2025-09-21 14:55
**Phase:** 2 - Context Discovery

Based on your requirements document and codebase analysis, here are the key questions to understand your project scope:

## Q1: Should we rename this project to avoid conflicts with existing EspStepperServer?
**Default if unknown:** Yes (using "LilyGo-StepperController" or "T-Motor-Controller")
**Why this default:** You mentioned it's already someone else's project, so renaming prevents confusion and potential conflicts.

## Q2: Will the mobile webapp need to display real-time position/speed updates while the motor is moving?
**Default if unknown:** Yes (essential for lamp positioning control)
**Why this default:** For a lamp with moving arms, users would want visual feedback of current position and movement status.

## Q3: Should the limit switches act as emergency stops that immediately halt all movement?
**Default if unknown:** Yes (safety-critical for preventing mechanical damage)
**Why this default:** You emphasized "making sure the project does not destroy itself" - immediate stops prevent overextension.

## Q4: Do you want the stepper to automatically return to a "home" position on startup or after errors?
**Default if unknown:** No (manual control preferred for lamps)
**Why this default:** Lamps typically stay in their last position when powered on, rather than auto-homing.

## Q5: Should the debug WebSocket stream include motor telemetry (position, speed, current) or just serial/log output?
**Default if unknown:** Both (comprehensive debugging information)
**Why this default:** For development and troubleshooting, both application logs and real-time motor status would be valuable.

---

**Next Steps:**
- Answer each question to proceed to detailed technical questions
- Questions will be asked one at a time with opportunity to clarify
- All answers will be recorded before proceeding to Phase 4