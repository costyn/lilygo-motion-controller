# Revised Closed-Loop Plan

**Date:** 2026-11-01

This replaces the control approach in `06-requirements-spec.md`. The 2025 implementation never
aligned the encoder with the step counter, moved the target to apply corrections, and ran soft
limits at full speed while freewheeling. Correction during movement is dropped: missed steps
usually mean the motor ran out of torque, which a 50Hz correction can't recover.

1. **Measure only** (implemented): log step counter against encoder per move, with no actuation.
2. **Align**: one conversion with measured sign and scale, offset captured while stopped and after calibration.
3. **Detect lost steps**: stop and raise a fault past about 64 microsteps (7.2 degrees, one electrical cycle).
4. **Correct at end of move**: once stopped, reset the step counter from the encoder and return to the
   original target with limited retries. Runs in `loop()`, so only one core touches the stepper.
5. **Track freewheeling**: resync inside `moveTo()` before the move starts. Soft limits are optional and
   would run at a slow speed.

## Rig results

- Encoder runs opposite to the step counter, scale about 1.0 (`steps=3576 encoder=-3561`).
- Tracking is tight on a clean move: `peakError=24` at 7294 steps/sec.
- A hand-held stall gave `finalError=110`, so the warning threshold is 48 steps.
- Step timing diagnostics were added to investigate grinding at speed: `maxStepGapUs`, `tmcSwitches`, `maxTmcSwitchUs`.
- Sharing Core 1 with the web tasks paused stepping for about 2ms, limiting clean speed to about 12,200 steps/sec. With Core 1 to itself, pauses are 28us and final error is about 1 step.
- 16,000 steps/sec is clean. 18,000 and above stalls at 60,000 steps/sec^2, losing about 4,000 steps per move. Acceleration up to 120,000 did not cause slips at 10,000 steps/sec.
- Peak error on clean moves reached 383, while stalls reached 3,500 or more, so in-move stall detection needs a threshold around 500 steps. End-of-move detection stays at 48.
- Limit switches save the step counter as the limit, so a slip corrupts the saved limit. The counter also restarts at 0 on boot, so saved limits are stale after any reboot.

- TMC2209 answers on UART address 0b00 (version 0x21) with both MS1/MS2 switches up, so firmware register settings reach the driver. Microsteps come from firmware, and moving a switch would change the UART address.
- Step 5 is partly done: the step counter follows the shaft while freewheeling. Limits are still stale after a reboot or slip.

- Saved limits are fixed: calibration is required after boot, MIN is position 0, and repeated calibrations agree within 2 steps. Switch presses correct the position instead of rewriting limits.
- Driver mode updates moved to Core 0 after they paused stepping for about 7ms. Pauses are now about 50us.

## Step 1 measurements to collect

Keep "Freewheel After Movement" off so the rotor is held while the final reading settles.

| Test | Expect |
|---|---|
| Boot | `magnet: detected`, no `noMagnet` warnings |
| Turn the shaft by hand one way, then back | `Encoder moved while motor idle` lines following the shaft |
| 3200 step move each way at low speed | `scale` near +1.000 or -1.000, `finalError` under 16 |
| Same moves at Max Speed | `peakError` gives the lag allowance for step 3 |
| Stall the rack by hand at low speed | `finalError` near a multiple of 64 |
| Any `parityMismatch` warnings | Zero confirms the assumed even parity |

Avoid calibration as a measurement: it reverses without stopping, so it reports as one move.
