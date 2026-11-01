#include "ClosedLoopController.h"
#include "../MotorController/MotorController.h"
#include "../Configuration/Configuration.h"
#include "util.h"

// Global instance
ClosedLoopController closedLoopController;

ClosedLoopController::ClosedLoopController()
    : encoderPositionSteps(0),
      magnetDetected(false),
      positionError(0),
      aligned(false),
      stalled(false),
      trackingCalibration(false),
      calibrationReferenceCount(0),
      stallCount(0),
      lastStallSpeed(0),
      driverSampleSpeed(0),
      driverStatusAtPeak(0),
      driverFaultBits(0),
      driverSampleCycles(0),
      correctionAttempts(0),
      lastReferenceCount(0),
      measuring(false),
      lastCycleMicros(0),
      maxCycleUs(0),
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
    // A cycle over a half turn long would miscount turns, so report the longest one
    unsigned long now = micros();
    if (now - lastCycleMicros > maxCycleUs)
    {
        maxCycleUs = now - lastCycleMicros;
    }
    lastCycleMicros = now;

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
    updateAlignment(steps);
    updateMeasurement(steps);
    reportFrameQuality();

    lastSteps = steps;
    lastEncoderSteps = encoderPositionSteps;
}

void ClosedLoopController::updateAlignment(long steps)
{
    uint32_t referenceCount = motorController.getReferenceCount();
    if (referenceCount != lastReferenceCount || !motorController.isCalibrated() || motorController.isCalibrating())
    {
        // The step counter was given a new reference, so the old link is meaningless
        lastReferenceCount = referenceCount;
        aligned = false;
    }

    if (!aligned && motorController.isCalibrated() && !motorController.isCalibrating() &&
        !motorController.isMoving() && !measuring)
    {
        alignment.align(steps, encoderPositionSteps);
        aligned = true;
        correctionAttempts = 0;
    }

    positionError = aligned ? alignment.error(steps, encoderPositionSteps) : 0;
}

void ClosedLoopController::updateMeasurement(long steps)
{
    // Calibration resets the step counter at the MIN switch, so it isn't one comparable move
    if (motorController.isCalibrating())
    {
        measuring = false;
        idleMotion.reset(encoderPositionSteps);
        checkCalibrationTracking(steps);
        return;
    }
    trackingCalibration = false;

    if (motorController.isMoving())
    {
        if (!measuring)
        {
            // Baseline from the last sample, taken while still stationary
            measurement.start(lastSteps, lastEncoderSteps);
            measuring = true;
            peakSpeed = 0;
            maxCycleUs = 0;
            driverSampleSpeed = 0;
            driverStatusAtPeak = 0;
            driverFaultBits = 0;
            motorController.resetTimingStats();
        }

        settleCycles = 0;
        measurement.sample(steps, encoderPositionSteps);
        checkForStall(steps);
        sampleDriver();

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
            finishMove(steps);
        }
    }
    else
    {
        followShaftWhileFreewheeling();
    }
}

// Calibration has no known position to compare with, so check each leg's travel instead.
// A motor that isn't turning would otherwise run forever, as nothing would hit a switch.
void ClosedLoopController::checkCalibrationTracking(long steps)
{
    uint32_t referenceCount = motorController.getReferenceCount();
    if (!trackingCalibration || referenceCount != calibrationReferenceCount)
    {
        calibrationLeg.start(steps, encoderPositionSteps);
        calibrationReferenceCount = referenceCount;
        trackingCalibration = true;
        return;
    }

    calibrationLeg.sample(steps, encoderPositionSteps);
    if (magnetDetected && PositionAlignment::isStalled(calibrationLeg.getFinalError()))
    {
        LOG_ERROR("Calibration aborted: motor moved %ld steps but the encoder only %ld. Check for a jam",
                  calibrationLeg.getStepTravel(), calibrationLeg.getEncoderTravel());
        stallCount = stallCount + 1;
        lastStallSpeed = (long)fabs(motorController.getCommandedSpeed());
        motorController.emergencyStop();
    }
}

// A UART read takes a few ms, so sample at 20Hz
void ClosedLoopController::sampleDriver()
{
    if (++driverSampleCycles < 10)
    {
        return;
    }
    driverSampleCycles = 0;

    uint32_t status = motorController.readDriverStatus();
    driverFaultBits |= status & 0xFF; // Overtemperature, short and open load flags

    float speed = fabs(motorController.getCommandedSpeed());
    if (speed >= driverSampleSpeed)
    {
        driverSampleSpeed = speed;
        driverStatusAtPeak = status;
    }
}

void ClosedLoopController::checkForStall(long steps)
{
    if (aligned && magnetDetected && !stalled && PositionAlignment::isStalled(alignment.error(steps, encoderPositionSteps)))
    {
        LOG_WARN("Stall detected at position %ld, encoder disagrees by %ld steps",
                 steps, alignment.error(steps, encoderPositionSteps));
        stalled = true;
        stallCount = stallCount + 1;
        lastStallSpeed = (long)peakSpeed;
        motorController.requestStop();
    }
}

void ClosedLoopController::finishMove(long steps)
{
    bool wasStalled = stalled;
    stalled = false;

    if (!aligned || !magnetDetected)
    {
        return;
    }

    long error = alignment.error(steps, encoderPositionSteps);
    if (!PositionAlignment::hasLostSteps(error))
    {
        correctionAttempts = 0;
        return;
    }

    // A stall ended early, so its target is still to come. Otherwise the step counter
    // stopped where it was told to, which may be short of the target after a jog stop.
    long destination = wasStalled ? motorController.getTargetPosition() : steps;

    if (++correctionAttempts > MAX_CORRECTION_ATTEMPTS)
    {
        LOG_ERROR("Giving up after %d position corrections, emergency stop", MAX_CORRECTION_ATTEMPTS);
        correctionAttempts = 0;
        motorController.emergencyStop();
    }

    LOG_WARN("Correcting %ld lost steps, returning to position %ld", error, destination);
    motorController.requestCorrection(-error, destination, config.getMaxSpeed() / 2);
}

void ClosedLoopController::reportMove()
{
    LOG_INFO("Encoder move: steps=%ld encoder=%ld scale=%.3f peakError=%ld finalError=%ld peakSpeed=%.0f "
             "maxStepGapUs=%lu tmcSwitches=%lu maxTmcSwitchUs=%lu maxCycleUs=%lu",
             measurement.getStepTravel(), measurement.getEncoderTravel(), measurement.getScale(),
             measurement.getPeakError(), measurement.getFinalError(), peakSpeed,
             motorController.getMaxStepGapUs(), motorController.getTmcSwitchCount(),
             motorController.getMaxTmcSwitchUs(), maxCycleUs);
    LOG_INFO("Driver at %.0f steps/sec: stealthChop=%d current=%d/31 faultBits=0x%02X",
             driverSampleSpeed, (driverStatusAtPeak >> 30) & 1, (driverStatusAtPeak >> 16) & 0x1F, driverFaultBits);
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
