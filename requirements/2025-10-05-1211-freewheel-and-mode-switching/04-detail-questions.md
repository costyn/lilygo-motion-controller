# Detail Questions

## Q1: Should the freewheel configuration default to ENABLED or DISABLED for new installations?
**Default if unknown:** Disabled (motor holds position by default, safer for existing users upgrading firmware)

**Context:** This affects the default value in Configuration constructor and the behavior users see on first boot or after resetting config.

## Q2: Should jogStop() always freewheel (current behavior) or respect the freewheel configuration setting?
**Default if unknown:** Respect configuration (consistent behavior across all movement types)

**Context:** Currently `jogStop()` at line 129 always does `digitalWrite(EN_PIN, HIGH)`. This question determines if we keep that line or change it to check config.

## Q3: For the buzzing issue, should we experiment with different ihold values (2-5) or keep it at 1 as in factory example?
**Default if unknown:** Keep at 1 (match factory example, let freewheel config solve the issue)

**Context:** Current `driver->ihold(1)` at line 71. Higher values = more holding torque but more heat/power. Factory example uses 1.

## Q4: Should movement completion detection use a debounce/delay to avoid rapid freewheel toggling during micro-adjustments?
**Default if unknown:** No (simple state machine is sufficient - only trigger on transition from moving to stopped)

**Context:** AccelStepper's `distanceToGo()` might oscillate near target. State machine with `wasMoving` flag should handle this, but we could add a small delay (50-100ms) if needed.

## Q5: Should the mode switching fix use abs(stepper->speed()) for the actual motor speed, or should it continue using monitorSpeed from encoder once encoder hardware is fixed?
**Default if unknown:** Use stepper->speed() permanently (commanded speed is more reliable than encoder for mode switching logic)

**Context:** `monitorSpeed` from encoder doesn't work currently. Using `stepper->speed()` gives us the actual commanded speed which is what we need for SpreadCycle/StealthChop switching threshold.
