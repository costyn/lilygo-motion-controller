import { describe, it, expect, vi, beforeEach } from 'vitest'
import { render, screen, waitFor } from '@testing-library/react'
import { userEvent } from '@testing-library/user-event'
import App from './App'
import { MockWebSocket } from './test/setup'

// Mock getWebSocketUrl to return a simple test URL
vi.mock('@/lib/utils', async () => {
  const actual = await vi.importActual('@/lib/utils')
  return {
    ...actual,
    getWebSocketUrl: (endpoint: string) => `ws://localhost:80${endpoint}`
  }
})

describe('App - Limit Warning', () => {
  beforeEach(() => {
    MockWebSocket.reset()
  })

  it('should show warning when min and max limits are equal', async () => {
    render(<App />)

    // Wait for connection
    await waitFor(() => expect(MockWebSocket.getLastInstance()).not.toBeNull())

    const ws = MockWebSocket.getLastInstance()!

    // Send config with equal limits
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 8000,
      acceleration: 16000,
      minLimit: 1000,
      maxLimit: 1000,
      useStealthChop: true
    })

    // Warning should appear
    await waitFor(() => {
      expect(screen.getByText('Motor Movement Disabled')).toBeInTheDocument()
    })

    expect(screen.getByText(/minimum and maximum positions are equal/i)).toBeInTheDocument()
    expect(screen.getByText(/1000 steps/i)).toBeInTheDocument()
  })

  it('should not show warning when limits are different', async () => {
    render(<App />)

    await waitFor(() => expect(MockWebSocket.getLastInstance()).not.toBeNull())

    const ws = MockWebSocket.getLastInstance()!

    // Send config with different limits
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 8000,
      acceleration: 16000,
      minLimit: 0,
      maxLimit: 2000,
      useStealthChop: true
    })

    // Warning should not appear
    await waitFor(() => {
      expect(screen.queryByText('Motor Movement Disabled')).not.toBeInTheDocument()
    })
  })

  it('should allow dismissing the warning', async () => {
    const user = userEvent.setup()
    render(<App />)

    await waitFor(() => expect(MockWebSocket.getLastInstance()).not.toBeNull())

    const ws = MockWebSocket.getLastInstance()!

    // Send config with equal limits
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 8000,
      acceleration: 16000,
      minLimit: 500,
      maxLimit: 500,
      useStealthChop: true
    })

    // Warning should appear
    await waitFor(() => {
      expect(screen.getByText('Motor Movement Disabled')).toBeInTheDocument()
    })

    // Find and click dismiss button
    const dismissButton = screen.getByLabelText('Dismiss alert')
    await user.click(dismissButton)

    // Warning should disappear
    await waitFor(() => {
      expect(screen.queryByText('Motor Movement Disabled')).not.toBeInTheDocument()
    })
  })

  it('should open settings dialog when Settings link is clicked in warning', async () => {
    const user = userEvent.setup()
    render(<App />)

    await waitFor(() => expect(MockWebSocket.getLastInstance()).not.toBeNull())

    const ws = MockWebSocket.getLastInstance()!

    // Send config with equal limits
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 8000,
      acceleration: 16000,
      minLimit: 0,
      maxLimit: 0,
      useStealthChop: true
    })

    // Warning should appear
    await waitFor(() => {
      expect(screen.getByText('Motor Movement Disabled')).toBeInTheDocument()
    })

    // Find the warning alert and click the Settings link within it
    const alert = screen.getByRole('alert')
    const settingsLink = alert.querySelector('button')
    expect(settingsLink).not.toBeNull()
    await user.click(settingsLink!)

    // Settings dialog should open
    await waitFor(() => {
      expect(screen.getByText('Motor Configuration')).toBeInTheDocument()
    })
  })

  it('should hide warning when limits change from equal to different', async () => {
    render(<App />)

    await waitFor(() => expect(MockWebSocket.getLastInstance()).not.toBeNull())

    const ws = MockWebSocket.getLastInstance()!

    // Send config with equal limits
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 8000,
      acceleration: 16000,
      minLimit: 100,
      maxLimit: 100,
      useStealthChop: true
    })

    // Warning should appear
    await waitFor(() => {
      expect(screen.getByText('Motor Movement Disabled')).toBeInTheDocument()
    })

    // Update config with different limits
    ws.simulateMessage({
      type: 'config',
      maxSpeed: 8000,
      acceleration: 16000,
      minLimit: 0,
      maxLimit: 2000,
      useStealthChop: true
    })

    // Warning should disappear
    await waitFor(() => {
      expect(screen.queryByText('Motor Movement Disabled')).not.toBeInTheDocument()
    })
  })

  it('should not show warning when disconnected', () => {
    render(<App />)

    // Before connection, warning should not be visible even with default config
    expect(screen.queryByText('Motor Movement Disabled')).not.toBeInTheDocument()
  })
})
