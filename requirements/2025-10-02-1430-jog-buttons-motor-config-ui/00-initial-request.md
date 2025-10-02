# Initial Request

**Date:** 2025-10-02 14:30
**Session ID:** jog-buttons-motor-config-ui

## User Request

In this requirements session, we will be implementing 2 specific features on the webapp. Please see README.md and CLAUDE.md for more info. Also previous requirements sessions can be found in requirements/

### Feature/Fix 1: Jog Button Behavior
The "jog" buttons don't work as they should. Clicking them does move the motor a little bit, but holding it down does not make it keep going. Let's try a bit to see if we can get it working; if not I have a plan B.

### Feature 2: Motor Config UI
The MotorConfig has a type on both sides, and motorconfig parameters can be set and retrieved through the websocket (this works), but there is no place yet in the UI to manipulate these parameters by the end user.

I'm not sure what would be good UX: make a separate panel below the debug panel or add a hamburger menu type popover. What do you think?

(As far as I know) the controller sends the config upon connect (or the webapp asks, I'm not sure), so the parameters should be set in the config section. If adjusted and the user hits 'apply', the new settings are sent to the controller.

## Context
- Project: LilyGo Motion Controller webapp
- Previous requirements sessions available in requirements/ folder
- WebSocket communication already functional
- Motor config can be set/retrieved via WebSocket
