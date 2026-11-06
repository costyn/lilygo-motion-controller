# Software Architecture

```
src/
├── main.cpp                       # Setup and FreeRTOS tasks
├── util.cpp / util.h              # Logging macros, device name
└── modules/
    ├── Configuration/             # ESP32 Preferences: motor settings, limits, program
    ├── MotorController/           # TMC2209 + AccelStepper, factory-accurate init
    ├── ClosedLoopController/      # MT6816 encoder tracking, stall stop, lost-step correction
    ├── LimitSwitch/               # Interrupt-driven switches, one instance per switch
    ├── PatternRunner/             # Presets and patterns, trapezoid planning (PatternPlan.h)
    ├── ButtonController/          # Optional on-board debug buttons
    └── WebServer/                 # WiFiManager, WebSocket, read-only REST API, OTA, mDNS
webapp/                            # React + TypeScript UI, built into SPIFFS
```

## Tasks

| Task | Core | Rate | Job |
|------|------|------|-----|
| `loop()` | 1 | continuous | `motorController.update()`: step pulses. Nothing else runs here |
| ClosedLoopTask | 0 | 5 ms | Reads the encoder, supervises moves |
| InputTask | 0 | 100 ms | Buttons, limit switch events, `PatternRunner` |
| WebServerTask | 0 | 50 ms | WebSocket cleanup, ElegantOTA |

Core 1 is reserved for `loop()` because any pause there stops the step pulses. AsyncTCP runs on core 0 too (`CONFIG_ASYNC_TCP_RUNNING_CORE=0`). Don't log or use the UART from core 1 during a move.

## Modules

- **Configuration**: persists motor settings, limits and the pattern program in NVRAM.
- **MotorController**: TMC2209 setup matching the manufacturer's example. Picks the chopper mode per move (StealthChop up to 10,000 steps/sec, SpreadCycle above) and never changes it mid-move. Validates and clamps speed and acceleration.
- **ClosedLoopController**: reads the encoder at 200 Hz. It stops on a stall, corrects lost steps at the end of a move, and triggers an emergency stop after three failed corrections in a row. It never steers a running move. Only this task uses the TMC2209 UART after startup.
- **LimitSwitch**: the interrupt only sets a flag. `update()` in InputTask does the logging, position correction and broadcast. Triggers call `emergencyStop()`, so checking the emergency flag is enough to know whether the motor may move.
- **PatternRunner**: runs the stored program on the controller, so patterns carry on with no browser connected.
- **WebServer**: WiFiManager portal, `/ws` control socket, `/debug` log stream, `/api/*` status, ElegantOTA, mDNS. WiFiManager must be included before ESPAsyncWebServer.

## Position and calibration

Movement is refused after boot until calibrated, because the motor may have moved while off. MIN is position 0. A switch press corrects the position rather than rewriting the limits, which also repairs lost steps. For rigs without switches, **Set Min Here** and **Set Max Here** set the limits by hand.
