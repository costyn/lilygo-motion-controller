#include "MotorController.h"
#include "../Configuration/Configuration.h"
#include "../LimitSwitch/LimitSwitch.h"
#include "../ClosedLoopController/EncoderTracker.h"
#include "util.h"
#include <Arduino.h>
#include <string>

// Pin definitions
#define R_SENSE 0.11f
#define EN_PIN 2
#define DIR_PIN 18
#define STEP_PIN 23
#define CLK_PIN 19
#define SPREAD_PIN 4
#define SW_RX 26
#define SW_TX 27
#define DRIVER_ADDRESS 0b00
#define SPI_MT_CS 15
#define SPI_CLK 14
#define SPI_MISO 12
#define SPI_MOSI 13

// Static member initialization
double MotorController::lastLocation = 0;
double MotorController::currentLocation = 0;
double MotorController::monitorSpeed = 0;
float MotorController::motorSpeed = 0;
int8_t MotorController::direction = 1;
volatile long MotorController::livePosition = 0;
volatile MotorController::StopRequest MotorController::stopRequest = MotorController::STOP_NONE;

// Global instance
MotorController motorController;

// Static callback wrappers for limit switches
static void onMinLimitCallback(long position)
{
    motorController.onMinLimitReached(position);
}

static void onMaxLimitCallback(long position)
{
    motorController.onMaxLimitReached(position);
}

MotorController::MotorController()
{
    serialDriver = &Serial1;
    driver = new TMC2209Stepper(serialDriver, R_SENSE, DRIVER_ADDRESS);
    stepper = new AccelStepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
    mt6816 = new SPIClass(HSPI);

    targetPosition = 0;
    emergencyStopActive = false;
    calibrationState = CAL_IDLE;
    calibrationSpeed = 0;
    calibrated = false;
    maxSwitchPosition = 0;
    switchesReferenced = false;
    manualMinSet = false;
    lastUpdateMicros = 0;
    wantSpreadCycle = false;
    spreadCycleActive = false;
    pendingPositionShift = 0;
    moving = false;
    correctionRequested = false;
    correctionShift = 0;
    correctionTarget = 0;
    correctionSpeed = 0;
    referenceCount = 0;
    tmcUartAddress = -1;
    tmcVersion = 0;
    resetTimingStats();
}

void MotorController::resetTimingStats()
{
    maxStepGapUs = 0;
}

bool MotorController::begin()
{
    LOG_INFO("Initializing Motor Controller...");

    // Initialize pins exactly like factory code
    pinMode(CLK_PIN, OUTPUT);
    pinMode(SPREAD_PIN, OUTPUT);
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(SPREAD_PIN, LOW); // HIGH inverts en_spreadCycle, swapping the chopper modes

    pinMode(EN_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);

    // Initialize TMC2209 exactly like factory code
    serialDriver->begin(115200, SERIAL_8N1, SW_RX, SW_TX);
    probeTMCAddress();
    driver->begin(); //  SPI: Init CS pins and possible SW SPI pins
    // UART: Init SW UART (if selected) with default 115200 baudrate
    driver->push();
    driver->pdn_disable(true);

    uint32_t text = driver->IOIN();
    LOG_DEBUG("TMC2209 IOIN : 0X%X", text);

    driver->toff(5);           // Enables driver in software
    driver->rms_current(2000); // Set motor RMS current
    driver->microsteps(16);    // Set microsteps to 1/8th
    driver->ihold(HOLD_CURRENT);

    driver->en_spreadCycle(false); // StealthChop until a fast move asks for SpreadCycle
    driver->pwm_autoscale(true);   // Needed for stealthChop

    // Initialize AccelStepper exactly like factory code
    stepper->setMaxSpeed(config.getMaxSpeed());         // 100mm/s @ 80 steps/mm
    stepper->setAcceleration(config.getAcceleration()); // 2000mm/s^2
    stepper->setEnablePin(EN_PIN);
    stepper->setPinsInverted(false, false, true);
    stepper->enableOutputs();

    digitalWrite(EN_PIN, HIGH); // Disable driver until movement

    // Register calibration callbacks with limit switches
    minLimitSwitch.setLimitCallback(onMinLimitCallback);
    maxLimitSwitch.setLimitCallback(onMaxLimitCallback);

    LOG_INFO("Motor Controller initialized successfully");
    return true;
}

// Read-only check of which UART address the TMC2209 answers on (set by MS1/MS2)
void MotorController::probeTMCAddress()
{
    for (uint8_t address = 0; address < 4; address++)
    {
        TMC2209Stepper probe(serialDriver, R_SENSE, address);
        uint8_t version = probe.version();
        LOG_INFO("TMC2209 UART address 0b%d%d: version 0x%02X", address >> 1, address & 1, version);
        if (version != 0 && version != 0xFF && tmcUartAddress < 0)
        {
            tmcUartAddress = address;
            tmcVersion = version;
        }
    }
}

// Separate encoder initialization (called from InputTask like factory code)
bool MotorController::initEncoder()
{
    LOG_INFO("Initializing MT6816 Encoder...");

    mt6816->begin(SPI_CLK, SPI_MISO, SPI_MOSI, SPI_MT_CS);
    pinMode(SPI_MT_CS, OUTPUT);
    mt6816->setClockDivider(SPI_CLOCK_DIV4);
    // lastLocation = (double)readEncoder();

    LOG_INFO("MT6816 Encoder initialized successfully");
    return true;
}

void MotorController::moveTo(long position, int speed)
{
    if (emergencyStopActive)
    {
        LOG_WARN("Cannot move - emergency stop active");
        return;
    }

    if (isCalibrating())
    {
        LOG_WARN("Cannot move - calibration in progress");
        return;
    }

    if (!calibrated)
    {
        LOG_WARN("Cannot move - calibration required");
        return;
    }

    digitalWrite(EN_PIN, LOW); // Enable motor

    // Clamp speed to safe limits (already validated, but extra safety check)
    if (speed < MIN_SPEED)
        speed = MIN_SPEED;
    if (speed > MAX_SPEED)
        speed = MAX_SPEED;

    wantSpreadCycle = speed > QUIET_MAX_SPEED;
    targetPosition = position;
    stepper->setMaxSpeed(speed);
    stepper->moveTo(position);
    moving = stepper->distanceToGo() != 0;

    LOG_INFO("Moving to position: %ld at speed: %d steps/sec", position, speed);
}

void MotorController::jogStop()
{
    // Stop motor movement without triggering emergency stop flag
    // CRITICAL: Call stop() first to clear AccelStepper's internal target state
    stepper->stop(); // This sets new target to current position with deceleration
    // Then override with immediate stop (no deceleration ramp)
    stepper->setCurrentPosition(stepper->currentPosition());
    stepper->setSpeed(0);

    // Respect freewheel configuration
    if (config.getFreewheelAfterMove())
    {
        digitalWrite(EN_PIN, HIGH); // Freewheel
    }

    LOG_INFO("Motor jog stopped");
}

void MotorController::emergencyStop()
{
    // Stop motor immediately
    stepper->stop();                                         // Clear AccelStepper's internal target state first
    stepper->setCurrentPosition(stepper->currentPosition()); // Stop NOW (override deceleration)
    stepper->setSpeed(0);
    digitalWrite(EN_PIN, HIGH); // Disable motor => freewheel
    emergencyStopActive = true;

    // Stop calibration if in progress
    stopCalibration();

    LOG_WARN("EMERGENCY STOP ACTIVATED");
}

void MotorController::clearEmergencyStop()
{
    emergencyStopActive = false;
    LOG_INFO("Emergency stop cleared");
}

long MotorController::getCurrentPosition() const
{
    return stepper->currentPosition();
}

// Raw 16-bit MT6816 frame: angle, no-magnet warning and parity bits
uint16_t MotorController::readEncoderFrame()
{
    uint16_t temp[2];
    digitalWrite(SPI_MT_CS, LOW);
    mt6816->beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE3));
    temp[0] = mt6816->transfer16(0x8300) & 0xFF;
    mt6816->endTransaction();
    digitalWrite(SPI_MT_CS, HIGH);

    digitalWrite(SPI_MT_CS, LOW);
    mt6816->beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE3));
    temp[1] = mt6816->transfer16(0x8400) & 0xFF;
    mt6816->endTransaction();
    digitalWrite(SPI_MT_CS, HIGH);

    return temp[0] << 8 | temp[1];
}

// Unused for now, no closed loop feature yet:
int MotorController::readEncoder()
{
    return readEncoderFrame() >> 2;
}

// Unused for now, no closed loop feature yet:
double MotorController::calculateSpeed(float ms)
{
    double speedT = 0;
    currentLocation = (double)readEncoder();

    if (currentLocation == lastLocation)
    {
        speedT = direction = 0;
    }
    else
    {
        double tempT = abs(currentLocation - lastLocation);
        if (tempT < 8192)
        {
            speedT = (tempT * 360) / 16384;
            direction = currentLocation > lastLocation ? 1 : -1;
        }
        else
        {
            speedT = ((currentLocation > lastLocation ? 16384 - currentLocation + lastLocation : 16384 - lastLocation + currentLocation) * 360) / 16384;
            direction = currentLocation > lastLocation ? -1 : 1;
        }
    }

    speedT = direction * (speedT * ms / 1000);
    lastLocation = currentLocation;
    return speedT;
}

uint32_t MotorController::getTMCStatus()
{
    return driver->IOIN();
}

void MotorController::update()
{
    // Track movement state for completion detection
    static bool wasMoving = false;
    bool isMoving = !emergencyStopActive && (stepper->distanceToGo() != 0);

    // Time between update() calls while moving: no steps can be sent during a gap
    unsigned long now = micros();
    if (wasMoving && isMoving && now - lastUpdateMicros > maxStepGapUs)
    {
        maxStepGapUs = now - lastUpdateMicros;
    }
    lastUpdateMicros = now;

    // Stop first. MIN is ignored while calibration backs off it, because that is switch bounce.
    StopRequest stop = stopRequest;
    if (stop != STOP_NONE)
    {
        stopRequest = STOP_NONE;
        if (!(stop == STOP_MIN_SWITCH && calibrationState == CAL_FINDING_MAX))
        {
            jogStop();
            isMoving = false;
        }
    }

    // Shift and move together, so the destination is measured from the corrected position.
    // A move started in the meantime takes priority.
    if (correctionRequested && !isMoving)
    {
        correctionRequested = false;
        stepper->setCurrentPosition(stepper->currentPosition() + correctionShift);
        moveTo(correctionTarget, correctionSpeed);
    }
    else if (correctionRequested)
    {
        correctionRequested = false;
    }

    // Apply step counter shifts only while idle, so they can't disturb a move
    if (pendingPositionShift != 0 && !isMoving)
    {
        long shift = pendingPositionShift;
        pendingPositionShift = 0;
        stepper->setCurrentPosition(stepper->currentPosition() + shift);
    }

    livePosition = stepper->currentPosition();

    // Handle calibration state machine
    if (calibrationState == CAL_COMPLETE)
    {
        // Transition from CAL_COMPLETE back to CAL_IDLE
        calibrationState = CAL_IDLE;
        calibrationSpeed = 0;
        LOG_INFO("Calibration sequence finished");
    }

    // Handle movement
    if (emergencyStopActive)
    {
        stepper->setSpeed(0);
        digitalWrite(EN_PIN, HIGH); // Always freewheel during emergency stop
    }
    else if (isMoving)
    {
        // Motor is moving - call run() to step motor
        // A move can start while the branch below is freewheeling, so enable it again here
        if (!wasMoving)
        {
            digitalWrite(EN_PIN, LOW);
        }
        stepper->run();
        wasMoving = true;
    }
    else if (wasMoving)
    {
        // Motor just stopped moving
        if (config.getFreewheelAfterMove())
        {
            digitalWrite(EN_PIN, HIGH); // Freewheel
            LOG_INFO("Movement complete - freewheeling");
        }
        else
        {
            // Motor enabled but idle - holding position
            LOG_INFO("Movement complete - holding position");
        }
        wasMoving = false;
    }
    // else: motor is stopped and we've already logged it

    moving = !emergencyStopActive && stepper->distanceToGo() != 0;
}

void MotorController::setAcceleration(long accel)
{
    // Clamp acceleration to safe limits
    if (accel < MIN_ACCELERATION)
    {
        LOG_WARN("Acceleration %ld below minimum, clamping to %ld", accel, MIN_ACCELERATION);
        accel = MIN_ACCELERATION;
    }
    else if (accel > MAX_ACCELERATION)
    {
        LOG_WARN("Acceleration %ld above maximum, clamping to %ld", accel, MAX_ACCELERATION);
        accel = MAX_ACCELERATION;
    }

    stepper->setAcceleration(accel);
    LOG_INFO("Acceleration set to: %ld steps/sec²", accel);
}

void MotorController::setMaxSpeed(long speed)
{
    // Clamp speed to safe limits
    if (speed < MIN_SPEED)
    {
        LOG_WARN("Speed %ld below minimum, clamping to %ld", speed, MIN_SPEED);
        speed = MIN_SPEED;
    }
    else if (speed > MAX_SPEED)
    {
        LOG_WARN("Speed %ld above maximum, clamping to %ld", speed, MAX_SPEED);
        speed = MAX_SPEED;
    }

    stepper->setMaxSpeed(speed);
    LOG_INFO("Max speed set to: %ld steps/sec", speed);
}

void MotorController::setCurrentPosition(long position)
{
    stepper->setCurrentPosition(position);
}

void MotorController::startCalibration()
{
    if (emergencyStopActive)
    {
        LOG_WARN("Cannot start calibration - emergency stop active");
        return;
    }

    if (isCalibrating())
    {
        LOG_WARN("Cannot start calibration - already calibrating");
        return;
    }

    calibrated = false;
    switchesReferenced = false;
    manualMinSet = false;
    calibrationState = CAL_FINDING_MIN;
    // Direct drive: slow configured speed, so the stop latency doesn't crash into the switches
    calibrationSpeed = config.getCalibrationDirectDrive() ? config.getCalibrationSpeed() : config.getMaxSpeed();

    wantSpreadCycle = calibrationSpeed > QUIET_MAX_SPEED;

    // Already on the MIN switch: driving further would only push against the end stop
    if (minLimitSwitch.isPressed())
    {
        onMinLimitReached(getCurrentPosition());
        return;
    }

    // Move toward MIN limit (large negative position)
    // Note: This bypasses the isCalibrating() check in moveTo() because we haven't set the state yet
    // We need to manually enable motor and start movement
    digitalWrite(EN_PIN, LOW); // Enable motor

    // Clamp speed to safe limits
    long speed = calibrationSpeed;
    if (speed < MIN_SPEED)
        speed = MIN_SPEED;
    if (speed > MAX_SPEED)
        speed = MAX_SPEED;

    stepper->setMaxSpeed(speed);
    stepper->moveTo(-999999); // Move toward MIN limit

    LOG_INFO("Calibration started - finding MIN limit at speed %ld steps/sec (%.0f%% of max)",
             speed, (speed * 100.0 / config.getMaxSpeed()));
}

void MotorController::stopCalibration()
{
    if (calibrationState != CAL_IDLE)
    {
        calibrationState = CAL_IDLE;
        calibrationSpeed = 0;
        LOG_INFO("Calibration stopped");
    }
}

void MotorController::onMinLimitReached(long position)
{
    if (calibrationState == CAL_FINDING_MIN)
    {
        LOG_INFO("Calibration: MIN limit reached at position %ld", position);
        referencePosition(position, 0);

        // Transition to finding MAX limit
        calibrationState = CAL_FINDING_MAX;

        // Move toward MAX limit (large positive position)
        digitalWrite(EN_PIN, LOW); // Enable motor

        // Use same calibration speed
        long speed = calibrationSpeed;
        if (speed < MIN_SPEED)
            speed = MIN_SPEED;
        if (speed > MAX_SPEED)
            speed = MAX_SPEED;

        stepper->setMaxSpeed(speed);
        stepper->moveTo(999999); // Move toward MAX limit

        LOG_INFO("Calibration: now finding MAX limit at speed %ld steps/sec", speed);
    }
    else if (switchesReferenced)
    {
        referencePosition(position, 0);
    }
}

void MotorController::onMaxLimitReached(long position)
{
    if (calibrationState == CAL_FINDING_MAX)
    {
        LOG_INFO("Calibration: MAX limit reached at position %ld", position);

        // Calibration complete
        maxSwitchPosition = position;
        config.saveLimitPositions(0, position);
        calibrated = true;
        switchesReferenced = true;
        calibrationState = CAL_COMPLETE;

        LOG_INFO("Calibration complete - min: %ld, max: %ld",
                 config.getLimitPos1(), config.getLimitPos2());
    }
    else if (calibrationState == CAL_FINDING_MIN)
    {
        LOG_ERROR("Calibration aborted: MAX switch hit while finding MIN. Check motor direction and switch wiring");
        stopCalibration();
    }
    else if (switchesReferenced)
    {
        referencePosition(position, maxSwitchPosition);
    }
}

const char *MotorController::setMinHere()
{
    if (isCalibrating() || isMoving())
    {
        return "Cannot set a limit while the motor is moving";
    }

    // Before calibration this point becomes position 0, as the MIN switch would
    if (!calibrated)
    {
        referencePosition(getCurrentPosition(), 0);
        manualMinSet = true;
        config.saveLimitPositions(0, config.getLimitPos2());
        return nullptr;
    }

    long position = getCurrentPosition();
    if (position >= config.getMaxLimit())
    {
        return "Min must be below Max";
    }

    config.saveLimitPositions(position, config.getLimitPos2());
    return nullptr;
}

const char *MotorController::setMaxHere()
{
    if (isCalibrating() || isMoving())
    {
        return "Cannot set a limit while the motor is moving";
    }

    if (!calibrated && !manualMinSet)
    {
        return "Set Min first";
    }

    long position = getCurrentPosition();
    if (position <= config.getMinLimit())
    {
        return "Max must be above Min";
    }

    config.saveLimitPositions(config.getLimitPos1(), position);
    calibrated = true;
    return nullptr;
}

// Applies a changed setting straight away, otherwise it would wait for the next move to end
void MotorController::applyFreewheelSetting()
{
    if (!isMoving() && !isCalibrating() && !emergencyStopActive)
    {
        digitalWrite(EN_PIN, config.getFreewheelAfterMove() ? HIGH : LOW);
    }
}

void MotorController::updateChopperMode()
{
    if (wantSpreadCycle != spreadCycleActive)
    {
        spreadCycleActive = wantSpreadCycle;
        driver->en_spreadCycle(spreadCycleActive);
    }
}

void MotorController::requestCorrection(long shift, long target, int speed)
{
    correctionShift = shift;
    correctionTarget = target;
    correctionSpeed = speed;
    correctionRequested = true;
}

// A switch gives the true position, which also repairs the step counter after a slip
void MotorController::referencePosition(long measuredPosition, long actualPosition)
{
    long shift = actualPosition - measuredPosition;
    stepper->setCurrentPosition(stepper->currentPosition() + shift);
    referenceCount = referenceCount + 1;

    if (calibrated && labs(shift) >= LOST_STEP_THRESHOLD)
    {
        LOG_WARN("Limit switch corrected position by %ld steps", shift);
    }
}