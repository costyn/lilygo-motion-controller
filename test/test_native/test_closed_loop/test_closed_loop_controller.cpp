#include <unity.h>
#include "../../../src/modules/ClosedLoopController/EncoderTracker.h"

void setUp(void) {}
void tearDown(void) {}

// ============================================================================
// Frame decoding
// ============================================================================

void test_decode_extracts_14_bit_angle(void) {
    EncoderSample sample = decodeEncoderFrame((uint16_t)(0x2ABC << 2));
    TEST_ASSERT_EQUAL_UINT16(0x2ABC, sample.angle);
}

void test_decode_reads_no_magnet_bit(void) {
    TEST_ASSERT_TRUE(decodeEncoderFrame(0x0002).noMagnet);
    TEST_ASSERT_FALSE(decodeEncoderFrame(0x0000).noMagnet);
}

void test_decode_all_ones_frame_is_flagged_no_magnet(void) {
    TEST_ASSERT_TRUE(decodeEncoderFrame(0xFFFF).noMagnet);
}

void test_decode_parity_passes_on_even_bit_count(void) {
    TEST_ASSERT_TRUE(decodeEncoderFrame(0x0000).parityOk);
    TEST_ASSERT_TRUE(decodeEncoderFrame(0x0005).parityOk);
    TEST_ASSERT_TRUE(decodeEncoderFrame(0x8001).parityOk);
}

void test_decode_parity_fails_on_odd_bit_count(void) {
    TEST_ASSERT_FALSE(decodeEncoderFrame(0x0001).parityOk);
    TEST_ASSERT_FALSE(decodeEncoderFrame(0x8000).parityOk);
    TEST_ASSERT_FALSE(decodeEncoderFrame(0x0007).parityOk);
}

// ============================================================================
// Multi-turn tracking
// ============================================================================

void test_tracker_starts_at_zero_whatever_the_boot_angle(void) {
    MultiTurnTracker tracker;
    tracker.reset(12345);
    tracker.update(12345);
    TEST_ASSERT_EQUAL_INT32(0, tracker.steps());
}

void test_tracker_follows_forward_motion(void) {
    MultiTurnTracker tracker;
    tracker.reset(1000);
    tracker.update(9000);
    TEST_ASSERT_EQUAL_INT32(-1562, tracker.steps()); // 8000 counts, encoder runs inverted
}

void test_tracker_crosses_zero_forward(void) {
    MultiTurnTracker tracker;
    tracker.reset(16000);
    tracker.update(400);
    TEST_ASSERT_EQUAL_INT32(-153, tracker.steps()); // 784 counts
}

void test_tracker_crosses_zero_backward(void) {
    MultiTurnTracker tracker;
    tracker.reset(400);
    tracker.update(16000);
    TEST_ASSERT_EQUAL_INT32(153, tracker.steps());
}

void test_tracker_counts_multiple_turns(void) {
    MultiTurnTracker tracker;
    tracker.reset(0);
    for (int turn = 0; turn < 5; turn++) {
        tracker.update(5000);
        tracker.update(10000);
        tracker.update(15000);
        tracker.update(0);
    }
    TEST_ASSERT_EQUAL_INT32(-5 * STEPS_PER_REV, tracker.steps());
}

void test_tracker_returns_to_start_after_turning_back(void) {
    MultiTurnTracker tracker;
    tracker.reset(100);
    tracker.update(6000);
    tracker.update(12000);
    tracker.update(2000);
    tracker.update(12000);
    tracker.update(6000);
    tracker.update(100);
    TEST_ASSERT_EQUAL_INT32(0, tracker.steps());
}

// ============================================================================
// Move measurement
// ============================================================================

void test_measurement_reports_scale(void) {
    MoveMeasurement move;
    move.start(100, 500);
    move.sample(1100, 1500);

    TEST_ASSERT_EQUAL_INT32(1000, move.getStepTravel());
    TEST_ASSERT_EQUAL_INT32(1000, move.getEncoderTravel());
    TEST_ASSERT_FLOAT_WITHIN(0.001f, 1.0f, move.getScale());
    TEST_ASSERT_EQUAL_INT32(0, move.getFinalError());
}

void test_measurement_shows_wrong_direction_as_negative_scale(void) {
    MoveMeasurement move;
    move.start(0, 0);
    move.sample(1000, -1000);

    TEST_ASSERT_FLOAT_WITHIN(0.001f, -1.0f, move.getScale());
}

void test_measurement_keeps_peak_error_during_move(void) {
    MoveMeasurement move;
    move.start(0, 0);
    move.sample(1000, 700);
    move.sample(2000, 1995);

    TEST_ASSERT_EQUAL_INT32(300, move.getPeakError());
    TEST_ASSERT_EQUAL_INT32(5, move.getFinalError());
}

void test_measurement_flags_rig_lost_step_move(void) {
    MoveMeasurement move;
    move.start(0, 0);
    move.sample(-2778, -2668);

    TEST_ASSERT_EQUAL_INT32(110, move.getFinalError());
    TEST_ASSERT_TRUE(move.hasLostSteps());
}

void test_measurement_allows_rig_settling_error(void) {
    MoveMeasurement move;
    move.start(0, 0);
    move.sample(3576, 3561);

    TEST_ASSERT_EQUAL_INT32(15, move.getFinalError());
    TEST_ASSERT_FALSE(move.hasLostSteps());
}

void test_measurement_handles_no_step_travel(void) {
    MoveMeasurement move;
    move.start(0, 0);
    move.sample(0, 25);

    TEST_ASSERT_EQUAL_FLOAT(0.0f, move.getScale());
}

// ============================================================================
// Idle motion
// ============================================================================

void test_idle_motion_ignores_encoder_noise(void) {
    IdleMotionTracker idle;
    idle.reset(1000);

    TEST_ASSERT_EQUAL_INT32(0, idle.takeTravel(1007));
    TEST_ASSERT_EQUAL_INT32(0, idle.takeTravel(993));
}

void test_idle_motion_hands_over_real_travel_once(void) {
    IdleMotionTracker idle;
    idle.reset(1000);

    TEST_ASSERT_EQUAL_INT32(50, idle.takeTravel(1050));
    TEST_ASSERT_EQUAL_INT32(0, idle.takeTravel(1050));
    TEST_ASSERT_EQUAL_INT32(-80, idle.takeTravel(970));
}

void test_idle_motion_accumulates_slow_turning_without_loss(void) {
    IdleMotionTracker idle;
    idle.reset(0);

    long total = 0;
    for (long encoder = 3; encoder <= 300; encoder += 3) {
        total += idle.takeTravel(encoder);
    }

    TEST_ASSERT_EQUAL_INT32(297, total);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();

    RUN_TEST(test_decode_extracts_14_bit_angle);
    RUN_TEST(test_decode_reads_no_magnet_bit);
    RUN_TEST(test_decode_all_ones_frame_is_flagged_no_magnet);
    RUN_TEST(test_decode_parity_passes_on_even_bit_count);
    RUN_TEST(test_decode_parity_fails_on_odd_bit_count);

    RUN_TEST(test_tracker_starts_at_zero_whatever_the_boot_angle);
    RUN_TEST(test_tracker_follows_forward_motion);
    RUN_TEST(test_tracker_crosses_zero_forward);
    RUN_TEST(test_tracker_crosses_zero_backward);
    RUN_TEST(test_tracker_counts_multiple_turns);
    RUN_TEST(test_tracker_returns_to_start_after_turning_back);

    RUN_TEST(test_measurement_reports_scale);
    RUN_TEST(test_measurement_shows_wrong_direction_as_negative_scale);
    RUN_TEST(test_measurement_keeps_peak_error_during_move);
    RUN_TEST(test_measurement_flags_rig_lost_step_move);
    RUN_TEST(test_measurement_allows_rig_settling_error);
    RUN_TEST(test_measurement_handles_no_step_travel);

    RUN_TEST(test_idle_motion_ignores_encoder_noise);
    RUN_TEST(test_idle_motion_hands_over_real_travel_once);
    RUN_TEST(test_idle_motion_accumulates_slow_turning_without_loss);

    return UNITY_END();
}
