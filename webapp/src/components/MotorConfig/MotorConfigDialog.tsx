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
    if (value < 100 || value > 100000) {
      return "Speed must be between 100 and 100,000 steps/sec"
    }
    return null
  }

  const validateAcceleration = (value: number) => {
    if (value < 100 || value > 500000) {
      return "Acceleration must be between 100 and 500,000 steps/sec²"
    }
    return null
  }

  // Handle input changes with validation
  const handleMaxSpeedChange = (value: string) => {
    const numValue = parseInt(value)
    if (value === '' || isNaN(numValue)) {
      // Allow empty/invalid input during editing, but set error
      setMaxSpeed(value === '' ? 0 : numValue)
      setMaxSpeedError(value === '' ? 'Required' : 'Invalid number')
    } else {
      setMaxSpeed(numValue)
      setMaxSpeedError(validateMaxSpeed(numValue))
    }
  }

  const handleAccelerationChange = (value: string) => {
    const numValue = parseInt(value)
    if (value === '' || isNaN(numValue)) {
      // Allow empty/invalid input during editing, but set error
      setAcceleration(value === '' ? 0 : numValue)
      setAccelerationError(value === '' ? 'Required' : 'Invalid number')
    } else {
      setAcceleration(numValue)
      setAccelerationError(validateAcceleration(numValue))
    }
  }

  // Check if form is valid (must have values and pass validation)
  const isFormValid = maxSpeed > 0 && acceleration > 0 && !maxSpeedError && !accelerationError

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
              Maximum motor speed. Exceeding your motor's capability may cause skipped steps. Consult your motor datasheet.
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
              How quickly motor reaches target speed. Exceeding limits may cause skipped steps during acceleration.
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
