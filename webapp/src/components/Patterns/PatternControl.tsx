import { Button } from '@/components/ui/button'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import type { Pattern } from '@/types'

interface PatternControlProps {
  isConnected: boolean
  isMoving: boolean
  emergencyStop: boolean
  needsCalibration?: boolean
  patterns: Pattern[]
  runningPattern: string // Empty when none is running
  round: number
  repeat: number // 0 for until stopped
  onRun: (index: number) => void
  onStop: () => void
  onEdit: () => void
}

// Patterns run on the controller, so they carry on if this page closes
export function PatternControl({
  isConnected,
  isMoving,
  emergencyStop,
  needsCalibration = false,
  patterns,
  runningPattern,
  round,
  repeat,
  onRun,
  onStop,
  onEdit
}: PatternControlProps) {
  const isRunning = runningPattern !== ''
  const startDisabled = !isConnected || emergencyStop || needsCalibration || isMoving || isRunning

  return (
    <Card>
      <CardHeader className="pb-3">
        <div className="flex items-center justify-between">
          <CardTitle className="text-lg">Patterns</CardTitle>
          <Button variant="outline" size="sm" onClick={onEdit} disabled={!isConnected || isRunning}>
            Edit
          </Button>
        </div>
      </CardHeader>
      <CardContent className="space-y-3">
        {patterns.length === 0 ? (
          <p className="text-sm text-muted-foreground">No patterns yet. Add some with Edit.</p>
        ) : (
          <div className="grid grid-cols-2 gap-2">
            {patterns.map((pattern, index) => (
              <Button
                key={`${index}-${pattern.name}`}
                variant={runningPattern === pattern.name ? 'default' : 'outline'}
                onClick={() => onRun(index)}
                disabled={startDisabled}
              >
                {pattern.name}
              </Button>
            ))}
          </div>
        )}

        {isRunning && (
          <div className="flex items-center gap-2">
            <span className="flex-1 text-sm">
              {runningPattern}: round {round}{repeat > 0 ? ` of ${repeat}` : ''}
            </span>
            <Button variant="destructive" onClick={onStop} disabled={!isConnected}>
              Stop
            </Button>
          </div>
        )}

        {needsCalibration && (
          <p className="text-xs text-muted-foreground">Calibrate first to run a pattern.</p>
        )}
      </CardContent>
    </Card>
  )
}
