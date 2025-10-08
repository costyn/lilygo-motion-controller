import { describe, it, expect, vi, beforeEach } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/react'
import { MotorConfigDialog } from './MotorConfigDialog'
import type { MotorConfig } from '@/types'

describe('MotorConfigDialog - Dialog Behavior', () => {
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
    open: true,
    onOpenChange: vi.fn(),
    currentConfig: defaultConfig,
    onApply: vi.fn(),
    isConnected: true,
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should render dialog when open is true', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    expect(screen.getByText('Motor Configuration')).toBeInTheDocument()
  })

  it('should not render when open is false', () => {
    render(<MotorConfigDialog {...defaultProps} open={false} />)

    expect(screen.queryByText('Motor Configuration')).not.toBeInTheDocument()
  })

  it('should call onOpenChange when dialog closed', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const cancelButton = screen.getByText('Revert')
    fireEvent.click(cancelButton)

    // Revert just resets values, doesn't close dialog
    expect(defaultProps.onOpenChange).not.toHaveBeenCalled()
  })
})

describe('MotorConfigDialog - Input Validation', () => {
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
    open: true,
    onOpenChange: vi.fn(),
    currentConfig: defaultConfig,
    onApply: vi.fn(),
    isConnected: true,
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should show error when maxSpeed is below 100', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const maxSpeedInput = screen.getByLabelText('Max Speed')
    fireEvent.change(maxSpeedInput, { target: { value: '50' } })

    expect(screen.getByText(/Speed must be between 100 and 100,000/)).toBeInTheDocument()
  })

  it('should show error when maxSpeed is above 100000', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const maxSpeedInput = screen.getByLabelText('Max Speed')
    fireEvent.change(maxSpeedInput, { target: { value: '150000' } })

    expect(screen.getByText(/Speed must be between 100 and 100,000/)).toBeInTheDocument()
  })

  it('should show error when acceleration is below 100', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const accelerationInput = screen.getByLabelText('Acceleration')
    fireEvent.change(accelerationInput, { target: { value: '50' } })

    expect(screen.getByText(/Acceleration must be between 100 and 500,000/)).toBeInTheDocument()
  })

  it('should show error when acceleration is above 500000', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const accelerationInput = screen.getByLabelText('Acceleration')
    fireEvent.change(accelerationInput, { target: { value: '600000' } })

    expect(screen.getByText(/Acceleration must be between 100 and 500,000/)).toBeInTheDocument()
  })

  it('should show error when min limit is greater than or equal to max limit', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const minLimitInput = screen.getByLabelText('Min Limit')
    fireEvent.change(minLimitInput, { target: { value: '6000' } })

    expect(screen.getByText(/Min must be less than max/)).toBeInTheDocument()
  })

  it('should accept valid maxSpeed value', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const maxSpeedInput = screen.getByLabelText('Max Speed')
    fireEvent.change(maxSpeedInput, { target: { value: '12000' } })

    expect(screen.queryByText(/Speed must be between/)).not.toBeInTheDocument()
  })
})

describe('MotorConfigDialog - Apply Changes', () => {
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
    open: true,
    onOpenChange: vi.fn(),
    currentConfig: defaultConfig,
    onApply: vi.fn(),
    isConnected: true,
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should call onApply with changed values only', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const maxSpeedInput = screen.getByLabelText('Max Speed')
    fireEvent.change(maxSpeedInput, { target: { value: '12000' } })

    const accelerationInput = screen.getByLabelText('Acceleration')
    fireEvent.change(accelerationInput, { target: { value: '25000' } })

    const applyButton = screen.getByText('Apply')
    fireEvent.click(applyButton)

    expect(defaultProps.onApply).toHaveBeenCalledWith({
      maxSpeed: 12000,
      acceleration: 25000
    })
    expect(defaultProps.onOpenChange).toHaveBeenCalledWith(false)
  })

  it('should keep apply button enabled when disconnected', () => {
    render(<MotorConfigDialog {...defaultProps} isConnected={false} />)

    const applyButton = screen.getByText('Apply')
    expect(applyButton).toBeEnabled()
  })

  it('should keep apply button enabled when no changes made', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const applyButton = screen.getByText('Apply')
    expect(applyButton).toBeEnabled()
  })

  it('should keep apply button enabled when form is invalid', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const maxSpeedInput = screen.getByLabelText('Max Speed')
    fireEvent.change(maxSpeedInput, { target: { value: '50' } }) // Invalid: below 100

    const applyButton = screen.getByText('Apply')
    expect(applyButton).toBeEnabled()
  })

  it('should toggle StealthChop mode', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const stealthChopSwitch = screen.getByLabelText('StealthChop Mode')
    fireEvent.click(stealthChopSwitch)

    const applyButton = screen.getByText('Apply')
    fireEvent.click(applyButton)

    expect(defaultProps.onApply).toHaveBeenCalledWith({
      useStealthChop: false
    })
  })
})

describe('MotorConfigDialog - Revert Functionality', () => {
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
    open: true,
    onOpenChange: vi.fn(),
    currentConfig: defaultConfig,
    onApply: vi.fn(),
    isConnected: true,
  }

  beforeEach(() => {
    vi.clearAllMocks()
  })

  it('should revert changes when revert button clicked', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const maxSpeedInput = screen.getByLabelText('Max Speed') as HTMLInputElement
    fireEvent.change(maxSpeedInput, { target: { value: '12000' } })

    expect(maxSpeedInput.value).toBe('12000')

    const revertButton = screen.getByText('Revert')
    fireEvent.click(revertButton)

    expect(maxSpeedInput.value).toBe('10000')
  })

  it('should disable revert button when no changes made', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const revertButton = screen.getByText('Revert')
    expect(revertButton).toBeDisabled()
  })

  it('should enable revert button when changes are made', () => {
    render(<MotorConfigDialog {...defaultProps} />)

    const maxSpeedInput = screen.getByLabelText('Max Speed')
    fireEvent.change(maxSpeedInput, { target: { value: '12000' } })

    const revertButton = screen.getByText('Revert')
    expect(revertButton).not.toBeDisabled()
  })
})
