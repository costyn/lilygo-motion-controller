# Troubleshooting

## Web interface unreachable
- Try `http://lilygo-motioncontroller.local/`, then the device IP (shown on the serial monitor and in the debug stream).
- Some networks block mDNS. Use the IP address instead.
- If WiFi never connected, the device opens the **LilyGo-MotionController** access point. Join it and configure WiFi at 192.168.4.1.

## Motor won't move
- Movement is refused until calibrated. Click **Calibrate now**, or use **Set Min Here** / **Set Max Here**.
- Check for an emergency stop in `/api/status` and press **Reset**.
- Check limit switch states, TMC2209 wiring and power.

## Motor stalls or stops mid-move
- The encoder stops the motor after 500 steps of error. Lower Max Speed or Acceleration, or raise Motor Current (within your motor's rating).
- With no magnet on the rear shaft, turn off **Encoder Fitted**. Stalls then go unnoticed.

## Position drifts
- Check the MT6816 connection, magnet alignment and microstep settings.
- Hitting a limit switch corrects the position.

## Debug stream not working
- Check the main interface works first.
- Use the IP address instead of the hostname: `ws://192.168.x.x/debug`.
- Check the browser console for WebSocket errors.

## Getting more information
- Serial monitor: `pio device monitor --baud 115200`
- Debug WebSocket: see [API Reference](api.md#debug-stream)
- Log detail is set by `-DLOG_LEVEL=...` in the build flags (default: debug, see `src/util.h`).
