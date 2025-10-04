# Context Findings - Tech Debt Review

## Professional Analysis: Abstraction Layer Question

### Current Architecture Assessment

**WebServer ↔ MotorController Coupling:**
After reviewing the codebase, I have a **strong professional opinion: You do NOT need an abstraction layer**. Here's why:

#### ✅ What's Actually Good:
1. **Clean Module Boundaries**: Your modules are well-separated with clear responsibilities
2. **Direct Communication Works**: WebServer → MotorController is straightforward and appropriate
3. **Low Complexity**: The current coupling is thin - WebServer mostly calls public methods
4. **Safety Already Exists**: MotorController has MIN/MAX validation built-in (lines 36-39, 247-281)

#### ❌ Why Abstraction Layer Would Be Bad:
1. **Unnecessary Indirection**: Would add complexity without solving a real problem
2. **ESP32 Constraints**: Extra layer = more RAM/Flash usage for no benefit
3. **No Multiple Implementations**: You have one motor controller, not multiple drivers
4. **YAGNI Violation**: "You Aren't Gonna Need It" - classic over-engineering

#### 🎯 The Real Problem (Not Coupling):
Looking at `WebServer.cpp`, the issue isn't "too much coupling" - it's **code organization within WebServer**:

- **Lines 207-427**: `handleWebSocketMessage()` is a 220-line function with massive if-else chains
- **Duplicate JSON serialization**: Lines 355-365, 410-420 (config messages)
- **Duplicate broadcasts**: `broadcastStatus()` called 8 times throughout
- **Mixed concerns**: Command parsing, validation, business logic all mixed

### Real Issues Identified

## Issue #1: handleWebSocketMessage() Complexity ⭐ CRITICAL

**Location:** `src/modules/WebServer/WebServer.cpp:207-427`

**Problem:**
- 220-line function with nested if-else chains
- Code duplication: JSON creation for config messages (lines 355-365, 410-420)
- Scattered `broadcastStatus()` calls (8 occurrences)
- Command validation mixed with business logic

**Solution:** Extract command handlers into separate methods using Command Pattern:
```cpp
// Instead of one giant function, use dispatch table:
void handleMoveCommand(JsonDocument& doc);
void handleJogStartCommand(JsonDocument& doc);
void handleConfigCommand(JsonDocument& doc);
```

**Impact:**
- Reduces complexity from O(n) if-else to O(1) lookup
- Easier testing (each handler is testable)
- Eliminates duplication
- ~500 bytes flash savings from deduplication

**Files to modify:**
- `src/modules/WebServer/WebServer.h` - add command handler methods
- `src/modules/WebServer/WebServer.cpp` - refactor handleWebSocketMessage()

---

## Issue #2: Duplicate Switch Handlers (DRY Violation)

**Location:** `src/main.cpp` (need to read to confirm)

**Problem:** `onSwitch1Pressed` and `onSwitch2Pressed` are very similar

**Investigation needed:**
- Read main.cpp button handlers
- Identify common patterns
- Extract to parameterized function

**Expected Solution:**
```cpp
void handleLimitSwitchPressed(int switchNumber, long position) {
    // Unified logic
}
```

---

## Issue #3: Naming Inconsistency: stopGently() vs jogStop

**Location:**
- C++: `src/modules/MotorController/MotorController.cpp:130` (stopGently)
- WebSocket command: "jogStop" (line 333)

**Problem:**
- Backend method: `stopGently()`
- WebSocket command: `"jogStop"`
- Confusing disconnect between API and implementation

**Solutions (pick one):**
1. **Rename C++ method:** `stopGently()` → `jogStop()` (RECOMMENDED)
2. **Rename WebSocket command:** `"jogStop"` → `"stopGently"`
3. **Add alias:** Keep both, document why

**Recommendation:** Option 1 - rename C++ to `jogStop()` for consistency

**Files to modify:**
- `src/modules/MotorController/MotorController.h:54`
- `src/modules/MotorController/MotorController.cpp:130`
- `src/modules/WebServer/WebServer.cpp:335` (call site)

---

## Issue #4: Hardware Button Logic in main.cpp

**Location:** `src/main.cpp:38-40, 50-55` (button declarations and callbacks)

**Problem:** main.cpp contains business logic that should be in a module

**Solution:** Create `ButtonController` module:
```
src/modules/ButtonController/
├── ButtonController.h
└── ButtonController.cpp
```

**Design:**
```cpp
class ButtonController {
public:
    bool begin();
    void update(); // Called from InputTask
private:
    OneButton button1, button2, button3;
    void onButton1Press();
    void onButton2Click();
    void onButton3Press();
};
```

**Files to create:**
- `src/modules/ButtonController/ButtonController.h`
- `src/modules/ButtonController/ButtonController.cpp`

**Files to modify:**
- `src/main.cpp` - remove button logic, add ButtonController

---

## Issue #5: Duplicate "stop" Command Handlers

**Location:** `src/modules/WebServer/WebServer.cpp`

**Problem:** TWO handlers for "stop" command:
- Line 280-287: First "stop" handler (calls `motorController.stop()`)
- Line 339-343: Second "stop" handler (calls `motorController.emergencyStop()`)

**This is a BUG!** Second handler will never execute due to first match.

**Solution:**
- Keep ONE "stop" handler → emergency stop
- "jogStop" already exists for gentle stop

**Files to modify:**
- `src/modules/WebServer/WebServer.cpp:280-287` (remove duplicate)

---

## WebApp Code Review Findings

### Overall Assessment: ✅ VERY CLEAN

**Strengths:**
- Well-structured React with TypeScript
- Clean separation: hooks, components, types
- Proper state management with refs for jog controls
- Good WebSocket handling with reconnection logic

### Minor Issues Found:

#### WebApp Issue #1: Duplicate emergencyStop Command

**Location:** `webapp/src/hooks/useMotorController.tsx:180-182`

**Problem:**
```typescript
const emergencyStop = useCallback(() => {
  return sendCommand({ command: 'emergency-stop' })  // ← Wrong command!
}, [sendCommand])
```

**Backend expects:** `"stop"` (for emergency stop)
**Frontend sends:** `"emergency-stop"` (not handled!)

**Solution:** Change to `{ command: 'stop' }` or align with backend

---

#### WebApp Issue #2: Unused Emergency Stop Method

**Location:** `webapp/src/hooks/useMotorController.tsx:180-182`

**Problem:** `emergencyStop()` method is defined but never used in components

**Investigation:** Check if this is intentional or dead code

---

## Summary of Changes Required

### High Priority (Critical):
1. ✅ Fix duplicate "stop" handler bug (WebServer.cpp)
2. ✅ Refactor handleWebSocketMessage() into command handlers
3. ✅ Fix emergencyStop command mismatch (webapp)

### Medium Priority (Quality):
4. ✅ Extract button logic to ButtonController module
5. ✅ Unify switch handlers (DRY violation)
6. ✅ Rename stopGently() → jogStop() for consistency

### Impact Assessment:
- **Flash usage:** Expect ~500-1000 bytes reduction from deduplication
- **RAM usage:** Minimal change (±100 bytes)
- **Build risk:** LOW (changes are incremental and testable)
- **Breaking changes:** None if done carefully

### Test Strategy:
1. Run existing unit tests before/after
2. Compile check: `pio run` after each module change
3. WebApp compile: `npm run build` (tsc check)
4. Manual verification needed for button logic

---

## Files Requiring Modification

### C++ Files:
1. `src/modules/WebServer/WebServer.h` - add command handler methods
2. `src/modules/WebServer/WebServer.cpp` - refactor message handling
3. `src/modules/MotorController/MotorController.h` - rename stopGently
4. `src/modules/MotorController/MotorController.cpp` - rename stopGently
5. `src/main.cpp` - extract button logic
6. `src/modules/ButtonController/ButtonController.h` - NEW
7. `src/modules/ButtonController/ButtonController.cpp` - NEW

### WebApp Files:
8. `webapp/src/hooks/useMotorController.tsx` - fix emergencyStop command

### Testing:
9. `test/test_motor_controller/test_motor.cpp` - verify after changes

---

## Architectural Recommendations

### ✅ Keep Current Design:
- Direct WebServer → MotorController communication
- No abstraction layer needed
- Module boundaries are appropriate

### 🔧 Improve Code Quality:
- Command Pattern for WebSocket handlers
- Extract repeated patterns (DRY)
- Move business logic out of main.cpp
- Consistent naming across layers

### 📋 Future Considerations:
- Consider adding integration tests for WebSocket protocol
- Document command protocol in OpenAPI/JSON schema
- Add enum for command strings to prevent typos
