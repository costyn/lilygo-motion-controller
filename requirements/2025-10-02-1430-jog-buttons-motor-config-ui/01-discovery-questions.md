# Discovery Questions

**Phase:** Context Discovery
**Date:** 2025-10-02

## Q1: Should the jog buttons continue movement while held down without sending repetitive move commands?
**Default if unknown:** Yes (continuous jogging is standard UX for motor control interfaces - hold to move, release to stop)

## Q2: Should the motor config UI be accessible on mobile devices in a touch-friendly way?
**Default if unknown:** Yes (the entire webapp is mobile-responsive, config UI should match)

## Q3: Should changes to motor config parameters be immediately saved to the controller's NVRAM?
**Default if unknown:** Yes (persist settings permanently so they survive reboots - matches limit switch position learning behavior)

## Q4: Should the config UI include validation to prevent invalid values that could damage hardware?
**Default if unknown:** Yes (safety first - prevent overspeed, invalid acceleration, or out-of-bounds limits)

## Q5: Should users be able to revert config changes without applying them?
**Default if unknown:** Yes (allow users to experiment with values and cancel if they don't want to commit changes)
