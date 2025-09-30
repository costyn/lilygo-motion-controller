# Detail Questions - Phase 4

**Session**: 2025-09-30-2007-protocol-sync-updates
**Phase**: Expert Requirements
**Created**: 2025-09-30T20:20:00Z

## Technical Implementation Questions

Now that I understand the codebase architecture, I need to clarify specific implementation details:

---

### Q1: Should position updates be every 100ms and full status every 500ms?
**Default if unknown:** Yes (100ms position, 500ms full status)

**Context:** WebServerTask runs every 50ms, so we can check timing precisely. 100ms gives smooth UI updates (10 updates/sec), while 500ms reduces bandwidth for full status (2 updates/sec).

**Alternative:** Different intervals like 200ms position / 1000ms status if bandwidth is a concern.

---

### Q2: Should we add debug logging for broadcast events (when updates are sent)?
**Default if unknown:** Yes (LOG_DEBUG level for timing visibility)

**Context:** Would log messages like:
- `[DEBUG] Broadcasting position update: 1500 (100ms elapsed)`
- `[DEBUG] Broadcasting full status (500ms elapsed)`
- `[DEBUG] Movement completed - sending final status`

This helps with development/debugging but can be compiled out for production.

---

### Q3: Should broadcast intervals be hard-coded constants or configurable defines?
**Default if unknown:** Yes (use #define constants for easy tuning)

**Context:** Using defines like:
```cpp
#define POSITION_BROADCAST_INTERVAL_MS 100
#define STATUS_BROADCAST_INTERVAL_MS 500
```

Allows easy adjustment without modifying logic. Could potentially be made runtime-configurable later.

---

### Q4: When emergency stop is triggered, should it immediately send a status update?
**Default if unknown:** Yes (immediate broadcast on emergency stop)

**Context:** Emergency stop is called from `WebServer.cpp` line 326 and button callbacks. Adding an immediate broadcast ensures webapp instantly reflects the stopped state, in addition to the final status when movement actually completes.

---

### Q5: Should the first broadcast happen immediately when movement starts, or wait for the first interval?
**Default if unknown:** Yes (immediate broadcast on movement start)

**Context:** When `moveTo()` is called, should we send an immediate status update showing `isMoving: true`, or wait up to 100ms for the first scheduled broadcast? Immediate update provides better UX responsiveness.

---

## Implementation Notes

All questions relate to the modifications in `src/modules/WebServer/WebServer.cpp` and `WebServer.h`. The timing logic will be added to the existing `update()` method which is called every 50ms from WebServerTask.
