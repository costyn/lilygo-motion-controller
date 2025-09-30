# Detail Answers - Phase 4

**Session**: 2025-09-30-2007-protocol-sync-updates
**Date**: 2025-09-30T20:25:00Z

## Technical Implementation Answers

### Q1: Should position updates be every 100ms and full status every 500ms?
**Answer:** Yes

**Details:**
- Position updates: 100ms (10 updates/second)
- Full status updates: 500ms (2 updates/second)
- **Only during movement** - no periodic broadcasts when idle
- WebServerTask runs every 50ms, perfect for timing checks

---

### Q2: Should we add debug logging for broadcast events?
**Answer:** Yes

**Details:**
- Use LOG_DEBUG level for broadcast events
- Log position updates, status updates, and movement completion
- User noted debug WebSocket available at `/debug` for testing
- Can be compiled out for production if needed

---

### Q3: Should broadcast intervals be hard-coded or use defines?
**Answer:** Use #define constants

**Details:**
- User preference: "please always use defines for 'magic numbers'"
- Define constants like:
  - `POSITION_BROADCAST_INTERVAL_MS 100`
  - `STATUS_BROADCAST_INTERVAL_MS 500`
- Makes tuning easy without changing logic

---

### Q4: Should emergency stop trigger immediate status broadcast?
**Answer:** Yes

**Details:**
- Immediately broadcast status when `emergencyStop()` is called
- Ensures webapp instantly shows emergency stop state
- In addition to final status when movement completes

---

### Q5: Should first broadcast happen immediately when movement starts?
**Answer:** Yes

**Details:**
- Immediately send status update when `moveTo()` is called
- Shows `isMoving: true` without waiting for first interval
- Better UX - instant feedback in webapp

---

## Implementation Summary

**Broadcast Strategy:**
1. **Movement Start**: Immediate status broadcast
2. **During Movement**:
   - Position updates every 100ms
   - Full status every 500ms
3. **Emergency Stop**: Immediate status broadcast
4. **Movement End**: Immediate final status broadcast
5. **Idle**: No periodic broadcasts

**Technical Approach:**
- Add timing state to `WebServerClass` (3 unsigned long variables + 1 bool)
- Enhance `update()` method with broadcast timing logic
- Use #define constants for all intervals
- Add LOG_DEBUG logging for broadcast events
- Leverage existing `broadcastStatus()` and `broadcastPosition()` methods
