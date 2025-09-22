# LilyGo Motion Controller - TODO List

## 🔴 Missing Features (from original requirements, and those added afterwards)

### High Priority - Should Have
- [ ] **mDNS Support** - Access device via `lilygo-motioncontroller.local`
- [ ] **Debug Serial WebSocket Stream** - Separate WebSocket path (`/debug`) for serial output streaming when client is connected
- [ ] **Write incoming websocket commands to serial** - Even though in production there will be no serial attached, for development purposes it's useful to have incoming websocket commands be written to serial output.
- [ ] **Unified output with timestamps to Serial** - All serial output should be written out in similar format, with a timestamp, then the name of the function that's doing the output. For example: 
    Serial.printf("%s: %s: Webserver started. URL http://%s/\n", timeToString().c_str(), SGN, WiFi.localIP());
    This example is from the WebServer.cpp.
- [ ] **Unit Tests** - Test modules: priority is MotorController calculationfunctions like calculateSpeed() and updateTMCMode()

### Medium Priority - Could Have
- [X] **Dynamic TMC2209 Mode Switching** - Advanced SpreadCycle/StealthChop optimization based on load/speed: Already in updateTMCMode()?
- [ ] **Smooth Acceleration/Deceleration** - Currently using basic AccelStepper, may need custom curves (is the same as easing?)
- [ ] **Physical Button Controls**
  - Button 1: Move clockwise until limit switch hit
  - Button 2: Emergency stop (✅ partially implemented)
  - Button 3: Move counterclockwise until limit switch hit
- [ ] **Standalone mode auto redirect** - When the wifimanager isn't used and times out, the project should create it's own AP without a password. If the user then connects to the AP, would be cool if they were automatically directed to the webapp. 
- [ ] **Custom Movement Playlists** - Predetermined movement sequences/loops
- [ ] **Playlist WebSocket Control** - Send movement sequences from webapp

### Low Priority - Nice to Have
- [ ] **Bluetooth Support** - Original requirement mentioned "wifi and bluetooth" control
- [ ] **Position Profiles** - Save/recall common positions
- [ ] **Advanced Speed Ramping** - More sophisticated acceleration profiles
- [ ] **OLED Display Support** - Basic status display (hardware supported)

## 🟡 Infrastructure Improvements

### Code Quality
- [ ] **Unit Tests** - Test individual modules
- [ ] **Integration Tests** - Test WebSocket/REST API
- [ ] **Hardware-in-Loop Testing** - Automated hardware testing setup
- [ ] **Performance Profiling** - Memory usage and timing analysis

### Documentation
- [ ] **API Documentation** - OpenAPI/Swagger spec
- [ ] **Web UI Examples** - Sample HTML/JS client implementations
- [ ] **Hardware Setup Guide** - Wiring diagrams and assembly instructions
- [ ] **Troubleshooting Database** - Common issues and solutions

### Development Experience
- [ ] **VSCode Tasks** - Build/upload/monitor shortcuts
- [ ] **GitHub Actions** - CI/CD pipeline
- [ ] **Pre-commit Hooks** - Code formatting and basic checks
- [ ] **Development Docker Container** - Consistent dev environment

## 🟢 Architecture Enhancements

### Security
- [ ] **Rate Limiting** - Prevent API abuse
- [ ] **Input Validation** - Robust parameter checking

### Reliability
- [ ] **Watchdog Timer** - Hardware reset on system hang
- [ ] **Error Recovery** - Graceful handling of communication failures
- [ ] **Firmware Rollback** - Safe OTA update with rollback capability

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
- [ ] **Ethernet Support** - Wired network option
- [ ] **ArtNet Support** - Control through ArtNet/DMX
- [ ] **WLED Support** - Control from WLED through API calls

### Compatibility
- [ ] **Different Stepper Drivers** - Support TMC5160, DRV8825, etc.
- [ ] **Multiple ESP32 Variants** - ESP32-S2, ESP32-S3, ESP32-C3
- [ ] **Arduino Framework Updates** - Keep up with ESP-IDF changes
- [ ] **Platform.IO Updates** - Maintain compatibility

## 📋 Implementation Notes

### Immediate Next Steps (when hardware available)
1. **mDNS Setup** - Easy local network access
2. **Hardware Testing** - Verify all modules work correctly
3. **Debug Serial Stream** - High priority for debugging without serial access
4. **Physical Button Implementation** - Complete the button control logic

### Architecture Decisions Needed
- **Movement Playlist Storage** - SPIFFS vs NVRAM vs external storage
- **Bluetooth Implementation** - Classic vs BLE vs both
- **Multi-motor Architecture** - Single controller vs distributed
- **Real-time Requirements** - Current timing analysis and optimization

### Dependencies to Monitor
- **ESP32Async/ESPAsyncWebServer** - Watch for updates/changes
- **ArduinoJson v7+** - API changes and performance improvements
- **TMCStepper Library** - New features and bug fixes
- **AccelStepper** - Alternative libraries for smoother motion

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

### v2.0 - Platform Extensions
- Bluetooth support
- Multi-motor capability
- Security features
- Comprehensive testing suite

---

**Last Updated**: [Current Date]
**Status**: Core functionality complete, extensions in planning