import { useEffect, useRef, useState } from 'react'
import { Dialog, DialogContent, DialogHeader, DialogTitle, DialogFooter } from '@/components/ui/dialog'
import { Button } from '@/components/ui/button'
import type { Program } from '@/types'
import type { CommandError } from '@/hooks/useMotorController'

interface PatternEditorDialogProps {
  open: boolean
  onOpenChange: (open: boolean) => void
  program: Program
  commandError: CommandError | null
  isConnected: boolean
  onSave: (program: Program | null) => void // null restores the defaults
}

export function PatternEditorDialog({
  open,
  onOpenChange,
  program,
  commandError,
  isConnected,
  onSave
}: PatternEditorDialogProps) {
  const [text, setText] = useState('')
  const [error, setError] = useState<string | null>(null)
  const [saving, setSaving] = useState(false)
  const errorIdAtSave = useRef(0)

  useEffect(() => {
    if (open) {
      setText(JSON.stringify(program, null, 2))
      setError(null)
      setSaving(false)
    }
    // Only when opened, so a broadcast doesn't overwrite unsaved edits
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [open])

  // The controller answers a save with the new program, or with an error
  useEffect(() => {
    if (saving) {
      setSaving(false)
      onOpenChange(false)
    }
    // eslint-disable-next-line react-hooks/exhaustive-deps
  }, [program])

  useEffect(() => {
    if (saving && commandError && commandError.id !== errorIdAtSave.current) {
      setSaving(false)
      setError(commandError.message)
    }
  }, [commandError, saving])

  const send = (newProgram: Program | null) => {
    errorIdAtSave.current = commandError?.id ?? 0
    setError(null)
    setSaving(true)
    onSave(newProgram)
  }

  const handleSave = () => {
    try {
      send(JSON.parse(text))
    } catch (e) {
      setError(`Not valid JSON: ${(e as Error).message}`)
    }
  }

  return (
    <Dialog open={open} onOpenChange={onOpenChange}>
      <DialogContent className="max-w-2xl">
        <DialogHeader>
          <DialogTitle>Edit Patterns</DialogTitle>
        </DialogHeader>

        <div className="space-y-2 text-xs text-muted-foreground">
          <p>
            <b>presets</b>: quick positions, in % of travel. <b>patterns</b>: each has a <b>name</b>,
            {' '}<b>repeat</b> (0 for until stopped) and up to 8 <b>segments</b>.
          </p>
          <p>
            A segment moves <b>to</b> a % of travel, taking <b>seconds</b> or at a <b>speed</b> in % of Max
            Speed. It speeds up over the first <b>ramp</b> share of the distance and slows down over the last
            (0.05-0.5, where 0.5 peaks halfway), then waits <b>hold</b> seconds. A pattern first moves to where
            its last segment ends.
          </p>
        </div>

        <textarea
          aria-label="Patterns JSON"
          className="w-full h-80 rounded-md border bg-background p-2 font-mono text-xs"
          value={text}
          onChange={(e) => setText(e.target.value)}
          spellCheck={false}
        />
        {error && <p className="text-sm text-red-600">{error}</p>}

        <DialogFooter className="gap-2">
          <Button variant="outline" onClick={() => send(null)} disabled={!isConnected || saving}>
            Restore defaults
          </Button>
          <Button onClick={handleSave} disabled={!isConnected || saving}>
            {saving ? 'Saving...' : 'Save'}
          </Button>
        </DialogFooter>
      </DialogContent>
    </Dialog>
  )
}
