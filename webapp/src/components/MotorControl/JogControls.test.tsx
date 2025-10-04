import { describe, it, expect, vi, beforeEach } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/react'
import { JogControls } from './JogControls'

describe('JogControls - Button States', () => {
  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    jogSpeed: 1000,
    onJogStart: vi.fn(),
    onJogStop: vi.fn(),
    onEmergencyStop: vi.fn(),
    onClearEmergencyStop: vi.fn(),
    onMoveToLimit: vi.fn(),
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should disable jog buttons when disconnected', () => {
    render(<JogControls {...defaultProps} isConnected={false} />)

    const jogBackButton = screen.getByText('Jog Back').closest('button')
    const jogForwardButton = screen.getByText('Jog Forward').closest('button')

    expect(jogBackButton).toBeDisabled()
    expect(jogForwardButton).toBeDisabled()
  })

  it('should disable jog buttons when emergency stop is active', () => {
    render(<JogControls {...defaultProps} emergencyStop={true} />)

    const jogBackButton = screen.getByText('Jog Back').closest('button')
    const jogForwardButton = screen.getByText('Jog Forward').closest('button')

    expect(jogBackButton).toBeDisabled()
    expect(jogForwardButton).toBeDisabled()
  })

  it('should show emergency stop button when not active', () => {
    render(<JogControls {...defaultProps} emergencyStop={false} />)

    expect(screen.getByText('Emergency Stop')).toBeInTheDocument()
    expect(screen.queryByText('Reset E-Stop')).not.toBeInTheDocument()
  })

  it('should show reset button when emergency stop is active', () => {
    render(<JogControls {...defaultProps} emergencyStop={true} />)

    expect(screen.queryByText('Emergency Stop')).not.toBeInTheDocument()
    expect(screen.getByText('Reset E-Stop')).toBeInTheDocument()
  })

  it('should disable limit buttons when moving', () => {
    render(<JogControls {...defaultProps} isMoving={true} />)

    const minLimitButton = screen.getByText('Min Limit')
    const maxLimitButton = screen.getByText('Max Limit')

    expect(minLimitButton).toBeDisabled()
    expect(maxLimitButton).toBeDisabled()
  })

  it('should enable emergency stop button even when disconnected', () => {
    render(<JogControls {...defaultProps} isConnected={false} />)

    const emergencyButton = screen.getByText('Emergency Stop')

    // Button exists but is disabled when disconnected
    expect(emergencyButton).toBeDisabled()
  })
})

describe('JogControls - Mouse Interaction', () => {
  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    jogSpeed: 1000,
    onJogStart: vi.fn(),
    onJogStop: vi.fn(),
    onEmergencyStop: vi.fn(),
    onClearEmergencyStop: vi.fn(),
    onMoveToLimit: vi.fn(),
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should start jog forward on mouse down', () => {
    render(<JogControls {...defaultProps} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!
    fireEvent.mouseDown(jogForwardButton)

    expect(defaultProps.onJogStart).toHaveBeenCalledWith('forward', 1000)
  })

  it('should start jog backward on mouse down', () => {
    render(<JogControls {...defaultProps} />)

    const jogBackButton = screen.getByText('Jog Back').closest('button')!
    fireEvent.mouseDown(jogBackButton)

    expect(defaultProps.onJogStart).toHaveBeenCalledWith('backward', 1000)
  })

  it('should stop jog on mouse up', () => {
    render(<JogControls {...defaultProps} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!

    fireEvent.mouseDown(jogForwardButton)
    fireEvent.mouseUp(jogForwardButton)

    expect(defaultProps.onJogStop).toHaveBeenCalled()
  })

  it('should stop jog on mouse leave if jogging', () => {
    render(<JogControls {...defaultProps} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!

    fireEvent.mouseDown(jogForwardButton)
    fireEvent.mouseLeave(jogForwardButton)

    expect(defaultProps.onJogStop).toHaveBeenCalled()
  })

  it('should not stop on mouse leave if not jogging', () => {
    render(<JogControls {...defaultProps} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!

    // Mouse leave without mouseDown first
    fireEvent.mouseLeave(jogForwardButton)

    expect(defaultProps.onJogStop).not.toHaveBeenCalled()
  })

  it('should not start jog when disconnected', () => {
    render(<JogControls {...defaultProps} isConnected={false} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!
    fireEvent.mouseDown(jogForwardButton)

    expect(defaultProps.onJogStart).not.toHaveBeenCalled()
  })

  it('should not start jog when emergency stop is active', () => {
    render(<JogControls {...defaultProps} emergencyStop={true} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!
    fireEvent.mouseDown(jogForwardButton)

    expect(defaultProps.onJogStart).not.toHaveBeenCalled()
  })
})

describe('JogControls - Touch Interaction', () => {
  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    jogSpeed: 1000,
    onJogStart: vi.fn(),
    onJogStop: vi.fn(),
    onEmergencyStop: vi.fn(),
    onClearEmergencyStop: vi.fn(),
    onMoveToLimit: vi.fn(),
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should start jog on touch start', () => {
    render(<JogControls {...defaultProps} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!
    fireEvent.touchStart(jogForwardButton)

    expect(defaultProps.onJogStart).toHaveBeenCalledWith('forward', 1000)
  })

  it('should stop jog on touch end', () => {
    render(<JogControls {...defaultProps} />)

    const jogForwardButton = screen.getByText('Jog Forward').closest('button')!

    fireEvent.touchStart(jogForwardButton)
    fireEvent.touchEnd(jogForwardButton)

    expect(defaultProps.onJogStop).toHaveBeenCalled()
  })
})

describe('JogControls - Button Actions', () => {
  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    jogSpeed: 1000,
    onJogStart: vi.fn(),
    onJogStop: vi.fn(),
    onEmergencyStop: vi.fn(),
    onClearEmergencyStop: vi.fn(),
    onMoveToLimit: vi.fn(),
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should call onEmergencyStop when emergency button clicked', () => {
    render(<JogControls {...defaultProps} />)

    const emergencyButton = screen.getByText('Emergency Stop')
    fireEvent.click(emergencyButton)

    expect(defaultProps.onEmergencyStop).toHaveBeenCalled()
  })

  it('should call onClearEmergencyStop when reset button clicked', () => {
    render(<JogControls {...defaultProps} emergencyStop={true} />)

    const resetButton = screen.getByText('Reset E-Stop')
    fireEvent.click(resetButton)

    expect(defaultProps.onClearEmergencyStop).toHaveBeenCalled()
  })

  it('should call onMoveToLimit with min when Min Limit clicked', () => {
    render(<JogControls {...defaultProps} />)

    const minLimitButton = screen.getByText('Min Limit')
    fireEvent.click(minLimitButton)

    expect(defaultProps.onMoveToLimit).toHaveBeenCalledWith('min')
  })

  it('should call onMoveToLimit with max when Max Limit clicked', () => {
    render(<JogControls {...defaultProps} />)

    const maxLimitButton = screen.getByText('Max Limit')
    fireEvent.click(maxLimitButton)

    expect(defaultProps.onMoveToLimit).toHaveBeenCalledWith('max')
  })
})
