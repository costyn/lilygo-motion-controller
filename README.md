# LilyGo Motion Controller

A modular wireless stepper motor controller for LilyGo T-Motor hardware with TMC2209 driver and MT6816 encoder.

## Features

### Core Functionality
- **WebSocket-based Control Interface** - Real-time motor control via web browser
- **Smart Limit Switch Handling** - Automatic position learning and persistence
- **Automatic TMC2209 Optimization** - Dynamic switching between StealthChop/SpreadCycle modes
- **Configuration Persistence** - Settings saved to ESP32 NVRAM
- **Over-the-Air Updates** - Wireless firmware updates via ElegantOTA
- **Real-time Position Feedback** - High-precision encoder position reporting
- **WiFi Configuration Portal** - Automatic WiFiManager setup without hardcoded credentials
- **Modular Architecture** - Clean separation of concerns for maintainability

### Developer & Debugging Features
- **mDNS Network Discovery** - Access device via `http://lilygo-motioncontroller.local/`
- **Debug WebSocket Stream** - Real-time serial output via `/debug` WebSocket for remote debugging
- **Unified Logging System** - Consistent timestamped logging with LOG_ERROR/WARN/INFO/DEBUG macros
- **WebSocket Command Logging** - All incoming commands logged for development troubleshooting
- **Cross-Platform Unit Testing** - Native tests for motor calculation functions (no hardware required)

## Hardware Requirements

- **Controller**: LilyGo T-Motor with ESP32 Pico with onboard:
  - **Stepper Driver**: TMC2209
  - **Encoder**: MT6816 magnetic encoder (16384 pulses/rotation)
  - **Debug Buttons**: GPIO 34, 35, 36 (optional, for manual testing)
- **Motor**: 17HS19-2004S1 (1.8°, 59Ncm, 2.0A/phase) or compatible
- **Limit Switches**: Connected to IO21 and IO22

## Pin Configuration
All pins except the 2 limit switches are already configured as described below on the LilyGo T-Motor PCB.

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

### WebSocket Commands (Primary Interface)

**WebSocket is the primary control interface.** Connect to `/ws` and send JSON messages:

```json
// Move to absolute position
{"command": "move", "position": 1000, "speed": 50}

// Continuous jogging (hold to move)
{"command": "jogStart", "direction": "forward"}  // or "backward"
{"command": "jogStop"}

// Stop motor gently (without emergency flag)
{"command": "stop"}

// Emergency stop (requires reset to resume)
{"command": "emergency-stop"}

// Clear emergency stop
{"command": "reset"}

// Request status update
{"command": "status"}

// Get current configuration
{"command": "getConfig"}

// Update configuration (auto-saved to NVRAM)
{
  "command": "setConfig",
  "maxSpeed": 14400,
  "acceleration": 80000,
  "useStealthChop": true
}
```

**Backward Compatibility:** Legacy `"cmd": "goto"` format still supported for older clients.

### REST API (Read-Only)

**For monitoring and debugging only. Use WebSocket for all control operations.**

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/status` | Get current motor status, position, and limit switch states |
| GET | `/api/config` | Get motor configuration (speed, acceleration, limits, mode) |

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
cd lilygo-motion-controller

# Build firmware
pio run -e pico32

# Flash to device
pio run -e pico32 -t upload

# Monitor serial output
pio device monitor --baud 115200
```

### Memory Usage

- **RAM**: ~16% (52KB)
- **Flash**: ~83% (1.09MB)

### Unit Testing

Run cross-platform unit tests for motor calculation functions:

```bash
# Run unit tests on local development machine
pio test -e native

# Expected output: 20 test cases passed
# Tests calculateSpeed() and updateTMCMode() logic
```

## Configuration

### First-Time Setup

1. **Flash firmware** to ESP32
2. **Connect to WiFi portal**: Device creates "LilyGo-MotionController" access point
3. **Configure WiFi**: Connect via captive portal at 192.168.4.1
4. **Access web interface**: Navigate to device IP address or `http://lilygo-motioncontroller.local/`

### Motor Configuration

Default settings can be modified via WebSocket or web interface Settings dialog:

- **Max Speed**: 14,400 steps/second (default) - Range: 100-100,000 steps/sec
- **Acceleration**: 80,000 steps/second² (default) - Range: 100-500,000 steps/sec²
- **Microsteps**: 16 (1/16th stepping)
- **RMS Current**: 2000mA
- **StealthChop Mode**: Enabled by default (quieter operation, less torque)
- **StealthChop Threshold**: Automatic switching at 50% of max speed

**Motor-Specific Tuning:** Validation ranges accommodate various motors (e.g., Sanyo Denki 103-547-52500, NEMA 17). Exceeding your motor's capability may cause skipped steps but won't damage hardware. Consult your motor datasheet for optimal settings.

### Limit Switch Learning

1. **Manual positioning**: Use buttons or web interface to move motor
2. **Trigger limits**: Move to both extreme positions
3. **Automatic saving**: Positions are automatically learned and saved to NVRAM

## Safety Features

- **Emergency Stop**: Immediate motor halt via button, web interface, or WebSocket
- **Limit Switch Protection**: Automatic stop when limits are triggered
- **Watchdog Protection**: FreeRTOS task monitoring prevents system lockup
- **TMC2209 Thermal Protection**: Built-in driver overtemperature protection

## Debugging Features

### Debug WebSocket Stream

Connect to real-time serial output via WebSocket for remote debugging:

```javascript
// Open browser console (F12) and connect to debug stream
const debugWs = new WebSocket('ws://lilygo-motioncontroller.local/debug');
debugWs.onmessage = function(event) {
    console.log('Debug:', event.data);
};
```

**What you'll see:**
- Real-time log messages with timestamps: `[HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message`
- WebSocket command logging: All incoming commands from web interface
- Motor control events: Movement commands, limit switch triggers, emergency stops
- System status: WiFi connections, mDNS registration, module initialization

### Log Levels

The system uses structured logging with different levels:
- `LOG_ERROR`: Critical errors and failures
- `LOG_WARN`: Warnings like limit switch triggers or emergency stops
- `LOG_INFO`: General information like motor movements and connections
- `LOG_DEBUG`: Detailed debugging information (compile-time configurable)

### Network Access

- **Primary URL**: `http://lilygo-motioncontroller.local/` (mDNS)
- **Fallback**: `http://[device-ip-address]/` (if mDNS unavailable)
- **Debug Stream**: `ws://lilygo-motioncontroller.local/debug`
- **Control Stream**: `ws://lilygo-motioncontroller.local/ws`

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
   - Try mDNS first: `http://lilygo-motioncontroller.local/`
   - Verify device IP address via serial monitor or debug WebSocket
   - Check network allows mDNS traffic (some corporate networks block it)

5. **Debug WebSocket Not Working**
   - Ensure main web interface works first
   - Try direct IP instead of hostname: `ws://192.168.x.x/debug`
   - Check browser console for WebSocket connection errors
   - Verify no firewall blocking WebSocket connections

### Debug Information

Multiple ways to access debug information:
- **Serial Monitor**: `pio device monitor --baud 115200`
- **Debug WebSocket**: Connect via browser console to `/debug` endpoint
- **Log Levels**: Adjust LOG_LEVEL in build flags for more/less detail

**Available information:**
- Module initialization status with timestamps
- WiFi connection details and mDNS registration
- TMC2209 register values and mode switching
- Encoder position readings and speed calculations
- WebSocket connection events and command processing

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