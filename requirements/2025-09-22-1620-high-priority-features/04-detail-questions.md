# Expert Detail Questions

## Q6: Should the debug WebSocket at `/debug` automatically connect all web clients, or require explicit connection by developers?
**Default if unknown:** Require explicit connection (prevents accidental performance impact on production use)

## Q7: For the unified serial format, should we create logging macros (like LOG_INFO, LOG_ERROR) or standardize the existing printf pattern?
**Default if unknown:** Standardize existing printf pattern (maintains consistency with current codebase and user's example)

## Q8: Should mDNS use hostname "lilygo-motioncontroller" to match the existing WiFiManager AP name "LilyGo-MotionController"?
**Default if unknown:** Yes (consistent branding and easier user recognition)

## Q9: Should unit tests for MotorController calculation functions run in both native and ESP32 environments, or only native?
**Default if unknown:** Only native (ESP32 tests would be integration tests, calculation functions are platform-independent)

## Q10: Should the debug WebSocket buffer serial output when no clients are connected, or only stream when clients are active?
**Default if unknown:** Only stream when clients are active (prevents unnecessary memory usage and performance impact)