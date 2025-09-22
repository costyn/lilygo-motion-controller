# LilyGo Motion Controller

A modular wireless stepper motor controller for LilyGo T-Motor hardware with TMC2209 driver and MT6816 encoder.

## Features

- **WebSocket-based Control Interface** - Real-time motor control via web browser
- **Smart Limit Switch Handling** - Automatic position learning and persistence
- **Automatic TMC2209 Optimization** - Dynamic switching between StealthChop/SpreadCycle modes
- **Configuration Persistence** - Settings saved to ESP32 NVRAM
- **Over-the-Air Updates** - Wireless firmware updates via ElegantOTA
- **Real-time Position Feedback** - High-precision encoder position reporting
- **WiFi Configuration Portal** - Automatic WiFiManager setup without hardcoded credentials
- **Modular Architecture** - Clean separation of concerns for maintainability

## Hardware Requirements

- **Controller**: LilyGo T-Motor with ESP32 Pico with onboard:
  - **Stepper Driver**: TMC2209
  - **Encoder**: MT6816 magnetic encoder (16384 pulses/rotation)
  - **Debug Buttons**: GPIO 34, 35, 36 (optional, for manual testing)
- **Motor**: 17HS19-2004S1 (1.8°, 59Ncm, 2.0A/phase) or compatible
- **Limit Switches**: Connected to IO21 and IO22

## Pin Configuration

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

## Software Architecture

The system uses a modular architecture with clean separation of concerns:

```
src/
├── main.cpp                    # FreeRTOS task coordination
├── modules/
│   ├── Configuration/          # ESP32 Preferences management
│   ├── MotorController/        # TMC2209 + MT6816 control
│   ├── LimitSwitch/           # Debounced limit switch handling
│   └── WebServer/             # WiFi + WebSocket + REST API
```

### Core Modules

- **Configuration**: Persistent storage of motor parameters, limits, and WiFi settings
- **MotorController**: Factory-accurate TMC2209 initialization and MT6816 encoder integration
- **LimitSwitch**: Debounced switch monitoring with position learning
- **WebServer**: WiFiManager integration, WebSocket control, and REST API

## API Reference

### REST Endpoints

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/status` | Get current motor status and position |
| POST | `/api/move` | Move to position (`position`, `speed` parameters) |
| POST | `/api/stop` | Emergency stop |
| POST | `/api/reset` | Clear emergency stop |
| GET | `/api/config` | Get motor configuration |
| POST | `/api/config` | Update motor configuration (`maxSpeed`, `acceleration`, `useStealthChop` parameters) |

### WebSocket Commands

Send JSON messages to `/ws`:

```json
// Move motor
{"command": "move", "position": 1000, "speed": 50}

// Emergency stop
{"command": "stop"}

// Clear emergency stop
{"command": "reset"}

// Request status update
{"command": "status"}

// Get current configuration
{"command": "getConfig"}

// Update configuration
{
  "command": "setConfig",
  "maxSpeed": 8000,
  "acceleration": 16000,
  "useStealthChop": true
}
```

### WebSocket Responses

```json
// Status broadcast
{
  "type": "status",
  "position": 1000,
  "isMoving": false,
  "emergencyStop": false,
  "limitSwitches": {
    "min": false,
    "max": false,
    "any": false
  }
}

// Position update
{
  "type": "position",
  "position": 1500
}

// Configuration response (getConfig)
{
  "type": "config",
  "maxSpeed": 8000,
  "acceleration": 16000,
  "minLimit": -5000,
  "maxLimit": 5000,
  "useStealthChop": true
}

// Configuration update confirmation (setConfig)
{
  "type": "configUpdated",
  "status": "success"
}
```

## Building and Flashing

### Prerequisites

- PlatformIO Core or VS Code with PlatformIO extension
- ESP32 development environment

### Build Process

```bash
# Clone repository
git clone <repository-url>
cd Esp-stepperserver

# Build firmware
pio run -e pico32

# Flash to device
pio run -e pico32 -t upload

# Monitor serial output
pio device monitor --baud 115200
```

### Memory Usage

- **RAM**: ~15% (48KB)
- **Flash**: ~80% (1MB)

## Configuration

### First-Time Setup

1. **Flash firmware** to ESP32
2. **Connect to WiFi portal**: Device creates "LilyGo-MotionController" access point
3. **Configure WiFi**: Connect via captive portal at 192.168.4.1
4. **Access web interface**: Navigate to device IP address

### Motor Configuration

Default settings can be modified via the Configuration module:

- **Max Speed**: 8000 steps/second
- **Acceleration**: 16000 steps/second²
- **Microsteps**: 16 (1/16th stepping)
- **RMS Current**: 2000mA
- **StealthChop Threshold**: 50% of max speed

### Limit Switch Learning

1. **Manual positioning**: Use buttons or web interface to move motor
2. **Trigger limits**: Move to both extreme positions
3. **Automatic saving**: Positions are automatically learned and saved to NVRAM

## Safety Features

- **Emergency Stop**: Immediate motor halt via button, web interface, or WebSocket
- **Limit Switch Protection**: Automatic stop when limits are triggered
- **Watchdog Protection**: FreeRTOS task monitoring prevents system lockup
- **TMC2209 Thermal Protection**: Built-in driver overtemperature protection

## Troubleshooting

### Common Issues

1. **WiFi Connection Failed**
   - Check credentials in captive portal
   - Reset WiFi settings: hold button during boot

2. **Motor Not Moving**
   - Check emergency stop status via `/api/status`
   - Verify limit switch states
   - Check TMC2209 wiring and power

3. **Position Inaccuracy**
   - Verify MT6816 SPI connections
   - Check encoder mounting and magnet alignment
   - Confirm microstep settings match mechanical setup

4. **Web Interface Unreachable**
   - Check WiFi connection status
   - Verify device IP address via serial monitor
   - Try accessing via mDNS: `http://lilygo-motioncontroller.local`

### Debug Information

Enable detailed logging via serial monitor (115200 baud):
- Module initialization status
- WiFi connection details
- TMC2209 register values
- Encoder position readings
- WebSocket connection events

## Development

### Adding Features

The modular architecture makes extending functionality straightforward:

1. **Create new module** in `src/modules/`
2. **Add initialization** to `main.cpp setup()`
3. **Update tasks** as needed for real-time operation
4. **Expose via API** through WebServer module

### Code Style

- Factory hardware initialization patterns preserved
- Modern C++14 features used where appropriate
- Real-time safety considerations for motor control
- Consistent error handling and logging

## License

[License information to be added]

## Contributing

[Contributing guidelines to be added]

## Acknowledgments

- LilyGo for T-Motor hardware platform
- TMC2209 stepper driver reference implementations
- ESP32 community libraries and examples