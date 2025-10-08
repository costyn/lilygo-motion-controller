import React, { useState } from 'react'
import { ThemeProvider } from './components/ThemeProvider'
import { ThemeToggle } from './components/ui/theme-toggle'
import { MotorStatus } from './components/MotorControl/MotorStatus'
import { JogControls } from './components/MotorControl/JogControls'
import { PositionControl } from './components/MotorControl/PositionControl'
import DebugConsole from './components/DebugConsole/DebugConsole'
import { MotorConfigDialog } from './components/MotorConfig/MotorConfigDialog'
import { useMotorController } from './hooks/useMotorController'
import { Button } from './components/ui/button'
import { Alert, AlertTitle, AlertDescription } from './components/ui/alert'
import { ExternalLink, RefreshCw, Settings, AlertTriangle } from 'lucide-react'

// Build-time constants injected by Vite
declare const __BUILD_TIME__: string

function AppContent() {
  const [configDialogOpen, setConfigDialogOpen] = useState(false)
  const [showLimitWarning, setShowLimitWarning] = useState(false)

  const {
    connectionState,
    isConnected,
    motorStatus,
    motorConfig,
    moveTo,
    emergencyStop,
    clearEmergencyStop,
    updateConfig,
    jogStart,
    jogStop,
    manualReconnect
  } = useMotorController()

  // Shared jog speed state (30% of max speed by default)
  const [jogSpeed, setJogSpeed] = useState(Math.round(motorConfig.maxSpeed * 0.3))

  // Update jog speed when motor config changes
  React.useEffect(() => {
    setJogSpeed(Math.round(motorConfig.maxSpeed * 0.3))
  }, [motorConfig.maxSpeed])

  // Check if limits are equal and show warning
  React.useEffect(() => {
    if (isConnected && motorConfig.minLimit === motorConfig.maxLimit) {
      setShowLimitWarning(true)
    } else {
      setShowLimitWarning(false)
    }
  }, [isConnected, motorConfig.minLimit, motorConfig.maxLimit])

  const handleJogStart = (direction: 'forward' | 'backward', speed: number) => {
    jogStart(direction, speed)
  }

  const handleJogStop = () => {
    jogStop()
  }

  const handleMoveToLimit = (limit: 'min' | 'max') => {
    const targetPosition = limit === 'min' ? motorConfig.minLimit : motorConfig.maxLimit
    const speed = jogSpeed // Use jog speed for limit moves
    moveTo(targetPosition, speed)
  }

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="border-b">
        <div className="container mx-auto px-4 py-4">
          <div className="flex flex-col sm:flex-row items-start sm:items-center justify-between gap-3">
            <h1 className="text-2xl font-bold">LilyGo Motion Controller</h1>
            <div className="flex items-center gap-2 flex-wrap">
              {!isConnected && (
                <Button
                  variant="outline"
                  size="sm"
                  onClick={manualReconnect}
                  disabled={connectionState.isConnecting}
                >
                  <RefreshCw className={`h-4 w-4 mr-2 ${connectionState.isConnecting ? 'animate-spin' : ''
                    }`} />
                  Reconnect
                </Button>
              )}
              <Button
                variant="outline"
                size="sm"
                onClick={() => setConfigDialogOpen(true)}
              >
                <Settings className="h-4 w-4 mr-2" />
                Settings
              </Button>
              <Button
                variant="outline"
                size="sm"
                asChild
              >
                <a
                  href="/update"
                  target="_blank"
                  rel="noopener noreferrer"
                  className="flex items-center"
                >
                  <ExternalLink className="h-4 w-4 mr-2" />
                  OTA Update
                </a>
              </Button>
              <ThemeToggle />
            </div>
          </div>
        </div>
      </header>

      {/* Main Content */}
      <main className="container mx-auto px-4 py-6">
        <div className="grid gap-6 md:grid-cols-1 lg:grid-cols-2">
          {/* Limit Warning */}
          {showLimitWarning && (
            <div className="lg:col-span-2">
              <Alert variant="warning" onDismiss={() => setShowLimitWarning(false)}>
                <AlertTriangle className="h-4 w-4" />
                <AlertTitle>Motor Movement Disabled</AlertTitle>
                <AlertDescription>
                  The minimum and maximum positions are equal ({motorConfig.minLimit} steps).
                  The motor cannot move until limits are configured. Please open{' '}
                  <button
                    onClick={() => setConfigDialogOpen(true)}
                    className="underline font-medium hover:no-underline"
                  >
                    Settings
                  </button>
                  {' '}to set the limit positions.
                </AlertDescription>
              </Alert>
            </div>
          )}

          {/* Motor Status */}
          <div className="lg:col-span-2">
            <MotorStatus
              motorStatus={motorStatus}
              connectionState={connectionState}
            />
          </div>

          {/* Jog Controls */}
          <JogControls
            isConnected={isConnected}
            isMoving={motorStatus.isMoving}
            emergencyStop={motorStatus.emergencyStop}
            jogSpeed={jogSpeed}
            onJogStart={handleJogStart}
            onJogStop={handleJogStop}
            onEmergencyStop={emergencyStop}
            onClearEmergencyStop={clearEmergencyStop}
            onMoveToLimit={handleMoveToLimit}
          />

          {/* Position Control */}
          <PositionControl
            isConnected={isConnected}
            isMoving={motorStatus.isMoving}
            emergencyStop={motorStatus.emergencyStop}
            currentPosition={motorStatus.position}
            motorConfig={motorConfig}
            jogSpeed={jogSpeed}
            onJogSpeedChange={setJogSpeed}
            onMoveTo={moveTo}
          />
        </div>
        <DebugConsole />
      </main>

      {/* Footer */}
      <footer className="border-t mt-8">
        <div className="container mx-auto px-4 py-4 text-center text-sm text-muted-foreground">
          <p>
            LilyGo Motion Controller WebApp - Connect via{' '}
            <code className="bg-muted px-1 rounded">lilygo-motioncontroller.local</code>
          </p>
          <p className="text-xs mt-1 opacity-60">
            Build: {new Date(__BUILD_TIME__).toLocaleString()}
          </p>
        </div>
      </footer>

      {/* Motor Config Dialog */}
      <MotorConfigDialog
        open={configDialogOpen}
        onOpenChange={setConfigDialogOpen}
        currentConfig={motorConfig}
        onApply={updateConfig}
        isConnected={isConnected}
      />
    </div>
  )
}

function App() {
  return (
    <ThemeProvider defaultTheme="system">
      <AppContent />
    </ThemeProvider>
  )
}

export default App