# LilyGo Motion Controller - TODO List

## 🔴 Missing Features (from original requirements, and those added afterwards)

### Bugs & New Feature Wishes
- [ ] Config feature to add: freewheel after movement or not. Partially implemented now by manipulating digitalWrite(EN_PIN, HIGH); in various places, but it now only works after jogging and e-stop. Does not work after slider or quick positions
- [ ] Bug: when not in freewheel mode after movement, the motor slightly buzzes and gets warm, also the tmc controller gets warm.
- [ ] Bug: I never see the automatic switch between stealthchop and spreadcycle happen in the logs

### High Priority - Should Have
- [✅] **mDNS Support** - Access device via `lilygo-motioncontroller.local` ✅ **COMPLETED**
- [✅] **Debug Serial WebSocket Stream** - Separate WebSocket path (`/debug`) for serial output streaming when client is connected ✅ **COMPLETED**
- [✅] **Write incoming websocket commands to serial** - Even though in production there will be no serial attached, for development purposes it's useful to have incoming websocket commands be written to serial output. ✅ **COMPLETED**
- [✅] **Unified output with timestamps to Serial** - All serial output should be written out in similar format, with a timestamp, then the name of the function that's doing the output. Now using LOG_ERROR/WARN/INFO/DEBUG macros with format: `[HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message` ✅ **COMPLETED**
- [✅] **Unit Tests** - Test modules: priority is MotorController calculation functions like calculateSpeed() and updateTMCMode() ✅ **COMPLETED**

### Tech Debt
- 🔴 Abstraction layer: datamodel between webcontoller and motorcontroller. Web controller is doing too much calculations and knows too much about motorcontroller [not doing because not needed]
- ✅ Too much hardware button logic in main.ccp. Needs to be moved to a separate component.
- ✅ Check for duplicate or very similar code. Keep it DRY! :) 
  - onSwitch1Pressed and onSwitch2Pressed are very similar. 
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


## 📋 Implementation Notes

### Immediate Next Steps (when hardware available)
1. **mDNS Setup** - Easy local network access
2. **Hardware Testing** - Verify all modules work correctly
3. **Debug Serial Stream** - High priority for debugging without serial access
4. **Physical Button Implementation** - Complete the button control logic

### Architecture Decisions Needed
- **Movement Playlist Storage** - SPIFFS vs NVRAM vs external storage
- **Real-time Requirements** - Current timing analysis and optimization

## 🎯 Version Planning

### v1.1 - Basic Completeness
- Debug serial WebSocket stream
- Complete button controls
- mDNS support
- Hardware testing validation

### v1.2 - Enhanced Control
- Movement playlists
- Advanced TMC2209 optimization
- Performance profiling and optimization

---

**Last Updated**: [Current Date]
**Status**: Core functionality complete, extensions in planning