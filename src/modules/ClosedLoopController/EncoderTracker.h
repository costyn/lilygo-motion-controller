#pragma once

#include <stdint.h>

// Pure encoder maths with no hardware dependencies, so native tests use this code directly.

// Motor: 200 steps/rev x 16 microsteps. Encoder: 14-bit MT6816.
static constexpr int32_t STEPS_PER_REV = 3200;
static constexpr int32_t ENCODER_COUNTS_PER_REV = 16384;

// Measured on the rig: encoder counts run opposite to the step counter
static constexpr int8_t ENCODER_DIRECTION = -1;

// Slips come in electrical cycles of 64 microsteps. This allows for the 15-step
// settling error measured on moves without a slip.
static constexpr long LOST_STEP_THRESHOLD = 48;

struct EncoderSample
{
    uint16_t angle; // 0-16383
    bool noMagnet;
    bool parityOk;
};

// Frame layout: bits 15-2 angle, bit 1 no-magnet warning, bit 0 parity.
// Parity is assumed to be even across all 16 bits, unconfirmed against the datasheet.
inline EncoderSample decodeEncoderFrame(uint16_t frame)
{
    uint16_t bits = frame;
    bits ^= bits >> 8;
    bits ^= bits >> 4;
    bits ^= bits >> 2;
    bits ^= bits >> 1;

    EncoderSample sample;
    sample.angle = frame >> 2;
    sample.noMagnet = (frame & 0x0002) != 0;
    sample.parityOk = (bits & 1) == 0;
    return sample;
}

// Accumulates single-turn angles into a multi-turn position, relative to the first angle.
// Assumes less than half a turn between updates: at 50Hz that is 25 rev/s (80,000 steps/sec).
class MultiTurnTracker
{
private:
    int64_t counts = 0;
    uint16_t lastAngle = 0;

public:
    void reset(uint16_t angle)
    {
        counts = 0;
        lastAngle = angle;
    }

    void update(uint16_t angle)
    {
        int32_t delta = (int32_t)angle - (int32_t)lastAngle;
        if (delta >= ENCODER_COUNTS_PER_REV / 2)
            delta -= ENCODER_COUNTS_PER_REV;
        else if (delta < -ENCODER_COUNTS_PER_REV / 2)
            delta += ENCODER_COUNTS_PER_REV;

        counts += delta;
        lastAngle = angle;
    }

    long steps() const
    {
        return (long)(ENCODER_DIRECTION * counts * STEPS_PER_REV / ENCODER_COUNTS_PER_REV);
    }
};

// Encoder travel to hand on to the step counter while the motor is idle. Small
// movements are held back so encoder noise doesn't jitter the reported position.
class IdleMotionTracker
{
private:
    long reference = 0;

public:
    static constexpr long MIN_TRAVEL_STEPS = 8;

    void reset(long encoderSteps) { reference = encoderSteps; }

    long takeTravel(long encoderSteps)
    {
        long travel = encoderSteps - reference;
        if (travel > -MIN_TRAVEL_STEPS && travel < MIN_TRAVEL_STEPS)
            return 0;

        reference = encoderSteps;
        return travel;
    }
};

// Compares step counter travel with encoder travel over one move
class MoveMeasurement
{
private:
    long startSteps = 0;
    long startEncoder = 0;
    long stepTravel = 0;
    long encoderTravel = 0;
    long peakError = 0;

    static long magnitude(long value) { return value < 0 ? -value : value; }

public:
    void start(long steps, long encoderSteps)
    {
        startSteps = steps;
        startEncoder = encoderSteps;
        stepTravel = 0;
        encoderTravel = 0;
        peakError = 0;
    }

    void sample(long steps, long encoderSteps)
    {
        stepTravel = steps - startSteps;
        encoderTravel = encoderSteps - startEncoder;

        long error = magnitude(stepTravel - encoderTravel);
        if (error > peakError)
            peakError = error;
    }

    long getStepTravel() const { return stepTravel; }
    long getEncoderTravel() const { return encoderTravel; }
    long getPeakError() const { return peakError; }
    long getFinalError() const { return magnitude(stepTravel - encoderTravel); }
    bool hasLostSteps() const { return getFinalError() >= LOST_STEP_THRESHOLD; }

    // Encoder travel per step travelled: 1.0 means direction and scale are right
    float getScale() const
    {
        return stepTravel == 0 ? 0.0f : (float)encoderTravel / (float)stepTravel;
    }
};
