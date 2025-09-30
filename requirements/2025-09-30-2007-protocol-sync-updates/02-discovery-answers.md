# Discovery Answers - Phase 2

**Session**: 2025-09-30-2007-protocol-sync-updates
**Date**: 2025-09-30T20:07:00Z

## Answers

### Q1: Should status updates be sent at a fixed time interval during movement?
**Answer:** Yes

**Details:** Regular updates should be sent during movement. When motor is not moving, do not send regular messages.

---

### Q2: Should the update rate be different for when the motor is moving vs. idle?
**Answer:** Yes

**Details:**
- **Moving**: Regular updates at defined interval
- **Idle**: No periodic updates (only on-demand via "status" command or events)

---

### Q3: Should both full status and position-only updates be used during movement?
**Answer:** Yes (default)

**Details:** Use two different message types with different frequencies:
- **Position-only updates**: More frequent (lightweight payload)
- **Full status updates**: Less frequent (comprehensive state)

---

### Q4: Should the controller send a final status update when movement completes?
**Answer:** Yes

**Details:** When motor reaches target or stops (emergency/limit), immediately send final status update to ensure webapp has exact final state.

---

### Q5: Should updates be sent only when values actually change (delta-based)?
**Answer:** No (default - periodic updates)

**Details:** Send periodic updates at regular intervals regardless of value changes. This acts as a "heartbeat" to detect connection/motor issues.

---

## Summary

**Update Strategy:**
1. **During Movement**:
   - Send position-only updates frequently (e.g., every 100ms)
   - Send full status updates less frequently (e.g., every 500ms)
   - Continue regardless of whether values change (heartbeat)

2. **When Idle**:
   - No periodic updates
   - Only respond to explicit "status" commands
   - Only send updates on state changes (e.g., emergency stop triggered)

3. **On Movement Completion**:
   - Send immediate final status update
   - Ensures webapp has exact final state

4. **Message Types**:
   - `PositionUpdate`: Lightweight, frequent
   - `MotorStatus`: Comprehensive, less frequent
