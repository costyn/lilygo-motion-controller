#pragma once

#include <math.h>
#include <stdint.h>

// Pure pattern maths with no hardware dependencies, so native tests use this code directly.

static constexpr uint8_t MAX_PATTERNS = 8;
static constexpr uint8_t MAX_SEGMENTS = 8;
static constexpr uint8_t MAX_PRESETS = 12;
static constexpr uint8_t MAX_NAME_LENGTH = 24;

// Each segment ramps up over this fraction of its distance and down over the same at the
// end, so every move eases in and out. 0.5 reaches top speed at the halfway point.
static constexpr float MIN_RAMP = 0.05f;
static constexpr float MAX_RAMP = 0.5f;
static constexpr float DEFAULT_RAMP = 0.25f;

// One move of a pattern. Give either seconds or speed.
struct Segment
{
    float to;      // Target, % of travel: 0 is MIN, 100 is MAX
    float seconds; // Duration of the move, or 0 to use speed
    float speed;   // % of Max Speed, used when seconds is 0
    float ramp;    // Fraction of the distance spent speeding up, and again slowing down.
                   // Below 0 uses the configured Acceleration instead.
    float hold;    // Seconds to wait at the target
};

struct Pattern
{
    char name[MAX_NAME_LENGTH + 1];
    uint8_t repeat; // Times through the segments, 0 for until stopped
    uint8_t segmentCount;
    Segment segments[MAX_SEGMENTS];
};

struct MovePlan
{
    long target;       // Steps
    long speed;        // Steps/sec
    long acceleration; // Steps/sec², 0 for the configured Acceleration
};

inline long percentToSteps(float percent, long minLimit, long maxLimit)
{
    if (percent < 0)
        percent = 0;
    if (percent > 100)
        percent = 100;
    return minLimit + lroundf(percent / 100.0f * (float)(maxLimit - minLimit));
}

// Speed and acceleration for a trapezoid that covers the segment as asked. Speed is
// capped at Max Speed, so a move that would need more takes longer instead.
inline MovePlan planSegment(const Segment &segment, long from, long minLimit, long maxLimit,
                            long maxSpeed, long maxAcceleration)
{
    MovePlan plan;
    plan.target = percentToSteps(segment.to, minLimit, maxLimit);
    plan.acceleration = 0;

    float distance = fabsf((float)(plan.target - from));
    bool configuredRamp = segment.ramp < 0;
    float ramp = configuredRamp ? DEFAULT_RAMP : fminf(fmaxf(segment.ramp, MIN_RAMP), MAX_RAMP);

    float speed;
    if (segment.seconds > 0)
    {
        // Ramping over a fraction r of the distance at each end takes d(1 + 2r) / v
        speed = distance * (1.0f + 2.0f * ramp) / segment.seconds;
    }
    else
    {
        speed = segment.speed / 100.0f * (float)maxSpeed;
    }

    if (speed > (float)maxSpeed)
        speed = (float)maxSpeed;
    if (speed < 1.0f)
        speed = 1.0f;
    plan.speed = lroundf(speed);

    if (!configuredRamp && distance > 0)
    {
        float acceleration = speed * speed / (2.0f * ramp * distance);
        if (acceleration > (float)maxAcceleration)
            acceleration = (float)maxAcceleration;
        if (acceleration < 1.0f)
            acceleration = 1.0f;
        plan.acceleration = lroundf(acceleration);
    }

    return plan;
}
