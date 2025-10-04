import { describe, it, expect, vi, beforeEach, afterEach } from 'vitest'
import { renderHook, waitFor } from '@testing-library/react'
import { useMotorController } from './useMotorController'
import { MockWebSocket } from '../test/setup'

// Mock getWebSocketUrl to return a simple test URL
vi.mock('@/lib/utils', async () => {
  const actual = await vi.importActual('@/lib/utils')
  return {
    ...actual,
    getWebSocketUrl: (endpoint: string) => `ws://localhost:80${endpoint}`
  }
})

describe('useMotorController - Connection Management', () => {
  beforeEach(() => {
    MockWebSocket.reset()
  })

  it('should start with disconnected state', () => {
    const { result } = renderHook(() => useMotorController())
    expect(result.current.isConnected).toBe(false)
    expect(result.current.connectionState.reconnectAttempts).toBe(0)
  })

  it('should connect and update connection state', async () => {
    const { result } = renderHook(() => useMotorController())

    await waitFor(() => expect(result.current.isConnected).toBe(true))
    expect(result.current.isConnecting).toBe(false)
  })

  it('should send initial status and config requests on connect', async () => {
    renderHook(() => useMotorController())

    await waitFor(() => expect(MockWebSocket.getLastInstance()).not.toBeNull())

    const instance = MockWebSocket.getLastInstance()!
    await waitFor(() => expect(instance.sentMessages.length).toBeGreaterThanOrEqual(2))

    const commands = instance.sentMessages.map(msg => JSON.parse(msg).command)
    expect(commands).toContain('status')
    expect(commands).toContain('getConfig')
  })

  it('should cleanup WebSocket on unmount', async () => {
    const { unmount } = renderHook(() => useMotorController())

    await waitFor(() => expect(MockWebSocket.getLastInstance()).not.toBeNull())

    const ws = MockWebSocket.getLastInstance()!
    const closeSpy = vi.spyOn(ws, 'close')

    unmount()
    expect(closeSpy).toHaveBeenCalled()
  })
})

describe('useMotorController - Message Handling', () => {
  beforeEach(() => {
    MockWebSocket.reset()
  })

  it('should update motorStatus on status message', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    const ws = MockWebSocket.getLastInstance()!
    ws.simulateMessage({
      type: 'status',
      position: 1234,
      isMoving: true,
      emergencyStop: false,
      limitSwitches: { min: false, max: false, any: false }
    })

    await waitFor(() => expect(result.current.motorStatus.position).toBe(1234))
    expect(result.current.motorStatus.isMoving).toBe(true)
  })

  it('should update position only on position message', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    const ws = MockWebSocket.getLastInstance()!
    ws.simulateMessage({
      type: 'status',
      position: 100,
      isMoving: true,
      emergencyStop: false,
      limitSwitches: { min: false, max: false, any: false }
    })

    await waitFor(() => expect(result.current.motorStatus.position).toBe(100))

    ws.simulateMessage({ type: 'position', position: 500 })

    await waitFor(() => expect(result.current.motorStatus.position).toBe(500))
    expect(result.current.motorStatus.isMoving).toBe(true)
  })

  it('should update motorConfig on config message', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    const ws = MockWebSocket.getLastInstance()!
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 10000,
      acceleration: 20000,
      minLimit: -1000,
      maxLimit: 1000,
      useStealthChop: false
    })

    await waitFor(() => expect(result.current.motorConfig.maxSpeed).toBe(10000))
    expect(result.current.motorConfig.acceleration).toBe(20000)
    expect(result.current.motorConfig.useStealthChop).toBe(false)
  })

  it('should set connection error on error message', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    const ws = MockWebSocket.getLastInstance()!
    ws.simulateMessage({
      type: 'error',
      message: 'Motor fault detected'
    })

    await waitFor(() => expect(result.current.connectionState.lastError).toBe('Motor fault detected'))
  })

  it('should handle malformed JSON gracefully', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    const consoleSpy = vi.spyOn(console, 'error').mockImplementation(() => {})

    const ws = MockWebSocket.getLastInstance()!
    if (ws.onmessage) {
      ws.onmessage(new MessageEvent('message', { data: 'invalid json' }))
    }

    await waitFor(() => expect(consoleSpy).toHaveBeenCalled())
    expect(result.current.isConnected).toBe(true)

    consoleSpy.mockRestore()
  })
})

describe('useMotorController - Command Sending', () => {
  beforeEach(() => {
    MockWebSocket.reset()
  })

  it('should send moveTo command with correct parameters', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    result.current.moveTo(1000, 5000)

    const ws = MockWebSocket.getLastInstance()!
    await waitFor(() => {
      const moveCmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'move')
      expect(moveCmd).toBeDefined()
    })

    const moveCmd = JSON.parse(ws.sentMessages.find(msg => JSON.parse(msg).command === 'move')!)
    expect(moveCmd).toEqual({ command: 'move', position: 1000, speed: 5000 })
  })

  it('should send emergencyStop command', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    result.current.emergencyStop()

    const ws = MockWebSocket.getLastInstance()!
    await waitFor(() => {
      const cmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'emergencyStop')
      expect(cmd).toBeDefined()
    })
  })

  it('should send jogStart command', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    result.current.jogStart('forward', 3000)

    const ws = MockWebSocket.getLastInstance()!
    await waitFor(() => {
      const cmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'jogStart')
      expect(cmd).toBeDefined()
    })

    const jogCmd = JSON.parse(ws.sentMessages.find(msg => JSON.parse(msg).command === 'jogStart')!)
    expect(jogCmd).toEqual({ command: 'jogStart', direction: 'forward', speed: 3000 })
  })

  it('should send jogStop command', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    result.current.jogStop()

    const ws = MockWebSocket.getLastInstance()!
    await waitFor(() => {
      const cmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'jogStop')
      expect(cmd).toBeDefined()
    })
  })

  it('should send reset command on clearEmergencyStop', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    result.current.clearEmergencyStop()

    const ws = MockWebSocket.getLastInstance()!
    await waitFor(() => {
      const cmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'reset')
      expect(cmd).toBeDefined()
    })
  })

  it('should send setConfig command with parameters', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    result.current.updateConfig({
      maxSpeed: 12000,
      acceleration: 25000,
      useStealthChop: true
    })

    const ws = MockWebSocket.getLastInstance()!
    await waitFor(() => {
      const cmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'setConfig')
      expect(cmd).toBeDefined()
    })

    const configCmd = JSON.parse(ws.sentMessages.find(msg => JSON.parse(msg).command === 'setConfig')!)
    expect(configCmd).toMatchObject({
      command: 'setConfig',
      maxSpeed: 12000,
      acceleration: 25000,
      useStealthChop: true
    })
  })

  it('should return false when sending command while disconnected', () => {
    const { result } = renderHook(() => useMotorController())
    const success = result.current.moveTo(1000, 5000)
    expect(success).toBe(false)
  })
})

describe('useMotorController - State Defaults', () => {
  beforeEach(() => {
    MockWebSocket.reset()
  })

  it('should use default motorConfig until received from server', () => {
    const { result } = renderHook(() => useMotorController())

    expect(result.current.motorConfig.maxSpeed).toBe(8000)
    expect(result.current.motorConfig.acceleration).toBe(16000)
    expect(result.current.motorConfig.useStealthChop).toBe(true)
  })

  it('should use default motorStatus until received from server', () => {
    const { result } = renderHook(() => useMotorController())

    expect(result.current.motorStatus.position).toBe(0)
    expect(result.current.motorStatus.isMoving).toBe(false)
    expect(result.current.motorStatus.emergencyStop).toBe(false)
  })

  it('should update config when received from server', async () => {
    const { result } = renderHook(() => useMotorController())
    await waitFor(() => expect(result.current.isConnected).toBe(true))

    const ws = MockWebSocket.getLastInstance()!
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 15000,
      acceleration: 30000,
      minLimit: -2000,
      maxLimit: 2000,
      useStealthChop: false
    })

    await waitFor(() => expect(result.current.motorConfig.maxSpeed).toBe(15000))
    expect(result.current.motorConfig.acceleration).toBe(30000)
  })
})
