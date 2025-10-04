# Detail Questions - Tech Debt Review

**Purpose:** Clarify implementation decisions now that we understand the code deeply

---

## Q1: Should we completely remove the confusing `stop()` method and consolidate to just `emergencyStop()` and `stopGently()`?
**Default if unknown:** Yes (two clear methods better than three confusing ones)

**Rationale:** Currently `stop()` actually sets emergency flag but has misleading name. `emergencyStop()` just wraps it. Simpler to have two well-named methods.

---

## Q2: Should the refactored WebSocket handlers use a dispatch table (map/switch) or individual if-else checks?
**Default if unknown:** Yes (dispatch table for O(1) lookup vs O(n) if-else)

**Rationale:** With 10+ commands, a dispatch table is more maintainable. Slightly more flash but much cleaner code.

---

## Q3: Should button logic stay in main.cpp or be extracted to a new ButtonController module?
**Default if unknown:** Yes (extract to ButtonController module)

**Rationale:** Follows existing modular architecture pattern. Makes main.cpp cleaner and button logic testable.

---

## Q4: Should we unify the similar switch handlers (onSwitch1Pressed/onSwitch2Pressed) into a single parameterized function?
**Default if unknown:** Yes (DRY principle, reduce duplication)

**Rationale:** Code duplication maintenance burden. Need to read handlers first to confirm similarity.

---

## Q5: Should the WebApp's unused `emergencyStop()` method be removed or properly wired up to UI?
**Default if unknown:** No (wire up to UI, don't remove)

**Rationale:** Emergency stop is safety-critical functionality. Should be accessible in UI, just currently not implemented.

---
