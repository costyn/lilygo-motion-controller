# Initial Request

**Date:** 2025-10-13
**Topic:** Transition from Open Loop to Closed Loop Control System

## User's Request

In this session we are going to be planning on how to make the project from open loop control to a closed loop control system. The magnetic encoder now works and readEncoder() can deliver highly accurate (to 0.1 degree) _absolute_ position sensing (wraps at 360).

In order to ensure no steps are missed, and to accurately keep track of shaft position when in freewheeling mode, a closed loop algorithm should be implemented. I have no experience in this area and will be relying on you to do your research (online in existing Arduino/C libraries?). For example the SimpleFOC (https://docs.simplefoc.com/code) is a project that dives deep into closed loop control. Unfortunately the libraries don't seem to support our TMC2208 driver, but perhaps we can learn something from their PID implementation. Please do web research on an effective closed loop control system for our situation. Please also try to think of other use cases for feedback from our magentic encoder, other than what I mentioned at the start. This is a new area for me, so I'll need some guidance.

Currently readEncoder() is called from calculateSpeed() but I"m not really sure if it does anything useful. No need to keep this.

In implementation, a separate class/module might be necessary to keep the motorcontrol class from getting too big.

## Key Points
- Magnetic encoder (MT6816) provides 0.1 degree accuracy (absolute positioning, wraps at 360°)
- Current system is open loop (AccelStepper based)
- Goals: Ensure no steps are missed, track position accurately in freewheel mode
- User requests research on closed loop control systems for stepper motors
- SimpleFOC mentioned as potential reference for PID implementation
- Current readEncoder() usage may not be useful, can be removed
- Separate module/class recommended to keep MotorController manageable
