# Expert Detail Answers

## Q6: Should the debug WebSocket at `/debug` automatically connect all web clients, or require explicit connection by developers?
**Answer:** Automatic connection for any client, but webapp only connects after pressing a debug button (controlled at UI level, not connection level)

## Q7: For the unified serial format, should we create logging macros (like LOG_INFO, LOG_ERROR) or standardize the existing printf pattern?
**Answer:** Create logging macros with log levels (LOG_INFO, LOG_ERROR, etc.) for better organization

## Q8: Should mDNS use hostname "lilygo-motioncontroller" to match the existing WiFiManager AP name "LilyGo-MotionController"?
**Answer:** Yes, and put the name in a #define for easy customization in other projects

## Q9: Should unit tests for MotorController calculation functions run in both native and ESP32 environments, or only native?
**Answer:** Only native (local dev machine) for testing calculation function outputs

## Q10: Should the debug WebSocket buffer serial output when no clients are connected, or only stream when clients are active?
**Answer:** Prefer circular buffer (~50-100 lines) sent on client connect + live streaming, but fallback to simple streaming if too complex