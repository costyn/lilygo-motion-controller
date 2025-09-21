#include <Arduino.h>
#include "esp_system.h"
#include <TMCStepper.h>
#include <AccelStepper.h>
#include "WiFi.h"
#include <HTTPClient.h>

/*
This T-Motor example is a factory test/demo sketch that:

  What it does:
  - Sets up a TMC2209 stepper driver with encoder (MT6816) feedback
  - Creates a multi-tasking system with FreeRTOS
  - Displays motor position and speed on a small OLED screen
  - Provides button controls to increase/decrease speed or stop
  - Connects to WiFi and makes a test HTTP request to baidu.com (just for
  connectivity testing)

  The WiFi part (lines 145-201) is just a factory test to verify the
  ESP32's WiFi connectivity - it scans networks, connects to the hardcoded
   WiFi credentials, and makes a simple HTTP GET request. This is purely
  for testing the board's WiFi functionality.

  Will your stepper motor move?
  No, not immediately. The motor starts with Motor_Speed = 0 (line 64), so
   it won't move until you press the buttons:
  - Button 1 (GPIO 36): Increases speed
  - Button 2 (GPIO 34): Decreases speed
  - Button 3 (GPIO 35): Stops motor

  The sketch continuously runs stepper.runSpeed() in the main loop, so
  once you set a speed using the buttons, the motor will start moving.

  To connect your stepper motor: Wire it to the TMC2209 driver outputs,
  and the driver connects to the ESP32 via pins 23 (STEP), 18 (DIR), and 2
   (ENABLE).

   The hardware serial connection to the TMC2209 serves a completely
  different purpose than the basic STEP/DIR control:

  STEP/DIR/ENABLE pins (lines 210-213):
  - These provide basic stepper control - just tell the driver when to
  step, which direction, and enable/disable
  - This is the minimal interface that works with any stepper driver

  UART Serial connection (lines 215-228):
  - This accesses the TMC2209's advanced configuration registers
  - Allows you to configure things like:
    - Motor current (driver.rms_current(2000) - line 223)
    - Microstepping resolution (driver.microsteps(16) - line 224)
    - StealthChop vs SpreadCycle modes (driver.en_spreadCycle(true) -
  line 227)
    - Stallguard sensitivity, chopper settings, etc.
    - Read diagnostic information (driver.IOIN() - line 220)

  Do you need it?
  - For basic operation: No, STEP/DIR/ENABLE is sufficient
  - For optimal performance: Yes, the UART lets you tune the driver for
  your specific motor and application
  - For diagnostics: Yes, you can read driver status, detect stalls,
  monitor temperature, etc.

  Without the UART connection, the TMC2209 would use its default
  settings (which might work fine) but you'd lose the ability to
  optimize current settings, microstepping, and access advanced features
   like sensorless homing or stall detection.

  The example uses both interfaces - UART for configuration, STEP/DIR
  for motion control.
*/

#define WIFI_SSID ""     //"your ssid";
#define WIFI_PASSWORD "" //"your password";

#define MAX_SPEED (180 * steps_per_mm)
// If the pin connection optocoupler is 1 :OUTPUT
// If the pin connection MOS is 0 :INPUT
#define EN_DIR_STEP_OUTPUT 1

#define R_SENSE 0.11f // Match to your driver                         \
// SilentStepStick series use 0.11              \
// UltiMachine Einsy and Archim2 boards use 0.2 \
// Panucatt BSD2660 uses 0.1                    \
// Watterott TMC5160 uses 0.075

#define EN_PIN 2    // enable (CFG6)
#define DIR_PIN 18  // direction
#define STEP_PIN 23 // step

#define CLK_PIN 19
#define SPREAD_PIN 4

#define SW_RX 26 // TMC2208/TMC2224 SoftwareSerial receive pin
#define SW_TX 27 // TMC2208/TMC2224 SoftwareSerial transmit pin

#define DRIVER_ADDRESS 0b00 // TMC2209 Driver address according to MS1 and MS2

#define BTN1 36 // NEXT
#define BTN2 34 // ENTER
#define BTN3 35 // MENU

#define SPI_MT_CS 15 // MT6816
#define SPI_CLK 14
#define SPI_MISO 12
#define SPI_MOSI 13

#define IIC_SCL 21
#define IIC_SDA 22

#define iStep 25
#define iDIR 32
#define iEN 33

HardwareSerial SerialDriver(Serial1);
TMC2209Stepper driver(&SerialDriver, R_SENSE, DRIVER_ADDRESS);
AccelStepper stepper(stepper.DRIVER, STEP_PIN, DIR_PIN);
SPIClass MT6816;
HTTPClient http;

constexpr uint32_t steps_per_mm = 80;
static double Monitor_Speed;
static float Motor_Speed;

int8_t Direction = 1;
double _lastLocation = 0;
double _currentLocation = 0;

void motor_init(void);
void MT6816_init(void);
int MT6816_read(void);
double ReadSpeed(float ms);
void SpeedAdd();
void SpeedSub();
void SpeedStop();
void Task1(void *pvParameters);
void Task2(void *pvParameters);

void setup()
{
    Serial.begin(115200);
    motor_init();
    stepper.setSpeed(Motor_Speed);
    xTaskCreatePinnedToCore(Task1, "Task1", 1024 * 10, NULL, 2, NULL, 0);
    xTaskCreatePinnedToCore(Task2, "Task2", 1024 * 20, NULL, 2, NULL, 0);
}

void loop()
{
    stepper.runSpeed();
}

void Task1(void *pvParameters)
{
    bool quite = false;
    ledcSetup(0, 1500, 8);
    ledcSetup(1, 1500, 8);
    ledcSetup(2, 1500, 8);

    ledcAttachPin(iStep, 0);
    ledcAttachPin(iDIR, 1);
    ledcAttachPin(iEN, 2);

    ledcWrite(0, 0xFF);
    ledcWrite(1, 0xFF);
    ledcWrite(2, 0xFF);

    for (uint8_t i = 0; i < 3; i++)
    {
        ledcWrite(i, 0x80);
        delay(300);
        ledcWrite(i, 0xFF);
        delay(300);
    }

    ledcDetachPin(iStep);
    ledcDetachPin(iDIR);
    ledcDetachPin(iEN);

    pinMode(iStep, INPUT);
    pinMode(iDIR, INPUT);
    pinMode(iEN, INPUT);

    MT6816_init();

    while (1)
    {
        Monitor_Speed = ReadSpeed(100);
        delay(100);
    }
}

void Task2(void *pvParameters)
{
    bool isConnected = false;
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    Serial.println("scan start");

    int n = WiFi.scanNetworks();
    Serial.println("scan done");
    if (n == 0)
    {
        Serial.println("no networks found");
    }
    else
    {
        Serial.print(n);
        Serial.println(" networks found");
        for (int i = 0; i < n; ++i)
        {
            // Print SSID and RSSI for each network found
            Serial.print(i + 1);
            Serial.print(": ");
            Serial.print(WiFi.SSID(i));
            Serial.print(" (");
            Serial.print(WiFi.RSSI(i));
            Serial.print(")");
            Serial.println((WiFi.encryptionType(i) == WIFI_AUTH_OPEN) ? " " : "*");
            delay(10);
        }

        if (!isConnected)
        {
            WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
        }
        while (WiFi.status() != WL_CONNECTED)
        {
            delay(500);
            Serial.print(".");
        }
        Serial.println("");
        Serial.println("WiFi connected");
        Serial.println("IP address: ");
        Serial.println(WiFi.localIP());
        isConnected = true;
    }

    vTaskDelete(NULL);
}

void motor_init(void)
{
    pinMode(CLK_PIN, OUTPUT);
    pinMode(SPREAD_PIN, OUTPUT);
    digitalWrite(CLK_PIN, LOW);
    digitalWrite(SPREAD_PIN, HIGH);

    pinMode(EN_PIN, OUTPUT);
    pinMode(STEP_PIN, OUTPUT);
    pinMode(DIR_PIN, OUTPUT);
    digitalWrite(EN_PIN, LOW); // Enable driver in hardware

    SerialDriver.begin(115200, SERIAL_8N1, SW_RX, SW_TX);
    driver.begin(); //  SPI: Init CS pins and possible SW SPI pins
    // UART: Init SW UART (if selected) with default 115200 baudrate
    driver.push();
    driver.pdn_disable(true);
    uint32_t text = driver.IOIN();
    Serial.printf("TMC2209 IOIN : 0X%X\r\n", text);
    driver.toff(5);           // Enables driver in software
    driver.rms_current(2000); // Set motor RMS current
    driver.microsteps(16);    // Set microsteps to 1/16th
    driver.ihold(1);

    driver.en_spreadCycle(true); // Toggle spreadCycle on TMC2208/2209/2224
    driver.pwm_autoscale(true);  // Needed for stealthChop

    // driver->TSTEP();
    stepper.setMaxSpeed(MAX_SPEED);               // 100mm/s @ 80 steps/mm
    stepper.setAcceleration(1000 * steps_per_mm); // 2000mm/s^2
    stepper.setEnablePin(EN_PIN);
    stepper.setPinsInverted(false, false, true);
    stepper.enableOutputs();
}

void MT6816_init(void)
{
    MT6816.begin(SPI_CLK, SPI_MISO, SPI_MOSI, SPI_MT_CS);
    pinMode(SPI_MT_CS, OUTPUT);
    MT6816.setClockDivider(SPI_CLOCK_DIV4);
    _lastLocation = (double)MT6816_read();
}

int MT6816_read(void)
{
    uint16_t temp[2];
    digitalWrite(SPI_MT_CS, LOW);
    MT6816.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE3));
    temp[0] = MT6816.transfer16(0x8300) & 0xFF;
    MT6816.endTransaction();
    digitalWrite(SPI_MT_CS, HIGH);

    digitalWrite(SPI_MT_CS, LOW);
    MT6816.beginTransaction(SPISettings(400000, MSBFIRST, SPI_MODE3));
    temp[1] = MT6816.transfer16(0x8400) & 0xFF;
    MT6816.endTransaction();
    digitalWrite(SPI_MT_CS, HIGH);
    return (int)(temp[0] << 6 | temp[1] >> 2);
}

double ReadSpeed(float ms)
{
    double Speed_t = 0;
    _currentLocation = (double)MT6816_read();

    if (_currentLocation == _lastLocation)
        Speed_t = Direction = 0;
    else
    {
        double temp_t = abs(_currentLocation - _lastLocation);
        if (temp_t < 8192)
        { // The displacement per unit time above the maximum is considered to be reversed
            Speed_t = (temp_t * 360) / 16384;
            Direction = _currentLocation > _lastLocation ? 1 : -1;
        }
        else
        {
            Speed_t = ((_currentLocation > _lastLocation ? 16384 - _currentLocation + _lastLocation : 16384 - _lastLocation + _currentLocation) * 360) / 16384;
            Direction = _currentLocation > _lastLocation ? -1 : 1;
        }
    }
    Speed_t = Direction * (Speed_t * ms / 1000);
    _lastLocation = _currentLocation;
    return Speed_t;
}

void SpeedAdd()
{
    if (Motor_Speed < MAX_SPEED)
    {
        Motor_Speed += MAX_SPEED / 10;
        stepper.setSpeed(Motor_Speed);
        Serial.printf("Motor_Speed:%f\r\n", Motor_Speed);
    }
}
void SpeedSub()
{
    if ((-1 * Motor_Speed) < MAX_SPEED)
    {
        Motor_Speed -= MAX_SPEED / 10;
        stepper.setSpeed(Motor_Speed);
        Serial.printf("Motor_Speed:%f\r\n", Motor_Speed);
    }
}
void SpeedStop()
{
    Motor_Speed = 0;
    stepper.setSpeed(Motor_Speed);
    Serial.printf("Motor_Speed:0\r\n");
}