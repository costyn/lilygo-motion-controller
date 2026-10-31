#pragma once

#include <Arduino.h>
#include "EncoderTracker.h"

/**
 * ClosedLoopController - MT6816 encoder tracking, currently MEASURE-ONLY
 *
 * This module never moves the motor. It reads the encoder at 50Hz and logs how encoder
 * travel compares with the step counter for each move, and warns about lost steps.
 * While the motor is freewheeling, the step counter follows the shaft.
 *
 * Encoder positions are relative to boot: the first reading is zero.
 */
class ClosedLoopController
{
private:
    MultiTurnTracker tracker;
    MoveMeasurement measurement;
    IdleMotionTracker idleMotion;

    volatile long encoderPositionSteps;
    volatile bool magnetDetected;

    // Move measurement state
    bool measuring;
    uint8_t settleCycles;
    float peakSpeed;
    long lastSteps;
    long lastEncoderSteps;

    // Encoder frame quality
    uint32_t noMagnetFrames;
    uint32_t parityErrorFrames;
    uint32_t reportedBadFrames;
    unsigned long lastFrameReportTime;

    static constexpr uint8_t SETTLE_CYCLES = 10;                  // 200ms after the step counter stops
    static constexpr unsigned long FRAME_REPORT_INTERVAL_MS = 10000;

    void updateMeasurement(long steps);
    void reportMove();
    void followShaftWhileFreewheeling();
    void reportFrameQuality();

public:
    // Constructor
    ClosedLoopController();

    // Initialize the controller (call after motorController.initEncoder())
    bool begin();

    // Main update function (called from ClosedLoopTask at 20ms intervals)
    void update();

    // Status queries (for WebSocket reporting)
    long getEncoderPositionSteps() const { return encoderPositionSteps; }
    bool isMagnetDetected() const { return magnetDetected; }
};

extern ClosedLoopController closedLoopController;
