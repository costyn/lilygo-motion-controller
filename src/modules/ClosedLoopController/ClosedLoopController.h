#pragma once

#include <Arduino.h>
#include "EncoderTracker.h"

/**
 * ClosedLoopController - position supervision using the MT6816 encoder
 *
 * Reads the encoder at 200Hz and compares it with the step counter. It stops the motor
 * on a stall, and after a move that lost steps it corrects the step counter and finishes
 * the move at a safer speed. It never steers a running move, because lost steps mean the
 * motor ran out of torque. While the motor is freewheeling, the step counter follows the shaft.
 *
 * All motor changes are requests applied in loop(). Without calibration, or without a
 * magnet, it only tracks position and the motor runs open loop.
 *
 * Encoder positions are relative to boot: the first reading is zero.
 */
class ClosedLoopController
{
private:
    MultiTurnTracker tracker;
    MoveMeasurement measurement;
    IdleMotionTracker idleMotion;
    PositionAlignment alignment;

    volatile long encoderPositionSteps;
    volatile bool magnetDetected;
    volatile long positionError;

    // Correction state
    bool aligned;
    bool stalled;
    uint8_t correctionAttempts;
    uint32_t lastReferenceCount;

    // Move measurement state
    bool measuring;
    unsigned long lastCycleMicros;
    uint32_t maxCycleUs;
    uint8_t settleCycles;
    float peakSpeed;
    long lastSteps;
    long lastEncoderSteps;

    // Encoder frame quality
    uint32_t noMagnetFrames;
    uint32_t parityErrorFrames;
    uint32_t reportedBadFrames;
    unsigned long lastFrameReportTime;

    static constexpr uint8_t SETTLE_CYCLES = 40;                  // 200ms after the step counter stops
    static constexpr uint8_t MAX_CORRECTION_ATTEMPTS = 3;
    static constexpr unsigned long FRAME_REPORT_INTERVAL_MS = 10000;

    void updateAlignment(long steps);
    void updateMeasurement(long steps);
    void checkForStall(long steps);
    void finishMove(long steps);
    void reportMove();
    void followShaftWhileFreewheeling();
    void reportFrameQuality();

public:
    // Constructor
    ClosedLoopController();

    // Initialize the controller (call after motorController.initEncoder())
    bool begin();

    // Main update function (called from ClosedLoopTask at 5ms intervals)
    void update();

    // Status queries (for WebSocket reporting)
    long getEncoderPositionSteps() const { return encoderPositionSteps; }
    bool isMagnetDetected() const { return magnetDetected; }
    long getPositionError() const { return positionError; }
};

extern ClosedLoopController closedLoopController;
