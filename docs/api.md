# API Reference

**WebSocket is the control interface.** Connect to `/ws` and send JSON. The REST API is read-only.

## WebSocket commands

```json
// Move to an absolute position, in steps
{"command": "move", "position": 1000, "speed": 1800}

// Continuous jogging (hold to move)
{"command": "jogStart", "direction": "forward", "speed": 1800}  // or "backward"
{"command": "jogStop"}

// Share the speed slider with other browsers. Moves nothing
{"command": "setSpeed", "speed": 1800}

// Cycle min -> max -> min, 1-10 times. Any move, jog or stop ends it
{"command": "cycle", "count": 5, "speed": 1800}

// Emergency stop (latches until reset) and clear it
{"command": "emergencyStop"}
{"command": "reset"}

// Calibration
{"command": "calibrate"}      // Find MIN switch (position 0), then MAX
{"command": "setMinHere"}     // Manual limits, for rigs without switches
{"command": "setMaxHere"}

// Status and configuration
{"command": "status"}
{"command": "getConfig"}
{"command": "setConfig", "maxSpeed": 14400, "acceleration": 15000,
 "motorCurrent": 800, "encoderFitted": true, "freewheelAfterMove": false,
 "calibrationSpeed": 500, "calibrationDirectDrive": true}   // saved to NVRAM
```

Speed is 100-50,000 steps/sec and acceleration 100-200,000 steps/sec². Out-of-range values are clamped, with a warning in the log.

Legacy `{"cmd": "goto", "position": N, "speed": N}` is still accepted for older clients. There is no generic `stop` command: use `jogStop` for a gentle stop and `emergencyStop` for the latched one.

## Patterns

Presets and patterns are one JSON program, saved on the controller and edited in the webapp under **Patterns > Edit**. Patterns run on the controller. Any other move, jog or stop ends one, but a limit switch at the end the move is heading for doesn't.

```json
{
  "presets": [0, 10, 25, 45, 75, 85, 90, 100],
  "patterns": [
    {"name": "Breath", "repeat": 0, "segments": [
      {"to": 100, "speed": 20, "ramp": 0.2, "hold": 2},
      {"to": 35, "seconds": 8, "ramp": 0.2, "hold": 4}
    ]}
  ]
}
```

- **presets**: quick positions, in % of travel. Up to 12.
- **repeat**: rounds to run, or 0 for until stopped. Up to 8 patterns with up to 8 segments each.
- **to**: target in % of travel. 0 is Min, 100 is Max.
- **seconds** or **speed**: how long the move takes, or its top speed as a % of Max Speed. Speed is capped at Max Speed, so a move asked to go faster takes longer.
- **ramp**: share of the distance spent speeding up, and again slowing down, 0.05 to 0.5. Default 0.25.
- **hold**: seconds to wait at the target. At least 0.5 s, so the encoder can check the move.

A pattern starts by moving to where its last segment ends, so every round is complete.

```json
{"command": "runPattern", "index": 0}       // Index in the order listed
{"command": "getProgram"}                   // Answered with {"type": "program", "program": {...}}
{"command": "setProgram", "program": {...}} // null restores the defaults
```

## WebSocket responses

| `type` | Contents |
|--------|----------|
| `status` | position, isMoving, emergencyStop, limitSwitches, calibration state, encoder health, speed, targetPosition, running pattern |
| `position` | position only, during moves |
| `config` | the settings from `getConfig` |
| `configUpdated` | `status` of a `setConfig` |
| `program` | presets and patterns |
| `error` | `message` for a refused command |

The exact fields are in `webapp/src/types/index.ts`.

## REST (read-only)

| Method | Endpoint | Description |
|--------|----------|-------------|
| GET | `/api/status` | Motor status, position, limit switch states |
| GET | `/api/config` | Motor configuration |

## Debug stream

`ws://lilygo-motioncontroller.local/debug` streams the serial log, one line per message, in the format `[HH:MM:SS.mmm] [LEVEL] [FUNCTION]: message`. The webapp shows it in the Debug Console panel. From a browser console:

```javascript
const ws = new WebSocket('ws://lilygo-motioncontroller.local/debug');
ws.onmessage = e => console.log(e.data);
```

Levels are `LOG_ERROR`, `LOG_WARN`, `LOG_INFO` and `LOG_DEBUG`. Every incoming WebSocket command is logged.
