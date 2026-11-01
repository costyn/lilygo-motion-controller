# LilyGo Motion Controller - TODO List

## 🔴 Missing Features (from original requirements, and those added afterwards)

### Bugs & New Feature Wishes v1.5
- [✅] Magnetic Encoder feedback ✅ **COMPLETED** - Stall stop, end-of-move correction, hand tracking
- [🔴] ~Optional SimpleFOC https://simplefoc.com/ integration~ SimpleFOC does not work with the TMC2209
- [ ] Failing to set up the web portal should not be a fatal error. Onboard buttons still work.
  - [ ] On wifi fail, do not wait for portal/put the wifi config portal on a separate thread
- [✅] Calibration/auto-learn limits ✅ **COMPLETED** - Limit switch calibration, or Set Min/Max by hand
- [ ] Encoder homing for rigs with hard stops: creep until the encoder stops following

[ 30377][E][WebServer.cpp:638] _handleRequest(): request handler not found
[ 30442][E][WebServer.cpp:638] _handleRequest(): request handler not found
*wm:15 networks found
[ 35027][E][WebServer.cpp:638] _handleRequest(): request handler not found
*wm:config portal has timed out 
*wm:config portal exiting 
[00:01:06.097] [WARN] [begin]: Failed to connect to WiFi
[00:01:06.098] [ERROR] [setup]: FATAL: Failed to initialize Web Server

- [ ] What needs to be done to save multiple wifi passwords??


### Bugs & New Feature Wishes v1.4
- [✅] Config feature to add: freewheel after movement or not. ✅ **COMPLETED** - Works for all movement types (jog, slider, quick positions)
- [✅] Bug: when not in freewheel mode after movement, the motor slightly buzzes and gets warm. ✅ **FIXED** - Only call `stepper->run()` when `distanceToGo() != 0`
- [✅] Bug: I never see the automatic switch between stealthchop and spreadcycle happen in the logs ✅ **FIXED** - Use `abs(stepper->speed())` instead of uninitialized `motorSpeed`
- [✅] Bug: 200-500ms delay between hitting limit switch and motor stopping ✅ **FIXED** - Replaced OneButton polling with ESP32 hardware interrupts (microsecond response)

### High Priority - Should Have
- [✅] **mDNS Support** - Access device via `lilygo-motioncontroller.local` ✅ **COMPLETED**
- [✅] **Debug Serial WebSocket Stream** - Separate WebSocket path (`/debug`) for serial output streaming when client is connected ✅ **COMPLETED**
- [✅] **Write incoming websocket commands to serial** - Even though in production there will be no serial attached, for development purposes it's useful to have incoming websocket commands be written to serial output. ✅ **COMPLETED**
- [✅] **Unified output with timestamps to Serial** - All serial output should be written out in similar format, with a timestamp, then the name of the function that's doing the output. Now using LOG_ERROR/WARN/INFO/DEBUG macros with format: `[HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message` ✅ **COMPLETED**
- [✅] **Unit Tests** - Test modules: priority is MotorController calculation functions like calculateSpeed() and updateTMCMode() ✅ **COMPLETED**

### Tech Debt

Open items, most important first. Add new debt here as it appears.

**Reliability**
- [ ] Stepper access crosses cores without locking. `jogStop()`, `moveTo()`, `referencePosition()` and `emergencyStop()` run on Core 0 while `loop()` uses AccelStepper on Core 1. This already caused one motor-disabled race. Route every stepper change through requests handled in `update()`, as stall stops and corrections do.
- [ ] Calibration on a rig that can't reach a switch runs until `moveTo(±999999)` ends, about 11 minutes at 1,500 steps/sec, because the encoder follows normally. Add a travel limit or timeout.
- [ ] The calibration abort has never fired on the rig (motor too strong to hold). Test it by unplugging the MIN switch.
- [ ] One WebSocket client was dropped during fast moves and not investigated. It was seen before the web tasks moved to Core 0.
- [ ] Encoder parity is assumed to be even and only counted. Confirm it against the datasheet, then skip bad frames.

**Maintainability**
- [ ] Microsteps (16) are hardcoded in `begin()`, the TPWMTHRS calculation and `STEPS_PER_REV`. Use one constant.
- [ ] Speed and acceleration limits are duplicated in firmware and webapp. Send them with the config.
- [ ] Stall (500), lost-step (48) and StealthChop handover (10,000) values are tuned on the unloaded rig and hardcoded. Check them under load, and consider settings.
- [ ] Dead code: `calculateSpeed()`, `readEncoder()`, `monitorSpeed`, `motorSpeed`, `direction`, `needsLimitRecovery`, `limitRecoveryPosition`, `getTMCStatus()`, and `emergencyStopWithRecovery()`, which is declared but never defined.
- [ ] Clamp calibration speed once. It is repeated in `startCalibration()` and `onMinLimitReached()`.
- [ ] Wrong comments in `MotorController::begin()`: "1/8th" microsteps, "100mm/s" and "2000mm/s^2".
- [ ] `setConfig` stores speed, acceleration and limits unclamped, and typed limits can sit outside the switches. Clamping only happens when they are used.
- [ ] Leftover NVS key `stealthChop` on existing devices. Harmless.

**Testing and tooling**
- [ ] Native tests can't compile `MotorController` or `LimitSwitch`, so the calibration flow and ISR logic have no unit tests. `test_motor_calculations` tests a copy of the logic. Move more of it into pure classes like `EncoderTracker.h`.
- [ ] `platform = espressif32` is unpinned, so a platform update can break builds. Pin it, e.g. `espressif32@6.9.0`.
- [ ] CI cache keys still say `pioarduino`.
- [ ] `LOG_LEVEL` is DEBUG in all builds, so every WebSocket message is logged.

**Decided**
- 🔴 Abstraction layer: datamodel between webcontoller and motorcontroller. Web controller is doing too much calculations and knows too much about motorcontroller [not doing because not needed]
- ✅ Too much hardware button logic in main.ccp. Needs to be moved to a separate component.
- ✅ Check for duplicate or very similar code. Keep it DRY! :)
  - ✅ onSwitch1Pressed and onSwitch2Pressed were very similar - **REFACTORED** to single-switch LimitSwitch class with two instances
  - ✅ Redundant safety checks (`!minLimitSwitch.isTriggered() && !maxLimitSwitch.isTriggered() && !motorController.isEmergencyStopActive()`) simplified to just `!motorController.isEmergencyStopActive()`
- ✅ stopGently has no corresponding json command but is just called jogStop. May be a bit confusing.
- ✅ massive chains of if-else statements in handleWebSocketMessage(). Can this be made more elegant? Also code duplication of log messages and broadcastStatus(); littered around. 


### Medium Priority Features - Could Have
- [✅] **Dynamic TMC2209 Mode Switching** - Advanced SpreadCycle/StealthChop optimization based on load/speed: Already in updateTMCMode()?
- [✅] **Smooth Acceleration/Deceleration** - Currently using basic AccelStepper, may need custom curves (is the same as easing?)
- [✅] **Physical Button Controls**
  - Button 1: Jog clockwise until min
  - Button 2: Emergency stop (✅ partially implemented)
  - Button 3: Jog counterclockwise until max
- [ ] **Standalone mode auto redirect** - When the wifimanager isn't used and times out, the project should create it's own AP without a password. If the user then connects to the AP, would be cool if they were automatically directed to the webapp. 
- [ ] **Custom Movement Playlists** - Predetermined movement sequences/loops
- [ ] **Playlist WebSocket Control** - Send movement sequences from webapp

### Low Priority Features - Nice to Have
- [ ] **Position Profiles** - Save/recall common positions
- [ ] **Advanced Speed Ramping** - More sophisticated acceleration profiles

## 🟡 Infrastructure Improvements

### Code Quality
- [✅] **Unit Tests** - Test individual modules
- [✅] **Integration Tests** - Test WebSocket/REST API
- [🔴] **Hardware-in-Loop Testing** - Automated hardware testing setup [won't do]
- [🔴] **Performance Profiling** - Memory usage and timing analysis [won't do ]

### Documentation
- [🔴] **API Documentation** - OpenAPI/Swagger spec
- [✅] **Web UI Examples** - Sample HTML/JS client implementations
- [ ] **Hardware Setup Guide** - Wiring diagrams and assembly instructions
- [✅] **Troubleshooting Database** - Common issues and solutions

### Development Experience
- [✅] **VSCode Tasks** - Build/upload/monitor shortcuts
- [✅] **GitHub Actions** - CI/CD pipeline

## 🟢 Architecture Enhancements

### Security
- [ ] **Rate Limiting** - Prevent API abuse
- [✅] **Input Validation** - Robust parameter checking

### Reliability
- [ ] **Watchdog Timer** - Hardware reset on system hang
- [ ] **Error Recovery** - Graceful handling of communication failures

### Performance
- [ ] **Real-time Priority** - FreeRTOS task priority optimization
- [ ] **Memory Optimization** - Reduce RAM usage for larger projects
- [ ] **WebSocket Compression** - Reduce bandwidth usage
- [ ] **Cache Headers** - Optimize static file serving

## 🔵 Hardware Extensions

### Sensors
- [ ] **Current Monitoring** - TMC2209 current sensing
- [ ] **Temperature Monitoring** - Driver and motor temperature
- [ ] **Load Cell Integration** - Force/torque measurement

### I/O Expansion
- [ ] **GPIO Expander Support** - More inputs/outputs via I2C
- [ ] **Analog Input Monitoring** - Monitor external sensors

## 🟣 Future Platform Support

### Communication
- [ ] **ArtNet Support** - Control through ArtNet/DMX
- [ ] **WLED Support** - Control from WLED through API calls

### Architecture Decisions Needed
- **Movement Playlist Storage** - SPIFFS vs NVRAM vs external storage
- **Real-time Requirements** - Current timing analysis and optimization

---

**Last Updated**: November 1, 2026
**Status**: Core functionality complete with all major bugs fixed, extensions in planning