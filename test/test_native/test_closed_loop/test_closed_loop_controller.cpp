#include <unity.h>
#include <stdint.h>

// ============================================================================
// Test helper functions that replicate ClosedLoopController calculation logic
// without hardware dependencies
// ============================================================================

// Constants from ClosedLoopController (motor-specific)
static constexpr float STEPS_PER_REV = 3200.0f;           // 200 steps/rev × 16 microsteps
static constexpr float ENCODER_COUNTS_PER_REV = 16384.0f; // 14-bit encoder
static constexpr float STEPS_PER_ENCODER_COUNT = STEPS_PER_REV / ENCODER_COUNTS_PER_REV;
static constexpr float DEADBAND_THRESHOLD_DEGREES = 3.0f;

// ============================================================================
// Rotation Counter Logic (Multi-turn tracking with wrap detection)
// ============================================================================

/**
 * Updates rotation counter based on encoder wrap-around detection
 * Returns the updated rotation count
 */
int32_t updateRotationCounterLogic(int32_t currentRotationCount, uint16_t currentRaw, uint16_t lastRaw) {
    int16_t delta = currentRaw - lastRaw;

    // Detect wrap-around using half-scale threshold (8192)
    if (delta > 8192) {
        // Large positive delta means wrapped backward (16384 → 0)
        return currentRotationCount - 1;
    } else if (delta < -8192) {
        // Large negative delta means wrapped forward (0 → 16384)
        return currentRotationCount + 1;
    }

    return currentRotationCount; // No wrap detected
}

/**
 * Converts encoder position (rotation count + raw value) to motor steps
 */
long encoderToSteps(int32_t rotationCount, uint16_t encoderRaw) {
    int64_t totalEncoderCounts = (int64_t)rotationCount * (int64_t)ENCODER_COUNTS_PER_REV + (int64_t)encoderRaw;
    return (long)(totalEncoderCounts * STEPS_PER_ENCODER_COUNT);
}

/**
 * Converts steps to degrees for deadband comparison
 */
float stepsToDegrees(long steps) {
    return (abs(steps) / STEPS_PER_REV) * 360.0f;
}

/**
 * Converts degrees to steps for deadband threshold
 */
long degreesToSteps(float degrees) {
    return (long)((degrees / 360.0f) * STEPS_PER_REV);
}

// ============================================================================
// TEST CASES: Rotation Counter Wrap Detection
// ============================================================================

void test_rotation_counter_no_wrap_forward(void) {
    int32_t rotationCount = 0;
    uint16_t lastRaw = 1000;
    uint16_t currentRaw = 1500;

    int32_t result = updateRotationCounterLogic(rotationCount, currentRaw, lastRaw);

    TEST_ASSERT_EQUAL_INT32(0, result); // No wrap, count unchanged
}

void test_rotation_counter_no_wrap_backward(void) {
    int32_t rotationCount = 0;
    uint16_t lastRaw = 1500;
    uint16_t currentRaw = 1000;

    int32_t result = updateRotationCounterLogic(rotationCount, currentRaw, lastRaw);

    TEST_ASSERT_EQUAL_INT32(0, result); // No wrap, count unchanged
}

void test_rotation_counter_wrap_forward(void) {
    int32_t rotationCount = 0;
    uint16_t lastRaw = 16000; // Near max (16383)
    uint16_t currentRaw = 100; // Wrapped to near zero

    // Delta = 100 - 16000 = -15900
    // Delta < -8192, so should increment rotation count
    int32_t result = updateRotationCounterLogic(rotationCount, currentRaw, lastRaw);

    TEST_ASSERT_EQUAL_INT32(1, result); // Forward wrap, count incremented
}

void test_rotation_counter_wrap_backward(void) {
    int32_t rotationCount = 1;
    uint16_t lastRaw = 100;   // Near zero
    uint16_t currentRaw = 16000; // Wrapped to near max

    // Delta = 16000 - 100 = 15900
    // Delta > 8192, so should decrement rotation count
    int32_t result = updateRotationCounterLogic(rotationCount, currentRaw, lastRaw);

    TEST_ASSERT_EQUAL_INT32(0, result); // Backward wrap, count decremented
}

void test_rotation_counter_multiple_rotations_forward(void) {
    int32_t rotationCount = 5;
    uint16_t lastRaw = 16000;
    uint16_t currentRaw = 100;

    int32_t result = updateRotationCounterLogic(rotationCount, currentRaw, lastRaw);

    TEST_ASSERT_EQUAL_INT32(6, result); // Should be at 6 rotations now
}

void test_rotation_counter_multiple_rotations_backward(void) {
    int32_t rotationCount = 5;
    uint16_t lastRaw = 100;
    uint16_t currentRaw = 16000;

    int32_t result = updateRotationCounterLogic(rotationCount, currentRaw, lastRaw);

    TEST_ASSERT_EQUAL_INT32(4, result); // Should be at 4 rotations now
}

void test_rotation_counter_exactly_at_threshold(void) {
    int32_t rotationCount = 0;
    uint16_t lastRaw = 0;
    uint16_t currentRaw = 8192; // Exactly at half-scale

    // Delta = 8192, which is NOT > 8192, so no wrap
    int32_t result = updateRotationCounterLogic(rotationCount, currentRaw, lastRaw);

    TEST_ASSERT_EQUAL_INT32(0, result); // No wrap at boundary
}

// ============================================================================
// TEST CASES: Coordinate Conversion (Encoder to Steps)
// ============================================================================

void test_encoder_to_steps_zero_position(void) {
    int32_t rotationCount = 0;
    uint16_t encoderRaw = 0;

    long result = encoderToSteps(rotationCount, encoderRaw);

    TEST_ASSERT_EQUAL_INT32(0, result); // Zero position
}

void test_encoder_to_steps_one_rotation(void) {
    int32_t rotationCount = 1;
    uint16_t encoderRaw = 0;

    long result = encoderToSteps(rotationCount, encoderRaw);

    // 1 rotation × 16384 counts × 0.1953125 steps/count = 3200 steps
    TEST_ASSERT_EQUAL_INT32(3200, result);
}

void test_encoder_to_steps_half_rotation(void) {
    int32_t rotationCount = 0;
    uint16_t encoderRaw = 8192; // Half of 16384

    long result = encoderToSteps(rotationCount, encoderRaw);

    // 8192 counts × 0.1953125 steps/count = 1600 steps
    TEST_ASSERT_EQUAL_INT32(1600, result);
}

void test_encoder_to_steps_multiple_rotations(void) {
    int32_t rotationCount = 10;
    uint16_t encoderRaw = 0;

    long result = encoderToSteps(rotationCount, encoderRaw);

    // 10 rotations × 3200 steps/rotation = 32000 steps
    TEST_ASSERT_EQUAL_INT32(32000, result);
}

void test_encoder_to_steps_fractional_position(void) {
    int32_t rotationCount = 2;
    uint16_t encoderRaw = 4096; // Quarter rotation

    long result = encoderToSteps(rotationCount, encoderRaw);

    // (2 × 16384 + 4096) × 0.1953125 = 7200 steps
    TEST_ASSERT_EQUAL_INT32(7200, result);
}

void test_encoder_to_steps_negative_rotations(void) {
    int32_t rotationCount = -1;
    uint16_t encoderRaw = 0;

    long result = encoderToSteps(rotationCount, encoderRaw);

    // -1 rotation × 3200 steps = -3200 steps
    TEST_ASSERT_EQUAL_INT32(-3200, result);
}

// ============================================================================
// TEST CASES: Deadband Calculation (Steps ↔ Degrees)
// ============================================================================

void test_steps_to_degrees_zero(void) {
    long steps = 0;

    float result = stepsToDegrees(steps);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, result);
}

void test_steps_to_degrees_one_rotation(void) {
    long steps = 3200; // Full rotation

    float result = stepsToDegrees(steps);

    TEST_ASSERT_EQUAL_FLOAT(360.0f, result);
}

void test_steps_to_degrees_half_rotation(void) {
    long steps = 1600; // Half rotation

    float result = stepsToDegrees(steps);

    TEST_ASSERT_EQUAL_FLOAT(180.0f, result);
}

void test_steps_to_degrees_quarter_rotation(void) {
    long steps = 800; // Quarter rotation

    float result = stepsToDegrees(steps);

    TEST_ASSERT_EQUAL_FLOAT(90.0f, result);
}

void test_steps_to_degrees_deadband_threshold(void) {
    // Deadband is 3° - how many steps is that?
    long expectedSteps = degreesToSteps(DEADBAND_THRESHOLD_DEGREES);

    // Convert back to degrees to verify
    float result = stepsToDegrees(expectedSteps);

    TEST_ASSERT_FLOAT_WITHIN(0.1f, 3.0f, result);
}

void test_degrees_to_steps_zero(void) {
    float degrees = 0.0f;

    long result = degreesToSteps(degrees);

    TEST_ASSERT_EQUAL_INT32(0, result);
}

void test_degrees_to_steps_one_rotation(void) {
    float degrees = 360.0f;

    long result = degreesToSteps(degrees);

    TEST_ASSERT_EQUAL_INT32(3200, result);
}

void test_degrees_to_steps_deadband_threshold(void) {
    float degrees = 3.0f;

    long result = degreesToSteps(degrees);

    // 3° = (3 / 360) × 3200 = 26.666... ≈ 26 steps
    TEST_ASSERT_INT32_WITHIN(1, 27, result); // Allow ±1 for rounding
}

void test_degrees_to_steps_small_angle(void) {
    float degrees = 1.0f;

    long result = degreesToSteps(degrees);

    // 1° = (1 / 360) × 3200 = 8.888... ≈ 9 steps
    TEST_ASSERT_INT32_WITHIN(1, 9, result);
}

// ============================================================================
// TEST CASES: Proportional Correction Calculation
// ============================================================================

void test_proportional_correction_small_error(void) {
    // Error within deadband should not trigger correction
    long errorSteps = 20; // ~2.25° (within 3° deadband)
    float Kp = 0.5f;

    long deadbandSteps = degreesToSteps(DEADBAND_THRESHOLD_DEGREES);
    bool shouldCorrect = abs(errorSteps) > deadbandSteps;

    TEST_ASSERT_FALSE(shouldCorrect); // Should NOT correct
}

void test_proportional_correction_large_error(void) {
    // Error exceeding deadband should trigger correction
    long errorSteps = 100; // ~11.25° (exceeds 3° deadband)
    float Kp = 0.5f;

    long deadbandSteps = degreesToSteps(DEADBAND_THRESHOLD_DEGREES);
    bool shouldCorrect = abs(errorSteps) > deadbandSteps;

    TEST_ASSERT_TRUE(shouldCorrect); // Should correct

    // Correction amount = Kp × error = 0.5 × 100 = 50 steps
    long correction = (long)(errorSteps * Kp);
    TEST_ASSERT_EQUAL_INT32(50, correction);
}

void test_proportional_correction_exactly_at_deadband(void) {
    // Error exactly at deadband boundary
    long deadbandSteps = degreesToSteps(DEADBAND_THRESHOLD_DEGREES);
    long errorSteps = deadbandSteps; // Exactly 3°

    bool shouldCorrect = abs(errorSteps) > deadbandSteps;

    TEST_ASSERT_FALSE(shouldCorrect); // Should NOT correct (not > threshold)
}

void test_proportional_correction_large_missed_steps(void) {
    // Simulate large missed step scenario (half rotation error)
    long errorSteps = 1600; // 180° error
    float Kp = 0.5f;

    long deadbandSteps = degreesToSteps(DEADBAND_THRESHOLD_DEGREES);
    TEST_ASSERT_TRUE(abs(errorSteps) > deadbandSteps); // Should correct

    // Correction = 0.5 × 1600 = 800 steps (gradual correction)
    long correction = (long)(errorSteps * Kp);
    TEST_ASSERT_EQUAL_INT32(800, correction);
}

// ============================================================================
// Unity Test Framework Setup
// ============================================================================

void setUp(void) {
    // Set up before each test (if needed)
}

void tearDown(void) {
    // Clean up after each test (if needed)
}

void setup() {
    UNITY_BEGIN();

    // Rotation counter tests
    RUN_TEST(test_rotation_counter_no_wrap_forward);
    RUN_TEST(test_rotation_counter_no_wrap_backward);
    RUN_TEST(test_rotation_counter_wrap_forward);
    RUN_TEST(test_rotation_counter_wrap_backward);
    RUN_TEST(test_rotation_counter_multiple_rotations_forward);
    RUN_TEST(test_rotation_counter_multiple_rotations_backward);
    RUN_TEST(test_rotation_counter_exactly_at_threshold);

    // Coordinate conversion tests
    RUN_TEST(test_encoder_to_steps_zero_position);
    RUN_TEST(test_encoder_to_steps_one_rotation);
    RUN_TEST(test_encoder_to_steps_half_rotation);
    RUN_TEST(test_encoder_to_steps_multiple_rotations);
    RUN_TEST(test_encoder_to_steps_fractional_position);
    RUN_TEST(test_encoder_to_steps_negative_rotations);

    // Deadband calculation tests
    RUN_TEST(test_steps_to_degrees_zero);
    RUN_TEST(test_steps_to_degrees_one_rotation);
    RUN_TEST(test_steps_to_degrees_half_rotation);
    RUN_TEST(test_steps_to_degrees_quarter_rotation);
    RUN_TEST(test_steps_to_degrees_deadband_threshold);
    RUN_TEST(test_degrees_to_steps_zero);
    RUN_TEST(test_degrees_to_steps_one_rotation);
    RUN_TEST(test_degrees_to_steps_deadband_threshold);
    RUN_TEST(test_degrees_to_steps_small_angle);

    // Proportional correction tests
    RUN_TEST(test_proportional_correction_small_error);
    RUN_TEST(test_proportional_correction_large_error);
    RUN_TEST(test_proportional_correction_exactly_at_deadband);
    RUN_TEST(test_proportional_correction_large_missed_steps);

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
