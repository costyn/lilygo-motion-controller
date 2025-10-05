# Initial Request

**Date:** 2025-10-05 12:11
**Session:** Freewheel and Mode Switching

## User Request

I want to address a couple features/bugs in this session:
- [ ] Config feature to add: freewheel after movement or not. Partially implemented now by manipulating digitalWrite(EN_PIN, HIGH); in various places, but it now only works after jogging and e-stop. Does not work after slider or quick positions
- [ ] Bug: when not in freewheel mode after movement, the motor slightly buzzes and gets warm, also the tmc controller gets warm. It's doing something weird.
- [ ] Bug: I never see the automatic switch between stealthchop and spreadcycle happen in the logs

## Summary

Three items to address:
1. **Freewheel Configuration**: Add proper configuration for freewheel-after-movement behavior, currently inconsistent across different movement commands
2. **Motor Buzzing Bug**: Motor and TMC controller getting warm with buzzing when freewheel disabled
3. **Mode Switching Bug**: StealthChop/SpreadCycle automatic switching not logging or not working
