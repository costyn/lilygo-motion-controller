# LilyGo Motion Controller WebApp

React-based web interface for controlling the LilyGo Motion Controller hardware via WebSocket communication.

## Overview

This mobile-first webapp provides real-time control of stepper motors through an intuitive touch-optimized interface. It connects directly to the ESP32 controller via WebSocket for low-latency motor control.

## Features

### Current Features
- **Real-time Motor Control**: WebSocket-based communication with auto-reconnect
- **Continuous Jog Controls**: Press-and-hold for smooth continuous movement (with backend `jogStart`/`jogStop` commands)
- **Position Control**: Slider-based positioning with drag-to-move functionality
- **Motor Configuration UI**: Settings dialog for speed, acceleration, and StealthChop mode
- **Emergency Stop**: Immediate motor halt with reset capability
- **Status Monitoring**: Live position, speed, and connection status
- **Mobile-First Design**: Touch-optimized interface for phone/tablet use
- **Dark Theme**: Professional dark UI with consistent styling
- **Limit Detection**: Visual indicators for limit switch states
- **Debug Console**: Real-time serial output via `/debug` WebSocket in collapsible panel

### Future Enhancements
- Position presets and profiles
- Multiple motor support
- Advanced diagnostics dashboard

## Technology Stack

- **Framework**: React 18 + TypeScript
- **Build Tool**: Vite 6
- **UI Components**: shadcn/ui (Radix UI + Tailwind CSS)
- **Icons**: Lucide React
- **Styling**: Tailwind CSS with CSS-in-JS utilities
- **WebSocket**: Native WebSocket API with custom reconnection logic
- **Target**: ESP32 SPIFFS deployment

## Quick Start

### Development Setup

```bash
# Install dependencies
pnpm install

# Start development server (with ESP32 proxy)
pnpm run dev

# Build for production
pnpm run build

# Lint code
pnpm run lint
```

### Production Build & Deploy

```bash
# Build and copy to ESP32 SPIFFS data directory
pnpm run build

# Files are automatically copied to ../data/ for SPIFFS upload
# Upload to ESP32 (from project root):
pio run --target uploadfs
```

## Build System

### Compression & Optimization

The build system uses **gzip compression** to minimize SPIFFS usage:

- **CSS**: 14.6 KB → 3.8 KB (74% reduction)
- **JavaScript**: 203 KB → 63 KB (69% reduction)
- **Total**: 278 KB → 68 KB (75% reduction)

Only compressed `.gz` files are deployed to save ESP32 flash space. The ESPAsyncWebServer automatically serves compressed files transparently to browsers.

### File Structure
```
dist/                           # Vite build output
├── index.html                  # Entry point
├── index-[hash].css           # Bundled styles
├── index-[hash].css.gz        # Compressed styles ✓
├── index-[hash].js            # Bundled JavaScript
└── index-[hash].js.gz         # Compressed JavaScript ✓

../data/                        # ESP32 SPIFFS directory
├── index.html                  # → Copied
├── index-[hash].css.gz        # → Copied (compressed only)
└── index-[hash].js.gz         # → Copied (compressed only)
```

## WebSocket API

### Connection
- **Control**: `ws://lilygo-motioncontroller.local/ws`
- **Debug**: `ws://lilygo-motioncontroller.local/debug` (serial output stream)

### Commands Sent to `/ws`
```typescript
// Move to absolute position
{
  "command": "move",
  "position": 1000,
  "speed": 50
}

// Continuous jogging (true backend continuous movement)
{
  "command": "jogStart",
  "direction": "forward" | "backward"
}

{
  "command": "jogStop"
}

// Stop motor gently (no emergency flag)
{
  "command": "stop"
}

// Emergency stop (requires reset)
{
  "command": "emergency-stop"
}

// Clear emergency stop
{
  "command": "reset"
}

// Request status
{
  "command": "status"
}

// Get configuration
{
  "command": "getConfig"
}

// Update configuration (validated and saved to NVRAM)
{
  "command": "setConfig",
  "maxSpeed": 14400,
  "acceleration": 80000,
  "useStealthChop": true
}
```

**Note:** Legacy `"cmd": "goto"` format still supported for backward compatibility.

### Status Updates Received from `/ws`
```typescript
// Motor status (broadcast periodically)
{
  "type": "status",
  "position": 1000,
  "isMoving": false,
  "emergencyStop": false,
  "limitSwitches": {
    "min": false,
    "max": false,
    "any": false
  }
}

// Motor configuration
{
  "type": "config",
  "maxSpeed": 14400,
  "acceleration": 80000,
  "minLimit": -5000,
  "maxLimit": 50500,
  "useStealthChop": true
}

// Position updates (during movement)
{
  "type": "position",
  "position": 1250
}

// Configuration update confirmation
{
  "type": "configUpdated",
  "status": "success"
}

// Error messages
{
  "type": "error",
  "message": "Cannot jog: limit or emergency stop active"
}
```

## Development

### Project Structure
```
src/
├── components/
│   ├── ui/                     # shadcn/ui components
│   └── MotorControl/          # Motor control components
│       ├── MotorStatus.tsx    # Status display
│       ├── JogControls.tsx    # Manual jog controls
│       └── PositionControl.tsx # Position slider
├── hooks/
│   └── useMotorController.tsx # WebSocket client hook
├── lib/
│   └── utils.ts              # Utility functions
└── types/
    └── index.ts              # TypeScript definitions
```

### Key Components

#### `useMotorController` Hook
Manages WebSocket connection with:
- Automatic reconnection (limited retries with manual reconnect)
- Command queuing and validation
- Status and config state management
- Error handling and recovery
- Methods: `moveTo()`, `jogStart()`, `jogStop()`, `emergencyStop()`, `clearEmergencyStop()`, `updateConfig()`

#### `MotorConfigDialog` Component
- Radix Dialog with form validation
- Real-time validation for speed (100-100,000) and acceleration (100-500,000)
- StealthChop mode toggle
- Read-only limit position display
- Revert and Apply buttons
- Auto-saves to ESP32 NVRAM on apply

#### `PositionControl` Component
- Slider-based position control
- Uses `onValueCommit` to prevent command spam during drag
- Quick position buttons (0%, 25%, 50%, 75%, 100%)
- Speed percentage input with validation
- Move to min/max limit buttons

#### `JogControls` Component
- Press-and-hold directional controls
- **True continuous movement** via backend `jogStart`/`jogStop` commands
- Mouse and touch event support with leave detection
- Ref-based state tracking to prevent spurious stops
- Emergency stop integration

### Development Server

The dev server includes proxy configuration for ESP32 communication:

```typescript
proxy: {
  '/ws': {
    target: 'ws://lilygo-motioncontroller.local',
    ws: true
  },
  '/debug': {
    target: 'ws://lilygo-motioncontroller.local',
    ws: true
  }
}
```

Access at: `http://localhost:5173`

## ESP32 Integration

### mDNS Discovery
The webapp connects to `lilygo-motioncontroller.local` via mDNS, eliminating the need for IP address configuration.

### WebServer Compatibility
Built for ESPAsyncWebServer with:
- Static file serving from SPIFFS
- WebSocket endpoints for real-time communication
- Gzip compression support
- CORS headers for development

### SPIFFS Upload
```bash
# From project root
pio run --target uploadfs
```

## Browser Support

- **Primary**: Modern mobile browsers (iOS Safari, Chrome Mobile)
- **Secondary**: Desktop Chrome, Firefox, Safari
- **Requirements**: ES2015+, WebSocket support, gzip decompression

## Troubleshooting

### Build Issues
```bash
# Clear all caches
rm -rf dist node_modules/.vite* && pnpm run build

# Check for conflicting config files
ls -la vite.config.*
```

### Connection Issues
1. Ensure ESP32 is on same network
2. Check mDNS resolution: `ping lilygo-motioncontroller.local`
3. Verify WebSocket endpoint in browser dev tools
4. Check ESP32 serial output for WebSocket logs

### Development
```bash
# Start with detailed logging
pnpm run dev -- --debug

# Build with verbose output
pnpm run build -- --debug
```

## File Sizes

| File | Uncompressed | Compressed | Savings |
|------|-------------|-----------|---------|
| HTML | 456 B | - | - |
| CSS | 14.6 KB | 3.8 KB | 74% |
| JS | 203 KB | 63 KB | 69% |
| **Total** | **278 KB** | **68 KB** | **75%** |

## License

Part of the LilyGo Motion Controller project.