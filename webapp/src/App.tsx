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
import { ExternalLink, RefreshCw, Settings } from 'lucide-react'

function AppContent() {
  const [configDialogOpen, setConfigDialogOpen] = useState(false)

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

  const handleJogStart = (direction: 'forward' | 'backward') => {
    jogStart(direction)
  }

  const handleJogStop = () => {
    jogStop()
  }

  const handleMoveToLimit = (limit: 'min' | 'max') => {
    const targetPosition = limit === 'min' ? motorConfig.minLimit : motorConfig.maxLimit
    const speed = Math.round(motorConfig.maxSpeed * 0.5) // 50% speed for limit moves
    moveTo(targetPosition, speed)
  }

  return (
    <div className="min-h-screen bg-background">
      {/* Header */}
      <header className="border-b">
        <div className="container mx-auto px-4 py-4 flex items-center justify-between">
          <h1 className="text-2xl font-bold">LilyGo Motion Controller</h1>
          <div className="flex items-center gap-2">
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
      </header>

      {/* Main Content */}
      <main className="container mx-auto px-4 py-6">
        <div className="grid gap-6 md:grid-cols-1 lg:grid-cols-2">
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