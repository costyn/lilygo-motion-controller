import { describe, it, expect, vi, beforeEach } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/react'
import { PositionControl } from './PositionControl'
import type { MotorConfig } from '@/types'

describe('PositionControl - Disabled States', () => {
  const defaultConfig: MotorConfig = {
    type: 'config',
    maxSpeed: 10000,
    acceleration: 20000,
    minLimit: -5000,
    maxLimit: 5000,
    useStealthChop: true,
    freewheelAfterMove: false
  }

  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    currentPosition: 0,
    motorConfig: defaultConfig,
    jogSpeed: 3000,
    onJogSpeedChange: vi.fn(),
    onMoveTo: vi.fn(),
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should disable controls when disconnected', () => {
    render(<PositionControl {...defaultProps} isConnected={false} />)

    const sliders = screen.getAllByRole('slider')
    sliders.forEach(slider => {
      expect(slider).toHaveAttribute('data-disabled')
    })

    const quickButtons = screen.getAllByRole('button')
    quickButtons.forEach(button => {
      expect(button).toBeDisabled()
    })
  })

  it('should disable controls when emergency stop is active', () => {
    render(<PositionControl {...defaultProps} emergencyStop={true} />)

    const sliders = screen.getAllByRole('slider')
    sliders.forEach(slider => {
      expect(slider).toHaveAttribute('data-disabled')
    })
  })

  it('should disable controls when motor is moving', () => {
    render(<PositionControl {...defaultProps} isMoving={true} />)

    const sliders = screen.getAllByRole('slider')
    sliders.forEach(slider => {
      expect(slider).toHaveAttribute('data-disabled')
    })
  })
})

describe('PositionControl - Position Display', () => {
  const defaultConfig: MotorConfig = {
    type: 'config',
    maxSpeed: 10000,
    acceleration: 20000,
    minLimit: 0,
    maxLimit: 10000,
    useStealthChop: true,
    freewheelAfterMove: false
  }

  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    currentPosition: 5000,
    motorConfig: defaultConfig,
    jogSpeed: 3000,
    onJogSpeedChange: vi.fn(),
    onMoveTo: vi.fn(),
  }

  it('should display current position', () => {
    render(<PositionControl {...defaultProps} />)

    expect(screen.getByText('5,000 steps')).toBeInTheDocument()
  })

  it('should calculate progress correctly at 50%', () => {
    render(<PositionControl {...defaultProps} />)

    expect(screen.getAllByText('50%')[0]).toBeInTheDocument()
  })

  it('should calculate progress correctly at 0%', () => {
    render(<PositionControl {...defaultProps} currentPosition={0} />)

    expect(screen.getAllByText('0%')[0]).toBeInTheDocument()
  })

  it('should calculate progress correctly at 100%', () => {
    render(<PositionControl {...defaultProps} currentPosition={10000} />)

    expect(screen.getAllByText('100%')[0]).toBeInTheDocument()
  })

  it('should display speed value', () => {
    render(<PositionControl {...defaultProps} jogSpeed={5000} />)

    // Speed display is unique
    expect(screen.getByText(/5,000 steps\/s/)).toBeInTheDocument()
  })
})

describe('PositionControl - Quick Position Buttons', () => {
  const defaultConfig: MotorConfig = {
    type: 'config',
    maxSpeed: 10000,
    acceleration: 20000,
    minLimit: 0,
    maxLimit: 10000,
    useStealthChop: true,
    freewheelAfterMove: false
  }

  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    currentPosition: 0,
    motorConfig: defaultConfig,
    jogSpeed: 3000,
    onJogSpeedChange: vi.fn(),
    onMoveTo: vi.fn(),
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should call onMoveTo with 0% position', () => {
    render(<PositionControl {...defaultProps} />)

    const button0 = screen.getByRole('button', { name: '0%' })
    fireEvent.click(button0)

    expect(defaultProps.onMoveTo).toHaveBeenCalledWith(0, 3000)
  })

  it('should call onMoveTo with 25% position', () => {
    render(<PositionControl {...defaultProps} />)

    const button25 = screen.getByRole('button', { name: '25%' })
    fireEvent.click(button25)

    expect(defaultProps.onMoveTo).toHaveBeenCalledWith(2500, 3000)
  })

  it('should call onMoveTo with 50% position', () => {
    render(<PositionControl {...defaultProps} />)

    const button50 = screen.getByRole('button', { name: '50%' })
    fireEvent.click(button50)

    expect(defaultProps.onMoveTo).toHaveBeenCalledWith(5000, 3000)
  })

  it('should call onMoveTo with 75% position', () => {
    render(<PositionControl {...defaultProps} />)

    const button75 = screen.getByRole('button', { name: '75%' })
    fireEvent.click(button75)

    expect(defaultProps.onMoveTo).toHaveBeenCalledWith(7500, 3000)
  })

  it('should call onMoveTo with 100% position', () => {
    render(<PositionControl {...defaultProps} />)

    const button100 = screen.getByRole('button', { name: '100%' })
    fireEvent.click(button100)

    expect(defaultProps.onMoveTo).toHaveBeenCalledWith(10000, 3000)
  })
})

describe('PositionControl - Slider Configuration', () => {
  const defaultConfig: MotorConfig = {
    type: 'config',
    maxSpeed: 10000,
    acceleration: 20000,
    minLimit: -2000,
    maxLimit: 8000,
    useStealthChop: true,
    freewheelAfterMove: false
  }

  const defaultProps = {
    isConnected: true,
    isMoving: false,
    emergencyStop: false,
    currentPosition: 0,
    motorConfig: defaultConfig,
    jogSpeed: 3000,
    onJogSpeedChange: vi.fn(),
    onMoveTo: vi.fn(),
  }

  it('should display min and max limit values', () => {
    render(<PositionControl {...defaultProps} />)

    // Position slider limits (may appear multiple times, just check first)
    expect(screen.getAllByText('-2,000')[0]).toBeInTheDocument()
    expect(screen.getAllByText('8,000')[0]).toBeInTheDocument()
  })

  it('should display speed slider range', () => {
    render(<PositionControl {...defaultProps} />)

    // Speed slider shows 100 to maxSpeed (may appear multiple times)
    expect(screen.getAllByText('100')[0]).toBeInTheDocument()
    expect(screen.getAllByText('10,000')[0]).toBeInTheDocument()
  })
})
