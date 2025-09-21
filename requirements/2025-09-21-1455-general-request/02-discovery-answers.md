# Discovery Answers

**Date:** 2025-09-21 14:55
**Phase:** 2 - Context Discovery (Complete)

## Q1: Should we rename this project to avoid conflicts with existing EspStepperServer?
**Answer:** Yes - "LilyGo-MotionController"
**Impact:** Project will be renamed to avoid conflicts and better reflect the specific hardware platform.

## Q2: Will the mobile webapp need to display real-time position/speed updates while the motor is moving?
**Answer:** Yes - including virtual representation of lamp + movement
**Impact:** Requires high-frequency WebSocket updates with position data from MT6816 encoder for smooth animations in the webapp.

## Q3: Should the limit switches act as emergency stops that immediately halt all movement?
**Answer:** Yes - with smart position learning
**Impact:** Emergency stop on trigger + save limit positions to NVRAM + automatic deceleration when approaching known limits + position persistence across power cycles.

## Q4: Do you want the stepper to automatically return to a "home" position on startup or after errors?
**Answer:** No - stay where it is, read current position from encoder
**Impact:** MT6816 encoder reads actual position on startup, system syncs to physical position. Critical since manual knob adjustment is possible during power-off.

## Q5: Should the debug WebSocket stream include motor telemetry (position, speed, current) or just serial/log output?
**Answer:** Both - comprehensive debugging information
**Impact:** Separate debug WebSocket endpoint with application logs + real-time motor telemetry (position, speed, encoder readings, TMC2209 diagnostics).

---

**Phase 2 Complete**. All discovery questions answered.
**Next:** Phase 3 - Targeted Context Gathering (Autonomous)