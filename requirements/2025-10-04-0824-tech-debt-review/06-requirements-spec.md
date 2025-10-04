# Requirements Specification - Tech Debt Refactoring

## Problem Statement

The LilyGo Motion Controller codebase has accumulated technical debt during rapid feature development. Key issues:
- Stop command confusion (3 overlapping methods, duplicate handlers)
- WebSocket message handler complexity (220-line if-else chain)
- Code duplication in limit switch handlers
- Business logic in main.cpp instead of modules
- Naming inconsistencies between backend and frontend

## Solution Overview

Refactor for clarity, maintainability, and consistency without changing functionality:
1. Clean up stop command semantics (2 clear methods instead of 3 confusing ones)
2. Modernize WebSocket handling with dispatch table pattern
3. Extract button logic to dedicated ButtonController module
4. Unify duplicate switch handlers (DRY principle)
5. Fix frontend/backend command naming mismatches

## Functional Requirements

### FR-1: Stop Command Cleanup
**Priority:** CRITICAL
- Remove confusing `MotorController::stop()` method
- Keep `emergencyStop()` - stops motor AND sets flag (requires manual reset)
- Keep `stopGently()` - stops motor WITHOUT flag (for jog stop)
- Rename to `jogStop()` for clarity (matches WebSocket command name)
- Fix WebSocket handlers: one "emergencyStop" command, keep "jogStop" command
- Fix WebApp to send correct "emergencyStop" command (not "emergency-stop")

### FR-2: WebSocket Dispatch Table
**Priority:** HIGH
- Replace 220-line if-else chain with dispatch table (map/switch pattern)
- Extract command handlers into separate methods
- Eliminate duplicate JSON serialization for config messages
- Centralize `broadcastStatus()` calls (currently scattered 8 times)

### FR-3: ButtonController Module
**Priority:** MEDIUM
- Create new module: `src/modules/ButtonController/`
- Move all button logic from main.cpp to ButtonController
- Follow existing modular architecture pattern
- Initialize in `setup()`, update in `InputTask`

### FR-4: Unify Switch Handlers
**Priority:** MEDIUM
- Replace `onSwitch1Pressed` and `onSwitch2Pressed` with single parameterized function
- Reduce code duplication (DRY principle)
- Maintain same functionality

### FR-5: Emergency Stop UI
**Priority:** MEDIUM
- Wire up existing `emergencyStop()` method in WebApp
- Add emergency stop button to UI (safety-critical feature)
- Ensure it sends correct "emergencyStop" command

## Technical Requirements

### TR-1: C++ Files to Modify

**MotorController Module:**
- `src/modules/MotorController/MotorController.h`
  - Remove `void stop();` declaration
  - Rename `void stopGently();` → `void jogStop();`
  - Keep `void emergencyStop();`

- `src/modules/MotorController/MotorController.cpp`
  - Remove `stop()` implementation
  - Rename `stopGently()` → `jogStop()`
  - Update `emergencyStop()` to do the work directly (not call removed `stop()`)

**WebServer Module:**
- `src/modules/WebServer/WebServer.h`
  - Add command handler method declarations:
    ```cpp
    void handleMoveCommand(JsonDocument& doc);
    void handleJogStartCommand(JsonDocument& doc);
    void handleJogStopCommand(JsonDocument& doc);
    void handleEmergencyStopCommand(JsonDocument& doc);
    void handleResetCommand(JsonDocument& doc);
    void handleStatusCommand(JsonDocument& doc);
    void handleGetConfigCommand(JsonDocument& doc);
    void handleSetConfigCommand(JsonDocument& doc);
    ```

- `src/modules/WebServer/WebServer.cpp`
  - Replace if-else chain in `handleWebSocketMessage()` with dispatch table
  - Remove duplicate "stop" handler (lines 339-343)
  - Rename first "stop" handler to "emergencyStop"
  - Extract each command block into separate handler method
  - Deduplicate config JSON serialization

**Main.cpp:**
- `src/main.cpp`
  - Remove button declarations (OneButton instances)
  - Remove button callback functions
  - Add ButtonController include and initialization
  - Update InputTask to call buttonController.update()

**New ButtonController Module:**
- `src/modules/ButtonController/ButtonController.h` (NEW)
  - Class definition with OneButton members
  - Public: begin(), update()
  - Private: callback methods

- `src/modules/ButtonController/ButtonController.cpp` (NEW)
  - Move button logic from main.cpp
  - Implement unified switch handler

### TR-2: WebApp Files to Modify

**useMotorController Hook:**
- `webapp/src/hooks/useMotorController.tsx`
  - Line 181: Change `"emergency-stop"` → `"emergencyStop"`
  - Keep `jogStop()` method as-is (already correct)

**UI Component (TBD):**
- Add emergency stop button to appropriate UI component
- Wire to `emergencyStop()` method from hook

### TR-3: Command Protocol Changes

**WebSocket Commands (Backend):**
- `"move"` - move to position (unchanged)
- `"jogStart"` - start continuous jog (unchanged)
- `"jogStop"` - gentle stop, no flag (unchanged)
- `"emergencyStop"` - emergency stop with flag (NEW, replaces "stop")
- `"reset"` - clear emergency stop (unchanged)
- `"status"` - get status (unchanged)
- `"getConfig"` - get config (unchanged)
- `"setConfig"` - set config (unchanged)

**Legacy Support:**
- Keep `"goto"` as alias for `"move"` (backward compat)
- Keep `"cmd"` field support alongside `"command"`

### TR-4: Build Constraints

- Monitor flash usage (currently at 83.1%)
- Expected change: -500 to -1000 bytes (deduplication savings)
- RAM usage should remain stable (±100 bytes)
- Compile check after each module change: `pio run`
- WebApp type check: `npm run build` (tsc)

## Implementation Hints

### Dispatch Table Pattern (C++)

Replace this:
```cpp
if (command == "move") {
    // 20 lines
} else if (command == "jogStart") {
    // 15 lines
} else if (command == "jogStop") {
    // 5 lines
}
// ... 10+ more commands
```

With this:
```cpp
// Type alias for handler function pointer
using CommandHandler = void (WebServerClass::*)(JsonDocument&);

// Dispatch table (in .cpp file)
static const std::map<String, CommandHandler> commandHandlers = {
    {"move", &WebServerClass::handleMoveCommand},
    {"goto", &WebServerClass::handleMoveCommand},  // alias
    {"jogStart", &WebServerClass::handleJogStartCommand},
    {"jogStop", &WebServerClass::handleJogStopCommand},
    {"emergencyStop", &WebServerClass::handleEmergencyStopCommand},
    {"reset", &WebServerClass::handleResetCommand},
    {"status", &WebServerClass::handleStatusCommand},
    {"getConfig", &WebServerClass::handleGetConfigCommand},
    {"setConfig", &WebServerClass::handleSetConfigCommand}
};

// In handleWebSocketMessage():
auto it = commandHandlers.find(command);
if (it != commandHandlers.end()) {
    (this->*(it->second))(doc);  // Call handler
} else {
    LOG_WARN("Unknown command: %s", command.c_str());
}
```

### Unified Switch Handler Pattern

Replace this:
```cpp
void onSwitch1Pressed() {
    long position = motorController.getCurrentPosition();
    config.setLimitPos1(position);
    // ... more code
}

void onSwitch2Pressed() {
    long position = motorController.getCurrentPosition();
    config.setLimitPos2(position);
    // ... more code
}
```

With this:
```cpp
void onSwitchPressed(int switchNum) {
    long position = motorController.getCurrentPosition();
    if (switchNum == 1) {
        config.setLimitPos1(position);
    } else {
        config.setLimitPos2(position);
    }
    // ... shared code
}

// Callbacks become:
void onSwitch1Pressed() { onSwitchPressed(1); }
void onSwitch2Pressed() { onSwitchPressed(2); }
```

## Acceptance Criteria

### AC-1: Stop Commands Work Correctly
- [ ] `jogStop()` stops motor without setting emergency flag
- [ ] `emergencyStop()` stops motor AND sets emergency flag
- [ ] Frontend emergency stop button sends correct command
- [ ] Emergency stop requires manual reset (existing "reset" command)
- [ ] No more duplicate "stop" handlers in WebServer

### AC-2: Code Quality Improved
- [ ] `handleWebSocketMessage()` uses dispatch table (no if-else chain)
- [ ] No duplicate JSON serialization code
- [ ] Button logic in ButtonController module (not main.cpp)
- [ ] Switch handlers unified (DRY compliant)

### AC-3: Build Success
- [ ] C++ project compiles: `pio run` succeeds
- [ ] Flash usage ≤ 83.1% (ideally reduced)
- [ ] RAM usage ≤ 16.0%
- [ ] WebApp compiles: `npm run build` succeeds
- [ ] TypeScript type checks pass

### AC-4: Functionality Preserved
- [ ] All existing WebSocket commands work
- [ ] Motor control behaves identically
- [ ] Limit switches work as before
- [ ] Config save/load unchanged
- [ ] UI controls functional

### AC-5: Testing
- [ ] Run existing unit tests: `pio test -e native`
- [ ] All tests pass (20 tests for MotorController)
- [ ] Manual verification of button behavior (if hardware available)

## Assumptions

1. No hardware testing required for this refactor (code-only changes)
2. Existing unit tests provide adequate regression coverage
3. WebSocket clients can handle "emergencyStop" command (no deployed systems)
4. ButtonController module follows same pattern as existing modules
5. Dispatch table overhead (<1KB flash) is acceptable for cleaner code

## Out of Scope

- New features or functionality
- Performance optimization beyond what refactoring provides
- Additional unit test coverage (use existing tests only)
- UI/UX redesign (just wire up emergency stop)
- Documentation updates (code comments sufficient)

## Risks & Mitigations

### Risk 1: Breaking Changes
**Mitigation:** Incremental changes with compile check after each module

### Risk 2: Flash Overflow
**Mitigation:** Monitor build output, dispatch table should actually reduce size

### Risk 3: Runtime Bugs
**Mitigation:** Compile checks, existing unit tests, careful review of stop logic

### Risk 4: Missing Stop Command Handler
**Mitigation:** Test both stop variants explicitly (jog and emergency)
