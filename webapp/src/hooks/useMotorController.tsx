import { useState, useEffect, useRef, useCallback } from 'react'
import { getWebSocketUrl } from '@/lib/utils'
import type {
  WebSocketMessage,
  ControlCommand,
  MotorStatus,
  MotorConfig,
  ConnectionState
} from '@/types'

const DEFAULT_STATUS: MotorStatus = {
  type: 'status',
  position: 0,
  isMoving: false,
  emergencyStop: false,
  limitSwitches: {
    min: false,
    max: false,
    any: false
  }
}

const DEFAULT_CONFIG: MotorConfig = {
  type: 'config',
  maxSpeed: 8000,
  acceleration: 16000,
  minLimit: -5000,
  maxLimit: 5000,
  useStealthChop: true
}

export function useMotorController() {
  const [connectionState, setConnectionState] = useState<ConnectionState>({
    isConnected: false,
    isConnecting: false,
    reconnectAttempts: 0,
    lastError: undefined
  })

  const [motorStatus, setMotorStatus] = useState<MotorStatus>(DEFAULT_STATUS)
  const [motorConfig, setMotorConfig] = useState<MotorConfig>(DEFAULT_CONFIG)

  const wsRef = useRef<WebSocket | null>(null)
  const reconnectTimeoutRef = useRef<NodeJS.Timeout | null>(null)
  const maxReconnectAttempts = 3
  const reconnectDelay = 2000

  const cleanup = useCallback(() => {
    if (reconnectTimeoutRef.current) {
      clearTimeout(reconnectTimeoutRef.current)
      reconnectTimeoutRef.current = null
    }

    if (wsRef.current) {
      wsRef.current.close()
      wsRef.current = null
    }
  }, [])

  const connect = useCallback(() => {
    if (wsRef.current?.readyState === WebSocket.OPEN ||
      wsRef.current?.readyState === WebSocket.CONNECTING) {
      return
    }

    const url = getWebSocketUrl('/ws')
    console.log('Connecting to motor controller:', url)

    const websocket = new WebSocket(url)
    wsRef.current = websocket

    setConnectionState(prev => ({
      ...prev,
      isConnecting: true,
      lastError: undefined
    }))

    websocket.onopen = () => {
      console.log('Motor controller WebSocket connected')
      setConnectionState({
        isConnected: true,
        isConnecting: false,
        reconnectAttempts: 0,
        lastError: undefined
      })

      // Request initial status and config
      sendCommand({ command: 'status' })
      sendCommand({ command: 'getConfig' })
    }

    websocket.onclose = (event) => {
      console.log('Motor controller WebSocket disconnected:', event.code, event.reason)
      setConnectionState(prev => ({
        ...prev,
        isConnected: false,
        isConnecting: false
      }))
      wsRef.current = null

      // Attempt reconnection if under max attempts
      if (connectionState.reconnectAttempts < maxReconnectAttempts) {
        reconnectTimeoutRef.current = setTimeout(() => {
          setConnectionState(prev => ({
            ...prev,
            reconnectAttempts: prev.reconnectAttempts + 1
          }))
          connect()
        }, reconnectDelay)
      }
    }

    websocket.onerror = (error) => {
      console.error('Motor controller WebSocket error:', error)
      setConnectionState(prev => ({
        ...prev,
        lastError: 'Connection failed'
      }))
    }

    websocket.onmessage = (event) => {
      try {
        const message: WebSocketMessage = JSON.parse(event.data)
        console.log('Received WebSocket message:', message)

        switch (message.type) {
          case 'status':
            setMotorStatus(message)
            break
          case 'position':
            setMotorStatus(prev => ({
              ...prev,
              position: message.position
            }))
            break
          case 'config':
            setMotorConfig(message)
            break
          case 'configUpdated':
            if (message.status === 'success') {
              // Refresh config after successful update
              sendCommand({ command: 'getConfig' })
            }
            break
          case 'error':
            console.error('Motor controller error:', message.message)
            setConnectionState(prev => ({
              ...prev,
              lastError: message.message
            }))
            break
        }
      } catch (error) {
        console.error('Error parsing WebSocket message:', error, event.data)
      }
    }
  }, [connectionState.reconnectAttempts])

  const sendCommand = useCallback((command: ControlCommand) => {
    if (wsRef.current?.readyState === WebSocket.OPEN) {
      const payload = JSON.stringify(command)
      console.log('Sending command:', payload)
      wsRef.current.send(payload)
      return true
    } else {
      console.warn('WebSocket not connected, cannot send command:', command)
      return false
    }
  }, [])

  // Motor control methods
  const moveTo = useCallback((position: number, speed: number) => {
    return sendCommand({ command: 'move', position, speed })
  }, [sendCommand])

  const stop = useCallback(() => {
    return sendCommand({ command: 'stop' })
  }, [sendCommand])

  const emergencyStop = useCallback(() => {
    return sendCommand({ command: 'emergency-stop' })
  }, [sendCommand])

  const clearEmergencyStop = useCallback(() => {
    return sendCommand({ command: 'reset' })
  }, [sendCommand])

  const updateConfig = useCallback((config: Partial<Omit<MotorConfig, 'type'>>) => {
    const command: any = { command: 'setConfig', ...config }
    return sendCommand(command)
  }, [sendCommand])

  const refreshStatus = useCallback(() => {
    return sendCommand({ command: 'status' })
  }, [sendCommand])

  const jogStart = useCallback((direction: 'forward' | 'backward') => {
    return sendCommand({ command: 'jogStart', direction })
  }, [sendCommand])

  const jogStop = useCallback(() => {
    return sendCommand({ command: 'jogStop' })
  }, [sendCommand])

  const manualReconnect = useCallback(() => {
    cleanup()
    setConnectionState({
      isConnected: false,
      isConnecting: false,
      reconnectAttempts: 0,
      lastError: undefined
    })
    connect()
  }, [cleanup, connect])

  // Initialize connection
  useEffect(() => {
    const timeoutId = setTimeout(connect, 500) // Small delay to allow component mounting

    const handleBeforeUnload = () => {
      cleanup()
    }

    window.addEventListener('beforeunload', handleBeforeUnload)

    return () => {
      clearTimeout(timeoutId)
      window.removeEventListener('beforeunload', handleBeforeUnload)
      cleanup()
    }
  }, [connect, cleanup])

  return {
    // Connection state
    connectionState,
    isConnected: connectionState.isConnected,
    isConnecting: connectionState.isConnecting,

    // Motor state
    motorStatus,
    motorConfig,

    // Control methods
    moveTo,
    stop,
    emergencyStop,
    clearEmergencyStop,
    updateConfig,
    refreshStatus,
    jogStart,
    jogStop,
    manualReconnect,

    // Raw WebSocket access for advanced use
    sendCommand
  }
}