#include <unity.h>

// Test helper functions that replicate the calculation logic without hardware dependencies

// Test calculateSpeed calculation logic
double calculateSpeedLogic(double currentLocation, double lastLocation, int8_t& direction) {
    double speedT = 0;
    if (currentLocation == lastLocation) {
        speedT = direction = 0;
    } else {
        double tempT = abs(currentLocation - lastLocation);
        if (tempT < 8192) {
            speedT = (tempT * 360) / 16384;
            direction = currentLocation > lastLocation ? 1 : -1;
        } else {
            tempT = 16384 - tempT;
            speedT = (tempT * 360) / 16384;
            direction = currentLocation > lastLocation ? -1 : 1;
        }
    }
    return speedT;
}

// Test updateTMCMode logic
bool shouldUseStealthChopLogic(float currentSpeed, float maxSpeed, float threshold) {
    float currentSpeedPercent = abs(currentSpeed) / maxSpeed;
    return currentSpeedPercent < threshold;
}

// Test cases for calculateSpeed
void test_calculateSpeed_no_movement(void) {
    int8_t direction = 0;
    double result = calculateSpeedLogic(1000.0, 1000.0, direction);

    TEST_ASSERT_EQUAL_DOUBLE(0.0, result);
    TEST_ASSERT_EQUAL_INT8(0, direction);
}

void test_calculateSpeed_forward_movement(void) {
    int8_t direction = 0;
    double result = calculateSpeedLogic(1100.0, 1000.0, direction);

    // Expected: (100 * 360) / 16384 = ~2.197 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.197, result);
    TEST_ASSERT_EQUAL_INT8(1, direction);
}

void test_calculateSpeed_backward_movement(void) {
    int8_t direction = 0;
    double result = calculateSpeedLogic(900.0, 1000.0, direction);

    // Expected: (100 * 360) / 16384 = ~2.197 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.01, 2.197, result);
    TEST_ASSERT_EQUAL_INT8(-1, direction);
}

void test_calculateSpeed_large_jump_forward(void) {
    int8_t direction = 0;
    // Large jump that crosses the 8192 threshold
    double result = calculateSpeedLogic(15000.0, 1000.0, direction);

    // Should use: tempT = 16384 - (15000 - 1000) = 16384 - 14000 = 2384
    // Expected: (2384 * 360) / 16384 = ~52.4 degrees
    TEST_ASSERT_DOUBLE_WITHIN(0.1, 52.4, result);
    TEST_ASSERT_EQUAL_INT8(-1, direction); // Direction inverted for wrap-around
}

void test_calculateSpeed_wrap_around_boundary(void) {
    int8_t direction = 0;
    // Test exactly at the 8192 boundary
    double result = calculateSpeedLogic(9192.0, 1000.0, direction);

    double tempT = abs(9192.0 - 1000.0); // 8192
    if (tempT < 8192) {
        // Should use normal calculation
        TEST_ASSERT_DOUBLE_WITHIN(0.1, 180.0, result); // (8192 * 360) / 16384 = 180
        TEST_ASSERT_EQUAL_INT8(1, direction);
    } else {
        // Should use wrap-around calculation
        double expectedTempT = 16384 - 8192; // 8192
        double expectedSpeed = (expectedTempT * 360) / 16384; // 180
        TEST_ASSERT_DOUBLE_WITHIN(0.1, expectedSpeed, result);
        TEST_ASSERT_EQUAL_INT8(-1, direction);
    }
}

// Test cases for TMC mode switching
void test_updateTMCMode_low_speed_stealth(void) {
    float currentSpeed = 1000.0;
    float maxSpeed = 8000.0;
    float threshold = 0.5;

    bool result = shouldUseStealthChopLogic(currentSpeed, maxSpeed, threshold);

    // 1000/8000 = 0.125 < 0.5, so should use StealthChop
    TEST_ASSERT_TRUE(result);
}

void test_updateTMCMode_high_speed_spreadcycle(void) {
    float currentSpeed = 5000.0;
    float maxSpeed = 8000.0;
    float threshold = 0.5;

    bool result = shouldUseStealthChopLogic(currentSpeed, maxSpeed, threshold);

    // 5000/8000 = 0.625 > 0.5, so should use SpreadCycle
    TEST_ASSERT_FALSE(result);
}

void test_updateTMCMode_threshold_boundary(void) {
    float currentSpeed = 4000.0; // Exactly 50% of 8000
    float maxSpeed = 8000.0;
    float threshold = 0.5;

    bool result = shouldUseStealthChopLogic(currentSpeed, maxSpeed, threshold);

    // 4000/8000 = 0.5, which is NOT < 0.5, so should use SpreadCycle
    TEST_ASSERT_FALSE(result);
}

void test_updateTMCMode_negative_speed(void) {
    float currentSpeed = -3000.0; // Negative speed (backward)
    float maxSpeed = 8000.0;
    float threshold = 0.5;

    bool result = shouldUseStealthChopLogic(currentSpeed, maxSpeed, threshold);

    // abs(-3000)/8000 = 0.375 < 0.5, so should use StealthChop
    TEST_ASSERT_TRUE(result);
}

void test_updateTMCMode_zero_speed(void) {
    float currentSpeed = 0.0;
    float maxSpeed = 8000.0;
    float threshold = 0.5;

    bool result = shouldUseStealthChopLogic(currentSpeed, maxSpeed, threshold);

    // 0/8000 = 0 < 0.5, so should use StealthChop
    TEST_ASSERT_TRUE(result);
}

void setUp(void) {
    // Set up before each test
}

void tearDown(void) {
    // Clean up after each test
}

void setup() {
    UNITY_BEGIN();

    // calculateSpeed tests
    RUN_TEST(test_calculateSpeed_no_movement);
    RUN_TEST(test_calculateSpeed_forward_movement);
    RUN_TEST(test_calculateSpeed_backward_movement);
    RUN_TEST(test_calculateSpeed_large_jump_forward);
    RUN_TEST(test_calculateSpeed_wrap_around_boundary);

    // updateTMCMode tests
    RUN_TEST(test_updateTMCMode_low_speed_stealth);
    RUN_TEST(test_updateTMCMode_high_speed_spreadcycle);
    RUN_TEST(test_updateTMCMode_threshold_boundary);
    RUN_TEST(test_updateTMCMode_negative_speed);
    RUN_TEST(test_updateTMCMode_zero_speed);

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