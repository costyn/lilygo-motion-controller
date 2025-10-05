# Detail Answers

## Q1: Should the freewheel configuration default to ENABLED or DISABLED for new installations?
**Answer:** Disabled

## Q2: Should jogStop() always freewheel (current behavior) or respect the freewheel configuration setting?
**Answer:** Respect config

## Q3: For the buzzing issue, should we experiment with different ihold values (2-5) or keep it at 1 as in factory example?
**Answer:** Experiment. Current hold value causes buzzing. But user suspects the root cause may be unnecessary commands being sent to the motor, possibly from floating point conversions flipping back and forth.

**Critical Discovery**: The buzzing is caused by `stepper->run()` being called continuously in the main loop even when `distanceToGo() == 0`. AccelStepper's `run()` method keeps sending micro-step corrections to maintain position, causing the TMC2209 to buzz.

**Solution**: Only call `stepper->run()` when motor is actually moving (`distanceToGo() != 0`). Keep ihold at 1.

## Q4: Should movement completion detection use a debounce/delay to avoid rapid freewheel toggling during micro-adjustments?
**Answer:** No. But keep the state machine simple - avoid lots of flags that make following the logic difficult.

## Q5: Should the mode switching fix use abs(stepper->speed()) for the actual motor speed, or should it continue using monitorSpeed from encoder once encoder hardware is fixed?
**Answer:** Use command speed (stepper->speed()). We will use the encoder to keep track of position when motor is freewheeling and turned by external force, but that's for the future.
