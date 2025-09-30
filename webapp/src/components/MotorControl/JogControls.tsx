import React, { useCallback, useRef } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { ChevronLeft, ChevronRight, Square, RotateCcw, ArrowLeft, ArrowRight } from 'lucide-react'

interface JogControlsProps {
  isConnected: boolean
  isMoving: boolean
  emergencyStop: boolean
  onJogStart: (direction: 'forward' | 'backward') => void
  onJogStop: () => void
  onEmergencyStop: () => void
  onClearEmergencyStop: () => void
  onMoveToLimit: (limit: 'min' | 'max') => void
}

export function JogControls({
  isConnected,
  isMoving,
  emergencyStop,
  onJogStart,
  onJogStop,
  onEmergencyStop,
  onClearEmergencyStop,
  onMoveToLimit
}: JogControlsProps) {
  const jogIntervalRef = useRef<NodeJS.Timeout | null>(null)
  const isJoggingRef = useRef(false)

  const startJog = useCallback((direction: 'forward' | 'backward') => {
    if (!isConnected || emergencyStop || isJoggingRef.current) return

    isJoggingRef.current = true
    onJogStart(direction)

    // Continue jogging while button is held
    jogIntervalRef.current = setInterval(() => {
      onJogStart(direction)
    }, 100) // Send jog commands every 100ms
  }, [isConnected, emergencyStop, onJogStart])

  const stopJog = useCallback(() => {
    if (jogIntervalRef.current) {
      clearInterval(jogIntervalRef.current)
      jogIntervalRef.current = null
    }

    if (isJoggingRef.current) {
      isJoggingRef.current = false
      onJogStop()
    }
  }, [onJogStop])

  const handleMouseDown = (direction: 'forward' | 'backward') => {
    startJog(direction)
  }

  const handleMouseUp = () => {
    stopJog()
  }

  // Touch events for mobile
  const handleTouchStart = (direction: 'forward' | 'backward') => {
    startJog(direction)
  }

  const handleTouchEnd = () => {
    stopJog()
  }

  const controlsDisabled = !isConnected || emergencyStop

  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="text-lg">Motion Controls</CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* Emergency Stop / Reset */}
        <div className="flex gap-2">
          {emergencyStop ? (
            <Button
              variant="outline"
              onClick={onClearEmergencyStop}
              disabled={!isConnected}
              className="flex-1"
            >
              <RotateCcw className="h-4 w-4 mr-2" />
              Reset E-Stop
            </Button>
          ) : (
            <Button
              variant="destructive"
              onClick={onEmergencyStop}
              disabled={!isConnected}
              className="flex-1"
            >
              <Square className="h-4 w-4 mr-2" />
              Emergency Stop
            </Button>
          )}
        </div>

        {/* Jog Controls */}
        {/* <div className="grid grid-cols-2 gap-2">
          <Button
            variant="outline"
            size="touch"
            disabled={controlsDisabled || isMoving}
            onMouseDown={() => handleMouseDown('backward')}
            onMouseUp={handleMouseUp}
            onMouseLeave={handleMouseUp}
            onTouchStart={() => handleTouchStart('backward')}
            onTouchEnd={handleTouchEnd}
            className="h-16 touch-button select-none"
          >
            <ArrowLeft className="h-5 w-5 mr-2" />
            <span>Jog Back</span>
          </Button>

          <Button
            variant="outline"
            size="touch"
            disabled={controlsDisabled || isMoving}
            onMouseDown={() => handleMouseDown('forward')}
            onMouseUp={handleMouseUp}
            onMouseLeave={handleMouseUp}
            onTouchStart={() => handleTouchStart('forward')}
            onTouchEnd={handleTouchEnd}
            className="h-16 touch-button select-none"
          >
            <span>Jog Forward</span>
            <ArrowRight className="h-5 w-5 ml-2" />
          </Button>
        </div> */}

        {/* Move to Limits */}
        <div className="border-t pt-4">
          <div className="text-sm font-medium mb-2">Move to Limits:</div>
          <div className="grid grid-cols-2 gap-2">
            <Button
              variant="secondary"
              disabled={controlsDisabled || isMoving}
              onClick={() => onMoveToLimit('min')}
            >
              Min Limit
            </Button>
            <Button
              variant="secondary"
              disabled={controlsDisabled || isMoving}
              onClick={() => onMoveToLimit('max')}
            >
              Max Limit
            </Button>
          </div>
        </div>
      </CardContent>
    </Card>
  )
}