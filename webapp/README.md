# LilyGo Motion Controller WebApp

React-based web interface for controlling the LilyGo Motion Controller hardware via WebSocket communication.

## Overview

This mobile-first webapp provides real-time control of stepper motors through an intuitive touch-optimized interface. It connects directly to the ESP32 controller via WebSocket for low-latency motor control.

## Features

### Phase 1 (Current)
- **Real-time Motor Control**: WebSocket-based communication with auto-reconnect
- **Jog Controls**: Press-and-hold directional controls with visual feedback
- **Position Control**: Slider-based positioning with drag-to-move functionality
- **Emergency Stop**: Immediate motor halt with reset capability
- **Status Monitoring**: Live position, speed, and connection status
- **Mobile-First Design**: Touch-optimized interface for phone/tablet use
- **Dark Theme**: Professional dark UI with consistent styling
- **Limit Detection**: Visual indicators for limit switch states

### Phase 2 (Future)
- Position presets and profiles
- Speed ramping controls
- Multiple motor support
- Settings persistence on controller
- Advanced diagnostics

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
- **Debug**: `ws://lilygo-motioncontroller.local/debug`

### Commands
```typescript
// Move to position
{
  "command": "move",
  "position": 1000,
  "speed": 50
}

// Jog controls
{
  "command": "jog",
  "direction": "forward" | "backward"
}

{
  "command": "stop"
}

// Emergency stop
{
  "command": "emergency_stop"
}

// Clear emergency stop
{
  "command": "reset"
}

// Move to limits
{
  "command": "move_to_limit",
  "limit": "min" | "max"
}
```

### Status Updates
```typescript
{
  "type": "status",
  "position": 1000,
  "target": 1500,
  "speed": 0,
  "isMoving": false,
  "emergencyStop": false,
  "limitMin": false,
  "limitMax": false,
  "motorConfig": {
    "minLimit": 0,
    "maxLimit": 10000,
    "maxSpeed": 1000
  }
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
- Automatic reconnection (limited retries)
- Command queuing and validation
- Status state management
- Error handling and recovery

#### `PositionControl` Component
- Slider-based position control
- Uses `onValueCommit` to prevent command spam during drag
- Quick position buttons (0%, 25%, 50%, 75%, 100%)
- Speed percentage input with validation

#### `JogControls` Component
- Press-and-hold directional controls
- Continuous jogging with 100ms intervals
- Touch and mouse event support
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