# Stop Commands Analysis

## Current State (INCONSISTENT ⚠️)

### C++ MotorController has 3 methods:

1. **`stop()`** (line 122-128)
   - Sets `emergencyStopActive = true`
   - Calls `stepper->stop()`
   - This IS an emergency stop!

2. **`stopGently()`** (line 130-137)
   - Does NOT set emergency flag
   - Stops immediately via `setCurrentPosition()`
   - Used for jogging

3. **`emergencyStop()`** (line 139-143)
   - Just calls `stop()` then logs
   - Redundant wrapper around `stop()`

### WebServer Commands (BROKEN):

1. **First "stop" handler** (line 280-287)
   - Calls `motorController.stop()`
   - This triggers emergency flag ✅

2. **"jogStop" handler** (line 333-337)
   - Calls `motorController.stopGently()`
   - Gentle stop, no flag ✅

3. **Second "stop" handler** (line 339-343) 🐛 **DUPLICATE - NEVER EXECUTES**
   - Calls `motorController.emergencyStop()`
   - Dead code due to first match

### WebApp (WRONG):

- `emergencyStop()` method sends `"emergency-stop"` command
- Backend has NO handler for `"emergency-stop"` ❌

## Desired Behavior (from user requirements):

1. **Gentle Stop** (jog stop):
   - Stops motor safely
   - Does NOT set emergency flag
   - No manual reset needed

2. **Emergency Stop**:
   - Stops motor safely
   - SETS emergency flag
   - Requires manual reset

## Proposed Solution

### C++ Changes:

**Keep:**
- `stopGently()` - for jog stop (no flag) ✅
- `emergencyStop()` - for emergency (sets flag) ✅

**Remove:**
- `stop()` - confusing middle ground ❌

**Refactor:**
```cpp
void MotorController::emergencyStop()
{
    emergencyStopActive = true;
    stepper->setSpeed(0);
    stepper->stop();
    LOG_WARN("EMERGENCY STOP ACTIVATED");
}

void MotorController::stopGently()
{
    // Already correct - no changes needed
    stepper->setCurrentPosition(stepper->currentPosition());
    stepper->setSpeed(0);
    LOG_INFO("Motor stopped gently");
}
```

### WebSocket Protocol:

**Commands:**
- `"jogStop"` → calls `motorController.stopGently()`
- `"emergencyStop"` → calls `motorController.emergencyStop()`
- Remove: duplicate "stop" handlers

### WebApp Changes:

```typescript
const emergencyStop = useCallback(() => {
  return sendCommand({ command: 'emergencyStop' })  // Fixed!
}, [sendCommand])

const jogStop = useCallback(() => {
  return sendCommand({ command: 'jogStop' })  // Already correct
}, [sendCommand])
```

## Implementation Plan

1. ✅ Remove `MotorController::stop()` method
2. ✅ Update `emergencyStop()` to do the work directly
3. ✅ Remove duplicate "stop" handler in WebServer (line 339-343)
4. ✅ Rename first "stop" handler to "emergencyStop"
5. ✅ Update WebApp to send correct command
6. ✅ Test both stop variants
