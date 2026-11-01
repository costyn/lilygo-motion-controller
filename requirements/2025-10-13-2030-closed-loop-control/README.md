# Closed-Loop Control Requirements - Session Summary

**Date:** 2025-10-13
**Status:** ✅ Complete

## Quick Links

- 📋 **[Requirements Specification](06-requirements-spec.md)** - Complete implementation guide
- 💬 **[Discovery Answers](02-discovery-answers.md)** - High-level decisions
- 🔧 **[Technical Details](05-detail-answers.md)** - Implementation-specific choices
- 🔍 **[Research Findings](03-context-findings.md)** - Background research and analysis

## Overview

This requirements gathering session defines the implementation of closed-loop stepper motor control using the MT6816 magnetic encoder. The system will detect and correct missed steps, track position during freewheel mode, and gracefully degrade to open-loop if the encoder fails.

## Key Decisions

### Control Strategy
- ✅ **Deadband approach** (not full PID) - Simple threshold-based correction
- ✅ **2-5° error tolerance** - Avoids constant buzzing
- ✅ **Correction only during powered movement** - Freewheel feature preserved

### Architecture
- ✅ **Dedicated FreeRTOS task** (ClosedLoopTask) - 20ms update interval
- ✅ **New module**: `src/modules/ClosedLoopController/`
- ✅ **Multi-turn tracking** - Rotation counter for Z-axis applications

### Behavior
- ✅ **Encoder as source of truth** - AccelStepper syncs to encoder on motor enable
- ✅ **Graceful degradation** - Continue in open-loop if encoder fails
- ✅ **No UI changes** - Transparent to end users

### Position Tracking
- ✅ **Reset rotation counter on boot** - Homing required after power cycle
- ✅ **Report encoder position** - WebSocket shows actual physical position

## Implementation Summary

### New Files to Create
```
src/modules/ClosedLoopController/
├── ClosedLoopController.h
└── ClosedLoopController.cpp
```

### Files to Modify
```
src/main.cpp                           - Add ClosedLoopTask FreeRTOS task
src/modules/MotorController/
├── MotorController.h                  - Add isMotorEnabled() method
└── MotorController.cpp                - Remove/deprecate calculateSpeed()
src/modules/WebServer/WebServer.cpp    - Update status JSON with encoder position
```

### Key Components

1. **Multi-Turn Tracking**: Rotation counter + wrap-around detection
2. **Deadband Correction**: `if (error > threshold) { correct() }`
3. **Freewheel Support**: Track position, sync on re-enable
4. **Encoder Health**: Detect failures, fall back to open-loop
5. **FreeRTOS Task**: 50Hz update loop on Core 0

## Use Cases Supported

1. ✅ **Detect missed steps during movement**
2. ✅ **Track position during freewheel mode**
3. ✅ **Automatic error correction** (lenient, no buzzing)
4. ✅ **Multi-turn Z-axis applications** (3D printer style)
5. ✅ **Graceful degradation** on encoder failure

## Next Steps

1. Review [06-requirements-spec.md](06-requirements-spec.md) thoroughly
2. Create ClosedLoopController module skeleton
3. Implement rotation tracking and coordinate conversions
4. Add FreeRTOS task integration
5. Test multi-turn tracking on bench
6. Integrate with MotorController and WebServer
7. Hardware testing with actual Z-axis setup
8. Tune deadband and proportional gain

## Research Summary

### What We Learned

- **SimpleFOC**: Excellent library but incompatible with TMC2209 (FOC vs step/dir)
- **TMC4361A**: Hardware closed-loop chip, requires additional hardware
- **Custom Implementation**: Best approach for TMC2209 + encoder systems
- **Arduino Libraries**: FastClosedStepper, ClosedStepper provide reference patterns
- **MT6816 Support**: SimpleFOC has MT6816 driver, can reference their SPI code

### Key Insights

- Deadband simpler than PID for stepper motors
- Multi-turn tracking essential for linear actuators
- Encoder wrap-around detection critical
- Freewheel mode requires special handling
- Graceful degradation increases system robustness

## Questions Asked and Answered

### Discovery Phase (5 questions)
1. **Primary goal?** → Both error correction AND freewheel tracking
2. **Auto-correct errors?** → Yes, but lenient (deadband approach)
3. **PID tuning UI?** → No, hardcoded defaults
4. **Multi-turn tracking?** → Yes, essential for Z-axis
5. **Graceful degradation?** → Yes, fall back to open-loop

### Technical Phase (5 questions)
6. **Task architecture?** → Dedicated FreeRTOS task (user suggestion)
7. **Control algorithm?** → Deadband (not continuous PID)
8. **Re-enable behavior?** → Sync AccelStepper to encoder
9. **Position reporting?** → Encoder position primary
10. **Rotation counter storage?** → Reset on boot (homing required)

## Target Application

**3D Printer Z-Axis Style Linear Actuator**
- Threaded rod drive
- Many rotations travel range
- Limit switches for homing
- Freewheel when idle (no buzzing)
- Position critical for layer alignment

---

**Session Duration:** ~45 minutes
**Questions Asked:** 10
**Research Sources:** 5+ web resources
**Files Analyzed:** 3 existing modules
**Implementation Readiness:** ✅ Ready to code
