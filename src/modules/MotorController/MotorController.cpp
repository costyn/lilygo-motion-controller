#include "MotorController.h"
#include "../Configuration/Configuration.h"
#include "util.h"
#include <Arduino.h>

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

// Global instance
MotorController motorController;

MotorController::MotorController()
{
    serialDriver = &Serial1;
    driver = new TMC2209Stepper(serialDriver, R_SENSE, DRIVER_ADDRESS);
    stepper = new AccelStepper(AccelStepper::DRIVER, STEP_PIN, DIR_PIN);
    mt6816 = new SPIClass(HSPI);

    targetPosition = 0;
    emergencyStopActive = false;
    useStealthChop = true;
    needsLimitRecovery = false;
    limitRecoveryPosition = 0;
}

bool MotorController::begin()
{
    LOG_INFO("Initializing Motor Controller...");

    // Initialize pins exactly like factory code
    pinMode(CLK_PIN, OUTPUT);
    pinMode(SPREAD_PIN, OUTPUT);
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(SPREAD_PIN, HIGH);

    pinMode(EN_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    digitalWrite(EN_PIN, LOW); // Enable driver in hardware

    // Initialize TMC2209 exactly like factory code
    serialDriver->begin(115200, SERIAL_8N1, SW_RX, SW_TX);
    driver->begin(); //  SPI: Init CS pins and possible SW SPI pins
    // UART: Init SW UART (if selected) with default 115200 baudrate
    driver->push();
    driver->pdn_disable(true);

    uint32_t text = driver->IOIN();
    LOG_DEBUG("TMC2209 IOIN : 0X%X", text);

    driver->toff(5);           // Enables driver in software
    driver->rms_current(2000); // Set motor RMS current
    driver->microsteps(16);    // Set microsteps to 1/16th
    driver->ihold(1);

    driver->en_spreadCycle(true); // Toggle spreadCycle on TMC2208/2209/2224
    driver->pwm_autoscale(true);  // Needed for stealthChop

    // Initialize AccelStepper exactly like factory code
    stepper->setMaxSpeed(config.getMaxSpeed());         // 100mm/s @ 80 steps/mm
    stepper->setAcceleration(config.getAcceleration()); // 2000mm/s^2
    stepper->setEnablePin(EN_PIN);
    stepper->setPinsInverted(false, false, true);
    stepper->enableOutputs();

    LOG_INFO("Motor Controller initialized successfully");
    return true;
}

// Separate encoder initialization (called from InputTask like factory code)
bool MotorController::initEncoder()
{
    LOG_INFO("Initializing MT6816 Encoder...");

    mt6816->begin(SPI_CLK, SPI_MISO, SPI_MOSI, SPI_MT_CS);
    pinMode(SPI_MT_CS, OUTPUT);
    mt6816->setClockDivider(SPI_CLOCK_DIV4);
    lastLocation = (double)readEncoder();

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

    // Clamp speed to safe limits (already validated, but extra safety check)
    if (speed < MIN_SPEED)
        speed = MIN_SPEED;
    if (speed > MAX_SPEED)
        speed = MAX_SPEED;

    targetPosition = position;
    stepper->setMaxSpeed(speed);
    stepper->moveTo(position);

    LOG_INFO("Moving to position: %ld at speed: %d steps/sec", position, speed);
}

void MotorController::stop()
{
    emergencyStopActive = true;
    stepper->setSpeed(0);
    stepper->stop();
    LOG_INFO("Motor stopped");
}

void MotorController::stopGently()
{
    // Stop motor movement without triggering emergency stop flag
    // Use setCurrentPosition to stop immediately (no deceleration ramp)
    stepper->setCurrentPosition(stepper->currentPosition());
    stepper->setSpeed(0);
    LOG_INFO("Motor stopped gently");
}

void MotorController::emergencyStop()
{
    stop();
    LOG_WARN("EMERGENCY STOP ACTIVATED");
}

void MotorController::emergencyStopWithRecovery(long limitPosition)
{
    stop();
    needsLimitRecovery = true;
    limitRecoveryPosition = limitPosition;
    LOG_WARN("EMERGENCY STOP ACTIVATED - will recover to position %ld after deceleration", limitPosition);
}

void MotorController::clearEmergencyStop()
{
    emergencyStopActive = false;
    needsLimitRecovery = false;
    LOG_INFO("Emergency stop cleared");
}

long MotorController::getCurrentPosition() const
{
    return stepper->currentPosition();
}

int MotorController::readEncoder()
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

    return (int)(temp[0] << 6 | temp[1] >> 2);
}

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

void MotorController::updateTMCMode()
{
    float currentSpeedPercent = abs(motorSpeed) / (float)config.getMaxSpeed();
    bool shouldUseStealthChop = currentSpeedPercent < STEALTH_CHOP_THRESHOLD;

    if (shouldUseStealthChop != useStealthChop)
    {
        useStealthChop = shouldUseStealthChop;
        driver->en_spreadCycle(!useStealthChop);
        LOG_DEBUG("TMC mode switched to %s", useStealthChop ? "StealthChop" : "SpreadCycle");
    }
}

void MotorController::setTMCMode(bool stealthChop)
{
    useStealthChop = stealthChop;
    driver->en_spreadCycle(!stealthChop);
    LOG_INFO("TMC mode manually set to %s", stealthChop ? "StealthChop" : "SpreadCycle");
}

uint32_t MotorController::getTMCStatus()
{
    return driver->IOIN();
}

void MotorController::update()
{
    // Calculate current speed from encoder
    monitorSpeed = calculateSpeed(100);

    // Update TMC mode based on speed
    updateTMCMode();

    // Handle movement
    if (emergencyStopActive)
    {
        stepper->setSpeed(0);

        // Check if recovery is needed after deceleration completes
        if (needsLimitRecovery && !stepper->isRunning())
        {
            long currentPos = stepper->currentPosition();

            // Only recover if we're not already at the limit position
            if (currentPos != limitRecoveryPosition)
            {
                LOG_INFO("Deceleration complete at position %ld, recovering to limit position %ld",
                         currentPos, limitRecoveryPosition);

                // Clear emergency stop to allow recovery movement
                emergencyStopActive = false;
                needsLimitRecovery = false;

                // Move back to limit position at slow speed
                stepper->setMaxSpeed(MIN_SPEED * 5); // 5x minimum speed = slow recovery
                stepper->moveTo(limitRecoveryPosition);

                LOG_INFO("Recovery move started");
            }
            else
            {
                // Already at limit position, just clear recovery flag but keep emergency stop
                needsLimitRecovery = false;
                LOG_INFO("Already at limit position %ld, emergency stop remains active", limitRecoveryPosition);
            }
        }
    }
    else
    {
        stepper->run();
    }
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