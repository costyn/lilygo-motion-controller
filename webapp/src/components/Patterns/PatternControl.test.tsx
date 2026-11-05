import { describe, it, expect, vi } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/react'
import { PatternControl } from './PatternControl'
import type { Pattern } from '@/types'

const patterns: Pattern[] = [
  { name: 'Butterfly', repeat: 0, segments: [{ to: 97, speed: 50, ramp: 0.5, hold: 2 }] },
  { name: 'Breath', repeat: 3, segments: [{ to: 100, speed: 20, ramp: 0.2, hold: 2 }] }
]

const defaultProps = {
  isConnected: true,
  isMoving: false,
  emergencyStop: false,
  patterns,
  runningPattern: '',
  round: 0,
  repeat: 0,
  onRun: vi.fn(),
  onStop: vi.fn(),
  onEdit: vi.fn()
}

describe('PatternControl', () => {
  it('should run the pattern that was clicked', () => {
    const onRun = vi.fn()
    render(<PatternControl {...defaultProps} onRun={onRun} />)

    fireEvent.click(screen.getByRole('button', { name: 'Breath' }))
    expect(onRun).toHaveBeenCalledWith(1)
  })

  it('should show progress and Stop while a pattern runs', () => {
    const onStop = vi.fn()
    render(<PatternControl {...defaultProps} runningPattern="Breath" round={2} repeat={3} onStop={onStop} />)

    expect(screen.getByText('Breath: round 2 of 3')).toBeInTheDocument()
    expect(screen.getByRole('button', { name: 'Butterfly' })).toBeDisabled()
    expect(screen.getByRole('button', { name: 'Edit' })).toBeDisabled()
    fireEvent.click(screen.getByRole('button', { name: 'Stop' }))
    expect(onStop).toHaveBeenCalled()
  })

  it('should leave out the round count for a pattern that runs until stopped', () => {
    render(<PatternControl {...defaultProps} runningPattern="Butterfly" round={4} />)
    expect(screen.getByText('Butterfly: round 4')).toBeInTheDocument()
  })

  it('should not start patterns before calibration', () => {
    render(<PatternControl {...defaultProps} needsCalibration={true} />)
    expect(screen.getByRole('button', { name: 'Butterfly' })).toBeDisabled()
    expect(screen.getByText('Calibrate first to run a pattern.')).toBeInTheDocument()
  })
})
