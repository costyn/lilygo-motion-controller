import { useRef } from 'react'
import { Button } from '@/components/ui/button'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { Square, RotateCcw, ArrowLeft, ArrowRight } from 'lucide-react'

interface JogControlsProps {
  isConnected: boolean
  isMoving: boolean
  emergencyStop: boolean
  jogSpeed: number
  onJogStart: (direction: 'forward' | 'backward', speed: number) => void
  onJogStop: () => void
  onEmergencyStop: () => void
  onClearEmergencyStop: () => void
  onMoveToLimit: (limit: 'min' | 'max') => void
}

export function JogControls({
  isConnected,
  isMoving,
  emergencyStop,
  jogSpeed,
  onJogStart,
  onJogStop,
  onEmergencyStop,
  onClearEmergencyStop,
  onMoveToLimit
}: JogControlsProps) {
  const isJoggingRef = useRef(false)
  const activeDirectionRef = useRef<'forward' | 'backward' | null>(null)
  const eventSourceRef = useRef<'touch' | 'mouse' | null>(null)
  const touchIdentifierRef = useRef<number | null>(null)

  const handleMouseDown = (e: React.MouseEvent, direction: 'forward' | 'backward') => {
    // Ignore mouse events if touch events were just used
    if (eventSourceRef.current === 'touch') return
    if (!isConnected || emergencyStop) return

    eventSourceRef.current = 'mouse'
    isJoggingRef.current = true
    activeDirectionRef.current = direction
    onJogStart(direction, jogSpeed)
  }

  const handleMouseUp = (e: React.MouseEvent) => {
    // Only process if this was initiated by mouse
    if (eventSourceRef.current !== 'mouse' || !isJoggingRef.current) return
    if (!isConnected) return

    isJoggingRef.current = false
    activeDirectionRef.current = null
    onJogStop()
  }

  const handleMouseLeave = (e: React.MouseEvent) => {
    // Only process if this was initiated by mouse and we're currently jogging
    if (eventSourceRef.current !== 'mouse' || !isJoggingRef.current) return
    if (!isConnected) return

    isJoggingRef.current = false
    activeDirectionRef.current = null
    onJogStop()
  }

  const handleTouchStart = (e: React.TouchEvent, direction: 'forward' | 'backward') => {
    if (!isConnected || emergencyStop) return

    // Store the first touch identifier
    if (e.touches.length > 0) {
      touchIdentifierRef.current = e.touches[0].identifier
    }

    eventSourceRef.current = 'touch'
    isJoggingRef.current = true
    activeDirectionRef.current = direction
    onJogStart(direction, jogSpeed)
  }

  const handleTouchEnd = (e: React.TouchEvent) => {
    // Only process if this was initiated by touch
    if (eventSourceRef.current !== 'touch' || !isJoggingRef.current) return
    if (!isConnected) return

    // Verify this is the same touch that started
    const touchEnded = e.changedTouches[0]
    if (touchIdentifierRef.current !== null && touchEnded.identifier !== touchIdentifierRef.current) {
      return
    }

    isJoggingRef.current = false
    activeDirectionRef.current = null
    touchIdentifierRef.current = null
    onJogStop()

    // Reset event source after a brief delay to prevent mouse event interference
    setTimeout(() => {
      eventSourceRef.current = null
    }, 300)
  }

  const handleTouchCancel = (e: React.TouchEvent) => {
    // Handle touch cancellation (e.g., when touch is interrupted)
    if (eventSourceRef.current !== 'touch') return

    if (isJoggingRef.current) {
      isJoggingRef.current = false
      activeDirectionRef.current = null
      touchIdentifierRef.current = null
      onJogStop()
    }

    eventSourceRef.current = null
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
        <div className="grid grid-cols-2 gap-2">
          <Button
            variant="outline"
            disabled={controlsDisabled}
            onMouseDown={(e) => handleMouseDown(e, 'backward')}
            onMouseUp={handleMouseUp}
            onMouseLeave={handleMouseLeave}
            onTouchStart={(e) => handleTouchStart(e, 'backward')}
            onTouchEnd={handleTouchEnd}
            onTouchCancel={handleTouchCancel}
            className="h-16 touch-button select-none"
          >
            <ArrowLeft className="h-5 w-5 mr-2" />
            <span>Jog Back</span>
          </Button>

          <Button
            variant="outline"
            disabled={controlsDisabled}
            onMouseDown={(e) => handleMouseDown(e, 'forward')}
            onMouseUp={handleMouseUp}
            onMouseLeave={handleMouseLeave}
            onTouchStart={(e) => handleTouchStart(e, 'forward')}
            onTouchEnd={handleTouchEnd}
            onTouchCancel={handleTouchCancel}
            className="h-16 touch-button select-none"
          >
            <span>Jog Forward</span>
            <ArrowRight className="h-5 w-5 ml-2" />
          </Button>
        </div>

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
