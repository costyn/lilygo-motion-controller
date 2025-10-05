# Discovery Answers

## Q1: Should the freewheel setting apply to ALL movement types (jog, slider, quick positions)?
**Answer:** Yes

## Q2: Should the freewheel setting be user-configurable via the web interface?
**Answer:** Yes, there is an existing settings dialog, MotorConfigDialog, in the webapp where this setting should be

## Q3: When freewheel is disabled, should the motor hold position indefinitely or only for a timeout period?
**Answer:** Indefinitely

## Q4: For the mode switching bug, should automatic switching be based on actual motor speed or requested speed?
**Answer:** Actual would be better, but be aware the encoder currently does not report any useful data. I have some hardware issues with mounting the encoder to the motor. But for testing purposes, the output shaft is not connecting to anything so shouldn't miss any steps and be accurate in following step commands from the controller.

**Key Insight:** Since encoder is not working, we should use the commanded/requested speed from AccelStepper as a fallback for mode switching logic.

## Q5: Should the motor buzzing issue be fixed by properly configuring hold current, or by always freewheeling after movement?
**Answer:** Configure hold properly
