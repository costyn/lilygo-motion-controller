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
