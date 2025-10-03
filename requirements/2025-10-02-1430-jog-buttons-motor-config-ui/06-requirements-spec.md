# Requirements Specification: Jog Buttons & Motor Config UI

**Session ID**: 2025-10-02-1430-jog-buttons-motor-config-ui
**Date**: 2025-10-02
**Status**: Ready for Implementation

---

## Problem Statement

### Feature 1: Jog Button Behavior
The current jog buttons move the motor in small increments but don't provide smooth continuous movement when held down. Users expect:
- Press and hold → motor moves continuously
- Release → motor stops immediately
- Natural, responsive feel for manual positioning

**Current Issue**: Jog buttons send repetitive 100-step move commands every 100ms, causing stuttery motion as the motor constantly re-targets to new positions.

**Impact**: Poor user experience for manual motor positioning, makes precise adjustment difficult.

---

### Feature 2: Motor Configuration UI
Motor configuration parameters (maxSpeed, acceleration, useStealthChop) can be set via WebSocket but there's no UI for end users to adjust these settings. Currently requires:
- Manual WebSocket commands, OR
- Firmware recompilation

**Current State**: Backend fully supports getConfig/setConfig commands with NVRAM persistence, but webapp has no UI to expose this functionality.

**Impact**: Users cannot tune motor performance without technical knowledge of WebSocket protocol.

---

## Solution Overview

### Feature 1: Continuous Jogging
Implement true continuous movement with new backend commands:
- Add `jogStart` command with direction parameter (forward/backward)
- Add `jogStop` command to halt jogging
- Backend moves motor continuously at configured jog speed
- Frontend sends jogStart on button press, jogStop on release

**Benefits**:
- Smooth, natural jogging behavior
- Responsive stop on button release
- Works with limit switches for safety

---

### Feature 2: Motor Config Dialog
Add Settings button in header that opens modal dialog with:
- Form fields for editable config parameters
- Real-time validation with error messages
- Apply/Revert buttons
- Mobile-friendly touch interface

**Benefits**:
- User-accessible configuration without technical knowledge
- Safe validation prevents hardware damage
- Changes persist to NVRAM across reboots

---

## Functional Requirements

### FR-1: Continuous Jog Start
**Priority**: MUST HAVE

**Behavior**:
- User presses and holds "Jog Forward" or "Jog Back" button
- Frontend sends WebSocket command: `{"command": "jogStart", "direction": "forward"}` (or "backward")
- Backend begins continuous movement in specified direction
- Movement continues until jogStop, limit switch triggered, or emergency stop

**Jog Speed**: 30% of configured maxSpeed (matches current implementation)

**Acceptance Criteria**:
- [ ] Motor starts moving within 50ms of button press
- [ ] Movement is smooth and continuous (no stuttering)
- [ ] Direction matches button pressed
- [ ] Works on both desktop (mouse) and mobile (touch)

---

### FR-2: Continuous Jog Stop
**Priority**: MUST HAVE

**Behavior**:
- User releases jog button
- Frontend sends WebSocket command: `{"command": "jogStop"}`
- Backend halts motor movement immediately
- If user moves mouse/finger off button while pressed, also stops (onMouseLeave/onTouchCancel)

**Acceptance Criteria**:
- [ ] Motor stops within 50ms of button release
- [ ] No drift after stop command
- [ ] Mouse leave event also triggers stop
- [ ] Touch cancel event also triggers stop

---

### FR-3: Jog Safety Limits
**Priority**: MUST HAVE

**Behavior**:
- If jogging would exceed limit positions, motor stops at limit
- If limit switch triggered during jog, motor stops immediately
- If emergency stop active, jog buttons are disabled

**Acceptance Criteria**:
- [ ] Cannot jog beyond minLimit or maxLimit
- [ ] Jog buttons disabled when emergencyStop is true
- [ ] Limit switch stops jog movement
- [ ] Backend handles safety, frontend reflects state

---

### FR-4: Settings Button in Header
**Priority**: MUST HAVE

**Location**: Header section of App.tsx, between "Reconnect" and "OTA Update" buttons

**Appearance**:
- Icon: Settings gear icon (from lucide-react)
- Label: "Settings" or icon-only
- Style: Matches existing header buttons (outline variant)

**Acceptance Criteria**:
- [ ] Button visible in header on all screen sizes
- [ ] Opens Motor Config dialog when clicked
- [ ] Disabled when not connected to controller
- [ ] Follows existing button styling patterns

---

### FR-5: Motor Config Dialog - UI Structure
**Priority**: MUST HAVE

**Implementation**: Radix UI Dialog component (modal overlay)

**Layout**:
```
┌─────────────────────────────────────┐
│ Motor Configuration            [×]  │ ← DialogHeader
├─────────────────────────────────────┤
│                                     │
│ Max Speed                           │
│ [8000] steps/sec                    │ ← Number input
│ ⚠️ Must be between 1000-12000       │ ← Validation error
│                                     │
│ Acceleration                        │
│ [16000] steps/sec²                  │
│                                     │
│ StealthChop Mode                    │
│ [●──] Enabled  (Quiet operation)    │ ← Switch with description
│                                     │
│ Limit Positions (Read-only)         │
│ Min: -5000  Max: 5000               │ ← Display only
│                                     │
├─────────────────────────────────────┤
│          [Revert] [Apply]           │ ← DialogFooter
└─────────────────────────────────────┘
```

**Acceptance Criteria**:
- [ ] Dialog opens when Settings button clicked
- [ ] Dark background overlay dims main content
- [ ] Mobile-responsive (full-screen on small devices)
- [ ] Escape key closes dialog
- [ ] Click outside closes dialog

---

### FR-6: Motor Config Dialog - Form Fields
**Priority**: MUST HAVE

**Editable Fields**:

1. **Max Speed** (number input)
   - Label: "Max Speed"
   - Units: "steps/sec"
   - Type: Number input (integer)
   - Validation: 1000 ≤ value ≤ 12000
   - Help text: "Maximum motor speed during movement"

2. **Acceleration** (number input)
   - Label: "Acceleration"
   - Units: "steps/sec²"
   - Type: Number input (integer)
   - Validation: 1000 ≤ value ≤ 30000
   - Help text: "How quickly motor reaches target speed"

3. **StealthChop Mode** (toggle switch)
   - Label: "StealthChop Mode"
   - Type: Switch (boolean)
   - States: Enabled (quiet) / Disabled (powerful)
   - Help text: "Enable for quieter operation, disable for more torque"

**Read-only Fields**:

4. **Limit Positions** (display only)
   - Label: "Limit Positions (Configured via limit switches)"
   - Display: "Min: {minLimit}  Max: {maxLimit}"
   - Style: Muted/secondary text color
   - No input fields (user confirmed read-only)

**Acceptance Criteria**:
- [ ] All fields pre-populated with current config values
- [ ] Number inputs accept only valid integers
- [ ] Switch toggles between true/false
- [ ] Limit positions shown but not editable
- [ ] Field labels have proper spacing and alignment

---

### FR-7: Motor Config Dialog - Validation
**Priority**: MUST HAVE

**Real-time Validation**:
- Validate on input change (not on blur)
- Show error message below invalid fields
- Apply button disabled until all fields valid
- Error messages in red text

**Validation Rules**:

**Max Speed**:
- Min: 1000 steps/sec
- Max: 12000 steps/sec
- Error: "Speed must be between 1000 and 12000 steps/sec"
- Rationale: Based on TMC2209 capabilities and 17HS19 motor specs

**Acceleration**:
- Min: 1000 steps/sec²
- Max: 30000 steps/sec²
- Error: "Acceleration must be between 1000 and 30000 steps/sec²"
- Rationale: Too low = sluggish, too high = stall risk

**Additional Validation** (if needed during implementation):
- Research actual TMC2209 limitations
- Consider encoder resolution (MT6816: 16384 counts/rev)
- Test with physical hardware for safe ranges

**Acceptance Criteria**:
- [ ] Invalid values show error message immediately
- [ ] Error messages are clear and actionable
- [ ] Apply button disabled when any field invalid
- [ ] Visual indication of invalid fields (red border)
- [ ] Valid input clears error message

---

### FR-8: Motor Config Dialog - Actions
**Priority**: MUST HAVE

**Revert Button**:
- Label: "Revert"
- Variant: "outline" (secondary)
- Position: Bottom left of dialog footer
- Behavior: Resets all form fields to current motorConfig state (from controller)
- No server communication, instant local reset

**Apply Button**:
- Label: "Apply"
- Variant: "default" (primary)
- Position: Bottom right of dialog footer
- Behavior:
  1. Send `setConfig` command with modified values
  2. Show loading state while waiting for response
  3. On success: Close dialog, show success toast (optional)
  4. On error: Keep dialog open, show error message
- Disabled when: Any field invalid OR no changes made

**Close (X) Button**:
- Position: Top right of dialog header
- Behavior: Same as pressing Escape or clicking overlay
- Discards unsaved changes (no confirmation needed per user preference)

**Acceptance Criteria**:
- [ ] Revert restores original values immediately
- [ ] Apply sends only modified fields to backend
- [ ] Apply disabled when form invalid
- [ ] Apply shows loading spinner during submit
- [ ] Dialog closes on successful apply
- [ ] Error message shown if apply fails

---

### FR-9: Config Persistence
**Priority**: MUST HAVE

**Backend Behavior** (already implemented):
- Backend receives `setConfig` command
- Updates Configuration module in memory
- Calls `config.saveConfiguration()` to persist to NVRAM
- Broadcasts updated config to all connected clients
- Sends `{"type": "configUpdated", "status": "success"}` response

**Frontend Behavior**:
- On successful configUpdated response, request fresh config
- Update motorConfig state with latest values
- All components using motorConfig automatically update (React state)

**Acceptance Criteria**:
- [ ] Config changes saved to ESP32 NVRAM
- [ ] Settings survive controller reboot
- [ ] All connected clients receive config updates
- [ ] Webapp reflects new config values after apply

---

### FR-10: Mobile Touch Support
**Priority**: MUST HAVE

**Jog Buttons**:
- Already have touch event handlers (onTouchStart/onTouchEnd)
- Should work with new jogStart/jogStop commands
- No changes needed to touch event logic

**Config Dialog**:
- Number inputs must work with mobile keyboards
- Switch component must be touch-friendly (large hit area)
- Dialog must be scrollable on small screens
- Form fields must have appropriate spacing for touch

**Acceptance Criteria**:
- [ ] Jog buttons work on mobile (continuous movement)
- [ ] Config dialog opens full-screen on mobile
- [ ] All form inputs accessible with touch
- [ ] No accidental double-taps or missed interactions
- [ ] Keyboard appears for number inputs

---

## Technical Requirements

### TR-1: Backend - New WebSocket Commands

**File**: `src/modules/WebServer/WebServer.cpp`

**Location**: Inside `handleWebSocketMessage()` function (after line 340)

**Implementation**:

```cpp
else if (command == "jogStart")
{
    if (doc["direction"].is<const char *>() || doc["direction"].is<String>())
    {
        String direction = doc["direction"].as<String>();

        if (!limitSwitch.isAnyTriggered() && !motorController.isEmergencyStopActive())
        {
            // Calculate jog speed (30% of max speed)
            int jogSpeed = config.getMaxSpeed() * 0.3;

            if (direction == "forward")
            {
                // Move to max limit at jog speed
                long targetPosition = config.getMaxLimit();
                motorController.moveTo(targetPosition, jogSpeed);
                LOG_INFO("Jog started: forward to %ld at speed %d", targetPosition, jogSpeed);
            }
            else if (direction == "backward")
            {
                // Move to min limit at jog speed
                long targetPosition = config.getMinLimit();
                motorController.moveTo(targetPosition, jogSpeed);
                LOG_INFO("Jog started: backward to %ld at speed %d", targetPosition, jogSpeed);
            }

            // Broadcast status to show movement started
            broadcastStatus();
            lastPositionBroadcast = millis();
            lastStatusBroadcast = millis();
        }
        else
        {
            ws.textAll("{\"type\":\"error\",\"message\":\"Cannot jog: limit or emergency stop active\"}");
        }
    }
}
else if (command == "jogStop")
{
    motorController.stop();
    LOG_INFO("Jog stopped");
    broadcastStatus();
}
```

**Key Design Decisions**:
- Reuse existing `moveTo()` and `stop()` methods (no new MotorController API needed)
- Set target to limit position (motor will stop at limit if jog held too long)
- Calculate jog speed as 30% of maxSpeed (matches current frontend behavior)
- Use existing safety checks (limit switches, emergency stop)
- Broadcast status immediately for responsive UI

**Acceptance Criteria**:
- [ ] jogStart command accepts "forward" or "backward" direction
- [ ] Motor moves continuously toward appropriate limit
- [ ] jogStop command halts movement immediately
- [ ] Safety checks prevent jogging when unsafe
- [ ] Error messages sent if jog blocked
- [ ] Status broadcasts show movement state

---

### TR-2: Backend - Add Command Types to Documentation

**File**: `webapp/src/types/index.ts`

**Location**: After line 88 (in ControlCommand union type)

**Add**:
```typescript
export interface JogStartCommand {
  command: 'jogStart';
  direction: 'forward' | 'backward';
}

export interface JogStopCommand {
  command: 'jogStop';
}

export type ControlCommand =
  | MoveCommand
  | StopCommand
  | EmergencyStopCommand
  | ResetCommand
  | StatusCommand
  | GetConfigCommand
  | SetConfigCommand
  | JogStartCommand
  | JogStopCommand;
```

**Acceptance Criteria**:
- [ ] TypeScript types match backend implementation
- [ ] Direction is type-safe (only "forward" | "backward")
- [ ] Commands included in ControlCommand union

---

### TR-3: Frontend - Update useMotorController Hook

**File**: `webapp/src/hooks/useMotorController.tsx`

**Location**: After line 195 (after refreshStatus method)

**Add Methods**:
```typescript
const jogStart = useCallback((direction: 'forward' | 'backward') => {
  return sendCommand({ command: 'jogStart', direction })
}, [sendCommand])

const jogStop = useCallback(() => {
  return sendCommand({ command: 'jogStop' })
}, [sendCommand])
```

**Update Return Object** (line 225):
```typescript
return {
  // ... existing exports
  jogStart,
  jogStop,
  // ... rest of exports
}
```

**Acceptance Criteria**:
- [ ] jogStart method accepts direction parameter
- [ ] jogStop method takes no parameters
- [ ] Both methods use existing sendCommand infrastructure
- [ ] Methods exported from hook

---

### TR-4: Frontend - Update JogControls Component

**File**: `webapp/src/components/MotorControl/JogControls.tsx`

**Changes Required**:

1. **Update Props Interface** (lines 6-15):
```typescript
interface JogControlsProps {
  isConnected: boolean
  isMoving: boolean
  emergencyStop: boolean
  onJogStart: (direction: 'forward' | 'backward') => void
  onJogStop: () => void  // Keep this, now calls backend jogStop
  onEmergencyStop: () => void
  onClearEmergencyStop: () => void
  onMoveToLimit: (limit: 'min' | 'max') => void
}
```

2. **Simplify Jog Logic** (remove lines 27-52, replace with):
```typescript
const handleMouseDown = (direction: 'forward' | 'backward') => {
  if (!isConnected || emergencyStop) return
  onJogStart(direction)
}

const handleMouseUp = () => {
  if (!isConnected) return
  onJogStop()
}

const handleTouchStart = (direction: 'forward' | 'backward') => {
  if (!isConnected || emergencyStop) return
  onJogStart(direction)
}

const handleTouchEnd = () => {
  if (!isConnected) return
  onJogStop()
}
```

3. **Remove** (no longer needed):
- `jogIntervalRef` ref
- `isJoggingRef` ref
- `setInterval` logic
- `clearInterval` logic

4. **Keep Existing**:
- Button event handlers (onMouseDown/Up/Leave, onTouchStart/End)
- Disabled state logic
- UI structure and styling

**Acceptance Criteria**:
- [ ] Jog buttons send jogStart on press
- [ ] Jog buttons send jogStop on release
- [ ] Mouse leave triggers jogStop
- [ ] Touch cancel triggers jogStop
- [ ] No more setInterval timing logic
- [ ] Disabled state prevents jogging when unsafe

---

### TR-5: Frontend - Update App.tsx Jog Handlers

**File**: `webapp/src/App.tsx`

**Current Code** (lines 24-41):
```typescript
const handleJogStart = (direction: 'forward' | 'backward') => {
  const jogDistance = 100
  const jogSpeed = Math.round(motorConfig.maxSpeed * 0.3)
  const currentPos = motorStatus.position

  let newPosition: number
  if (direction === 'forward') {
    newPosition = Math.min(currentPos + jogDistance, motorConfig.maxLimit)
  } else {
    newPosition = Math.max(currentPos - jogDistance, motorConfig.minLimit)
  }

  moveTo(newPosition, jogSpeed)
}

const handleJogStop = () => {
  stop()
}
```

**Replace With**:
```typescript
const handleJogStart = (direction: 'forward' | 'backward') => {
  jogStart(direction)
}

const handleJogStop = () => {
  jogStop()
}
```

**Update Destructuring** (line 13):
```typescript
const {
  // ... existing
  jogStart,
  jogStop,
  // ... rest
} = useMotorController()
```

**Acceptance Criteria**:
- [ ] handleJogStart calls hook's jogStart method
- [ ] handleJogStop calls hook's jogStop method
- [ ] Removed position calculation logic (now in backend)
- [ ] Destructure new methods from useMotorController

---

### TR-6: Frontend - Create MotorConfigDialog Component

**File**: `webapp/src/components/MotorConfig/MotorConfigDialog.tsx` (NEW FILE)

**Component Structure**:
```typescript
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogFooter } from '@/components/ui/dialog'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Switch } from '@/components/ui/switch'
import { Label } from '@/components/ui/label'
import { useState, useEffect } from 'react'
import type { MotorConfig } from '@/types'

interface MotorConfigDialogProps {
  open: boolean
  onOpenChange: (open: boolean) => void
  currentConfig: MotorConfig
  onApply: (config: Partial<Omit<MotorConfig, 'type'>>) => void
  isConnected: boolean
}

export function MotorConfigDialog({
  open,
  onOpenChange,
  currentConfig,
  onApply,
  isConnected
}: MotorConfigDialogProps) {
  // Form state
  const [maxSpeed, setMaxSpeed] = useState(currentConfig.maxSpeed)
  const [acceleration, setAcceleration] = useState(currentConfig.acceleration)
  const [useStealthChop, setUseStealthChop] = useState(currentConfig.useStealthChop)

  // Validation state
  const [maxSpeedError, setMaxSpeedError] = useState<string | null>(null)
  const [accelerationError, setAccelerationError] = useState<string | null>(null)

  // Reset form when dialog opens or config changes
  useEffect(() => {
    if (open) {
      setMaxSpeed(currentConfig.maxSpeed)
      setAcceleration(currentConfig.acceleration)
      setUseStealthChop(currentConfig.useStealthChop)
      setMaxSpeedError(null)
      setAccelerationError(null)
    }
  }, [open, currentConfig])

  // Validation functions
  const validateMaxSpeed = (value: number) => {
    if (value < 1000 || value > 12000) {
      return "Speed must be between 1000 and 12000 steps/sec"
    }
    return null
  }

  const validateAcceleration = (value: number) => {
    if (value < 1000 || value > 30000) {
      return "Acceleration must be between 1000 and 30000 steps/sec²"
    }
    return null
  }

  // Handle input changes with validation
  const handleMaxSpeedChange = (value: string) => {
    const numValue = parseInt(value)
    if (!isNaN(numValue)) {
      setMaxSpeed(numValue)
      setMaxSpeedError(validateMaxSpeed(numValue))
    }
  }

  const handleAccelerationChange = (value: string) => {
    const numValue = parseInt(value)
    if (!isNaN(numValue)) {
      setAcceleration(numValue)
      setAccelerationError(validateAcceleration(numValue))
    }
  }

  // Check if form is valid
  const isFormValid = !maxSpeedError && !accelerationError

  // Check if form has changes
  const hasChanges =
    maxSpeed !== currentConfig.maxSpeed ||
    acceleration !== currentConfig.acceleration ||
    useStealthChop !== currentConfig.useStealthChop

  // Handle revert
  const handleRevert = () => {
    setMaxSpeed(currentConfig.maxSpeed)
    setAcceleration(currentConfig.acceleration)
    setUseStealthChop(currentConfig.useStealthChop)
    setMaxSpeedError(null)
    setAccelerationError(null)
  }

  // Handle apply
  const handleApply = () => {
    if (!isFormValid) return

    const changes: Partial<Omit<MotorConfig, 'type'>> = {}
    if (maxSpeed !== currentConfig.maxSpeed) changes.maxSpeed = maxSpeed
    if (acceleration !== currentConfig.acceleration) changes.acceleration = acceleration
    if (useStealthChop !== currentConfig.useStealthChop) changes.useStealthChop = useStealthChop

    onApply(changes)
    onOpenChange(false)
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="sm:max-w-[425px]">
        <DialogHeader>
          <DialogTitle>Motor Configuration</DialogTitle>
        </DialogHeader>

        <div className="grid gap-4 py-4">
          {/* Max Speed */}
          <div className="grid gap-2">
            <Label htmlFor="maxSpeed">Max Speed</Label>
            <div className="flex items-center gap-2">
              <Input
                id="maxSpeed"
                type="number"
                value={maxSpeed}
                onChange={(e) => handleMaxSpeedChange(e.target.value)}
                className={maxSpeedError ? "border-red-500" : ""}
              />
              <span className="text-sm text-muted-foreground">steps/sec</span>
            </div>
            {maxSpeedError && (
              <p className="text-sm text-red-500">{maxSpeedError}</p>
            )}
            <p className="text-xs text-muted-foreground">
              Maximum motor speed during movement
            </p>
          </div>

          {/* Acceleration */}
          <div className="grid gap-2">
            <Label htmlFor="acceleration">Acceleration</Label>
            <div className="flex items-center gap-2">
              <Input
                id="acceleration"
                type="number"
                value={acceleration}
                onChange={(e) => handleAccelerationChange(e.target.value)}
                className={accelerationError ? "border-red-500" : ""}
              />
              <span className="text-sm text-muted-foreground">steps/sec²</span>
            </div>
            {accelerationError && (
              <p className="text-sm text-red-500">{accelerationError}</p>
            )}
            <p className="text-xs text-muted-foreground">
              How quickly motor reaches target speed
            </p>
          </div>

          {/* StealthChop Mode */}
          <div className="grid gap-2">
            <Label htmlFor="stealthchop">StealthChop Mode</Label>
            <div className="flex items-center gap-2">
              <Switch
                id="stealthchop"
                checked={useStealthChop}
                onCheckedChange={setUseStealthChop}
              />
              <span className="text-sm">
                {useStealthChop ? "Enabled (Quiet)" : "Disabled (Powerful)"}
              </span>
            </div>
            <p className="text-xs text-muted-foreground">
              Enable for quieter operation, disable for more torque
            </p>
          </div>

          {/* Limit Positions (Read-only) */}
          <div className="grid gap-2 pt-4 border-t">
            <Label className="text-muted-foreground">
              Limit Positions (Read-only)
            </Label>
            <p className="text-sm">
              Min: {currentConfig.minLimit} · Max: {currentConfig.maxLimit}
            </p>
            <p className="text-xs text-muted-foreground">
              Configured via physical limit switches
            </p>
          </div>
        </div>

        <DialogFooter>
          <Button
            variant="outline"
            onClick={handleRevert}
            disabled={!hasChanges}
          >
            Revert
          </Button>
          <Button
            onClick={handleApply}
            disabled={!isFormValid || !hasChanges || !isConnected}
          >
            Apply
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  )
}
```

**Notes**:
- May need to create `webapp/src/components/ui/label.tsx` if not exists (Radix Label component)
- Validation happens on input change (real-time)
- Apply button disabled when invalid or no changes
- Form resets when dialog opens

**Acceptance Criteria**:
- [ ] Dialog follows Radix UI patterns
- [ ] All form fields render correctly
- [ ] Validation works in real-time
- [ ] Revert restores original values
- [ ] Apply sends only changed fields
- [ ] Mobile-responsive layout

---

### TR-7: Frontend - Add Settings Button to App.tsx

**File**: `webapp/src/App.tsx`

**Changes**:

1. **Import Dialog** (top of file):
```typescript
import { MotorConfigDialog } from './components/MotorConfig/MotorConfigDialog'
import { Settings } from 'lucide-react'
import { useState } from 'react'
```

2. **Add State** (in AppContent function, around line 23):
```typescript
const [configDialogOpen, setConfigDialogOpen] = useState(false)
```

3. **Add Settings Button** (in header, after line 67, before OTA button):
```typescript
<Button
  variant="outline"
  size="sm"
  onClick={() => setConfigDialogOpen(true)}
  disabled={!isConnected}
>
  <Settings className="h-4 w-4 mr-2" />
  Settings
</Button>
```

4. **Add Dialog Component** (after footer, before closing divs, around line 133):
```typescript
<MotorConfigDialog
  open={configDialogOpen}
  onOpenChange={setConfigDialogOpen}
  currentConfig={motorConfig}
  onApply={updateConfig}
  isConnected={isConnected}
/>
```

5. **Import updateConfig** (add to destructuring at line 13):
```typescript
const {
  // ... existing
  updateConfig,
  // ... rest
} = useMotorController()
```

**Acceptance Criteria**:
- [ ] Settings button appears in header
- [ ] Button disabled when disconnected
- [ ] Clicking button opens config dialog
- [ ] Dialog receives current config
- [ ] Apply calls updateConfig hook method

---

### TR-8: Validation Value Research

**Action Item**: During implementation, research and document safe validation ranges:

**Max Speed** (currently estimated 1000-12000):
- Check TMC2209 datasheet for maximum step frequency
- Consider motor specs: 17HS19-2004S1 rated speed
- Test with hardware to find smooth operation range
- Update validation min/max if needed

**Acceleration** (currently estimated 1000-30000):
- Too low = sluggish movement
- Too high = motor stall / missed steps
- Depends on motor inertia and load
- Test with hardware to find reliable range

**Documentation Location**: Update validation comments in MotorConfigDialog.tsx with findings

**Acceptance Criteria**:
- [ ] Validation ranges tested on real hardware
- [ ] Min/max values prevent motor damage
- [ ] Comments explain rationale for limits
- [ ] User informed if values need adjustment

---

## Implementation Patterns to Follow

### Pattern 1: WebSocket Command Handling
Follow existing pattern in `WebServer.cpp`:
```cpp
else if (command == "commandName")
{
    // Parse parameters from JsonDocument
    // Validate parameters
    // Perform action
    // Send response or error
    // Broadcast status if state changed
    LOG_INFO("Action completed: details");
}
```

### Pattern 2: React Hook Methods
Follow existing pattern in `useMotorController.tsx`:
```typescript
const methodName = useCallback((params) => {
  return sendCommand({ command: 'commandName', ...params })
}, [sendCommand])
```

### Pattern 3: Form Validation
```typescript
const [value, setValue] = useState(initialValue)
const [error, setError] = useState<string | null>(null)

const validateValue = (val: number) => {
  if (/* invalid */) return "Error message"
  return null
}

const handleChange = (newValue: string) => {
  const numValue = parseInt(newValue)
  if (!isNaN(numValue)) {
    setValue(numValue)
    setError(validateValue(numValue))
  }
}
```

### Pattern 4: Dialog Component Structure
Follow Radix UI patterns from existing codebase:
```typescript
<Dialog open={open} onOpenChange={onOpenChange}>
  <DialogContent>
    <DialogHeader>
      <DialogTitle>Title</DialogTitle>
    </DialogHeader>
    {/* Body content */}
    <DialogFooter>
      {/* Action buttons */}
    </DialogFooter>
  </DialogContent>
</Dialog>
```

---

## Edge Cases and Error Handling

### Edge Case 1: Jog Started While Already Moving
**Scenario**: User presses jog button while motor moving to position from PositionControl

**Handling**:
- Backend: New moveTo() call replaces current target (existing behavior)
- Frontend: Jog buttons disabled when isMoving is true
- Result: Cannot jog while position movement active

**Code**: JogControls already disables buttons when isMoving (line 109, 124)

---

### Edge Case 2: Jog Held Until Limit Reached
**Scenario**: User holds jog button, motor reaches limit position

**Handling**:
- Backend: moveTo() targets limit position, motor stops at limit naturally
- Limit switch may trigger if physically wired (currently commented out per user)
- Frontend: Position updates show motor at limit
- User releases button: jogStop has no effect (motor already stopped)

**Result**: Safe behavior, no special handling needed

---

### Edge Case 3: Connection Lost During Jog
**Scenario**: WebSocket disconnects while jog button held

**Handling**:
- Frontend: Jog button disabled when !isConnected
- User must release and reconnect
- Backend: Motor continues to limit position (safe target)
- On reconnect: Fresh status shows current position

**Improvement** (optional): Add cleanup on disconnect to ensure jogStop attempted

---

### Edge Case 4: Config Dialog Open When Disconnected
**Scenario**: Connection drops while user editing config

**Handling**:
- Apply button already disabled when !isConnected
- User can still edit values
- Must reconnect before applying
- Form state preserved (can continue editing)

**Result**: No data loss, clear state via disabled button

---

### Edge Case 5: Multiple Clients Editing Config
**Scenario**: Two browsers open, both editing config simultaneously

**Handling**:
- Backend: Last write wins (both setConfig commands processed sequentially)
- Backend broadcasts updated config to all clients after each change
- Frontend: Dialog shows current config when opened
- If config changes while dialog open, user's edits take precedence on apply

**Potential Issue**: User A's changes may be overwritten by User B
**Mitigation**: Acceptable for single-user system, could add version checking if needed

---

### Edge Case 6: Invalid Config Values From Backend
**Scenario**: Backend sends config with values outside validation range

**Handling**:
- Frontend validation only prevents user input outside range
- If backend sends invalid values, form will show them
- Validation errors will appear if values outside range
- User can correct by entering valid values

**Unlikely**: Backend validates on setConfig, NVRAM won't have invalid values

---

### Edge Case 7: Rapid Jog Button Press/Release
**Scenario**: User rapidly taps jog button (press/release quickly)

**Handling**:
- Each press sends jogStart, each release sends jogStop
- Backend processes commands in order
- Motor may not have time to accelerate before stopping
- Result: Small incremental movements (similar to original behavior)

**Acceptable**: User error, natural behavior for rapid tapping

---

## Testing Strategy

### Unit Testing (Native Environment)
Not applicable - requires ESP32 hardware and WebSocket infrastructure for both features.

---

### Integration Testing (Hardware Required)

#### Test Scenario 1: Continuous Jog Forward
```
Setup: Motor at position 0, limits at -5000/+5000
Steps:
1. Open webapp in browser
2. Press and hold "Jog Forward" button
3. Observe motor movement
4. Release button after 2 seconds
5. Check position updates

Expected Results:
- Motor starts moving forward within 50ms
- Smooth continuous movement (no stuttering)
- Position updates show increasing values
- Motor stops within 50ms of release
- Final position > initial position
```

#### Test Scenario 2: Continuous Jog Backward
```
Setup: Motor at position 1000
Steps:
1. Press and hold "Jog Back" button
2. Observe motor movement
3. Release button after 2 seconds

Expected Results:
- Motor starts moving backward within 50ms
- Smooth continuous movement
- Position updates show decreasing values
- Motor stops within 50ms of release
- Final position < initial position
```

#### Test Scenario 3: Jog to Limit
```
Setup: Motor at position 4500, max limit at 5000
Steps:
1. Press and hold "Jog Forward"
2. Wait until motor reaches limit
3. Keep holding button for 1 more second
4. Release button

Expected Results:
- Motor moves forward to position 5000
- Motor stops at limit (doesn't overshoot)
- Continued button hold has no effect
- No errors or crashes
```

#### Test Scenario 4: Jog Stop on Mouse Leave
```
Setup: Motor at any position
Steps:
1. Press "Jog Forward" button
2. While motor moving, drag mouse off button (keep mouse button down)
3. Observe motor behavior

Expected Results:
- Motor starts moving on button press
- Motor stops when mouse leaves button
- Works same as releasing button
```

#### Test Scenario 5: Mobile Touch Jog
```
Setup: Open webapp on mobile device
Steps:
1. Touch and hold "Jog Forward" button
2. Observe motor movement
3. Release touch

Expected Results:
- Touch behaves same as mouse (continuous movement)
- Motor stops on touch release
- No accidental double-triggers
```

#### Test Scenario 6: Jog While Emergency Stop Active
```
Setup: Emergency stop triggered
Steps:
1. Try to press jog buttons
2. Observe button state and motor behavior

Expected Results:
- Jog buttons disabled (grayed out)
- Cannot click disabled buttons
- Motor does not move
```

#### Test Scenario 7: Open Config Dialog
```
Setup: Connected to controller
Steps:
1. Click Settings button in header
2. Observe dialog appearance

Expected Results:
- Dialog opens with overlay
- Background dims
- Form shows current config values
- All fields editable except limits
- Limit positions shown as read-only
```

#### Test Scenario 8: Edit and Apply Config
```
Setup: Config dialog open
Steps:
1. Change Max Speed from 8000 to 10000
2. Change Acceleration from 16000 to 20000
3. Toggle StealthChop from On to Off
4. Click Apply button
5. Wait for response

Expected Results:
- Input fields update as typed
- No validation errors shown
- Apply button enabled
- Dialog closes after apply
- Motor moves at new speed on next command
- Settings persisted (survive page reload)
```

#### Test Scenario 9: Config Validation - Invalid Speed
```
Setup: Config dialog open
Steps:
1. Enter Max Speed: 500 (below minimum)
2. Observe validation

Expected Results:
- Red error message appears: "Speed must be between 1000 and 12000"
- Input field gets red border
- Apply button disabled
```

#### Test Scenario 10: Config Validation - Invalid Acceleration
```
Setup: Config dialog open
Steps:
1. Enter Acceleration: 50000 (above maximum)
2. Observe validation

Expected Results:
- Red error message: "Acceleration must be between 1000 and 30000"
- Input field gets red border
- Apply button disabled
```

#### Test Scenario 11: Config Revert
```
Setup: Config dialog open, original values: Speed=8000, Accel=16000
Steps:
1. Change Speed to 10000
2. Change Accel to 20000
3. Click Revert button
4. Observe form values

Expected Results:
- Speed resets to 8000
- Accel resets to 16000
- All changes discarded
- Form matches current config
- No server communication
```

#### Test Scenario 12: Config Dialog Cancel
```
Setup: Config dialog open with unsaved changes
Steps:
1. Change Speed to 10000
2. Click X button or press Escape
3. Reopen dialog

Expected Results:
- Dialog closes immediately
- Changes discarded
- Reopened dialog shows original values
- No save confirmation (per user preference)
```

#### Test Scenario 13: Config Dialog Mobile View
```
Setup: Open webapp on mobile device
Steps:
1. Click Settings button
2. Observe dialog layout
3. Try editing fields with touch keyboard

Expected Results:
- Dialog fills screen on mobile
- Form fields spaced for touch
- Number keyboard appears for number inputs
- Switch is touch-friendly
- Can scroll if content doesn't fit
```

#### Test Scenario 14: Config Persistence After Reboot
```
Setup: Config dialog open
Steps:
1. Change Max Speed to 10000
2. Click Apply
3. Wait for success
4. Power cycle ESP32 controller
5. Reconnect webapp
6. Open config dialog

Expected Results:
- Dialog shows Speed = 10000
- Settings survived reboot
- NVRAM persistence working
```

---

## Acceptance Criteria Summary

### Feature 1: Continuous Jogging
- [ ] Press jog button → motor moves continuously
- [ ] Release jog button → motor stops immediately
- [ ] Works with mouse (desktop) and touch (mobile)
- [ ] Mouse leave stops jogging
- [ ] Disabled when emergency stop active
- [ ] Stops at limit positions safely
- [ ] Backend commands implemented (jogStart, jogStop)
- [ ] Frontend updated to use new commands
- [ ] No more stuttery incremental movement

### Feature 2: Motor Config UI
- [ ] Settings button in header opens dialog
- [ ] Dialog shows current config values
- [ ] Max Speed editable with validation (1000-12000)
- [ ] Acceleration editable with validation (1000-30000)
- [ ] StealthChop toggle switch works
- [ ] Limit positions shown but read-only
- [ ] Real-time validation with error messages
- [ ] Apply button disabled when invalid
- [ ] Revert button restores original values
- [ ] Apply sends changes to backend
- [ ] Config persists to NVRAM
- [ ] Mobile-responsive dialog
- [ ] All connected clients receive updates

---

## Assumptions

1. **Jog Speed**: 30% of maxSpeed is acceptable (can be adjusted in backend code if needed)
2. **Jog Target**: Using limit positions as jog targets is safe and acceptable
3. **Validation Ranges**: Estimated ranges (1000-12000, 1000-30000) will be validated during testing
4. **Limit Switches**: Currently commented out, jog safety relies on position limits only
5. **Single User**: Config editing conflicts not a concern (acceptable for single-user system)
6. **No Confirmation**: User okay with dialog closing on Apply without success message
7. **Network**: WebSocket connection reliable enough for real-time jogging
8. **Hardware**: User has controller available for testing both features

---

## Dependencies

### Backend Dependencies (No Changes)
All existing:
- `MotorController::moveTo()` - src/modules/MotorController/MotorController.h
- `MotorController::stop()` - src/modules/MotorController/MotorController.h
- `Configuration::getMaxSpeed()` - src/modules/Configuration/Configuration.h
- `Configuration::getMaxLimit()` - src/modules/Configuration/Configuration.h
- `Configuration::getMinLimit()` - src/modules/Configuration/Configuration.h
- WebSocket broadcast methods - src/modules/WebServer/WebServer.cpp

### Frontend Dependencies (Already Installed)
From `webapp/package.json`:
- React 18.3.1
- TypeScript 5.6.3
- Radix UI Dialog (@radix-ui/react-dialog 1.1.15)
- Radix UI Switch (@radix-ui/react-switch 1.2.6)
- Lucide React (icons - 0.469.0)
- Tailwind CSS (styling - 3.4.17)

### New Components Needed
- `webapp/src/components/MotorConfig/MotorConfigDialog.tsx` - NEW
- `webapp/src/components/ui/label.tsx` - MAY NEED (check if exists)

---

## Implementation Checklist

### Step 1: Backend - Add Jog Commands
- [ ] Open `src/modules/WebServer/WebServer.cpp`
- [ ] Find `handleWebSocketMessage()` function
- [ ] Add `jogStart` command handler after line 340
- [ ] Add `jogStop` command handler
- [ ] Test compilation: `pio run -e pico32`

### Step 2: Backend - Test Jog Commands
- [ ] Flash firmware to controller
- [ ] Open debug WebSocket in browser console
- [ ] Send test command: `{"command":"jogStart","direction":"forward"}`
- [ ] Verify motor moves continuously
- [ ] Send test command: `{"command":"jogStop"}`
- [ ] Verify motor stops
- [ ] Check serial logs for LOG_INFO messages

### Step 3: Frontend - Update Types
- [ ] Open `webapp/src/types/index.ts`
- [ ] Add JogStartCommand interface
- [ ] Add JogStopCommand interface
- [ ] Update ControlCommand union type
- [ ] Verify no TypeScript errors: `npm run build`

### Step 4: Frontend - Update Hook
- [ ] Open `webapp/src/hooks/useMotorController.tsx`
- [ ] Add `jogStart` method
- [ ] Add `jogStop` method
- [ ] Export both methods in return object
- [ ] Verify TypeScript compilation

### Step 5: Frontend - Update JogControls
- [ ] Open `webapp/src/components/MotorControl/JogControls.tsx`
- [ ] Remove jogIntervalRef and isJoggingRef
- [ ] Simplify handleMouseDown/Up functions
- [ ] Simplify handleTouchStart/End functions
- [ ] Remove setInterval/clearInterval logic
- [ ] Verify component compiles

### Step 6: Frontend - Update App.tsx Handlers
- [ ] Open `webapp/src/App.tsx`
- [ ] Simplify handleJogStart to call hook method
- [ ] Simplify handleJogStop to call hook method
- [ ] Add jogStart, jogStop to destructuring
- [ ] Verify compilation

### Step 7: Frontend - Test Jog Buttons
- [ ] Run webapp dev server: `npm run dev`
- [ ] Open in browser, connect to controller
- [ ] Test jog forward button (press and hold)
- [ ] Test jog backward button (press and hold)
- [ ] Test mouse leave behavior
- [ ] Test on mobile device (touch)
- [ ] Verify smooth continuous movement

### Step 8: Frontend - Create Config Dialog Component
- [ ] Create folder: `webapp/src/components/MotorConfig/`
- [ ] Create file: `MotorConfigDialog.tsx`
- [ ] Implement component per TR-6 specification
- [ ] Check if Label component exists, create if needed
- [ ] Verify TypeScript compilation
- [ ] Test component renders in isolation

### Step 9: Frontend - Add Settings Button
- [ ] Open `webapp/src/App.tsx`
- [ ] Import MotorConfigDialog component
- [ ] Import Settings icon from lucide-react
- [ ] Add configDialogOpen state
- [ ] Add Settings button to header
- [ ] Add MotorConfigDialog component
- [ ] Add updateConfig to destructuring
- [ ] Verify compilation

### Step 10: Frontend - Test Config Dialog
- [ ] Run webapp: `npm run dev`
- [ ] Click Settings button
- [ ] Verify dialog opens
- [ ] Test all form fields
- [ ] Test validation (enter invalid values)
- [ ] Test Revert button
- [ ] Test Apply button
- [ ] Verify mobile layout

### Step 11: Integration Testing
- [ ] Test jog buttons on hardware (all scenarios above)
- [ ] Test config dialog on hardware
- [ ] Verify config persistence (reboot test)
- [ ] Test on mobile device
- [ ] Check all edge cases listed above

### Step 12: Validation Tuning
- [ ] Research actual TMC2209 limits
- [ ] Test maxSpeed range on hardware
- [ ] Test acceleration range on hardware
- [ ] Update validation min/max if needed
- [ ] Document findings in code comments

### Step 13: Build and Deploy
- [ ] Build webapp: `npm run build`
- [ ] Copy dist to data folder
- [ ] Upload filesystem: `pio run -t uploadfs`
- [ ] Final firmware flash: `pio run -t upload`
- [ ] Test deployed version

### Step 14: Documentation
- [ ] Update CLAUDE.md with new features
- [ ] Update README.md WebSocket protocol section
- [ ] Add jogStart/jogStop to command list
- [ ] Document config UI in README
- [ ] Mark requirement session as complete

---

## Future Enhancements (Out of Scope)

These are explicitly NOT part of this requirement:

### Jog Related
1. **Variable Jog Speed**: Slider to adjust jog speed percentage
2. **Jog Step Mode**: Single-click buttons for fixed distance moves
3. **Jog Speed Presets**: Slow/Medium/Fast jog buttons
4. **Continuous Position Display**: Real-time position readout during jog

### Config Related
1. **Advanced Settings**: TMC2209 current, microsteps, StealthChop threshold
2. **Config Profiles**: Save/load multiple config presets
3. **Config Import/Export**: JSON file download/upload
4. **Config Changelog**: History of setting changes
5. **Manual Limit Entry**: Allow typing limit positions (currently read-only per user)
6. **Live Preview**: Show effect of changes before applying
7. **Units Conversion**: Display RPM alongside steps/sec

### General
1. **Multi-motor Support**: Control multiple motors simultaneously
2. **Macro Recording**: Record and replay movement sequences
3. **Position Bookmarks**: Save named positions beyond presets

---

## References

### Codebase Files Analyzed
- `src/modules/WebServer/WebServer.cpp` (lines 280-422) - WebSocket handler
- `src/modules/WebServer/WebServer.h` (lines 1-69) - Class definition
- `src/modules/Configuration/Configuration.h` (lines 1-54) - Config structure
- `src/modules/MotorController/MotorController.h` - Motor control API
- `webapp/src/components/MotorControl/JogControls.tsx` - Current jog implementation
- `webapp/src/components/MotorControl/PositionControl.tsx` - UI pattern reference
- `webapp/src/hooks/useMotorController.tsx` - WebSocket hook
- `webapp/src/types/index.ts` - Type definitions
- `webapp/src/App.tsx` - Main app structure
- `webapp/package.json` - Dependencies

### Requirements Documents
- `00-initial-request.md` - User's initial feature request
- `01-discovery-questions.md` - High-level discovery questions
- `02-discovery-answers.md` - User's answers and clarifications
- `03-context-findings.md` - Detailed codebase analysis
- `04-detail-questions.md` - Technical detail questions
- `05-detail-answers.md` - Technical answers and decisions

### External Documentation
- README.md - Project overview, API specification
- CLAUDE.md - Project history and implementation notes
- TMC2209 Datasheet - Stepper driver specifications
- MT6816 Datasheet - Encoder specifications
- Radix UI Documentation - Dialog, Switch components

---

## Sign-off

**Requirements Gathered By**: Claude Code
**Approved By**: User (Costyn)
**Date**: 2025-10-02
**Status**: ✅ Ready for Implementation

All discovery and detail questions answered. Codebase thoroughly analyzed. Implementation strategy defined with code examples. No blocking issues identified.

**Estimated Implementation Time**:
- Backend (jog commands): 15-20 minutes
- Frontend (jog buttons): 15-20 minutes
- Frontend (config dialog): 45-60 minutes
- Testing and validation: 30-45 minutes
- **Total**: ~2-2.5 hours

**Risk Level**: LOW
- Backend changes are minimal (reuse existing methods)
- Frontend follows established patterns
- No breaking changes to existing functionality
- Well-defined validation rules
- User-approved design decisions

**Notes**:
- Validation ranges need hardware testing to finalize
- Limit switch code remains commented out per user request
- Config dialog uses simple form approach (no preview)
- True continuous jogging approach selected over incremental moves
