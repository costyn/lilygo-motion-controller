# Initial Request

**Date:** 2025-09-21 14:55
**Session ID:** 2025-09-21-1455-esp-stepper-server

## User Request
Build a comprehensive ESP32-based wireless stepper controller system for LilyGo T-Motor board with TMC2209 driver and MT6816 encoder.

## Project Summary
- **Hardware**: LilyGo T-Motor with ESP32 Pico, TMC2209 stepper driver, MT6816 magnetic encoder
- **Primary Use Case**: Lamp with moving arms controlled via threaded rod mechanism
- **Control Method**: WebSocket-based communication with separate mobile webapp/remote
- **Key Focus**: Smooth, quiet operation with self-protection mechanisms

## MoSCoW Priorities from User
**Must Haves:**
- Follow manufacturer example implementation (main.cpp)
- Real-time stepper control using FreeRTOS
- Modular architecture for code reuse
- State synchronization with webapp
- ElegantOTA wireless firmware updates
- WebSocket server (ESPAsyncWebServer)
- Separate debug WebSocket endpoint

**Should Haves:**
- Smooth acceleration/deceleration
- TMC2209 configuration (SpreadCycle/StealthChop)
- Configuration persistence (SPIFFS/NVRAM)
- WiFiManager integration

**Could Haves:**
- Physical button controls with limit switches
- Custom movement playlists

## Hardware Details
- **Stepper**: 17HS19-2004S1 (1.8°, 59Ncm, 2.0A/phase)
- **Encoder**: MT6816 (16384 pulses/rotation)
- **Limit Switches**: IO21 & IO22 (pulled low when triggered)
- **Buttons**: 3 onboard for debugging

## Context
- Based on existing main.cpp with manufacturer example
- PlatformIO project structure
- Git shows active development on platformio.ini and main.cpp