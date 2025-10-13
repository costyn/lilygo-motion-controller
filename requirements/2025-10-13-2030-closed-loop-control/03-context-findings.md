# Context Findings

**Date:** 2025-10-13
**Phase:** Initial Research

## Current Codebase Analysis

### Existing Architecture
- **MotorController Module**: [src/modules/MotorController/MotorController.cpp](src/modules/MotorController/MotorController.cpp)
  - Uses AccelStepper library for open-loop control
  - TMC2209 stepper driver with UART communication
  - MT6816 magnetic encoder (14-bit, 0.1° accuracy) via SPI
  - Currently encoder only used in `calculateSpeed()` method (lines 188-215)
  - Encoder reads absolute position (0-16384 range, wraps at 360°)

### Current Encoder Usage
```cpp
int MotorController::readEncoder()
{
    // Lines 164-186: Reads 14-bit absolute position from MT6816
    // Returns raw value 0-16384 (0.022° resolution)
}

double MotorController::calculateSpeed(float ms)
{
    // Lines 188-215: Calculates angular velocity from encoder
    // Used for monitoring only, not for control
}
```

### Key Observations
1. Encoder infrastructure already exists and works reliably
2. AccelStepper maintains its own step counter (open-loop position)
3. No position error detection or correction currently implemented
4. Motor can miss steps without detection (especially in freewheel mode)
5. Modular architecture supports adding new modules easily

## Closed-Loop Control Research

### Industry Approaches

#### 1. **SimpleFOC Library** (https://docs.simplefoc.com)
- **Pros**:
  - Mature library with MT6816 support via Arduino-FOC-drivers
  - Comprehensive closed-loop implementation
  - Multiple control modes (position, velocity, torque)
  - Well-documented PID tuning
- **Cons**:
  - Designed for BLDC/FOC control (Field Oriented Control)
  - Does NOT support TMC2209 drivers (incompatible architecture)
  - Requires driver-level current control (not STEP/DIR interface)
  - Would require complete rewrite of motor control system

**Verdict**: Not suitable for our TMC2209-based system

#### 2. **TMC4361A Closed-Loop** (Trinamic/Analog Devices)
- Hardware-based closed-loop controller chip
- Works with TMC stepper drivers
- Requires additional hardware (TMC4361A chip)
- Not applicable to existing LilyGo hardware

#### 3. **Custom PID Implementation** (Recommended approach)
Several Arduino projects implement custom PID for stepper+encoder:

**FastClosedStepper**: https://github.com/gfall94/FastClosedStepper
- Uses FastAccelStepper + Encoder libraries
- Wraps PID control around step/dir interface

**ClosedStepper**: https://github.com/jmosbacher/ClosedStepper
- Uses AccelStepper + Encoder libraries (same as our current setup!)
- Implements PID to adjust target position based on encoder feedback

**Curious Scientist TMC2209+AS5600**: https://curiousscientist.tech/blog/stepper-motor-developing-platform-pid
- Similar hardware to ours (TMC2209 + magnetic encoder)
- Uses PID where error = (encoder_position - target_position)
- Output = speed adjustment or position correction

### Closed-Loop Control Algorithm (Standard Approach)

```
1. Set target position (in steps and in encoder units)
2. Loop:
   a. Read encoder actual position
   b. Calculate position error = target - actual
   c. Run PID controller on error
   d. Adjust motor speed/position based on PID output
   e. Call stepper->run() to execute steps
   f. Repeat until error < threshold
```

### PID Controller Basics

**Components**:
- **P (Proportional)**: Correction proportional to error
- **I (Integral)**: Eliminates steady-state error (accumulated error)
- **D (Derivative)**: Dampens oscillation (rate of change of error)

**Output**: Position correction or speed adjustment

**Tuning**: Start with P-only, add I for steady-state, add D if oscillating

### Key Challenges for Stepper+Encoder Closed-Loop

1. **Position Wrapping**: Encoder wraps at 360°, stepper doesn't
   - Solution: Track full rotations separately

2. **Coordinate Systems**: Steps vs. encoder units
   - Solution: Conversion math (steps ↔ degrees ↔ encoder counts)

3. **Step Quantization**: Stepper moves in discrete steps
   - Solution: Accept "close enough" threshold

4. **Real-time Performance**: PID loop must run fast enough
   - Current InputTask runs at 100ms - may need faster update rate

5. **Homing/Initialization**: Establish reference between step count and encoder
   - Solution: Homing routine to sync positions

## Potential Use Cases for Encoder Feedback

Beyond basic closed-loop position control:

1. **Step Loss Detection**: Compare encoder position to expected position
2. **Stall Detection**: Motor commanded to move but encoder shows no motion
3. **External Force Detection**: Encoder moves without motor command (collision, manual manipulation)
4. **Load Estimation**: Resistance to movement based on position error
5. **Dynamic Tuning**: Adjust acceleration/speed based on actual motion
6. **Freewheel Tracking**: Know exact position even when motor disabled
7. **Auto-homing**: Find zero position by detecting mechanical stop
8. **Vibration/Resonance Detection**: Encoder shows oscillation at certain speeds
9. **Precision Positioning**: Achieve sub-step accuracy using encoder feedback
10. **Position Verification**: WebSocket reports true encoder position vs commanded position

## Technical Constraints

### Hardware Limitations
- TMC2209: Step/Dir interface only (no direct current control)
- MT6816: 14-bit absolute encoder (16384 positions per rotation)
- ESP32: Sufficient processing power for PID loop
- Current Architecture: FreeRTOS tasks already handle high-frequency operations

### Software Constraints
- Must maintain compatibility with existing WebSocket API
- Should preserve AccelStepper for open-loop fallback
- Need to avoid breaking existing limit switch functionality
- Module should be optional/toggleable for testing

## Related Files to Modify/Create

### New Module (Recommended)
- `src/modules/ClosedLoopController/ClosedLoopController.h`
- `src/modules/ClosedLoopController/ClosedLoopController.cpp`

### Existing Files to Modify
- `src/modules/MotorController/MotorController.h` - Add closed-loop integration hooks
- `src/modules/MotorController/MotorController.cpp` - Remove/modify `calculateSpeed()` if not needed
- `src/main.cpp` - Initialize closed-loop controller, call update in main loop
- `src/modules/Configuration/Configuration.h` - Add PID tuning parameters to config
- `src/modules/WebServer/WebServer.cpp` - Add encoder position to status broadcasts

### Files to Review
- `src/modules/LimitSwitch/LimitSwitch.cpp` - May need coordination with closed-loop
- `platformio.ini` - May need to add PID library dependency

## Similar Features in Codebase

### Existing Control Loops
1. **TMC Mode Switching** (MotorController.cpp:217-232)
   - Simple threshold-based state machine
   - Updates based on speed feedback
   - Similar pattern could be used for closed-loop enable/disable

2. **Emergency Stop Recovery** (MotorController.cpp:29-30, 142-157)
   - State-based control with flag management
   - Could inform closed-loop error handling

### Existing Position Tracking
1. **AccelStepper Position** (open-loop)
   - `stepper->currentPosition()` - step counter
   - `stepper->targetPosition()` - desired position
   - `stepper->distanceToGo()` - remaining steps

2. **Encoder Position** (actual position)
   - `readEncoder()` - raw 14-bit value (0-16384)
   - Wraps at full rotation
   - No integration with step counting yet

## Assumptions & Questions for Discovery

Based on research, key questions to clarify:

1. **Primary Goal**: Is the main goal error detection/correction, or accurate tracking during freewheel?
2. **Control Strategy**: Should PID adjust speed or directly correct position?
3. **Performance Requirements**: How often should closed-loop update run (10ms? 50ms? 100ms?)?
4. **Error Tolerance**: What position error is acceptable before correction (0.5°? 1°? 5°)?
5. **Fallback Behavior**: Should system fall back to open-loop if encoder fails?
6. **Multi-turn Tracking**: Do we need to track absolute position across multiple rotations?
7. **User Control**: Should PID parameters be configurable via WebSocket, or compile-time constants?

## Next Steps

1. Ask discovery questions to clarify requirements
2. Determine if PID library is needed or custom implementation
3. Design ClosedLoopController module architecture
4. Define integration points with MotorController
5. Create detailed implementation plan
