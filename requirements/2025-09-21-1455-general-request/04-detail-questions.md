# Expert Technical Questions

**Date:** 2025-09-21 14:55
**Phase:** 4 - Expert Requirements (Technical Detail)

Based on deep codebase analysis and library research, these questions clarify specific implementation details:

## Q6: Should the WebSocket server run on the same FreeRTOS task as the current Task2, or create a dedicated Task3?
**Default if unknown:** Dedicated Task3 (better separation and memory management)
**Why this default:** Current Task2 deletes itself after WiFi connection. A persistent WebServer task on Core 0 following AsyncWebServer best practices would be more stable.

## Q7: Should we extend the current button handling in Task1 to include limit switch monitoring (IO21/IO22)?
**Default if unknown:** Yes (consistent with existing OneButton pattern)
**Why this default:** Task1 already handles button.tick() in a 100ms loop with encoder monitoring - adding limit switches to the same task maintains the current architecture.

## Q8: Should configuration data (WiFi credentials, motor settings, limit positions) be stored in SPIFFS or ESP32 Preferences/NVRAM?
**Default if unknown:** ESP32 Preferences (simpler, more reliable for small config data)
**Why this default:** SPIFFS requires filesystem setup, while Preferences namespace is built-in and perfect for key-value config storage like limit positions and motor parameters.

## Q9: Should the debug WebSocket endpoint be on a separate port (e.g., :81/debug) or same server with different path (/debug-ws)?
**Default if unknown:** Same server, different path (/debug-ws)
**Why this default:** Single AsyncWebServer instance is more memory-efficient and simpler to manage than multiple servers, especially with OTA integration.

## Q10: Should we modify the existing TMC2209 configuration (lines 274-275) to allow runtime switching between StealthChop and SpreadCycle via WebSocket?
**Default if unknown:** Yes (enable both modes with WebSocket control)
**Why this default:** Your requirements mention "anything for smooth operation" - runtime switching would allow optimization for different movement patterns (StealthChop for quiet positioning, SpreadCycle for high-speed moves).

---

**Next Steps:**
- Answer each question to finalize technical implementation details
- All answers will inform the comprehensive requirements specification
- Questions focus on architectural decisions that affect the entire system