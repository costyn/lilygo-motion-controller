# LilyGo-MotionController Requirements Specification

**Date:** 2025-09-21 14:55
**Version:** 1.0
**Project ID:** lilygo-motion-controller

---

## 1. Problem Statement & Solution Overview

**Problem:** Create a wireless stepper motor controller system for lamp positioning that prioritizes smooth, quiet operation while preventing mechanical damage.

**Solution:** ESP32-based controller using LilyGo T-Motor hardware with WebSocket communication, automatic TMC2209 optimization, and intelligent limit detection with position learning.

---

## 2. Functional Requirements

### 2.1 Core Motion Control (Must Have)
- **Real-time stepper control** using FreeRTOS for smooth operation
- **Position-based commands** from webapp with encoder feedback
- **Smooth acceleration/deceleration** profiles for lamp movement
- **Emergency stop capability** via limit switches (IO21/IO22)
- **Smart limit learning**: Save limit positions to ESP32 Preferences, enable predictive deceleration
- **Manual position sync**: Read actual position from MT6816 encoder on startup (handles manual knob adjustments during power-off)

### 2.2 WebSocket Communication (Must Have)
- **Control WebSocket (`/ws`)**: Bidirectional command/response for motor control
- **Debug WebSocket (`/debug-ws`)**: Serial output + motor telemetry streaming
- **High-frequency position updates** for real-time webapp visualization
- **State synchronization**: Send current position, speed, direction, config on webapp connection

### 2.3 Web Interface (Must Have)
- **Built webapp served from device** via SPIFFS filesystem
- **Virtual representation** of lamp + movement with smooth animations
- **Real-time position/speed display** during movement
- **Mobile-responsive design** for phone/tablet control

### 2.4 WiFi & Network Management (Must Have)
- **WiFiManager integration**: Captive portal for network configuration (replaces hardcoded credentials)
- **Connection persistence**: Automatic reconnection on network drops
- **Device discovery**: Static IP or mDNS for easy webapp access

### 2.5 Firmware Management (Must Have)
- **ElegantOTA wireless updates** integrated with WebServer
- **Version tracking**: Display current firmware version in webapp
- **Update safety**: Prevent updates during motor movement

---

## 3. Technical Requirements

### 3.1 Hardware Integration
- **Platform**: LilyGo T-Motor + ESP32 Pico + TMC2209 + MT6816 encoder
- **Stepper Motor**: 17HS19-2004S1 (1.8°, 59Ncm, 2.0A/phase)
- **Encoder Resolution**: MT6816 16384 pulses/rotation
- **Limit Switches**: IO21 & IO22, pulled low when triggered
- **Physical Buttons**: 3 onboard buttons for debugging (not primary control)

### 3.2 FreeRTOS Task Architecture
```
Task1 (Core 1): Input monitoring (100ms loop)
├── Button handling (OneButton.tick())
├── Limit switch monitoring (digitalRead IO21/IO22)
├── Encoder position reading (MT6816_read())
└── Speed calculation (ReadSpeed())

Task2 (Core 0): WebServer & Network
├── WiFiManager initialization
├── AsyncWebServer (port 80)
├── WebSocket handlers (/ws, /debug-ws)
├── ElegantOTA integration
└── SPIFFS static file serving

Main Loop: Continuous stepper.runSpeed()
```

### 3.3 Data Storage Strategy
- **ESP32 Preferences**: Configuration persistence
  - Motor settings (acceleration, max speed)
  - Learned limit positions
  - WiFi credentials (via WiFiManager)
- **SPIFFS**: Static webapp files (HTML/CSS/JS)

### 3.4 TMC2209 Auto-Optimization
- **Automatic mode switching** based on movement characteristics:
  - **StealthChop**: Slow positioning (<50% max speed), quiet operation
  - **SpreadCycle**: Fast movements, high torque demands
- **Runtime configuration**: 2000mA RMS current, 16 microsteps
- **Diagnostic monitoring**: IOIN register status for error detection

---

## 4. Implementation Hints & Patterns

### 4.1 Library Dependencies (platformio.ini)
```ini
lib_deps =
    AccelStepper                    # Already installed
    TMCStepper                      # Already installed
    OneButton                       # Already installed
    https://github.com/ESP32Async/ESPAsyncWebServer.git
    AsyncTCP-esphome @ ^2.0.0
    ayushsharma82/ElegantOTA @ ^3.0.0
    https://github.com/tzapu/WiFiManager.git

board_build.filesystem = spiffs     # Enable SPIFFS
```

### 4.2 WebSocket Message Protocols

**Control WebSocket (`/ws`)**:
```json
// Position Command
{"cmd": "goto", "position": 1250, "speed": 80}

// Status Response
{"type": "status", "position": 1250, "speed": 0, "direction": 0, "limits": [0, 2500]}

// Configuration
{"cmd": "config", "acceleration": 1000, "maxSpeed": 14400}
```

**Debug WebSocket (`/debug-ws`)**:
```json
{"type": "telemetry", "position": 1250, "encoderRaw": 8192, "motorCurrent": 2000, "driverStatus": "0x12345678"}
{"type": "log", "level": "info", "message": "Limit switch 1 triggered"}
```

### 4.3 File Modifications Required

**Core Files to Modify**:
- `src/main.cpp`: Replace Task2, extend Task1, add WebSocket handlers
- `platformio.ini`: Add library dependencies, enable SPIFFS
- `data/` directory: Create for webapp static files

**Key Integration Points**:
- Line 233: Replace WiFi.begin() with WiFiManager.autoConnect()
- Line 145-146: Modify xTaskCreatePinnedToCore for new WebServer task
- Line 185-187: Extend button handling with limit switch digitalRead
- Line 274-275: Make driver.en_spreadCycle() dynamic based on speed

---

## 5. Acceptance Criteria

### 5.1 Functional Testing
- [ ] Webapp loads from device IP address without external hosting
- [ ] Motor moves smoothly to commanded positions with real-time feedback
- [ ] Limit switches immediately stop movement and save positions
- [ ] System remembers physical position after power cycle
- [ ] WiFi configuration works via captive portal (no hardcoded credentials)
- [ ] OTA updates succeed without corrupting configuration
- [ ] Debug stream shows motor telemetry and application logs

### 5.2 Performance Testing
- [ ] Position updates at minimum 10Hz via WebSocket
- [ ] Movement acceleration/deceleration prevents mechanical stress
- [ ] Automatic StealthChop/SpreadCycle switching based on speed
- [ ] System responds to emergency stops within 100ms
- [ ] WebSocket connections remain stable during continuous operation

### 5.3 Safety Testing
- [ ] Limit switches prevent overextension in both directions
- [ ] Motor stops immediately on limit trigger without overshoot
- [ ] System handles manual position changes during power-off
- [ ] No movement commands accepted during OTA updates

---

## 6. Assumptions & Dependencies

**Assumptions**:
- User has working SPIFFS webapp example code available
- LilyGo T-Motor board matches Pico32 configuration
- Manual knob adjustment during power-off is infrequent but possible
- Users operate within visual range but want virtual representation

**Dependencies**:
- Stable WiFi network for WebSocket communication
- Proper 12V power supply for stepper motor operation
- Correct mechanical assembly with limit switches at safe positions

---

## 7. Future Considerations (Out of Scope)

**Could Have (Later Phases)**:
- Custom movement playlists/sequences
- Physical button controls for manual operation
- Bluetooth communication as WiFi fallback
- Remote configuration of TMC2209 parameters

**Won't Have (Current Phase)**:
- OLED display integration
- Remote stepper parameter configuration (current, microstepping)
- Mobile app (separate project scope)

---

**Requirements Complete**. Ready for implementation planning and development.