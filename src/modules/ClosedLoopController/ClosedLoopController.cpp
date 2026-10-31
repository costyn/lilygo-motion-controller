#include "ClosedLoopController.h"
#include "../MotorController/MotorController.h"
#include "util.h"

// Global instance
ClosedLoopController closedLoopController;

ClosedLoopController::ClosedLoopController()
    : encoderPositionSteps(0),
      magnetDetected(false),
      measuring(false),
      settleCycles(0),
      peakSpeed(0),
      lastSteps(0),
      lastEncoderSteps(0),
      noMagnetFrames(0),
      parityErrorFrames(0),
      reportedBadFrames(0),
      lastFrameReportTime(0)
{
}

bool ClosedLoopController::begin()
{
    EncoderSample sample = decodeEncoderFrame(motorController.readEncoderFrame());
    tracker.reset(sample.angle);
    magnetDetected = !sample.noMagnet;
    lastSteps = motorController.getCurrentPosition();

    LOG_INFO("Encoder tracking started in measure-only mode (angle: %u, magnet: %s)",
             sample.angle, magnetDetected ? "detected" : "NOT detected");
    return true;
}

void ClosedLoopController::update()
{
    EncoderSample sample = decodeEncoderFrame(motorController.readEncoderFrame());

    // Parity is only counted until the scheme is confirmed on hardware
    if (!sample.parityOk)
        parityErrorFrames++;
    if (sample.noMagnet)
        noMagnetFrames++;
    else
        tracker.update(sample.angle);

    magnetDetected = !sample.noMagnet;
    encoderPositionSteps = tracker.steps();

    long steps = motorController.getCurrentPosition();
    updateMeasurement(steps);
    reportFrameQuality();

    lastSteps = steps;
    lastEncoderSteps = encoderPositionSteps;
}

void ClosedLoopController::updateMeasurement(long steps)
{
    if (motorController.isMoving())
    {
        if (!measuring)
        {
            // Baseline from the last sample, taken while still stationary
            measurement.start(lastSteps, lastEncoderSteps);
            measuring = true;
            peakSpeed = 0;
            motorController.resetTimingStats();
        }

        settleCycles = 0;
        measurement.sample(steps, encoderPositionSteps);

        float speed = fabs(motorController.getCommandedSpeed());
        if (speed > peakSpeed)
            peakSpeed = speed;
    }
    else if (measuring)
    {
        // Keep sampling while the rotor settles
        measurement.sample(steps, encoderPositionSteps);
        if (++settleCycles >= SETTLE_CYCLES)
        {
            reportMove();
            measuring = false;
            idleMotion.reset(encoderPositionSteps);
        }
    }
    else
    {
        followShaftWhileFreewheeling();
    }
}

void ClosedLoopController::reportMove()
{
    if (measurement.hasLostSteps())
    {
        LOG_WARN("Lost steps detected on this move: finalError=%ld", measurement.getFinalError());
    }

    LOG_INFO("Encoder move: steps=%ld encoder=%ld scale=%.3f peakError=%ld finalError=%ld peakSpeed=%.0f "
             "maxStepGapUs=%lu tmcSwitches=%lu maxTmcSwitchUs=%lu",
             measurement.getStepTravel(), measurement.getEncoderTravel(), measurement.getScale(),
             measurement.getPeakError(), measurement.getFinalError(), peakSpeed,
             motorController.getMaxStepGapUs(), motorController.getTmcSwitchCount(),
             motorController.getMaxTmcSwitchUs());
}

void ClosedLoopController::followShaftWhileFreewheeling()
{
    if (motorController.isMotorEnabled())
    {
        idleMotion.reset(encoderPositionSteps);
        return;
    }

    long travel = idleMotion.takeTravel(encoderPositionSteps);
    if (travel != 0)
    {
        motorController.requestPositionShift(travel);
    }
}

void ClosedLoopController::reportFrameQuality()
{
    uint32_t badFrames = noMagnetFrames + parityErrorFrames;
    if (badFrames != reportedBadFrames && millis() - lastFrameReportTime >= FRAME_REPORT_INTERVAL_MS)
    {
        LOG_WARN("Encoder frame problems so far: noMagnet=%lu parityMismatch=%lu",
                 noMagnetFrames, parityErrorFrames);
        reportedBadFrames = badFrames;
        lastFrameReportTime = millis();
    }
}
