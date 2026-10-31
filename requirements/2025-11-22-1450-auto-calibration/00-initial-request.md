# Initial Request

**Date:** 2025-11-22 14:50
**Feature:** Auto-Calibration on Power-On

## User Request

In this session we are going to be implementing an auto-calibration feature. On power-on, the calibration should be performed.

1. Move to lower limit, set as 0%.
2. Move to upper limit, set as 100%.
3. Store total steps and report to webapp when new limits are set.
4. Add a Recalibrate button to the Settings dialog in the webapp, which will re-initiate calibration.

## Initial Observations

- This is a hardware calibration feature for a stepper motor controller
- Requires interaction with limit switches
- Needs to persist calibration data
- Includes both firmware and webapp components
- Safety-critical: involves automated motor movement
