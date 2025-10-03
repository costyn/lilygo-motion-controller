# Discovery Answers

**Phase:** Context Discovery
**Date:** 2025-10-02

## Q1: Should the jog buttons continue movement while held down without sending repetitive move commands?
**Answer:** Yes

---

## Q2: Should the motor config UI be accessible on mobile devices in a touch-friendly way?
**Answer:** Yes, the UI should be mobile friendly, also the config part. A button at the top to a 2nd page would also be acceptable. I don't want the front page to become too cluttered. There's already button for OTA and Darkmode. Another would be ok.

---

## Q3: Should changes to motor config parameters be immediately saved to the controller's NVRAM?
**Answer:** Yes, config should be saved to NVRAM

---

## Q4: Should the config UI include validation to prevent invalid values that could damage hardware?
**Answer:** Yes

---

## Q5: Should users be able to revert config changes without applying them?
**Answer:** Sure if it's not too much work. A revert/cancel/reset button would be nice

---

## Additional Context from Q&A

### Jog Implementation Approach
**Clarification**: User prefers Option B - True continuous movement with backend changes:
- New backend commands: `jogStart` (with direction) and `jogStop`
- Controller moves continuously while jog active
- Stop command halts immediately
- Requires C++ backend implementation but provides best UX

---

