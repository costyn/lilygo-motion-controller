# Expert Technical Answers

**Date:** 2025-09-21 14:55
**Phase:** 4 - Expert Requirements (Complete)

## Q6: Should the WebSocket server run on the same FreeRTOS task as the current Task2, or create a dedicated Task3?
**Answer:** Replace Task2 entirely with proper WebServer task using WiFiManager
**Impact:** Current Task2 is just example code that can be removed/adapted. New WebServer task will handle WiFiManager + AsyncWebServer + WebSocket + OTA on dedicated core.

## Q7: Should we extend the current button handling in Task1 to include limit switch monitoring (IO21/IO22)?
**Answer:** Yes - extend Task1 (defer to expertise for FreeRTOS architecture)
**Impact:** Limit switch monitoring added to existing Task1's 100ms input polling loop alongside button.tick() and encoder monitoring.

## Q8: Should configuration data be stored in SPIFFS or ESP32 Preferences/NVRAM?
**Answer:** ESP32 Preferences for config + SPIFFS for webapp files
**Impact:**
- ESP32 Preferences: motor settings, limit positions, WiFi credentials
- SPIFFS: serve built webapp files (HTML/CSS/JS) from device
- User has working example code for SPIFFS webapp serving

## Q9: Should the debug WebSocket endpoint be on separate port or same server with different path?
**Answer:** Same server, different path (/debug-ws)
**Impact:** Single AsyncWebServer instance handles:
- Main WebSocket: `/ws`
- Debug WebSocket: `/debug-ws`
- Static files: `/` (webapp from SPIFFS)
- OTA updates: `/update`

## Q10: Should TMC2209 configuration allow runtime switching between StealthChop and SpreadCycle?
**Answer:** Yes - but automatic optimization, not user-controlled
**Impact:** Controller automatically chooses:
- **StealthChop:** Slow, precise positioning (quiet for lamps)
- **SpreadCycle:** Fast movements or high torque needs
- Decision based on movement speed/acceleration automatically
- No user-facing controls - transparent optimization

---

**Phase 4 Complete**. All expert questions answered.
**Next:** Phase 5 - Comprehensive Requirements Specification