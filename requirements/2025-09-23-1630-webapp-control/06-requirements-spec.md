# Requirements Specification: LilyGo Motion Controller WebApp

**Timestamp:** 2025-09-23 16:40
**Project Context:** Home automation/lighting control using LilyGo Motion Controller

## Problem Statement

The current LilyGo Motion Controller has a basic HTML interface served from SPIFFS. The user needs a modern, mobile-first React webapp that provides intuitive control over stepper motor positioning for home automation projects, with real-time feedback and debugging capabilities.

## Solution Overview

Create a React/TypeScript webapp using Vite build system that communicates with the existing WebSocket endpoints (`/ws` for control, `/debug` for logging). The webapp will be built to the `/data` directory for SPIFFS upload and will provide mobile-optimized motor control interface.

## Functional Requirements

### Phase 1 - Core Control Interface

#### FR1: Motor Position Control
- **Absolute positioning**: Input field + button to move to specific position
- **Jog controls**: Forward/backward buttons with continuous movement while pressed
- **Limit navigation**: Buttons to move to learned min/max limit positions
- **Emergency stop**: Prominent stop button for immediate halt
- **Position display**: Real-time current position from encoder

#### FR2: System Status Display
- **Connection status**: Visual indicator of WebSocket connection state
- **Motor status**: Moving/stopped, emergency stop active
- **Limit switch status**: Visual indicators for min/max limit triggers
- **Motor configuration**: Display current speed, acceleration, stealth mode settings

#### FR3: WebSocket Communication
- **Auto-connect**: Connect to `/ws` on app load using `lilygo-motioncontroller.local` or IP
- **Reconnection**: Limited retry attempts (2-3) then manual reconnect button
- **Command validation**: Client-side validation against min/max limits before sending
- **Real-time updates**: Handle position broadcasts and status changes

#### FR4: User Interface
- **Mobile-first**: Optimized for mobile phones with responsive design
- **Theme support**: Dark/light mode with ThemeProvider (following LumiApp pattern)
- **Connection state UI**: Disable controls when offline, show connection status
- **OTA access**: Link/button to `/update` endpoint for firmware updates

#### FR5: Preset Positions (Phase 1)
- **Local storage**: Save/load preset positions with name, position, and speed
- **Preset management**: Add, edit, delete presets via UI
- **Quick access**: Buttons/list to execute saved presets

### Phase 2 - Advanced Features

#### FR6: Debug Console
- **Debug WebSocket**: Connect to `/debug` endpoint for real-time serial output
- **Auto-scroll**: Always show latest messages (scroll history nice-to-have)
- **Toggle panel**: Collapsible debug console panel/card

#### FR7: Playlist Editor
- **Sequence creation**: Define multi-step movement sequences
- **Step configuration**: Each step has position and speed settings
- **Playlist execution**: Send complete playlist to controller for NVRAM storage
- **Playlist management**: Save, edit, delete playlists

#### FR8: Multi-User Control (Nice-to-Have)
- **Exclusive lock**: 10-second command lock per client
- **Lock indication**: Show which client currently has control
- **Lock timeout**: Automatic release after inactivity

## Technical Requirements

### TR1: Technology Stack
- **Package Manager**: pnpm
- **Build System**: Vite with React plugin
- **Framework**: React 18+ with TypeScript
- **UI Library**: shadcn/ui components with Tailwind CSS
- **Target**: Modern browsers (Chrome, Safari, Firefox mobile)

### TR2: Project Structure
```
app/
├── src/
│   ├── components/
│   │   ├── ui/           # shadcn/ui components
│   │   ├── MotorControl/ # Motor control components
│   │   └── Debug/        # Debug console components
│   ├── hooks/            # Custom React hooks
│   ├── lib/              # Utilities and WebSocket client
│   └── types/            # TypeScript definitions
├── public/
└── package.json
```

### TR3: WebSocket Integration
- **Control endpoint**: `ws://lilygo-motioncontroller.local/ws`
- **Debug endpoint**: `ws://lilygo-motioncontroller.local/debug`
- **Protocol**: JSON messages matching existing controller API
- **Error handling**: Graceful degradation on connection loss

### TR4: Build Configuration
- **Output directory**: `/data` (for SPIFFS upload)
- **Asset optimization**: Gzip compression, file hashing
- **Build command integration**: Compatible with `pio run --target uploadfs`
- **Development server**: Local development with proxy to controller

### TR5: Data Persistence
- **Phase 1**: Browser localStorage for presets and theme preferences
- **Phase 2**: Controller NVRAM for playlists (requires controller enhancement)
- **Theme persistence**: User preference survives browser sessions

## Implementation Hints

### WebSocket Client Pattern
```typescript
// Follow LumiApp reconnection pattern
class MotorControlWebSocket {
  private reconnectAttempts = 0;
  private maxReconnectAttempts = 3;
  private reconnectInterval = 2000;
}
```

### Vite Configuration
```javascript
// Based on LumiApp vite.config.js pattern
{
  name: 'copy-to-data',
  closeBundle() {
    execSync('rm -rf ./data/*');
    execSync('cp dist/*.* ./data/');
  }
}
```

### Component Architecture
- `<MotorControlPanel>` - Main control interface
- `<JogControls>` - Forward/backward buttons with press handlers
- `<PositionDisplay>` - Real-time position and status
- `<PresetManager>` - Preset CRUD operations
- `<DebugConsole>` - Phase 2 debug output panel
- `<ThemeProvider>` - Dark/light mode context

### Controller Integration Points
- **Existing endpoints**: No controller changes needed for Phase 1
- **Limit validation**: Use `getConfig` response for min/max limits
- **Command format**: Support both `"command"` and legacy `"cmd"` fields

## Acceptance Criteria

### Phase 1 Complete When:
- [ ] Mobile-responsive interface loads from controller SPIFFS
- [ ] WebSocket connects automatically to `/ws` endpoint
- [ ] Jog buttons provide continuous movement while pressed
- [ ] Position input moves to absolute positions with validation
- [ ] Limit buttons move to min/max learned positions
- [ ] Emergency stop immediately halts motor
- [ ] Real-time position updates display correctly
- [ ] Theme toggle works with persistence
- [ ] Presets save/load from localStorage
- [ ] Connection state disables controls when offline
- [ ] OTA update link navigates to `/update`
- [ ] Build outputs to `/data` directory for SPIFFS upload

### Phase 2 Complete When:
- [ ] Debug console shows real-time serial output from `/debug`
- [ ] Playlist editor creates multi-step sequences
- [ ] Playlists execute on controller with NVRAM persistence

## Assumptions

- **Network**: Controller operates on local WiFi network only
- **Browser support**: Modern mobile browsers with WebSocket support
- **Controller stability**: Existing WebSocket endpoints remain stable
- **SPIFFS capacity**: Sufficient space for React build output (~500KB estimated)
- **Development environment**: User has Node.js, pnpm, and PlatformIO available

## Risk Mitigation

- **WebSocket reliability**: Implement connection state management and manual reconnect
- **Mobile performance**: Optimize bundle size and lazy-load Phase 2 features
- **Controller compatibility**: Use existing command formats to avoid firmware changes
- **SPIFFS limitations**: Monitor build size and implement compression as needed

## Success Metrics

- **User experience**: Intuitive mobile control interface
- **Performance**: Sub-100ms WebSocket command response
- **Reliability**: Graceful handling of connection interruptions
- **Maintainability**: Clean, modular React component architecture
- **Integration**: Seamless build-to-SPIFFS workflow