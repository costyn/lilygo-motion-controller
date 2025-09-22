# Initial Request

**Date**: 2025-09-22 16:20
**User Request**: Please check the TODO.md and plan on starting the High Priority features. A previous requirements session can be found in the /requirements directory. Also see CLAUDE.md and README.md for more details.

## High Priority Features Identified from TODO.md

### Should Have Features:
1. **mDNS Support** - Access device via `lilygo-motioncontroller.local`
2. **Debug Serial WebSocket Stream** - Separate WebSocket path (`/debug`) for serial output streaming when client is connected
3. **Write incoming websocket commands to serial** - Development debugging capability
4. **Unified output with timestamps to Serial** - All serial output with consistent format including timestamps and function names
5. **Unit Tests** - Focus on MotorController calculation functions like calculateSpeed() and updateTMCMode()

## Context from CLAUDE.md
- Project is currently COMPLETE ✅ with all core functionality implemented
- Modular architecture with Configuration, MotorController, LimitSwitch, and WebServer modules
- Factory-accurate hardware initialization for TMC2209 and MT6816
- WiFiManager + WebSocket + REST API fully functional
- Ready for hardware deployment
- Uses ESP32Async/ESPAsyncWebServer (maintained version)

## Current Status
- Core functionality implemented and compiling successfully
- Hardware testing pending (hardware at friend's location)
- Architecture supports extension for new features
- Memory usage: RAM 14.9%, Flash 80.6%