# Detail Answers - Tech Debt Review

## Q1: Should we completely remove the confusing `stop()` method and consolidate to just `emergencyStop()` and `stopGently()`?
**Answer:** Yes

**Details:** Remove the misleading `stop()` method. Keep two well-named methods with clear purposes.

---

## Q2: Should the refactored WebSocket handlers use a dispatch table (map/switch) or individual if-else checks?
**Answer:** Yes

**Details:** Use dispatch table for cleaner, more maintainable code. User is intrigued by the concept and wants to see it implemented.

---

## Q3: Should button logic stay in main.cpp or be extracted to a new ButtonController module?
**Answer:** Yes

**Details:** Extract to ButtonController module following existing modular architecture pattern.

---

## Q4: Should we unify the similar switch handlers (onSwitch1Pressed/onSwitch2Pressed) into a single parameterized function?
**Answer:** Yes

**Details:** Apply DRY principle to reduce code duplication and maintenance burden.

---

## Q5: Should the WebApp's unused `emergencyStop()` method be removed or properly wired up to UI?
**Answer:** No (don't remove)

**Details:** Wire up to UI properly. Emergency stop is safety-critical functionality that should be accessible.

---
