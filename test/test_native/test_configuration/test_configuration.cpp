#include <unity.h>

// Include mocked dependencies first (from mock/ directory in include path)
#include <Arduino.h>
#include <Preferences.h>
#include <util.h>

// Now include Configuration with mocked dependencies
#include "../../../src/modules/Configuration/Configuration.h"
#include "../../../src/modules/Configuration/Configuration.cpp"

// Constants from MotorController (for validation tests)
static constexpr long MIN_SPEED = 100;
static constexpr long MAX_SPEED = 100000;
static constexpr long MIN_ACCELERATION = 100;
static constexpr long MAX_ACCELERATION = 500000;

// Test instance
Configuration testConfig;

void setUp(void) {
    // Clear mock preferences storage before each test
    globalLongValues.clear();
    globalBoolValues.clear();

    // Reset to defaults before each test
    testConfig = Configuration();
}

void tearDown(void) {
    // Cleanup after each test
}

// ============================================================================
// Parameter Validation Tests (5 tests)
// ============================================================================

void test_setMaxSpeed_valid_range(void) {
    testConfig.begin();

    // Test a valid speed within range
    testConfig.setMaxSpeed(5000);
    TEST_ASSERT_EQUAL_INT32(5000, testConfig.getMaxSpeed());
}

void test_setMaxSpeed_stores_below_min(void) {
    testConfig.begin();

    // Configuration doesn't enforce MIN/MAX - that's MotorController's job
    // This test verifies Configuration stores the value as-is
    testConfig.setMaxSpeed(50);
    TEST_ASSERT_EQUAL_INT32(50, testConfig.getMaxSpeed());
}

void test_setMaxSpeed_stores_above_max(void) {
    testConfig.begin();

    // Configuration doesn't enforce MIN/MAX - that's MotorController's job
    // This test verifies Configuration stores the value as-is
    testConfig.setMaxSpeed(200000);
    TEST_ASSERT_EQUAL_INT32(200000, testConfig.getMaxSpeed());
}

void test_setAcceleration_valid_range(void) {
    testConfig.begin();

    // Test a valid acceleration within range
    testConfig.setAcceleration(10000);
    TEST_ASSERT_EQUAL_INT32(10000, testConfig.getAcceleration());
}

void test_setAcceleration_stores_extremes(void) {
    testConfig.begin();

    // Test storing minimum
    testConfig.setAcceleration(MIN_ACCELERATION);
    TEST_ASSERT_EQUAL_INT32(MIN_ACCELERATION, testConfig.getAcceleration());

    // Test storing maximum
    testConfig.setAcceleration(MAX_ACCELERATION);
    TEST_ASSERT_EQUAL_INT32(MAX_ACCELERATION, testConfig.getAcceleration());
}

// ============================================================================
// Limit Position Management Tests (5 tests)
// ============================================================================

void test_setLimitPos1_stores_correctly(void) {
    testConfig.begin();

    testConfig.setLimitPos1(1000);
    TEST_ASSERT_EQUAL_INT32(1000, testConfig.getLimitPos1());
}

void test_setLimitPos2_stores_correctly(void) {
    testConfig.begin();

    testConfig.setLimitPos2(5000);
    TEST_ASSERT_EQUAL_INT32(5000, testConfig.getLimitPos2());
}

void test_getMinLimit_returns_correct_value(void) {
    testConfig.begin();

    // Case 1: pos1 < pos2
    testConfig.setLimitPos1(100);
    testConfig.setLimitPos2(500);
    TEST_ASSERT_EQUAL_INT32(100, testConfig.getMinLimit());

    // Case 2: pos2 < pos1
    testConfig.setLimitPos1(500);
    testConfig.setLimitPos2(100);
    TEST_ASSERT_EQUAL_INT32(100, testConfig.getMinLimit());
}

void test_getMaxLimit_returns_correct_value(void) {
    testConfig.begin();

    // Case 1: pos1 > pos2
    testConfig.setLimitPos1(500);
    testConfig.setLimitPos2(100);
    TEST_ASSERT_EQUAL_INT32(500, testConfig.getMaxLimit());

    // Case 2: pos2 > pos1
    testConfig.setLimitPos1(100);
    testConfig.setLimitPos2(500);
    TEST_ASSERT_EQUAL_INT32(500, testConfig.getMaxLimit());
}

void test_saveLimitPositions_persists_both(void) {
    testConfig.begin();

    // Save both positions at once
    testConfig.saveLimitPositions(200, 800);

    // Verify both were saved
    TEST_ASSERT_EQUAL_INT32(200, testConfig.getLimitPos1());
    TEST_ASSERT_EQUAL_INT32(800, testConfig.getLimitPos2());
}

// ============================================================================
// StealthChop Mode Tests (3 tests)
// ============================================================================

void test_setUseStealthChop_true(void) {
    testConfig.begin();

    testConfig.setUseStealthChop(true);
    TEST_ASSERT_TRUE(testConfig.getUseStealthChop());
}

void test_setUseStealthChop_false(void) {
    testConfig.begin();

    testConfig.setUseStealthChop(false);
    TEST_ASSERT_FALSE(testConfig.getUseStealthChop());
}

void test_getUseStealthChop_returns_current_state(void) {
    testConfig.begin();

    // Test initial state (should be true by default)
    TEST_ASSERT_TRUE(testConfig.getUseStealthChop());

    // Toggle and verify
    testConfig.setUseStealthChop(false);
    TEST_ASSERT_FALSE(testConfig.getUseStealthChop());

    testConfig.setUseStealthChop(true);
    TEST_ASSERT_TRUE(testConfig.getUseStealthChop());
}

// ============================================================================
// Persistence Logic Tests (2 tests)
// ============================================================================

void test_saveConfiguration_persists_values(void) {
    testConfig.begin();

    // Set values
    testConfig.setMaxSpeed(12000);
    testConfig.setAcceleration(50000);
    testConfig.setLimitPos1(300);
    testConfig.setLimitPos2(900);
    testConfig.setUseStealthChop(false);

    // Save explicitly
    testConfig.saveConfiguration();

    // Create new instance and load - should get same values
    Configuration newConfig;
    newConfig.begin();

    TEST_ASSERT_EQUAL_INT32(12000, newConfig.getMaxSpeed());
    TEST_ASSERT_EQUAL_INT32(50000, newConfig.getAcceleration());
    TEST_ASSERT_EQUAL_INT32(300, newConfig.getLimitPos1());
    TEST_ASSERT_EQUAL_INT32(900, newConfig.getLimitPos2());
    TEST_ASSERT_FALSE(newConfig.getUseStealthChop());
}

void test_loadConfiguration_restores_defaults(void) {
    // Clear mock storage to simulate fresh NVRAM
    globalLongValues.clear();
    globalBoolValues.clear();

    Configuration freshConfig;
    freshConfig.begin();

    // Should have default values
    TEST_ASSERT_EQUAL_INT32(1000 * 80, freshConfig.getAcceleration()); // 80000
    TEST_ASSERT_EQUAL_INT32(180 * 80, freshConfig.getMaxSpeed());      // 14400
    TEST_ASSERT_EQUAL_INT32(0, freshConfig.getLimitPos1());
    TEST_ASSERT_EQUAL_INT32(2500, freshConfig.getLimitPos2());
    TEST_ASSERT_TRUE(freshConfig.getUseStealthChop());
}

// ============================================================================
// Test Runner
// ============================================================================

void setup() {
    UNITY_BEGIN();

    // Parameter Validation (5 tests)
    RUN_TEST(test_setMaxSpeed_valid_range);
    RUN_TEST(test_setMaxSpeed_stores_below_min);
    RUN_TEST(test_setMaxSpeed_stores_above_max);
    RUN_TEST(test_setAcceleration_valid_range);
    RUN_TEST(test_setAcceleration_stores_extremes);

    // Limit Position Management (5 tests)
    RUN_TEST(test_setLimitPos1_stores_correctly);
    RUN_TEST(test_setLimitPos2_stores_correctly);
    RUN_TEST(test_getMinLimit_returns_correct_value);
    RUN_TEST(test_getMaxLimit_returns_correct_value);
    RUN_TEST(test_saveLimitPositions_persists_both);

    // StealthChop Mode (3 tests)
    RUN_TEST(test_setUseStealthChop_true);
    RUN_TEST(test_setUseStealthChop_false);
    RUN_TEST(test_getUseStealthChop_returns_current_state);

    // Persistence Logic (2 tests)
    RUN_TEST(test_saveConfiguration_persists_values);
    RUN_TEST(test_loadConfiguration_restores_defaults);

    UNITY_END();
}

void loop() {
    // Empty loop for native testing
}

// For native platform, provide main function
#ifdef UNIT_TEST
int main(int argc, char **argv) {
    setup();
    return 0;
}
#endif
