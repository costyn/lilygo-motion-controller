# Context Findings

**Timestamp:** 2025-09-23 16:30

## LilyGo Motion Controller WebSocket Analysis

### WebSocket Endpoints
- **Main Control WebSocket**: `/ws` - handles motor commands and status
- **Debug WebSocket**: `/debug` - real-time serial output stream

### WebSocket Command Protocol
The controller supports both new and legacy command formats:
```json
// New format (preferred)
{"command": "move", "position": 1000, "speed": 50}
{"command": "stop"}
{"command": "reset"}
{"command": "status"}
{"command": "getConfig"}
{"command": "setConfig", "maxSpeed": 8000, "acceleration": 16000, "useStealthChop": true}

// Legacy format (webapp compatibility)
{"cmd": "goto", "position": 1000, "speed": 50}
```

### WebSocket Response Types
```json
// Status broadcast
{"type": "status", "position": 1000, "isMoving": false, "emergencyStop": false, "limitSwitches": {"min": false, "max": false, "any": false}}

// Position updates
{"type": "position", "position": 1500}

// Configuration responses
{"type": "config", "maxSpeed": 8000, "acceleration": 16000, "minLimit": -5000, "maxLimit": 5000, "useStealthChop": true}
{"type": "configUpdated", "status": "success"}
```

## Motor Controller Capabilities

### Available Commands
- `moveTo(position, speed)` - Move to absolute position
- `emergencyStop()` - Immediate halt
- `clearEmergencyStop()` - Resume operation
- `getCurrentPosition()` - Get current encoder position
- `isMoving()` - Check if motor is currently moving
- `isEmergencyStopActive()` - Check emergency stop state

### Limit Switch Integration
- Two limit switches: min (GPIO21) and max (GPIO22)
- Auto-position learning and persistence
- Safety lockouts when triggered
- Available status methods: `isMinTriggered()`, `isMaxTriggered()`, `isAnyTriggered()`

### Configuration Parameters
- `maxSpeed`: Maximum motor speed (steps/second)
- `acceleration`: Motor acceleration (steps/second²)
- `minLimit` / `maxLimit`: Learned limit positions
- `useStealthChop`: TMC2209 quiet mode toggle

## Existing Infrastructure

### Web Server Features
- **mDNS**: Device accessible via `http://lilygo-motioncontroller.local/`
- **SPIFFS**: Static file serving from `/data` directory
- **ElegantOTA**: Firmware updates at `/update` endpoint
- **WiFiManager**: Automatic WiFi setup without hardcoded credentials

### Current Web Interface
- Basic `index.html` served from SPIFFS
- Status display cards
- Basic control interface

## LumiApp Reference Architecture

### Technology Stack
- pnpm package manager
- Vite build system
- React + TypeScript
- Tailwind CSS + shadcn/ui

### Build Integration
```js
// From LumiApp vite.config.js - closeBundle plugin
closeBundle() {
    execSync('rm -rf /target/data/path/*');
    execSync('cp dist/*.* dist/*.gz /target/data/path/');
}
```

### Key Patterns
- Modular component structure
- Theme provider integration
- Auto-reconnect WebSocket handling
- Local storage for user preferences

## Technical Requirements Identified

### Phase 1 Features
1. **Jog Controls** - Forward/backward buttons with press-and-hold
2. **Limit Movement** - Buttons to move to learned min/max positions
3. **Auto-reconnect** - WebSocket reconnection on disconnect
4. **Theme Provider** - Dark/light mode with persistence
5. **OTA Link** - Button/link to `/update` endpoint

### Phase 2 Features
1. **Debug Console** - Real-time debug WebSocket display
2. **Preset Positions** - Browser localStorage initially
3. **Playlist Editor** - Multi-step movement sequences

### Build System
- Vite build targeting `/data` directory for SPIFFS upload
- Compatible with `pio run --target uploadfs`
- Gzip compression for efficient storage

## File Structure Analysis

### Controller Source Files
- `src/modules/WebServer/WebServer.cpp:269` - WebSocket message handler
- `src/modules/WebServer/WebServer.cpp:474` - Debug broadcast function
- `src/modules/MotorController/MotorController.h` - Motor control interface
- `src/modules/Configuration/Configuration.h` - Persistent settings

### Missing Implementation Areas
- No current playlist/sequence support in controller
- No preset position storage in controller NVRAM
- Current webapp is basic HTML (needs React replacement)

## Integration Points
- WebSocket endpoints are production-ready
- Controller supports real-time status broadcasts
- Debug WebSocket provides development visibility
- Configuration persistence works via NVRAM
- SPIFFS filesystem ready for React build output