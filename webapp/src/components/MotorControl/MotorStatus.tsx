import React from 'react'
import { Card, CardContent, CardHeader, CardTitle } from '@/components/ui/card'
import { formatPosition } from '@/lib/utils'
import type { MotorStatus as MotorStatusType, ConnectionState } from '@/types'
import { Wifi, WifiOff, CircleAlert, Activity, Square, Circle } from 'lucide-react'

interface MotorStatusProps {
  motorStatus: MotorStatusType
  connectionState: ConnectionState
}

export function MotorStatus({ motorStatus, connectionState }: MotorStatusProps) {
  const getConnectionIcon = () => {
    if (connectionState.isConnecting) {
      return <Activity className="h-4 w-4 animate-pulse text-yellow-500" />
    } else if (connectionState.isConnected) {
      return <Wifi className="h-4 w-4 text-green-500" />
    } else {
      return <WifiOff className="h-4 w-4 text-red-500" />
    }
  }

  const getConnectionText = () => {
    if (connectionState.isConnecting) {
      return `Connecting... (${connectionState.reconnectAttempts}/${3})`
    } else if (connectionState.isConnected) {
      return 'Connected'
    } else {
      return connectionState.lastError || 'Disconnected'
    }
  }

  return (
    <Card>
      <CardHeader className="pb-3">
        <CardTitle className="flex items-center justify-between text-lg">
          Motor Status
          <div className="flex items-center gap-2 text-sm font-normal">
            {getConnectionIcon()}
            <span className={`${connectionState.isConnected ? 'text-green-600' : 'text-red-600'}`}>
              {getConnectionText()}
            </span>
          </div>
        </CardTitle>
      </CardHeader>
      <CardContent className="space-y-4">
        {/* Position */}
        <div className="flex justify-between items-center">
          <span className="text-sm font-medium">Position:</span>
          <span className="text-lg font-mono tabular-nums">
            {formatPosition(motorStatus.position)}
          </span>
        </div>

        {/* Motor State */}
        <div className="flex justify-between items-center">
          <span className="text-sm font-medium">State:</span>
          <div className="flex items-center gap-2">
            {motorStatus.isMoving ? (
              <Activity className="h-4 w-4 text-blue-500 animate-pulse" />
            ) : (
              <Square className="h-4 w-4 text-gray-500" />
            )}
            <span className={motorStatus.isMoving ? 'text-blue-600' : 'text-gray-600'}>
              {motorStatus.isMoving ? 'Moving' : 'Stopped'}
            </span>
          </div>
        </div>

        {/* Emergency Stop */}
        {motorStatus.emergencyStop && (
          <div className="flex justify-between items-center p-2 bg-red-100 dark:bg-red-900/20 rounded-md">
            <span className="text-sm font-medium text-red-700 dark:text-red-300">
              Emergency Stop:
            </span>
            <div className="flex items-center gap-2">
              <CircleAlert className="h-4 w-4 text-red-500" />
              <span className="text-red-600 dark:text-red-400 font-medium">Active</span>
            </div>
          </div>
        )}

        {/* Limit Switches */}
        <div className="border-t pt-4">
          <div className="text-sm font-medium mb-2">Limit Switches:</div>
          <div className="grid grid-cols-2 gap-4">
            <div className="flex items-center justify-between">
              <span className="text-sm">Min:</span>
              <div className="flex items-center gap-1">
                <Circle className={`h-3 w-3 ${
                  motorStatus.limitSwitches.min
                    ? 'text-red-500 fill-current'
                    : 'text-gray-400'
                }`} />
                <span className={`text-xs ${
                  motorStatus.limitSwitches.min ? 'text-red-600' : 'text-gray-500'
                }`}>
                  {motorStatus.limitSwitches.min ? 'Triggered' : 'Open'}
                </span>
              </div>
            </div>
            <div className="flex items-center justify-between">
              <span className="text-sm">Max:</span>
              <div className="flex items-center gap-1">
                <Circle className={`h-3 w-3 ${
                  motorStatus.limitSwitches.max
                    ? 'text-red-500 fill-current'
                    : 'text-gray-400'
                }`} />
                <span className={`text-xs ${
                  motorStatus.limitSwitches.max ? 'text-red-600' : 'text-gray-500'
                }`}>
                  {motorStatus.limitSwitches.max ? 'Triggered' : 'Open'}
                </span>
              </div>
            </div>
          </div>
        </div>
      </CardContent>
    </Card>
  )
}