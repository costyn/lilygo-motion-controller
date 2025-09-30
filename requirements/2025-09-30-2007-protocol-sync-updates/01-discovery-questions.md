# Discovery Questions - Phase 2

**Session**: 2025-09-30-2007-protocol-sync-updates
**Phase**: Discovery
**Created**: 2025-09-30T20:07:00Z

## Context

Current implementation analysis:
- Controller has `broadcastStatus()` and `broadcastPosition()` methods
- `broadcastStatus()` is only called on: WebSocket connect and explicit "status" command
- `broadcastPosition()` exists but is **never called** anywhere in the codebase
- Webapp has `PositionUpdate` and `MotorStatus` types already defined
- Motor updates happen in main loop at high frequency, but no automatic broadcasts

## Questions

### Q1: Should status updates be sent at a fixed time interval during movement (e.g., every 100ms)?
**Default if unknown:** Yes (provides consistent update rate for smooth UI updates)

**Rationale:** Fixed intervals are predictable and easier for webapp to handle, preventing UI stuttering and providing smooth progress indication.

---

### Q2: Should the update rate be different for when the motor is moving vs. idle?
**Default if unknown:** Yes (higher rate during movement, lower when idle)

**Rationale:** Reduces bandwidth/CPU usage when motor is idle while providing real-time feedback during movement.

---

### Q3: Should both full status and position-only updates be used during movement?
**Default if unknown:** Yes (position updates more frequent, full status less frequent)

**Rationale:** Position-only updates (smaller payload) can be sent more frequently for smooth UI, while full status provides comprehensive state sync less often.

---

### Q4: Should the controller send a final status update when movement completes?
**Default if unknown:** Yes (ensures webapp knows exact final state)

**Rationale:** Guarantees synchronization at completion, especially important if movement stops early due to emergency stop or limit switches.

---

### Q5: Should updates be sent only when values actually change (delta-based)?
**Default if unknown:** No (send periodic updates even if position unchanged)

**Rationale:** Periodic updates act as "heartbeat" to confirm connection is alive and motor hasn't stalled. Prevents webapp timeout/uncertainty.

---

## Notes

- Current `MotorStatus` includes: position, isMoving, emergencyStop, limitSwitches
- Current `PositionUpdate` includes: position only
- WebSocket infrastructure already supports both message types
- Need to balance update frequency with ESP32 performance and network bandwidth
