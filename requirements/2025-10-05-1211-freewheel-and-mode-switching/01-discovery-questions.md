# Discovery Questions

## Q1: Should the freewheel setting apply to ALL movement types (jog, slider, quick positions)?
**Default if unknown:** Yes (consistent behavior across all movement types is better UX)

## Q2: Should the freewheel setting be user-configurable via the web interface?
**Default if unknown:** Yes (users should be able to toggle this behavior without reflashing firmware)

## Q3: When freewheel is disabled, should the motor hold position indefinitely or only for a timeout period?
**Default if unknown:** Indefinitely (safer default - motor maintains position until next command)

## Q4: For the mode switching bug, should automatic switching be based on actual motor speed or requested speed?
**Default if unknown:** Actual motor speed from encoder (more accurate representation of real motor state)

## Q5: Should the motor buzzing issue be fixed by properly configuring hold current, or by always freewheeling after movement?
**Default if unknown:** Configure hold current properly (allows users to choose between holding and freewheeling)
