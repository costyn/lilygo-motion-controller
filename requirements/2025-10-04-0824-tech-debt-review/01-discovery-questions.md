# Discovery Questions - Tech Debt Review

**Purpose:** Understand scope, priorities, and constraints for refactoring work

---

## Q1: Should the refactoring maintain 100% backward compatibility with existing WebSocket protocol?
**Default if unknown:** Yes (breaking changes require client updates and could affect deployed systems)

**Rationale:** The WebSocket protocol is used by the webapp and potentially other clients. Breaking changes would require coordinated updates.

---

## Q2: Are there any real-world deployment constraints that limit refactoring scope (e.g., memory, flash)?
**Default if unknown:** Yes (ESP32 has limited resources, current build is at 83.1% flash)

**Rationale:** Project is already at high flash usage. Major architectural changes could push beyond available resources.

---

## Q3: Should the new abstraction layer (datamodel) include validation/safety limits for all motor parameters?
**Default if unknown:** Yes (safety-critical system controlling physical hardware)

**Rationale:** Motor control systems require validation to prevent damage to hardware or injury.

---

## Q4: Can existing unit tests be used as regression tests during refactoring?
**Default if unknown:** Yes (tests exist for MotorController calculations)

**Rationale:** The project has native unit tests in place that should verify refactoring doesn't break core logic.

---

## Q5: Should this refactoring address all identified issues in one session, or prioritize the most critical ones first?
**Default if unknown:** No (prioritize critical issues first - abstraction layer and WebSocket handler)

**Rationale:** Large-scale refactoring carries risk. Incremental improvements with testing between changes is safer.

---
