# Unit Test Coverage Expansion Plan

**Date:** 2025-10-04
**Current Coverage:** 20 tests (MotorController calculations only)
**Goal:** Comprehensive coverage for all modules and critical paths

---

## Current State Analysis

### ✅ What's Currently Tested (C++)
- `MotorController::calculateSpeed()` - 5 tests
- `MotorController::updateTMCMode()` - 5 tests
- **Total: 10 test cases**

### ❌ What's NOT Tested (C++)
- **Configuration Module** - 0 tests
- **LimitSwitch Module** - 0 tests
- **ButtonController Module** - 0 tests
- **WebServer Module** - 0 tests
- **MotorController** (beyond calculations):
  - `moveTo()` parameter validation
  - `jogStop()` behavior
  - `emergencyStop()` flag management
  - Speed/acceleration clamping

### ❌ What's NOT Tested (WebApp)
- **No tests at all** - webapp has zero test coverage
- Critical hook logic untested
- WebSocket communication untested
- UI component state management untested

---

## Phase 1: C++ Core Module Tests (HIGH PRIORITY)

### 1.1 Configuration Module Tests
**File:** `test/test_native/test_configuration/test_configuration.cpp`

**Test Cases (15 tests):**

#### Parameter Validation (5 tests)
```cpp
- test_setMaxSpeed_valid_range
- test_setMaxSpeed_clamps_below_min    // Should clamp to MIN_SPEED (100)
- test_setMaxSpeed_clamps_above_max    // Should clamp to MAX_SPEED (100000)
- test_setAcceleration_valid_range
- test_setAcceleration_clamps_extremes
```

#### Limit Position Management (5 tests)
```cpp
- test_setLimitPos1_stores_correctly
- test_setLimitPos2_stores_correctly
- test_getMinLimit_returns_correct_value
- test_getMaxLimit_returns_correct_value
- test_saveLimitPositions_persists_both
```

#### StealthChop Mode (3 tests)
```cpp
- test_setUseStealthChop_true
- test_setUseStealthChop_false
- test_getUseStealthChop_returns_current_state
```

#### Persistence Logic (2 tests)
```cpp
- test_saveConfiguration_called
- test_loadConfiguration_restores_defaults
```

**Implementation Notes:**
- Use mock Preferences class (no actual NVRAM access)
- Test validation logic without ESP32 dependencies
- Focus on bounds checking and state management

---

### 1.2 MotorController Extended Tests
**File:** `test/test_native/test_motor_controller/test_motor_controller_extended.cpp`

**Test Cases (15 tests):**

#### Movement Commands (5 tests)
```cpp
- test_moveTo_sets_target_position
- test_moveTo_blocked_by_emergency_stop
- test_moveTo_clamps_speed_to_min_max
- test_jogStop_clears_movement
- test_jogStop_does_not_set_emergency_flag
```

#### Emergency Stop Logic (5 tests)
```cpp
- test_emergencyStop_sets_flag
- test_emergencyStop_prevents_movement
- test_clearEmergencyStop_resets_flag
- test_clearEmergencyStop_allows_movement_after
- test_isEmergencyStopActive_reflects_state
```

#### Parameter Clamping (5 tests)
```cpp
- test_setMaxSpeed_clamps_to_MIN_SPEED        // 100 steps/sec
- test_setMaxSpeed_clamps_to_MAX_SPEED        // 100000 steps/sec
- test_setAcceleration_clamps_to_MIN_ACCEL    // 100 steps/sec²
- test_setAcceleration_clamps_to_MAX_ACCEL    // 500000 steps/sec²
- test_moveTo_speed_parameter_clamped
```

**Implementation Notes:**
- Mock AccelStepper to test logic without hardware
- Verify flag state changes
- Test clamping against constants in MotorController.h

---

### 1.3 LimitSwitch Module Tests
**File:** `test/test_native/test_limit_switch/test_limit_switch.cpp`

**Test Cases (10 tests):**

#### Switch State Management (4 tests)
```cpp
- test_handleSwitchPressed_sets_trigger_flag_switch1
- test_handleSwitchPressed_sets_trigger_flag_switch2
- test_clearTriggers_resets_both_flags
- test_isAnyTriggered_returns_true_when_any_set
```

#### DRY Unified Handler (3 tests)
```cpp
- test_handleSwitchPressed_saves_correct_limit_pos1
- test_handleSwitchPressed_saves_correct_limit_pos2
- test_handleSwitchPressed_triggers_emergency_stop
```

#### Callback Mechanism (3 tests)
```cpp
- test_setLimitCallback_stores_callback
- test_handleSwitchPressed_calls_callback_with_correct_params
- test_handleSwitchPressed_no_crash_when_callback_null
```

**Implementation Notes:**
- Mock OneButton to simulate button presses
- Mock motorController.emergencyStop() to verify it's called
- Test callback invocation without actual hardware

---

### 1.4 ButtonController Module Tests
**File:** `test/test_native/test_button_controller/test_button_controller.cpp`

**Test Cases (12 tests):**

#### Jog Button Behavior (6 tests)
```cpp
- test_onButton1Press_starts_jog_backward
- test_onButton1Release_calls_jogStop
- test_onButton3Press_starts_jog_forward
- test_onButton3Release_calls_jogStop
- test_jog_blocked_when_limit_triggered
- test_jog_blocked_when_emergency_stop_active
```

#### Emergency Stop Button (3 tests)
```cpp
- test_onButton2Click_calls_emergencyStop
- test_emergency_button_works_anytime
- test_emergency_button_idempotent    // Can be pressed multiple times
```

#### Speed Calculation (3 tests)
```cpp
- test_jog_speed_is_30_percent_of_max
- test_jog_speed_updates_when_config_changes
- test_jog_targets_correct_limit_positions
```

**Implementation Notes:**
- Mock motorController.moveTo() to capture parameters
- Verify 30% speed calculation
- Test guard conditions (limit switches, emergency stop)

---

## Phase 2: C++ Integration Tests (MEDIUM PRIORITY)

### 2.1 WebSocket Command Handler Tests
**File:** `test/test_native/test_webserver/test_command_handlers.cpp`

**Test Cases (20 tests):**

#### Dispatch Table (3 tests)
```cpp
- test_handleMoveCommand_valid_parameters
- test_handleMoveCommand_missing_position_parameter
- test_handleMoveCommand_missing_speed_parameter
```

#### Command Validation (8 tests)
```cpp
- test_handleJogStartCommand_valid_forward
- test_handleJogStartCommand_valid_backward
- test_handleJogStartCommand_invalid_direction
- test_handleJogStartCommand_blocked_by_limit
- test_handleJogStartCommand_blocked_by_emergency
- test_handleJogStopCommand_calls_jogStop
- test_handleEmergencyStopCommand_triggers_emergency
- test_handleResetCommand_clears_triggers
```

#### Config Commands (6 tests)
```cpp
- test_handleSetConfigCommand_maxSpeed
- test_handleSetConfigCommand_acceleration
- test_handleSetConfigCommand_useStealthChop
- test_handleSetConfigCommand_multiple_params
- test_handleSetConfigCommand_invalid_params
- test_handleGetConfigCommand_returns_current_config
```

#### Error Handling (3 tests)
```cpp
- test_unknown_command_returns_error
- test_malformed_json_handled_gracefully
- test_missing_command_field_logs_warning
```

**Implementation Notes:**
- Mock AsyncWebSocket to avoid ESP32 dependencies
- Mock JsonDocument for command parsing
- Test each handler method independently
- Verify error messages sent to clients

---

## Phase 3: WebApp Tests (HIGH PRIORITY)

### 3.1 Setup Testing Infrastructure

**Install Dependencies:**
```bash
cd webapp
npm install --save-dev vitest @testing-library/react @testing-library/jest-dom @testing-library/user-event jsdom
```

**Create Test Config:**
```typescript
// webapp/vitest.config.ts
import { defineConfig } from 'vitest/config'
import react from '@vitejs/plugin-react'
import path from 'path'

export default defineConfig({
  plugins: [react()],
  test: {
    globals: true,
    environment: 'jsdom',
    setupFiles: './src/test/setup.ts',
  },
  resolve: {
    alias: {
      '@': path.resolve(__dirname, './src'),
    },
  },
})
```

**Setup File:**
```typescript
// webapp/src/test/setup.ts
import '@testing-library/jest-dom'
```

---

### 3.2 useMotorController Hook Tests
**File:** `webapp/src/hooks/useMotorController.test.tsx`

**Test Cases (25 tests):**

#### Connection Management (5 tests)
```typescript
- test_initial_state_is_disconnected
- test_connection_state_updates_on_open
- test_reconnection_attempts_limited_to_3
- test_manual_reconnect_resets_attempt_count
- test_cleanup_on_unmount_closes_websocket
```

#### WebSocket Message Handling (8 tests)
```typescript
- test_status_message_updates_motorStatus_state
- test_position_message_updates_position_only
- test_config_message_updates_motorConfig_state
- test_configUpdated_message_triggers_config_refresh
- test_error_message_sets_connection_error
- test_unknown_message_type_logged
- test_malformed_json_handled_gracefully
- test_multiple_rapid_messages_processed_correctly
```

#### Command Sending (7 tests)
```typescript
- test_moveTo_sends_correct_command
- test_emergencyStop_sends_emergencyStop_command
- test_jogStart_sends_correct_direction_and_speed
- test_jogStop_sends_jogStop_command
- test_clearEmergencyStop_sends_reset_command
- test_updateConfig_sends_setConfig_with_params
- test_commands_queued_when_disconnected
```

#### State Synchronization (5 tests)
```typescript
- test_initial_status_request_sent_on_connect
- test_initial_config_request_sent_on_connect
- test_motorConfig_defaults_used_until_received
- test_motorStatus_defaults_used_until_received
- test_jog_speed_updates_when_max_speed_changes
```

**Implementation Notes:**
- Mock WebSocket with `vi.fn()`
- Use `@testing-library/react` renderHook
- Test state updates and side effects
- Verify command payloads sent to WebSocket

---

### 3.3 JogControls Component Tests
**File:** `webapp/src/components/MotorControl/JogControls.test.tsx`

**Test Cases (12 tests):**

#### Button States (4 tests)
```typescript
- test_jog_buttons_disabled_when_disconnected
- test_jog_buttons_disabled_when_emergency_stop_active
- test_emergency_stop_button_always_enabled_when_connected
- test_limit_buttons_disabled_when_moving
```

#### Jog Interaction (5 tests)
```typescript
- test_mousedown_starts_jog_forward
- test_mousedown_starts_jog_backward
- test_mouseup_stops_jog
- test_mouseleave_stops_jog_only_if_active
- test_touchstart_and_touchend_work_like_mouse
```

#### Emergency Stop Toggle (3 tests)
```typescript
- test_shows_emergency_stop_button_when_not_active
- test_shows_reset_button_when_emergency_active
- test_emergency_stop_button_click_calls_callback
```

**Implementation Notes:**
- Use `@testing-library/user-event` for interactions
- Test button enable/disable logic
- Verify callbacks are called with correct parameters
- Test touch and mouse events separately

---

### 3.4 PositionControl Component Tests
**File:** `webapp/src/components/MotorControl/PositionControl.test.tsx`

**Test Cases (10 tests):**

#### Input Validation (4 tests)
```typescript
- test_position_input_clamped_to_min_limit
- test_position_input_clamped_to_max_limit
- test_speed_slider_clamped_to_max_speed
- test_speed_slider_minimum_is_100
```

#### Move Command (3 tests)
```typescript
- test_move_button_disabled_when_disconnected
- test_move_button_disabled_when_emergency_stop
- test_move_button_calls_moveTo_with_correct_params
```

#### Speed Slider (3 tests)
```typescript
- test_speed_slider_range_updates_with_config
- test_speed_slider_defaults_to_30_percent
- test_speed_value_displayed_correctly
```

**Implementation Notes:**
- Test input validation and clamping
- Verify slider ranges are dynamic based on config
- Test disabled states match requirements

---

### 3.5 MotorConfigDialog Component Tests
**File:** `webapp/src/components/MotorConfig/MotorConfigDialog.test.tsx`

**Test Cases (12 tests):**

#### Dialog Behavior (3 tests)
```typescript
- test_dialog_opens_when_open_prop_true
- test_dialog_closes_on_cancel
- test_dialog_closes_on_apply_success
```

#### Input Validation (6 tests)
```typescript
- test_maxSpeed_clamped_to_100_min
- test_maxSpeed_clamped_to_100000_max
- test_acceleration_clamped_to_100_min
- test_acceleration_clamped_to_500000_max
- test_invalid_input_shows_validation_error
- test_limit_positions_readonly_display
```

#### Apply Changes (3 tests)
```typescript
- test_apply_calls_updateConfig_with_changes
- test_apply_button_disabled_when_disconnected
- test_apply_button_disabled_when_no_changes
```

**Implementation Notes:**
- Test validation matches backend constants
- Verify only changed values sent to updateConfig
- Test read-only display of limit positions

---

## Phase 4: Advanced Tests (LOWER PRIORITY)

### 4.1 WebSocket Protocol Compliance Tests
**File:** `webapp/src/test/integration/websocket-protocol.test.ts`

**Test Cases (8 tests):**
```typescript
- test_all_commands_match_backend_spec
- test_json_structure_validation
- test_type_field_required_in_all_messages
- test_command_field_required_in_commands
- test_position_updates_high_frequency
- test_status_updates_medium_frequency
- test_config_updates_low_frequency
- test_reconnection_resends_initial_requests
```

---

### 4.2 Edge Case Tests
**File:** Various locations

**Test Cases (15 tests):**

#### C++ Edge Cases (8 tests)
```cpp
- test_moveTo_with_zero_speed
- test_moveTo_with_negative_position
- test_concurrent_jog_and_emergency_stop
- test_rapid_emergency_stop_clear_toggle
- test_limit_switch_during_jog
- test_encoder_wraparound_at_boundaries
- test_very_high_acceleration_values
- test_config_save_failure_handling
```

#### WebApp Edge Cases (7 tests)
```typescript
- test_websocket_disconnect_during_movement
- test_rapid_command_sending
- test_config_change_during_movement
- test_browser_tab_inactive_reconnection
- test_network_latency_simulation
- test_message_ordering_guaranteed
- test_state_recovery_after_reconnect
```

---

## Implementation Strategy

### Step 1: Infrastructure Setup (Week 1)
1. ✅ Create test directory structure
2. ✅ Configure vitest for webapp
3. ✅ Create mock utilities for hardware dependencies
4. ✅ Set up CI/CD to run tests automatically

### Step 2: Core Module Tests (Week 2-3)
1. Configuration module (highest ROI - catches validation bugs)
2. MotorController extended tests
3. LimitSwitch tests
4. ButtonController tests

### Step 3: WebApp Critical Path (Week 4)
1. useMotorController hook (most critical)
2. JogControls component
3. PositionControl component
4. MotorConfigDialog component

### Step 4: Integration & Edge Cases (Week 5)
1. WebSocket command handler tests
2. Protocol compliance tests
3. Edge case coverage
4. Performance/load tests

---

## Success Metrics

### Coverage Goals
- **C++ Modules:** >80% line coverage
- **WebApp:** >70% line coverage
- **Critical Paths:** 100% coverage (emergency stop, limit switches, parameter validation)

### Quality Gates
- All tests must pass before merging
- New features require tests
- Bug fixes require regression test
- CI/CD runs tests on every commit

---

## Testing Tools & Libraries

### C++ Testing
- ✅ **Unity** - Already in use, simple and effective
- **CMock** - For mocking hardware dependencies (optional)
- **CppUTest** - Alternative if more features needed

### WebApp Testing
- **Vitest** - Fast, Vite-native test runner
- **@testing-library/react** - Component testing best practices
- **@testing-library/user-event** - Realistic user interactions
- **jsdom** - Browser environment simulation
- **Mock Service Worker (MSW)** - WebSocket mocking (optional)

---

## Risk Areas Requiring Most Coverage

### 🔴 Critical (Must Test)
1. **Emergency stop logic** - Safety critical
2. **Parameter validation** - Prevents hardware damage
3. **Limit switch handling** - Prevents collisions
4. **WebSocket command dispatch** - Core functionality
5. **Speed/acceleration clamping** - Motor protection

### 🟡 High Priority (Should Test)
1. Configuration persistence
2. State synchronization between frontend/backend
3. Movement command execution
4. Error handling and recovery
5. UI disable states during emergency

### 🟢 Medium Priority (Nice to Have)
1. Edge cases and boundary conditions
2. Performance under load
3. Network resilience
4. Browser compatibility
5. Accessibility features

---

## Quick Start Guide

### Running C++ Tests
```bash
# Run all native tests
pio test -e native

# Run specific test
pio test -e native -f test_configuration

# Verbose output
pio test -e native -v
```

### Running WebApp Tests
```bash
cd webapp

# Run all tests
npm test

# Run in watch mode
npm test -- --watch

# Run with coverage
npm test -- --coverage

# Run specific test file
npm test -- useMotorController
```

---

## Continuous Integration

### GitHub Actions Workflow
```yaml
# .github/workflows/tests.yml
name: Tests

on: [push, pull_request]

jobs:
  cpp-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-python@v4
      - run: pip install platformio
      - run: pio test -e native

  webapp-tests:
    runs-on: ubuntu-latest
    steps:
      - uses: actions/checkout@v3
      - uses: actions/setup-node@v3
      - run: cd webapp && npm ci
      - run: cd webapp && npm test
      - run: cd webapp && npm run build
```

---

## Maintenance Plan

### Regular Tasks
- **Weekly:** Review test coverage reports
- **Monthly:** Update tests for new features
- **Quarterly:** Refactor flaky tests
- **Yearly:** Audit test suite effectiveness

### Test Hygiene
- Keep tests fast (<1s per test)
- One assertion per test when possible
- Clear, descriptive test names
- DRY: Extract common setup to helpers
- Document complex test scenarios

---

## Expected Outcomes

### Bug Prevention
- Catch parameter validation bugs before hardware testing
- Prevent emergency stop logic regressions
- Validate WebSocket protocol changes
- Ensure UI states match backend state

### Development Velocity
- Faster refactoring with confidence
- Quick feedback on breaking changes
- Easier onboarding for new contributors
- Self-documenting codebase through tests

### Quality Improvements
- Fewer production bugs
- Better error handling
- More robust state management
- Improved code maintainability

---

## Total Test Count Projection

| Module | Current | Planned | Total |
|--------|---------|---------|-------|
| MotorController | 10 | 15 | 25 |
| Configuration | 0 | 15 | 15 |
| LimitSwitch | 0 | 10 | 10 |
| ButtonController | 0 | 12 | 12 |
| WebServer | 0 | 20 | 20 |
| **C++ Subtotal** | **10** | **72** | **82** |
| useMotorController | 0 | 25 | 25 |
| JogControls | 0 | 12 | 12 |
| PositionControl | 0 | 10 | 10 |
| MotorConfigDialog | 0 | 12 | 12 |
| Integration/Edge | 0 | 23 | 23 |
| **WebApp Subtotal** | **0** | **82** | **82** |
| **Grand Total** | **10** | **154** | **164** |

---

## Next Steps

1. Review this plan and adjust priorities
2. Set up vitest for webapp testing
3. Create mock utilities for C++ hardware dependencies
4. Implement Phase 1 tests (Configuration module)
5. Set up CI/CD to run tests automatically
6. Establish coverage reporting

**Estimated Effort:** 5-6 weeks for full implementation
**Recommended Approach:** Incremental - ship Phase 1, then Phase 2, etc.
