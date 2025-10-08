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
  const [freewheelAfterMove, setFreewheelAfterMove] = useState(currentConfig.freewheelAfterMove)
  const [minLimit, setMinLimit] = useState<number | string>(currentConfig.minLimit)
  const [maxLimit, setMaxLimit] = useState<number | string>(currentConfig.maxLimit)

  // Validation state
  const [maxSpeedError, setMaxSpeedError] = useState<string | null>(null)
  const [accelerationError, setAccelerationError] = useState<string | null>(null)
  const [minLimitError, setMinLimitError] = useState<string | null>(null)
  const [maxLimitError, setMaxLimitError] = useState<string | null>(null)

  // Reset form when dialog opens or config changes
  useEffect(() => {
    if (open) {
      setMaxSpeed(currentConfig.maxSpeed)
      setAcceleration(currentConfig.acceleration)
      setUseStealthChop(currentConfig.useStealthChop)
      setFreewheelAfterMove(currentConfig.freewheelAfterMove)
      setMinLimit(currentConfig.minLimit)
      setMaxLimit(currentConfig.maxLimit)
      setMaxSpeedError(null)
      setAccelerationError(null)
      setMinLimitError(null)
      setMaxLimitError(null)
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

  const validateLimits = (min: number, max: number) => {
    if (min >= max) {
      return { min: "Min must be less than max", max: "Max must be greater than min" }
    }
    return { min: null, max: null }
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

  const handleMinLimitChange = (value: string) => {
    const numValue = parseInt(value)
    if (value === '' || isNaN(numValue)) {
      setMinLimit(value)
      setMinLimitError(value === '' ? 'Required' : 'Invalid number')
      setMaxLimitError(null)
    } else {
      setMinLimit(numValue)
      const maxLimitNum = typeof maxLimit === 'string' ? parseInt(maxLimit) : maxLimit
      if (!isNaN(maxLimitNum)) {
        const errors = validateLimits(numValue, maxLimitNum)
        setMinLimitError(errors.min)
        setMaxLimitError(errors.max)
      } else {
        setMinLimitError(null)
      }
    }
  }

  const handleMaxLimitChange = (value: string) => {
    const numValue = parseInt(value)
    if (value === '' || isNaN(numValue)) {
      setMaxLimit(value)
      setMaxLimitError(value === '' ? 'Required' : 'Invalid number')
      setMinLimitError(null)
    } else {
      setMaxLimit(numValue)
      const minLimitNum = typeof minLimit === 'string' ? parseInt(minLimit) : minLimit
      if (!isNaN(minLimitNum)) {
        const errors = validateLimits(minLimitNum, numValue)
        setMinLimitError(errors.min)
        setMaxLimitError(errors.max)
      } else {
        setMaxLimitError(null)
      }
    }
  }

  // Check if form is valid (must have values and pass validation)
  const isFormValid = maxSpeed > 0 && acceleration > 0 && !maxSpeedError && !accelerationError && !minLimitError && !maxLimitError

  // Check if form has changes
  const minLimitNum = typeof minLimit === 'string' ? parseInt(minLimit) : minLimit
  const maxLimitNum = typeof maxLimit === 'string' ? parseInt(maxLimit) : maxLimit
  const hasChanges =
    maxSpeed !== currentConfig.maxSpeed ||
    acceleration !== currentConfig.acceleration ||
    useStealthChop !== currentConfig.useStealthChop ||
    freewheelAfterMove !== currentConfig.freewheelAfterMove ||
    (!isNaN(minLimitNum) && minLimitNum !== currentConfig.minLimit) ||
    (!isNaN(maxLimitNum) && maxLimitNum !== currentConfig.maxLimit)

  // Handle revert
  const handleRevert = () => {
    setMaxSpeed(currentConfig.maxSpeed)
    setAcceleration(currentConfig.acceleration)
    setUseStealthChop(currentConfig.useStealthChop)
    setFreewheelAfterMove(currentConfig.freewheelAfterMove)
    setMinLimit(currentConfig.minLimit)
    setMaxLimit(currentConfig.maxLimit)
    setMaxSpeedError(null)
    setAccelerationError(null)
    setMinLimitError(null)
    setMaxLimitError(null)
  }

  // Handle apply
  const handleApply = () => {
    // Always close dialog, but only apply changes if form is valid and has changes
    if (isFormValid && hasChanges) {
      const changes: Partial<Omit<MotorConfig, 'type'>> = {}
      if (maxSpeed !== currentConfig.maxSpeed) changes.maxSpeed = maxSpeed
      if (acceleration !== currentConfig.acceleration) changes.acceleration = acceleration
      if (useStealthChop !== currentConfig.useStealthChop) changes.useStealthChop = useStealthChop
      if (freewheelAfterMove !== currentConfig.freewheelAfterMove) changes.freewheelAfterMove = freewheelAfterMove
      if (!isNaN(minLimitNum) && minLimitNum !== currentConfig.minLimit) changes.minLimit = minLimitNum
      if (!isNaN(maxLimitNum) && maxLimitNum !== currentConfig.maxLimit) changes.maxLimit = maxLimitNum

      onApply(changes)
    }
    onOpenChange(false)
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="sm:max-w-[425px]" onOpenAutoFocus={(e) => e.preventDefault()}>
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

          {/* Freewheel Mode */}
          <div className="grid gap-2">
            <Label htmlFor="freewheel">Freewheel After Movement</Label>
            <div className="flex items-center gap-2">
              <Switch
                id="freewheel"
                checked={freewheelAfterMove}
                onCheckedChange={setFreewheelAfterMove}
              />
              <span className="text-sm">
                {freewheelAfterMove ? "Enabled (Motor spins freely)" : "Disabled (Holds position)"}
              </span>
            </div>
            <p className="text-xs text-muted-foreground">
              Enable to let motor spin freely after movement completes. Disable to hold position (uses power).
            </p>
          </div>

          {/* Limit Positions */}
          <div className="grid gap-2 pt-4 border-t">
            <Label>Limit Positions</Label>
            <div className="grid grid-cols-2 gap-2">
              <div className="grid gap-2">
                <Label htmlFor="minLimit" className="text-sm">Min Limit</Label>
                <div className="flex items-center gap-2">
                  <Input
                    id="minLimit"
                    type="number"
                    value={minLimit}
                    onChange={(e) => handleMinLimitChange(e.target.value)}
                    className={minLimitError ? "border-red-500" : ""}
                  />
                  <span className="text-sm text-muted-foreground">steps</span>
                </div>
                {minLimitError && (
                  <p className="text-sm text-red-500">{minLimitError}</p>
                )}
              </div>
              <div className="grid gap-2">
                <Label htmlFor="maxLimit" className="text-sm">Max Limit</Label>
                <div className="flex items-center gap-2">
                  <Input
                    id="maxLimit"
                    type="number"
                    value={maxLimit}
                    onChange={(e) => handleMaxLimitChange(e.target.value)}
                    className={maxLimitError ? "border-red-500" : ""}
                  />
                  <span className="text-sm text-muted-foreground">steps</span>
                </div>
                {maxLimitError && (
                  <p className="text-sm text-red-500">{maxLimitError}</p>
                )}
              </div>
            </div>
            <p className="text-xs text-muted-foreground">
              Set manually or via physical limit switches. Limit switches will override these values if triggered.
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
          >
            Apply
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  )
}
