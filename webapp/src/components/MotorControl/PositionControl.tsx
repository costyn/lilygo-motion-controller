import React, { useState } from 'react'
import { Button } from '@/components/ui/button'
import { Input } from '@/components/ui/input'
import { Slider } from '@/components/ui/slider'
import { Progress } from '@/components/ui/progress'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { validatePosition, formatPosition } from '@/lib/utils'
import type { MotorConfig } from '@/types'
import { Target, AlertCircle } from 'lucide-react'

interface PositionControlProps {
  isConnected: boolean
  isMoving: boolean
  emergencyStop: boolean
  currentPosition: number
  motorConfig: MotorConfig
  onMoveTo: (position: number, speed: number) => void
}

export function PositionControl({
  isConnected,
  isMoving,
  emergencyStop,
  currentPosition,
  motorConfig,
  onMoveTo
}: PositionControlProps) {
  const [targetPosition, setTargetPosition] = useState([motorConfig.minLimit])
  const [targetSpeed, setTargetSpeed] = useState('50') // Default to 50% of max speed
  const [validationError, setValidationError] = useState('')

  // Calculate position progress
  const range = motorConfig.maxLimit - motorConfig.minLimit
  const progress = range > 0
    ? ((currentPosition - motorConfig.minLimit) / range) * 100
    : 0
  const clampedProgress = Math.max(0, Math.min(100, progress))

  const handlePositionChange = (value: number[]) => {
    setTargetPosition(value)
    setValidationError('')
  }

  const handleSpeedChange = (value: string) => {
    setTargetSpeed(value)
  }

  const handleMove = () => {
    const position = targetPosition[0]
    const speedPercent = parseInt(targetSpeed, 10)

    // Validation
    if (isNaN(speedPercent) || speedPercent < 1 || speedPercent > 100) {
      setValidationError('Speed must be between 1-100%')
      return
    }

    const actualSpeed = Math.round((speedPercent / 100) * motorConfig.maxSpeed)
    onMoveTo(position, actualSpeed)
  }

  const handleQuickPosition = (percentage: number) => {
    const range = motorConfig.maxLimit - motorConfig.minLimit
    const position = motorConfig.minLimit + Math.round((percentage / 100) * range)
    const speedPercent = parseInt(targetSpeed, 10) || 50
    const actualSpeed = Math.round((speedPercent / 100) * motorConfig.maxSpeed)

    // Update slider position and move motor
    setTargetPosition([position])
    onMoveTo(position, actualSpeed)
  }

  // Handle slider commit (when user stops dragging)
  const handleSliderCommit = (value: number[]) => {
    const position = value[0]
    const speedPercent = parseInt(targetSpeed, 10) || 50
    const actualSpeed = Math.round((speedPercent / 100) * motorConfig.maxSpeed)

    onMoveTo(position, actualSpeed)
  }

  const controlsDisabled = !isConnected || emergencyStop

  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="text-lg">Position Control</CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* Current Position Display */}
        <div className="space-y-2 pb-2 border-b">
          <div className="flex justify-between items-center">
            <span className="text-sm font-medium">Current Position</span>
            <span className="text-sm font-mono text-muted-foreground">
              {formatPosition(currentPosition)} steps
            </span>
          </div>
          <div className="flex items-center gap-3">
            <Progress value={clampedProgress} className="flex-1" />
            <span className="text-xs font-medium text-muted-foreground min-w-[3rem] text-right">
              {clampedProgress.toFixed(0)}%
            </span>
          </div>
        </div>

        {/* Position Slider */}
        <div>
          <div className="flex justify-between items-center mb-2">
            <label className="text-sm font-medium">
              Target Position
            </label>
            <span className="text-sm text-muted-foreground font-mono">
              {formatPosition(targetPosition[0])}
            </span>
          </div>
          <div className="px-2">
            <Slider
              value={targetPosition}
              onValueChange={handlePositionChange}
              onValueCommit={handleSliderCommit}
              min={motorConfig.minLimit}
              max={motorConfig.maxLimit}
              step={1}
              disabled={controlsDisabled}
              className="w-full"
            />
          </div>
          <div className="flex justify-between text-xs text-muted-foreground mt-1">
            <span>{formatPosition(motorConfig.minLimit)}</span>
            <span>{formatPosition(motorConfig.maxLimit)}</span>
          </div>
        </div>

        {/* Speed Input */}
        <div>
          <label htmlFor="target-speed" className="text-sm font-medium mb-2 block">
            Speed (1-100% of {motorConfig.maxSpeed.toLocaleString()} steps/s)
          </label>
          <Input
            id="target-speed"
            type="number"
            placeholder="Speed %..."
            value={targetSpeed}
            onChange={(e) => handleSpeedChange(e.target.value)}
            disabled={controlsDisabled}
            min={1}
            max={100}
          />
        </div>

        {/* Validation Error */}
        {validationError && (
          <div className="flex items-center gap-2 text-red-600 text-sm">
            <AlertCircle className="h-4 w-4" />
            {validationError}
          </div>
        )}

        {/* Info Text */}
        <div className="text-xs text-muted-foreground text-center py-1">
          Drag slider or use quick positions. Motor moves when you release the slider.
        </div>

        {/* Quick Position Buttons */}
        <div className="border-t pt-4">
          <div className="text-sm font-medium mb-2">Quick Positions:</div>
          <div className="grid grid-cols-5 gap-1">
            {[0, 25, 50, 75, 100].map((percentage) => (
              <Button
                key={percentage}
                variant="outline"
                size="sm"
                onClick={() => handleQuickPosition(percentage)}
                disabled={controlsDisabled || isMoving}
                className="text-xs"
              >
                {percentage}%
              </Button>
            ))}
          </div>
        </div>
      </CardContent>
    </Card>
  )
}