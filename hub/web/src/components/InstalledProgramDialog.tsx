import { useState } from 'react'
import { RefreshCw, Save } from 'lucide-react'
import { useHub } from '../context'
import { api, errorMessage } from '../lib/api'
import { copyProgram } from '../lib/programs'
import type { MachineRecipe } from '../types'
import { Button, ErrorBanner, Field, Modal } from './ui'
import { ProgramFields } from './Program'

export function InstalledProgramDialog({
  deviceID,
  initial,
  onClose,
  onSaved,
}: {
  deviceID: string
  initial: MachineRecipe
  onClose: () => void
  onSaved: () => void
}) {
  const hub = useHub()
  const [program, setProgram] = useState(() => copyProgram(initial))
  const [expected, setExpected] = useState(() => copyProgram(initial))
  const [busy, setBusy] = useState('')
  const [error, setError] = useState('')
  const [refreshRequired, setRefreshRequired] = useState(false)
  const target = hub.devices.find((device) => device.id === deviceID)
  const ready =
    target?.online && target.status?.ready && !target.status.busy && !target.status.stopped
  async function save() {
    setBusy('save')
    setError('')
    try {
      await api(`/devices/${deviceID}/recipes/${encodeURIComponent(initial.id)}`, {
        method: 'PUT',
        body: { recipe: program, expected },
      })
      onSaved()
      hub.notify('Saved program verified on the machine.')
      onClose()
    } catch (error) {
      setError(errorMessage(error))
      setRefreshRequired(true)
    } finally {
      setBusy('')
    }
  }
  async function reload() {
    setBusy('reload')
    setError('')
    try {
      const result = await api<{ recipes: MachineRecipe[] }>(`/devices/${deviceID}/recipes`)
      const current = result.recipes.find((recipe) => recipe.id === initial.id)
      if (!current)
        throw new Error(
          'This program is no longer on the machine. Close the editor and refresh its recipe list.'
        )
      setProgram(copyProgram(current))
      setExpected(copyProgram(current))
      setRefreshRequired(false)
      onSaved()
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy('')
    }
  }
  return (
    <Modal
      title="Edit the machine’s saved program"
      wide
      onClose={() => {
        if (!busy) onClose()
      }}
      actions={
        <>
          <Button variant="secondary" disabled={!!busy} onClick={onClose}>
            Cancel
          </Button>
          {refreshRequired ? (
            <Button variant="secondary" disabled={!!busy} onClick={() => void reload()}>
              <RefreshCw size={16} />
              {busy === 'reload' ? 'Reading…' : 'Reload current machine copy'}
            </Button>
          ) : (
            <Button disabled={!!busy || !ready} onClick={() => void save()}>
              <Save size={16} />
              {busy === 'save' ? 'Saving & verifying…' : 'Save to machine'}
            </Button>
          )}
        </>
      }
    >
      <p>
        These changes update the same saved program shown in Brew PicoPak and the brewer’s browser
        interface.
      </p>
      <ErrorBanner message={error} />
      {refreshRequired && (
        <p className="notice">
          Read the current machine copy before saving again. Reloading replaces the editor contents
          with what the brewer reports.
        </p>
      )}
      {!ready && (
        <p className="notice">
          The brewer must be online, idle and ready before its saved programs can be edited.
        </p>
      )}
      <fieldset disabled={!!busy || refreshRequired} className="program-fieldset">
        <Field label="Saved program name">
          <input
            value={program.name}
            maxLength={31}
            onChange={(event) => setProgram((value) => ({ ...value, name: event.target.value }))}
          />
        </Field>
        <ProgramFields program={program} onChange={setProgram} device />
      </fieldset>
    </Modal>
  )
}
