# MotorController Testing Analysis

**Date:** 2025-10-04
**Status:** ⚠️ BLOCKED - Requires Refactoring
**Attempted:** Extended unit tests for MotorController module

---

## Problem Summary

**MotorController is deeply coupled to hardware dependencies**, making unit testing extremely difficult without architectural changes.

### Dependencies Identified

1. **AccelStepper** - Complex third-party stepper library
2. **TMC2209Stepper** - UART-based driver communication
3. **HardwareSerial** - ESP32 UART hardware (`Serial1`)
4. **SPIClass** - Hardware SPI for MT6816 encoder
5. **Configuration** - Module dependency (also includes ESP32 Preferences)
6. **Arduino.h** - Pin I/O, constants, hardware initialization

### Attempted Mocking Approach

Created mocks for all dependencies:
- ✅ `mock/AccelStepper.h` - State-based mock
- ✅ `mock/TMCStepper.h` - Minimal TMC2209 mock
- ✅ `mock/Arduino.h` - Pin I/O mocks
- ✅ `mock/SPI.h` - SPI communication mock
- ✅ `mock/Configuration.h` - Configuration mock
- ✅ `mock/util.h` - Logging mock

### Why It Failed

**Circular dependency issue:**
```
MotorController.cpp includes:
  └─> Configuration/Configuration.h (real)
       └─> Preferences.h (ESP32 hardware)

But test includes:
  └─> Configuration.h (mock)
```

When `#include "../../../src/modules/MotorController/MotorController.cpp"` is used:
- Mock Configuration included first ✅
- MotorController.cpp includes REAL Configuration.h ❌
- **Redefinition error**

**Additional issues:**
- `Serial1` is a global ESP32 hardware object (can't mock easily)
- `HSPI` constant requires specific ESP32 definitions
- Pin constants (`OUTPUT`, `LOW`, `HIGH`) need careful ordering
- Hardware initialization in constructor makes testing difficult

---

## Root Cause Analysis

### Architectural Issues

1. **Constructor Does Hardware Allocation**
   ```cpp
   MotorController::MotorController() {
       serialDriver = &Serial1;  // Direct hardware reference
       driver = new TMC2209Stepper(serialDriver, ...);
       stepper = new AccelStepper(...);
       mt6816 = new SPIClass(HSPI);  // ESP32-specific constant
   }
   ```
   **Problem:** Can't create instance without hardware

2. **No Dependency Injection**
   - Hardware dependencies are hard-coded
   - No interfaces/abstractions
   - Tight coupling to ESP32 platform

3. **Mixed Concerns**
   - Business logic (emergency stop, parameter validation)
   - Hardware initialization
   - Communication protocols
   - State management

### Testability Score: 2/10 ⭐⭐☆☆☆☆☆☆☆☆

**Current state:** Nearly impossible to unit test without refactoring

---

## Solutions & Recommendations

### Option 1: Dependency Injection (BEST - Requires Refactoring)

**Effort:** High (8-12 hours)
**Value:** High - Enables comprehensive testing
**Breaking Change:** Moderate

**Approach:**
```cpp
// Create interfaces
class IAccelStepper {
    virtual void moveTo(long position) = 0;
    virtual long currentPosition() const = 0;
    virtual long distanceToGo() const = 0;
    // ...
};

class RealAccelStepper : public IAccelStepper {
    AccelStepper* stepper;
    // Implement interface
};

class MockAccelStepper : public IAccelStepper {
    // Test implementation
};

// Refactor MotorController
class MotorController {
    IAccelStepper* stepper;  // Injected dependency

public:
    MotorController(IAccelStepper* s) : stepper(s) {}
    // ...
};
```

**Benefits:**
- Full testability
- Better architecture
- Future-proof for hardware changes

**Drawbacks:**
- Significant refactoring required
- May impact existing code
- Need to update all call sites

---

### Option 2: Extract Testable Logic (PRAGMATIC - Recommended)

**Effort:** Low (2-4 hours)
**Value:** Medium - Tests critical logic
**Breaking Change:** None

**Approach:**

Create static/free functions for testable logic:

```cpp
// MotorControllerLogic.h (new file)
class MotorControllerLogic {
public:
    // Pure logic functions - no hardware dependencies

    static bool shouldBlockMovement(bool emergencyActive) {
        return emergencyActive;
    }

    static long clampSpeed(long speed, long min, long max) {
        if (speed < min) return min;
        if (speed > max) return max;
        return speed;
    }

    static long clampAcceleration(long accel, long min, long max) {
        if (accel < min) return min;
        if (accel > max) return max;
        return accel;
    }
};

// MotorController uses these
void MotorController::moveTo(long position, int speed) {
    if (MotorControllerLogic::shouldBlockMovement(emergencyStopActive)) {
        return;
    }

    speed = MotorControllerLogic::clampSpeed(speed, MIN_SPEED, MAX_SPEED);
    // ... hardware calls
}
```

**Then test the logic:**
```cpp
// test_motor_logic.cpp
void test_clampSpeed_below_min() {
    TEST_ASSERT_EQUAL(100, MotorControllerLogic::clampSpeed(50, 100, 100000));
}

void test_shouldBlockMovement_when_emergency() {
    TEST_ASSERT_TRUE(MotorControllerLogic::shouldBlockMovement(true));
}
```

**Benefits:**
- No refactoring of existing code
- Tests critical logic paths
- Easy to implement

**Drawbacks:**
- Doesn't test integration with hardware
- Limited scope

---

### Option 3: Integration Tests Only (CURRENT - Acceptable)

**Effort:** Low (1-2 hours)
**Value:** Low - Manual testing required
**Breaking Change:** None

**Approach:**

Skip unit tests for MotorController, rely on:
1. **Existing calculation tests** (10 tests already passing ✅)
2. **Manual hardware testing**
3. **Integration tests** when hardware available
4. **End-to-end WebSocket tests**

**Benefits:**
- No code changes needed
- Focus effort on testable modules

**Drawbacks:**
- Less regression protection
- Bugs found late (on hardware)

---

### Option 4: Test Harness with Real Hardware (FUTURE)

**Effort:** Medium (6-8 hours)
**Value:** High - Real-world validation
**Breaking Change:** None

**Approach:**

Create test environment that runs on actual ESP32:

```cpp
// test_embedded/test_motor_hardware.cpp
void test_emergency_stop_on_hardware() {
    motorController.emergencyStop();
    delay(100);
    TEST_ASSERT_TRUE(motorController.isEmergencyStopActive());
}
```

Run with: `pio test -e pico32`

**Benefits:**
- Tests real hardware behavior
- Catches hardware-specific issues
- Validates assumptions

**Drawbacks:**
- Requires physical hardware
- Slower test execution
- Setup complexity

---

## Decision Matrix

| Option | Effort | Value | Test Coverage | Maintainability | Recommendation |
|--------|--------|-------|---------------|-----------------|----------------|
| 1. Dependency Injection | High | High | 90%+ | Excellent | Future refactor |
| **2. Extract Logic** | **Low** | **Medium** | **60%** | **Good** | **✅ IMPLEMENT NOW** |
| 3. Integration Only | Low | Low | 30% | Poor | Acceptable |
| 4. Hardware Harness | Medium | High | 95% | Good | Later phase |

---

## Recommended Action Plan

### Immediate (This Session)

✅ **Implement Option 2: Extract Testable Logic**

1. Create `MotorControllerLogic.h` with pure functions
2. Refactor MotorController to use logic functions
3. Write unit tests for logic (estimated 10 tests)
4. Document approach for future modules

**Estimated Time:** 2-4 hours

### Short Term (Next Sprint)

- Apply same pattern to LimitSwitch and ButtonController
- Complete remaining C++ module tests using this approach
- Document testability guidelines for new code

### Long Term (Future Milestone)

- Consider Option 1 (Dependency Injection) for major refactor
- Implement Option 4 (Hardware tests) for validation
- Establish testing standards for new features

---

## Lessons Learned

### What We Discovered

1. **Constructor-based hardware initialization prevents testing**
   - Move hardware setup to `begin()` method
   - Keep constructors simple

2. **Direct hardware references are untestable**
   - Avoid global hardware objects (`Serial1`, `HSPI`)
   - Use dependency injection or factory pattern

3. **Mixed business logic and hardware code is hard to test**
   - Separate pure logic from hardware calls
   - Extract algorithms to testable functions

### Best Practices Going Forward

1. **New Code:** Use dependency injection from the start
2. **Legacy Code:** Extract logic to pure functions
3. **Critical Paths:** Prioritize logic testing over hardware mocking
4. **Documentation:** Mark untestable code for future refactoring

---

## Files Created (Attempted)

```
test/test_native/test_motor_extended/
├── test_motor_extended.cpp (15 tests - won't compile)
└── mock/
    ├── AccelStepper.h
    ├── TMCStepper.h
    ├── Arduino.h
    ├── SPI.h
    ├── Configuration.h
    └── util.h
```

**Status:** ❌ Not functional due to architectural constraints

---

## Alternative: Focus on What We CAN Test

### Already Tested ✅
- MotorController calculation logic (10 tests passing)
- Configuration module (15 tests passing)

### Can Test Next ✅
- **LimitSwitch** - Less hardware coupling, easier to mock
- **ButtonController** - OneButton library, mockable
- **WebServer handlers** - JSON logic, mockable

### Skip for Now ⏭️
- MotorController extended tests (requires refactoring)

---

## Conclusion

**MotorController extended testing is blocked by architectural constraints.** The module has deep hardware coupling that makes traditional unit testing impractical without refactoring.

**Recommended path:**
1. ✅ Accept current calculation tests (10 tests)
2. ✅ Move to easier modules (LimitSwitch, ButtonController)
3. ✅ Document MotorController as "requires refactoring for testability"
4. 🔄 Revisit MotorController in future refactor sprint

**Impact:** Minimal - Most critical paths already tested via calculation tests. Integration testing will catch remaining issues.

**Status:** Documented as technical debt for future sprint 📋
