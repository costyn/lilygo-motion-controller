#include <unity.h>
#include "../../../src/modules/PatternRunner/PatternPlan.h"

void setUp(void) {}
void tearDown(void) {}

static const long MIN_LIMIT = 0;
static const long MAX_LIMIT = 28629; // The lamp's measured travel
static const long MAX_SPEED = 4000;
static const long MAX_ACCELERATION = 200000;

static Segment segment(float to, float seconds, float speed, float ramp)
{
    Segment s = {to, seconds, speed, ramp, 0};
    return s;
}

// Time for a trapezoid move, from the planned speed and acceleration
static float moveSeconds(const MovePlan &plan, long distance)
{
    float rampDistance = (float)plan.speed * plan.speed / (2.0f * plan.acceleration);
    if (2 * rampDistance >= distance)
        return 2.0f * sqrtf((float)distance / plan.acceleration);
    return 2.0f * plan.speed / plan.acceleration + (distance - 2 * rampDistance) / plan.speed;
}

void test_percent_maps_onto_the_limits(void) {
    TEST_ASSERT_EQUAL(0, percentToSteps(0, MIN_LIMIT, MAX_LIMIT));
    TEST_ASSERT_EQUAL(28629, percentToSteps(100, MIN_LIMIT, MAX_LIMIT));
    TEST_ASSERT_EQUAL(14315, percentToSteps(50, MIN_LIMIT, MAX_LIMIT));
    TEST_ASSERT_EQUAL(1100, percentToSteps(50, 100, 2100));
}

void test_percent_outside_the_range_is_clamped(void) {
    TEST_ASSERT_EQUAL(0, percentToSteps(-5, MIN_LIMIT, MAX_LIMIT));
    TEST_ASSERT_EQUAL(28629, percentToSteps(120, MIN_LIMIT, MAX_LIMIT));
}

void test_speed_segment_uses_a_share_of_max_speed(void) {
    MovePlan plan = planSegment(segment(100, 0, 50, 0.5f), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    TEST_ASSERT_EQUAL(28629, plan.target);
    TEST_ASSERT_EQUAL(2000, plan.speed);
}

void test_half_ramp_reaches_top_speed_at_the_halfway_point(void) {
    MovePlan plan = planSegment(segment(100, 0, 50, 0.5f), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    float rampDistance = (float)plan.speed * plan.speed / (2.0f * plan.acceleration);
    TEST_ASSERT_FLOAT_WITHIN(75, 28629 / 2.0f, rampDistance); // Acceleration is rounded to whole steps/sec²
}

void test_ramp_covers_its_share_of_the_distance(void) {
    MovePlan plan = planSegment(segment(50, 0, 20, 0.2f), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    float rampDistance = (float)plan.speed * plan.speed / (2.0f * plan.acceleration);
    TEST_ASSERT_FLOAT_WITHIN(20, 0.2f * 14315, rampDistance);
}

void test_timed_segment_takes_the_time_asked(void) {
    MovePlan plan = planSegment(segment(100, 20, 0, 0.25f), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 20.0f, moveSeconds(plan, 28629));
}

void test_timed_triangle_takes_the_time_asked(void) {
    MovePlan plan = planSegment(segment(0, 15, 0, 0.5f), 28629, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    TEST_ASSERT_EQUAL(0, plan.target);
    TEST_ASSERT_FLOAT_WITHIN(0.1f, 15.0f, moveSeconds(plan, 28629));
}

void test_too_short_a_time_is_capped_at_max_speed(void) {
    MovePlan plan = planSegment(segment(100, 1, 0, 0.25f), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    TEST_ASSERT_EQUAL(MAX_SPEED, plan.speed);
}

void test_negative_ramp_uses_the_configured_acceleration(void) {
    MovePlan plan = planSegment(segment(100, 0, 100, -1), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    TEST_ASSERT_EQUAL(0, plan.acceleration);
    TEST_ASSERT_EQUAL(MAX_SPEED, plan.speed);
}

void test_ramp_is_kept_within_its_range(void) {
    MovePlan wide = planSegment(segment(100, 0, 50, 0.9f), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    MovePlan half = planSegment(segment(100, 0, 50, 0.5f), 0, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    TEST_ASSERT_EQUAL(half.acceleration, wide.acceleration);
}

void test_segment_already_at_its_target_does_not_divide_by_zero(void) {
    MovePlan plan = planSegment(segment(100, 10, 0, 0.25f), 28629, MIN_LIMIT, MAX_LIMIT, MAX_SPEED, MAX_ACCELERATION);
    TEST_ASSERT_EQUAL(28629, plan.target);
    TEST_ASSERT_TRUE(plan.speed >= 1);
    TEST_ASSERT_EQUAL(0, plan.acceleration);
}

int main(int argc, char **argv) {
    UNITY_BEGIN();
    RUN_TEST(test_percent_maps_onto_the_limits);
    RUN_TEST(test_percent_outside_the_range_is_clamped);
    RUN_TEST(test_speed_segment_uses_a_share_of_max_speed);
    RUN_TEST(test_half_ramp_reaches_top_speed_at_the_halfway_point);
    RUN_TEST(test_ramp_covers_its_share_of_the_distance);
    RUN_TEST(test_timed_segment_takes_the_time_asked);
    RUN_TEST(test_timed_triangle_takes_the_time_asked);
    RUN_TEST(test_too_short_a_time_is_capped_at_max_speed);
    RUN_TEST(test_negative_ramp_uses_the_configured_acceleration);
    RUN_TEST(test_ramp_is_kept_within_its_range);
    RUN_TEST(test_segment_already_at_its_target_does_not_divide_by_zero);
    return UNITY_END();
}
