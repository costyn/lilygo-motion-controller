# Context Findings

**Phase:** Targeted Context Gathering
**Date:** 2025-10-02

## Overview
Analyzed codebase to understand current implementation of jog controls and motor config management. Found both backend controller and frontend webapp infrastructure, with clear patterns to follow.

---

## Finding 1: Current Jog Button Implementation

### Location
- **Frontend**: `webapp/src/components/MotorControl/JogControls.tsx` (lines 1-160)
- **Hook**: `webapp/src/App.tsx` (lines 24-41)

### Current Behavior
The jog buttons use a **repetitive command approach**:
1. On button press: Send initial move command + start 100ms interval timer
2. While held: Send new move commands every 100ms via `setInterval`
3. Each command moves motor 100 steps (jogDistance) at 30% max speed
4. On release: Stop sending commands + send `stop()` command

### Problem Identified
```tsx
// JogControls.tsx lines 37-39
jogIntervalRef.current = setInterval(() => {
  onJogStart(direction)  // Sends multiple discrete move commands
}, 100)
```

This creates **incremental position moves** where:
- Motor receives target position: currentPos + 100
- Motor starts moving to that position
- 100ms later: New command changes target to currentPos + 100 again
- Result: Stuttery motion, motor constantly re-targeting

### Backend Support Check
Examined `src/modules/WebServer/WebServer.cpp` (lines 310-340):
- Controller supports `"move"` command with position/speed
- Controller supports `"stop"` command to halt movement
- **No dedicated "jog" or "continuous move" command found**

### Solution Options
**Option A (Preferred)**: Keep sending move commands but with larger increments
- Change jogDistance to larger value (e.g., 1000 or 10% of limit range)
- This gives motor time to build velocity before next command
- Simple, works with existing protocol

**Option B (Complex)**: Implement continuous velocity mode
- Would require backend changes to add "jog" command
- Controller would need to continuously move in direction until "stop"
- More invasive, requires C++ changes

---

## Finding 2: Motor Config Backend Implementation

### Location
- **Backend**: `src/modules/WebServer/WebServer.cpp` (lines 349-422)
- **Backend**: `src/modules/Configuration/Configuration.h` (lines 1-54)
- **Frontend Types**: `webapp/src/types/index.ts` (lines 20-33)

### Existing Backend Commands

#### getConfig Command (lines 349-362)
```cpp
else if (command == "getConfig") {
    JsonDocument configDoc;
    configDoc["type"] = "config";
    configDoc["maxSpeed"] = config.getMaxSpeed();
    configDoc["acceleration"] = config.getAcceleration();
    configDoc["minLimit"] = config.getMinLimit();
    configDoc["maxLimit"] = config.getMaxLimit();
    configDoc["useStealthChop"] = config.getUseStealthChop();
    // ... broadcasts to all clients
}
```

#### setConfig Command (lines 363-422)
```cpp
else if (command == "setConfig") {
    // Accepts optional fields:
    // - maxSpeed (long)
    // - acceleration (long)
    // - minLimit (long)
    // - maxLimit (long)
    // - useStealthChop (bool)

    // For each field provided:
    // 1. Update Configuration module (NVRAM)
    // 2. Update MotorController live settings
    // 3. Call config.saveConfiguration() - persists to NVRAM
    // 4. Broadcast success + send updated config to all clients
}
```

### Configuration Module Details
From `Configuration.h`:
```cpp
struct MotorConfig {
    long acceleration;     // steps/second² (default: 16000)
    long maxSpeed;         // steps/second (default: 8000)
    long limitPos1;        // encoder position
    long limitPos2;        // encoder position
    bool useStealthChop;   // TMC2209 mode (default: true)
}
```

### Frontend Integration
From `webapp/src/hooks/useMotorController.tsx`:
- Lines 88-89: On connect, automatically requests `status` and `getConfig`
- Lines 136-143: Handles `config` response, stores in `motorConfig` state
- Lines 188-191: `updateConfig()` method sends `setConfig` command
- **Config is already loaded on connect and stored in React state**

### Frontend Type Definitions
From `webapp/src/types/index.ts` (lines 20-27):
```typescript
export interface MotorConfig {
  type: 'config';
  maxSpeed: number;        // 8000 default
  acceleration: number;    // 16000 default
  minLimit: number;        // -5000 default
  maxLimit: number;        // 5000 default
  useStealthChop: boolean; // true default
}
```

---

## Finding 3: UI Architecture & Patterns

### Current UI Layout
From `webapp/src/App.tsx` (lines 49-133):
```
Header (lines 52-86)
├── Title: "LilyGo Motion Controller"
├── Reconnect Button (if disconnected)
├── OTA Update Button (opens /update in new tab)
└── Theme Toggle Button (dark/light mode)

Main Content (lines 89-121)
├── MotorStatus Card (full width, 2 columns)
├── JogControls Card (left column)
└── PositionControl Card (right column)

DebugConsole (line 121) - Below cards

Footer (lines 125-132)
```

### Available UI Components
From `webapp/src/components/ui/`:
- `button.tsx` - Shadcn button component with variants
- `card.tsx` - Card/CardHeader/CardTitle/CardContent
- `input.tsx` - Text input component
- `slider.tsx` - Radix slider component
- `switch.tsx` - Toggle switch component (@radix-ui/react-switch)
- `tabs.tsx` - Tab navigation (@radix-ui/react-tabs)
- `dialog.tsx` - Modal dialog (@radix-ui/react-dialog)

### Tech Stack
From `webapp/package.json`:
- **React 18.3.1** with TypeScript
- **Vite** build tool
- **Tailwind CSS** + tailwindcss-animate
- **Radix UI** components (headless, accessible)
- **Lucide React** icons
- **shadcn/ui** patterns (class-variance-authority)

---

## Finding 4: Similar Features for Pattern Reference

### PositionControl Component
Location: `webapp/src/components/MotorControl/PositionControl.tsx`

Shows pattern for:
- Input validation (position within limits)
- Speed slider with percentage display
- Disabled state handling
- Touch-friendly mobile UI
- Preset position management (localStorage)

### DebugConsole Component
Location: `webapp/src/components/DebugConsole/DebugConsole.tsx`

Shows pattern for:
- Collapsible panel below main content
- WebSocket connection to `/debug` endpoint
- Real-time message display with auto-scroll
- Clear button functionality

---

## Finding 5: Config UI Layout Options Analysis

### User Preference
From discovery answers:
- "A button at the top to a 2nd page would also be acceptable"
- "I don't want the front page to become too cluttered"
- "There's already button for OTA and Darkmode. Another would be ok"

### Option A: Separate Config Page (Recommended)
**Pros**:
- Clean separation of concerns
- Won't clutter main control interface
- Matches user's suggestion
- Can use more screen space for form
- Easier to add validation UI (error messages, hints)

**Cons**:
- Requires routing (could use hash-based routing to avoid complexity)
- User must navigate away from controls

**Implementation**:
- Add "Settings" button to header next to OTA/Theme
- Use Radix Dialog component (already installed) for modal overlay
- OR use simple conditional rendering for different "pages"

### Option B: Collapsible Panel (Like DebugConsole)
**Pros**:
- Follows existing pattern (DebugConsole)
- Stays on same page
- Easy to implement

**Cons**:
- Adds vertical height to page
- User mentioned not wanting clutter

### Option C: Hamburger Menu (Rejected)
**Pros**:
- Common mobile pattern

**Cons**:
- Adds complexity
- Not consistent with current UI style
- User said "button at top" would be acceptable

---

## Finding 6: Config Parameter Validation Requirements

### Hardware Limits
From README.md and Configuration module:

**Max Speed** (steps/second):
- Current default: 8000
- Safe range: 1000 - 12000 (based on TMC2209 capabilities)
- Motor spec: 1.8° per step × 16 microsteps = 0.1125° per microstep
- At 8000 steps/sec = 900 RPM (reasonable for 17HS19 motor)

**Acceleration** (steps/second²):
- Current default: 16000
- Safe range: 1000 - 30000
- Higher = faster ramp up, but may cause stall if too high

**Limit Positions** (encoder counts):
- Min/Max must not be equal
- Should have reasonable separation (min 100 steps?)
- Min must be < Max after sorting

**StealthChop Mode**:
- Boolean, no validation needed
- True = quiet, False = powerful (SpreadCycle)

---

## Finding 7: WebSocket Connection Management

### Auto-connection on Load
From `webapp/src/hooks/useMotorController.tsx` (lines 209-223):
- Connects automatically 500ms after component mount
- Requests `status` and `getConfig` on successful connection
- Config automatically populates `motorConfig` state
- 3 reconnection attempts with 2-second delay between

### Config Updates
- `updateConfig()` method (lines 188-191) sends partial updates
- Backend merges with existing config
- Backend broadcasts updated config to all connected clients (lines 405-416)
- Frontend re-fetches config on successful update (lines 140-142)

**Implication for Config UI**:
- Config values available immediately after connection
- Can populate form fields from `motorConfig` state
- Updates automatically sync across all connected clients
- "Revert" button can restore from current `motorConfig` state

---

## Finding 8: Previous Requirements Sessions

### Session: 2025-09-23 - Webapp Control
Initial webapp creation with:
- WebSocket communication
- Jog controls
- Position control
- Debug console

### Session: 2025-09-30 - Protocol Sync Updates
Added automatic position/status broadcasts during movement:
- Position updates every 100ms while moving
- Status updates every 500ms while moving
- Immediate updates on start/stop

**Relevance to Jog Fix**:
- Real-time position updates now available
- Could improve jog responsiveness
- Can see position changing during jog

---

## Technical Constraints

### ESP32 Backend
- **Memory**: Currently at 16% RAM usage (52KB used)
- **Flash**: Currently at 83% Flash usage (1.09MB)
- **Network**: WebSocket already proven stable
- **Configuration**: NVRAM persistence already implemented
- **Build Time**: ~30 seconds for full build

### Webapp
- **Build Output**: ~100KB gzipped (current)
- **Dependencies**: Radix UI already included, no new deps needed
- **Bundle Size**: Should stay under 150KB gzipped
- **Browser Support**: Modern browsers (ES2020+)

---

## Related Features

### Similar Config UIs in Other Projects
Pattern: Modal dialog with form fields
- Settings button in header
- Dialog opens overlay
- Form with labels, inputs, validation
- Cancel/Apply buttons at bottom
- Success/error feedback

### Motor Control Best Practices
- Always show current values
- Clear labels with units (steps/sec, steps/sec²)
- Prevent invalid combinations
- Confirm before applying critical changes
- Provide way to revert mistakes

---

## Files Requiring Modification

### Backend (None - Already Complete)
All backend functionality exists and is working:
- `src/modules/WebServer/WebServer.cpp` - getConfig/setConfig commands
- `src/modules/Configuration/Configuration.h` - NVRAM persistence
- WebSocket broadcast on config changes

### Frontend (New Components)
**New files to create**:
1. `webapp/src/components/MotorConfig/MotorConfigDialog.tsx` - Config form UI
2. `webapp/src/components/MotorConfig/MotorConfigForm.tsx` - Form fields and validation (optional, could be in Dialog)

**Files to modify**:
1. `webapp/src/App.tsx` - Add Settings button, wire up dialog
2. `webapp/src/components/MotorControl/JogControls.tsx` - Fix jog button behavior
3. `webapp/src/hooks/useMotorController.tsx` - Expose config methods (already has updateConfig)

---

## Implementation Patterns to Follow

### Pattern 1: Dialog with Form
```tsx
// Use Radix Dialog for modal overlay
import { Dialog, DialogContent, DialogHeader, DialogTitle } from '@/components/ui/dialog'

// State for open/closed
const [configOpen, setConfigOpen] = useState(false)

// Populate form with current config
const [formValues, setFormValues] = useState(motorConfig)

// On open, reset to current config
useEffect(() => {
  if (configOpen) {
    setFormValues(motorConfig)
  }
}, [configOpen, motorConfig])
```

### Pattern 2: Input Validation
```tsx
const validateMaxSpeed = (value: number) => {
  if (value < 1000 || value > 12000) {
    return "Speed must be between 1000 and 12000 steps/sec"
  }
  return null
}
```

### Pattern 3: Config Update
```tsx
const handleApply = () => {
  // Send to controller
  updateConfig({
    maxSpeed: formValues.maxSpeed,
    acceleration: formValues.acceleration,
    useStealthChop: formValues.useStealthChop
  })

  // Close dialog
  setConfigOpen(false)
}
```

### Pattern 4: Jog Distance Adjustment
```tsx
// Current: 100 steps
const jogDistance = 100

// Better: Scale to limit range
const limitRange = motorConfig.maxLimit - motorConfig.minLimit
const jogDistance = Math.max(100, Math.floor(limitRange * 0.05)) // 5% of range

// Or: Fixed larger value
const jogDistance = 500 // 5x current distance
```

---

## Assumptions

1. **Jog Fix**: Increasing jog distance/speed is acceptable solution (no backend changes needed)
2. **Config UI**: Modal dialog is acceptable UX (matches "separate page" user preference)
3. **Validation**: Client-side validation is sufficient (backend also validates)
4. **Units**: Display values in steps/sec and steps/sec² (no unit conversion needed)
5. **Limits**: User understands encoder counts (they set them via position learning)
6. **Mobile**: Dialog will work on mobile with touch-friendly inputs
7. **Testing**: User can test with hardware once implemented

---

## Summary

### Jog Buttons Fix
- ✅ Backend supports needed commands (move, stop)
- ✅ Current implementation identified (100-step increments every 100ms)
- ✅ Solution: Increase jog distance or adjust timing
- ⚠️ No backend changes needed (keep it simple)

### Motor Config UI
- ✅ Backend fully implemented and working (getConfig, setConfig, NVRAM save)
- ✅ Frontend already fetches and stores config in state
- ✅ UI components available (Dialog, Input, Switch, Button)
- ✅ Clear pattern to follow (similar to existing components)
- ⚠️ Need to create: Config dialog component + wire into App
- ⚠️ Need to add: Validation logic for safe values

**Ready to proceed to expert detail questions phase.**
