# Hardware Setup

## Parts list

- **[LilyGo TTGO T-Motor ESP32 Driver Module](https://www.tinytronics.nl/en/development-boards/microcontroller-boards/with-wi-fi/lilygo-ttgo-t-motor-esp32-motor-driver-module-tmc2209)**: ESP32 board with TMC2209 driver and MT6816 encoder.
- **[LilyGO TTGO TTL USB Serial Port Adapter for T-Internet PoE](https://www.tinytronics.nl/en/communication-and-signals/serial/usb/lilygo-ttgo-ttl-usb-serial-port-adapter-for-t-internet-poe)**: needed for the first flash. After that you can update over the air.
- **1mm pitch female headers**: optional, but handy for attaching the USB serial adapter.
- **[Stepper motor](https://www.tinytronics.nl/en/mechanics-and-actuators/motors/stepper-motors)**: NEMA 17, for example 17HS19-2004S1 (1.8°, 59Ncm, 2.0A/phase). Any compatible motor works.
- **[2x limit switches](https://www.tinytronics.nl/en/switches/manual-switches/micro-switches/micro-switch-with-lever)**: micro switches with lever, normally open. Optional, see [Without limit switches](#without-limit-switches).
- **12V DC power supply.** A [USB-C PD trigger board](https://www.amazon.nl/-/en/6pcs-power-supply-trigger-module/dp/B0DGXHN5CT) (not an affiliate link) is a handy universal source.

## Connections

### Power

Connect DC power to the **JST-XH 6P port**:

- **Pin 1**: positive (+)
- **Pin 2**: ground (-)
- **Pins 3-6**: unused in this project

### Stepper motor

Connect the motor phases to the screw terminals. Check the datasheet for the **A+, A-, B+, B-** pinout. Phase order varies by manufacturer. Wrong wiring won't damage anything, but the motor won't move correctly.

Set **Motor Current** in the Settings dialog to your motor's rated current or lower. The default is 400 mA, so an unknown motor is never overdriven.

### Limit switches

- **Switch 1**: GPIO 21 and GND (min limit)
- **Switch 2**: GPIO 22 and GND (max limit)

Switches are normally open and use the internal pull-ups. They are read by hardware interrupts, so the motor stops within microseconds of a trigger.

### Without limit switches

Limits can be set by hand instead. See [Calibration](../README.md#first-use). A motor with no magnet on its rear shaft can run without the encoder: turn off **Encoder Fitted** in Settings.

## Pin configuration

Everything except the two limit switches is already wired on the T-Motor PCB.

| Component | Pin | Description |
|-----------|-----|-------------|
| TMC2209 Enable | 2 | Driver enable |
| TMC2209 Step | 23 | Step signal |
| TMC2209 Direction | 18 | Direction signal |
| TMC2209 UART RX | 26 | Serial communication |
| TMC2209 UART TX | 27 | Serial communication |
| MT6816 SPI CS | 15 | Encoder chip select |
| MT6816 SPI CLK | 14 | Encoder SPI clock |
| MT6816 SPI MISO | 12 | Encoder SPI data in |
| MT6816 SPI MOSI | 13 | Encoder SPI data out |
| Limit Switch 1 | 21 | Min limit (with pullup) |
| Limit Switch 2 | 22 | Max limit (with pullup) |
| Debug Button 1 | 36 | Move forward |
| Debug Button 2 | 34 | Emergency stop |
| Debug Button 3 | 35 | Move backward |

The MT6816 reads 16384 pulses per rotation. The three debug buttons are on the board and optional.

Board dimensions: [`T-Motor.DXF`](images/T-Motor.DXF) and [`T-Motor.pdf`](T-Motor.pdf).
