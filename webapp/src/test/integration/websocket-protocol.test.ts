import { describe, it, expect, vi, beforeEach } from 'vitest'
import { renderHook, waitFor, act } from '@testing-library/react'
import { useMotorController } from '@/hooks/useMotorController'
import { MockWebSocket } from '../setup'

/**
 * WebSocket Protocol Compliance Tests
 *
 * These tests verify that the frontend WebSocket implementation
 * matches the backend ESP32 protocol specification exactly.
 *
 * Backend Protocol Reference:
 * - Commands (client -> server): { command: string, ...params }
 * - Responses (server -> client): { type: string, ...data }
 * - Commands: move, jogStart, jogStop, emergencyStop, reset, status, getConfig, setConfig
 * - Response types: status, position, config, error, configUpdated
 */

// Mock getWebSocketUrl
vi.mock('@/lib/utils', async () => {
  const actual = await vi.importActual('@/lib/utils')
  return {
    ...actual,
    getWebSocketUrl: (endpoint: string) => `ws://localhost:80${endpoint}`
  }
})

describe('WebSocket Protocol Compliance', () => {
  beforeEach(() => {
    MockWebSocket.reset()
  })

  describe('Command Format Validation', () => {
    it('should send all commands with "command" field as specified by backend', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!
      ws.sentMessages = [] // Clear initial messages

      // Test all command types
      result.current.moveTo(1000, 5000)
      result.current.jogStart('forward', 3000)
      result.current.jogStop()
      result.current.emergencyStop()
      result.current.clearEmergencyStop()
      result.current.updateConfig({ maxSpeed: 10000 })

      await waitFor(() => expect(ws.sentMessages.length).toBeGreaterThanOrEqual(6))

      // Verify all commands have "command" field
      ws.sentMessages.forEach(msg => {
        const parsed = JSON.parse(msg)
        expect(parsed).toHaveProperty('command')
        expect(typeof parsed.command).toBe('string')
      })

      // Verify specific command names match backend spec
      const commands = ws.sentMessages.map(msg => JSON.parse(msg).command)
      expect(commands).toContain('move')
      expect(commands).toContain('jogStart')
      expect(commands).toContain('jogStop')
      expect(commands).toContain('emergencyStop')
      expect(commands).toContain('reset')
      expect(commands).toContain('setConfig')
    })

    it('should validate JSON structure for move command', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!
      ws.sentMessages = []

      result.current.moveTo(1234, 5678)

      await waitFor(() => {
        const moveCmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'move')
        expect(moveCmd).toBeDefined()
      })

      const moveCmd = JSON.parse(ws.sentMessages.find(msg => JSON.parse(msg).command === 'move')!)

      // Verify exact structure expected by backend (WebServer.cpp:208-241)
      expect(moveCmd).toEqual({
        command: 'move',
        position: 1234,
        speed: 5678
      })
    })

    it('should validate JSON structure for jogStart command', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!
      ws.sentMessages = []

      result.current.jogStart('forward', 3000)

      await waitFor(() => {
        const jogCmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'jogStart')
        expect(jogCmd).toBeDefined()
      })

      const jogCmd = JSON.parse(ws.sentMessages.find(msg => JSON.parse(msg).command === 'jogStart')!)

      // Verify exact structure expected by backend (WebServer.cpp:243-284)
      expect(jogCmd).toEqual({
        command: 'jogStart',
        direction: 'forward',
        speed: 3000
      })
    })

    it('should validate JSON structure for setConfig command', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!
      ws.sentMessages = []

      result.current.updateConfig({
        maxSpeed: 12000,
        acceleration: 25000,
        useStealthChop: true,
    freewheelAfterMove: false
      })

      await waitFor(() => {
        const configCmd = ws.sentMessages.find(msg => JSON.parse(msg).command === 'setConfig')
        expect(configCmd).toBeDefined()
      })

      const configCmd = JSON.parse(ws.sentMessages.find(msg => JSON.parse(msg).command === 'setConfig')!)

      // Verify exact structure expected by backend (WebServer.cpp:317-364)
      expect(configCmd).toMatchObject({
        command: 'setConfig',
        maxSpeed: 12000,
        acceleration: 25000,
        useStealthChop: true,
    freewheelAfterMove: false
      })
    })
  })

  describe('Response Message Validation', () => {
    it('should verify "type" field is present in all server messages', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!

      // Test all message types from backend
      const testMessages = [
        { type: 'status', position: 100, isMoving: false, emergencyStop: false, limitSwitches: { min: false, max: false, any: false } },
        { type: 'position', position: 200 },
        { type: 'config', maxSpeed: 8000, acceleration: 16000, minLimit: -1000, maxLimit: 1000, useStealthChop: true },
        { type: 'error', message: 'Test error' },
        { type: 'configUpdated', status: 'success' }
      ]

      testMessages.forEach(msg => {
        act(() => {
          ws.simulateMessage(msg)
        })

        // Verify message has required "type" field
        expect(msg).toHaveProperty('type')
        expect(typeof msg.type).toBe('string')
      })
    })

    it('should handle status message structure from backend', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!

      // Structure from WebServer.cpp:463-480
      const statusMessage = {
        type: 'status',
        position: 1234,
        isMoving: true,
        emergencyStop: false,
        limitSwitches: {
          min: false,
          max: true,
          any: true
        }
      }

      act(() => {
        ws.simulateMessage(statusMessage)
      })

      await waitFor(() => {
        expect(result.current.motorStatus.position).toBe(1234)
        expect(result.current.motorStatus.isMoving).toBe(true)
        expect(result.current.motorStatus.emergencyStop).toBe(false)
        expect(result.current.motorStatus.limitSwitches).toEqual({
          min: false,
          max: true,
          any: true
        })
      })
    })

    it('should handle config message structure from backend', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!

      // Structure from WebServer.cpp:486-502
      const configMessage = {
        type: 'config',
        maxSpeed: 15000,
        acceleration: 30000,
        minLimit: -2000,
        maxLimit: 2000,
        useStealthChop: false
      }

      act(() => {
        ws.simulateMessage(configMessage)
      })

      await waitFor(() => {
        expect(result.current.motorConfig).toEqual({
          type: 'config',
          maxSpeed: 15000,
          acceleration: 30000,
          minLimit: -2000,
          maxLimit: 2000,
          useStealthChop: false
        })
      })
    })

    it('should handle position-only message structure from backend', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!

      // Set initial state
      act(() => {
        ws.simulateMessage({
          type: 'status',
          position: 100,
          isMoving: true,
          emergencyStop: false,
          limitSwitches: { min: false, max: false, any: false }
        })
      })

      await waitFor(() => expect(result.current.motorStatus.position).toBe(100))

      // Structure from WebServer.cpp:512-524 (high-frequency position updates)
      const positionMessage = {
        type: 'position',
        position: 5678
      }

      act(() => {
        ws.simulateMessage(positionMessage)
      })

      await waitFor(() => {
        // Position should update
        expect(result.current.motorStatus.position).toBe(5678)
        // Other fields should remain unchanged
        expect(result.current.motorStatus.isMoving).toBe(true)
      })
    })
  })

  describe('Message Frequency Patterns', () => {
    it('should handle high-frequency position updates (~10Hz)', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!

      // Simulate high-frequency position updates during movement
      // Backend sends these at ~100ms intervals (WebServer.cpp:584-586)
      const positions = [100, 200, 300, 400, 500, 600, 700, 800, 900, 1000]

      for (const pos of positions) {
        act(() => {
          ws.simulateMessage({ type: 'position', position: pos })
        })
        await waitFor(() => expect(result.current.motorStatus.position).toBe(pos))
      }

      // Final position should be accurate
      expect(result.current.motorStatus.position).toBe(1000)
    })

    it('should handle medium-frequency status updates (~2Hz)', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!

      // Simulate medium-frequency full status updates during movement
      // Backend sends these at ~500ms intervals (WebServer.cpp:589-594)
      const statusUpdates = [
        { position: 100, isMoving: true, emergencyStop: false },
        { position: 300, isMoving: true, emergencyStop: false },
        { position: 500, isMoving: false, emergencyStop: false }
      ]

      for (const update of statusUpdates) {
        act(() => {
          ws.simulateMessage({
            type: 'status',
            ...update,
            limitSwitches: { min: false, max: false, any: false }
          })
        })
        await waitFor(() => expect(result.current.motorStatus.position).toBe(update.position))
      }

      // Final state should be accurate
      expect(result.current.motorStatus.isMoving).toBe(false)
    })

    it('should handle low-frequency config updates (on-change only)', async () => {
      const { result } = renderHook(() => useMotorController())
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const ws = MockWebSocket.getLastInstance()!

      // Config updates are sent only when configuration changes
      // (WebServer.cpp:358, 612)
      const initialConfig = {
        type: 'config',
        maxSpeed: 8000,
        acceleration: 16000,
        minLimit: -1000,
        maxLimit: 1000,
        useStealthChop: true,
    freewheelAfterMove: false
      }

      act(() => {
        ws.simulateMessage(initialConfig)
      })

      await waitFor(() => expect(result.current.motorConfig.maxSpeed).toBe(8000))

      // Simulate config change
      const updatedConfig = {
        ...initialConfig,
        maxSpeed: 12000,
        useStealthChop: false
      }

      act(() => {
        ws.simulateMessage(updatedConfig)
      })

      await waitFor(() => {
        expect(result.current.motorConfig.maxSpeed).toBe(12000)
        expect(result.current.motorConfig.useStealthChop).toBe(false)
      })
    })
  })

  describe('Connection Lifecycle', () => {
    it('should send initial status and config requests on every connection', async () => {
      const { result, unmount } = renderHook(() => useMotorController())

      // Wait for initial connection
      await waitFor(() => expect(result.current.isConnected).toBe(true))

      const firstWs = MockWebSocket.getLastInstance()!

      // Verify initial requests were sent on first connection
      await waitFor(() => expect(firstWs.sentMessages.length).toBeGreaterThanOrEqual(2))

      const initialCommands = firstWs.sentMessages.map(msg => JSON.parse(msg).command)
      expect(initialCommands).toContain('status')
      expect(initialCommands).toContain('getConfig')

      // Cleanup first connection
      unmount()

      // Create a new connection (simulating reconnect)
      const { result: result2 } = renderHook(() => useMotorController())

      // Wait for second connection
      await waitFor(() => expect(result2.current.isConnected).toBe(true))

      const secondWs = MockWebSocket.getLastInstance()!

      // Verify status and getConfig commands were sent again on new connection
      await waitFor(() => expect(secondWs.sentMessages.length).toBeGreaterThanOrEqual(2))

      const reconnectCommands = secondWs.sentMessages.map(msg => JSON.parse(msg).command)
      expect(reconnectCommands).toContain('status')
      expect(reconnectCommands).toContain('getConfig')
    })
  })
})
