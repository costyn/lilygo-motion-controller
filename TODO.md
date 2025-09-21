# LilyGo Motion Controller - TODO List

## 🔴 Missing Features (from original requirements)

### High Priority - Should Have
- [ ] **Debug Serial WebSocket Stream** - Separate WebSocket path (`/debug`) for serial output streaming when client is connected
- [ ] **Bluetooth Support** - Original requirement mentioned "wifi and bluetooth" control
- [ ] **Smooth Acceleration/Deceleration** - Currently using basic AccelStepper, may need custom curves
- [ ] **Dynamic TMC2209 Mode Switching** - Advanced SpreadCycle/StealthChop optimization based on load/speed

### Medium Priority - Could Have
- [ ] **Physical Button Controls**
  - Button 1: Move clockwise until limit switch hit
  - Button 2: Emergency stop (✅ partially implemented)
  - Button 3: Move counterclockwise until limit switch hit
- [ ] **Custom Movement Playlists** - Predetermined movement sequences/loops
- [ ] **Playlist WebSocket Control** - Send movement sequences from webapp

### Low Priority - Nice to Have
- [ ] **mDNS Support** - Access device via `lilygo-motioncontroller.local`
- [ ] **Position Profiles** - Save/recall common positions
- [ ] **Advanced Speed Ramping** - More sophisticated acceleration profiles
- [ ] **Multi-motor Support** - Architecture ready, needs implementation
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
- [ ] **Authentication** - Basic auth for web interface
- [ ] **HTTPS Support** - SSL/TLS for secure communication
- [ ] **Rate Limiting** - Prevent API abuse
- [ ] **Input Validation** - Robust parameter checking

### Reliability
- [ ] **Watchdog Timer** - Hardware reset on system hang
- [ ] **Error Recovery** - Graceful handling of communication failures
- [ ] **Configuration Backup** - Automatic config backup/restore
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
- [ ] **Additional Encoders** - Support for external encoders

### I/O Expansion
- [ ] **GPIO Expander Support** - More inputs/outputs via I2C
- [ ] **Analog Input Monitoring** - Monitor external sensors
- [ ] **PWM Output Control** - Control external devices
- [ ] **CAN Bus Support** - Industrial communication protocol

## 🟣 Future Platform Support

### Communication
- [ ] **Modbus Support** - Industrial automation protocol
- [ ] **MQTT Integration** - IoT message broker support
- [ ] **LoRaWAN Support** - Long-range wireless communication
- [ ] **Ethernet Support** - Wired network option

### Compatibility
- [ ] **Different Stepper Drivers** - Support TMC5160, DRV8825, etc.
- [ ] **Multiple ESP32 Variants** - ESP32-S2, ESP32-S3, ESP32-C3
- [ ] **Arduino Framework Updates** - Keep up with ESP-IDF changes
- [ ] **Platform.IO Updates** - Maintain compatibility

## 📋 Implementation Notes

### Immediate Next Steps (when hardware available)
1. **Hardware Testing** - Verify all modules work correctly
2. **Debug Serial Stream** - High priority for debugging without serial access
3. **Physical Button Implementation** - Complete the button control logic
4. **mDNS Setup** - Easy local network access

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