# LilyGo Motion Controller

Wireless stepper motor control from your phone. Runs on the LilyGo T-Motor board (ESP32, TMC2209 driver, MT6816 encoder) with a touch-friendly web interface served by the board itself.

![Main Control Interface](docs/images/Main%20Control%20Interface.png)

## What it does

- **Web interface on the board**: jog, drag to a position, quick presets and live status. Reachable at `http://lilygo-motioncontroller.local/`
- **Patterns**: build movement sequences with speed, ramps and holds. They run on the controller, so they carry on with the browser closed
- **Encoder supervision**: stops on a stall and corrects lost steps at the end of a move
- **Calibration**: finds its limits with two micro switches, or set them by hand
- **Quiet or fast**: StealthChop for quiet moves, SpreadCycle for high speed, chosen per move
- **No hardcoded WiFi**: captive portal setup, then over-the-air updates
- **Live debug log** in the browser

## Quick start

1. **Get the hardware and wire it up**: see [Hardware Setup](docs/hardware.md).
2. **Flash**: connect a USB serial adapter (first flash only) and run:

   ```bash
   pio run -e pico32 -t upload     # firmware
   pio run -e pico32 -t uploadfs   # web interface (builds the webapp first)
   ```

   After that, `pio run -t upload` uses the default `pico32-OTA` environment and updates over WiFi. Tagged releases on GitHub also contain ready-made `.bin` files, uploadable through the `/update` page.
3. **Connect to WiFi**: on first boot the board opens a **LilyGo-MotionController** access point. Join it and enter your WiFi details at 192.168.4.1.
4. **Open the interface** at `http://lilygo-motioncontroller.local/` or the device IP.

<p align="center">
  <img src="webapp-qr-code.png" alt="Web Interface QR Code" width="160"/>
</p>

## First use

Movement is disabled after every power-up, because the motor may have been moved while it was off. Click **Calibrate now**: the motor finds the MIN switch (position 0), then the MAX switch. Hitting a switch later corrects the position.

No limit switches? Jog slowly to one end, click **Set Min Here** in Settings, jog to the other end and click **Set Max Here**. With an encoder fitted you can also move the load by hand.

## Screenshots

| Settings | Debug console |
|----------|---------------|
| ![Configuration Dialog](docs/images/Configuration%20Dialog.png) | ![Debug Console](docs/images/Debug%20Console.png) |

Settings: max speed, acceleration, motor current (default 400 mA, set it to your motor's rating or lower), encoder fitted, freewheel after movement, limit positions and calibration speed.

## Documentation

- [Hardware Setup](docs/hardware.md): parts, wiring, pin configuration
- [API Reference](docs/api.md): WebSocket commands, patterns, REST, debug stream
- [Software Architecture](docs/architecture.md): modules, tasks, calibration model
- [Troubleshooting](docs/troubleshooting.md)
- [Changelog](CHANGELOG.md) and [TODO](TODO.md)

## Development

Firmware is built with PlatformIO, the webapp with pnpm (`cd webapp && pnpm install && pnpm dev`). Notes for contributors and coding agents are in [CLAUDE.md](CLAUDE.md). Unit tests: `pio test -e native`.

## License

MIT License

Copyright 2025 Costyn van Dongen

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the “Software”), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

## Contributing

Feel free to submit PR's or issues here in Github.

## Acknowledgments

- LilyGo for T-Motor hardware platform and [reference project](https://github.com/Xinyuan-LilyGO/T-Motor)
- TMC2209 stepper driver reference implementations
- ESP32 community libraries and examples
- Claude Code for incredible agentic coding experience!
- [Claude Requirements Gathering System](https://github.com/rizethereum/claude-code-requirements-builder)