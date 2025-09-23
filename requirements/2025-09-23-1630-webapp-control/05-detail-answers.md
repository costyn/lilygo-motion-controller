# Expert Detail Answers

**Timestamp:** 2025-09-23 16:35

## Q6: Should the jog buttons (forward/back) use continuous movement while pressed or discrete step movements?
**Answer:** Continuous. Just be aware the project is not in an industrial setting. It will control lights and small projects in a private (home) setting.

## Q7: Should the webapp validate position limits client-side before sending commands to prevent invalid moves?
**Answer:** Yes, the controller should send those limits upon connect (if the controller does not have this feature yet, it should be made).

## Q8: For the debug console panel, should it auto-scroll to show the latest messages or allow users to scroll back through history?
**Answer:** The user should always be shown the latest messages. Being able to scroll up is nice but not a must.

## Q9: Should the preset positions in Phase 1 include both position and speed settings, or just positions?
**Answer:** Both speed and position.

## Q10: Should the webapp work offline after initial load (Progressive Web App) or require constant connectivity?
**Answer:** Offline mode doesn't make much sense, so yea only works while connected. Buttons and controls should be in a disabled state when offline.