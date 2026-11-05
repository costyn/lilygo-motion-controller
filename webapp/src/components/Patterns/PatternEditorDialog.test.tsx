import { describe, it, expect, vi } from 'vitest'
import { render, screen, fireEvent } from '@testing-library/react'
import { PatternEditorDialog } from './PatternEditorDialog'
import type { Program } from '@/types'

const program: Program = {
  presets: [0, 50, 100],
  patterns: [{ name: 'Breath', repeat: 0, segments: [{ to: 100, speed: 20, ramp: 0.2, hold: 2 }] }]
}

const defaultProps = {
  open: true,
  onOpenChange: vi.fn(),
  program,
  commandError: null,
  isConnected: true,
  onSave: vi.fn()
}

describe('PatternEditorDialog', () => {
  it('should show the program as JSON', () => {
    render(<PatternEditorDialog {...defaultProps} />)
    expect(JSON.parse((screen.getByLabelText('Patterns JSON') as HTMLTextAreaElement).value)).toEqual(program)
  })

  it('should refuse text that is not JSON', () => {
    const onSave = vi.fn()
    render(<PatternEditorDialog {...defaultProps} onSave={onSave} />)

    fireEvent.change(screen.getByLabelText('Patterns JSON'), { target: { value: '{ presets: ' } })
    fireEvent.click(screen.getByRole('button', { name: 'Save' }))
    expect(onSave).not.toHaveBeenCalled()
    expect(screen.getByText(/Not valid JSON/)).toBeInTheDocument()
  })

  it('should close once the controller sends the saved program', () => {
    const onSave = vi.fn()
    const onOpenChange = vi.fn()
    const { rerender } = render(<PatternEditorDialog {...defaultProps} onSave={onSave} onOpenChange={onOpenChange} />)

    fireEvent.click(screen.getByRole('button', { name: 'Save' }))
    expect(onSave).toHaveBeenCalledWith(program)
    rerender(<PatternEditorDialog {...defaultProps} program={{ ...program }} onSave={onSave} onOpenChange={onOpenChange} />)
    expect(onOpenChange).toHaveBeenCalledWith(false)
  })

  it('should show the controller error and stay open', () => {
    const onOpenChange = vi.fn()
    const { rerender } = render(<PatternEditorDialog {...defaultProps} onOpenChange={onOpenChange} />)

    fireEvent.click(screen.getByRole('button', { name: 'Save' }))
    rerender(<PatternEditorDialog {...defaultProps} onOpenChange={onOpenChange}
      commandError={{ message: 'Ramp must be between 0.05 and 0.5', id: 1 }} />)
    expect(screen.getByText('Ramp must be between 0.05 and 0.5')).toBeInTheDocument()
    expect(onOpenChange).not.toHaveBeenCalled()
  })

  it('should send null to restore the defaults', () => {
    const onSave = vi.fn()
    render(<PatternEditorDialog {...defaultProps} onSave={onSave} />)
    fireEvent.click(screen.getByRole('button', { name: 'Restore defaults' }))
    expect(onSave).toHaveBeenCalledWith(null)
  })
})
