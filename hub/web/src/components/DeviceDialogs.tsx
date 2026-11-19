import { useState, type FormEvent } from 'react'
import { KeyRound, Loader2, Plus, Save, Trash2 } from 'lucide-react'
import { useHub } from '../context'
import { api, errorMessage } from '../lib/api'
import type { Device } from '../types'
import { Button, ErrorBanner, Field, Modal } from './ui'

export function DeviceKeyNotice() {
  return (
    <div className="connection-advisory" role="status">
      <KeyRound size={16} />
      <p>
        The hub has the same saved API key for another machine. Use each brewer's own provisioned
        key when configuring its connection.
      </p>
    </div>
  )
}

export function DeviceDialog({ device, onClose }: { device?: Device; onClose: () => void }) {
  const hub = useHub()
  const [form, setForm] = useState({
    name: device?.name || '',
    model: device?.model || 'pico-pro',
    adapter: device?.adapter || (hub.session.demo ? 'simulator' : 'pico-standalone'),
    base_url: device?.base_url || '',
    api_key: '',
    location: device?.location || '',
    enabled: device?.enabled ?? true,
  })
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  async function submit(event: FormEvent) {
    event.preventDefault()
    if (busy) return
    setBusy(true)
    setError('')
    try {
      await api<Device>(device ? `/devices/${device.id}` : '/devices', {
        method: device ? 'PUT' : 'POST',
        body: form,
      })
      await hub.refresh()
      hub.notify(
        device ? 'Machine connection updated.' : 'Machine added. The hub is connecting now.'
      )
      onClose()
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  const models =
    form.adapter === 'simulator'
      ? hub.profiles.length
        ? hub.profiles.map((profile) => ({ id: profile.id, name: profile.name }))
        : [
            { id: 'pico-pro', name: 'Pico Pro' },
            { id: 'pico-s', name: 'Pico S' },
            { id: 'z', name: 'Pico Z' },
          ]
      : [
          { id: 'pico-pro', name: 'Pico Pro' },
          { id: 'pico-s', name: 'Pico S' },
        ]
  return (
    <Modal
      title={device ? 'Edit machine connection' : 'Add machine'}
      onClose={() => {
        if (!busy) onClose()
      }}
      actions={
        <>
          <Button variant="secondary" onClick={onClose} disabled={busy}>
            Cancel
          </Button>
          <Button form="device-form" type="submit" disabled={busy}>
            {busy ? (
              <Loader2 size={16} className="spin" />
            ) : device ? (
              <Save size={16} />
            ) : (
              <Plus size={16} />
            )}{' '}
            {device ? 'Save machine' : 'Add machine'}
          </Button>
        </>
      }
    >
      <form id="device-form" onSubmit={submit}>
        <div className="form-grid">
          <Field label="Machine name">
            <input
              autoFocus
              required
              maxLength={100}
              disabled={busy}
              value={form.name}
              onChange={(event) => setForm({ ...form, name: event.target.value })}
              placeholder="Pico Pro · Brew studio"
            />
          </Field>
          <Field label="Location">
            <input
              maxLength={160}
              disabled={busy}
              value={form.location}
              onChange={(event) => setForm({ ...form, location: event.target.value })}
              placeholder="Brew studio"
            />
          </Field>
          <Field
            label="Connection type"
            hint={device ? 'Fixed for this machine record.' : undefined}
          >
            <select
              disabled={!!device || busy}
              value={form.adapter}
              onChange={(event) =>
                setForm({
                  ...form,
                  adapter: event.target.value,
                  model: 'pico-pro',
                  base_url: '',
                  api_key: '',
                })
              }
            >
              {!hub.session.demo && (
                <option value="pico-standalone">Pico standalone firmware</option>
              )}
              <option value="simulator">Virtual brewer / simulator</option>
            </select>
          </Field>
          <Field label="Machine model">
            <select
              disabled={!!device || busy}
              value={form.model}
              onChange={(event) => setForm({ ...form, model: event.target.value })}
            >
              {models.map((model) => (
                <option key={model.id} value={model.id}>
                  {model.name}
                </option>
              ))}
            </select>
          </Field>
        </div>
        {form.adapter === 'pico-standalone' && (
          <>
            <Field
              label="Machine URL"
              hint="The hub must be able to reach this address on your network."
            >
              <input
                type="url"
                required
                maxLength={1024}
                disabled={busy}
                value={form.base_url}
                onChange={(event) => setForm({ ...form, base_url: event.target.value })}
                placeholder="http://picobrew.local"
              />
            </Field>
            <Field
              label="Machine API key"
              hint={
                device?.key_configured
                  ? 'Leave empty to keep the existing key. To replace it, enter the key already installed on this brewer.'
                  : 'Enter the key already provisioned on this brewer. Each physical machine should have its own key.'
              }
            >
              <input
                type="password"
                autoComplete="new-password"
                maxLength={1024}
                disabled={busy}
                required={!device?.key_configured}
                value={form.api_key}
                onChange={(event) => setForm({ ...form, api_key: event.target.value })}
              />
            </Field>
            {device?.key_reused && <DeviceKeyNotice />}
          </>
        )}
        <label className="check-field">
          <input
            type="checkbox"
            disabled={busy}
            checked={form.enabled}
            onChange={(event) => setForm({ ...form, enabled: event.target.checked })}
          />
          Monitor this machine
        </label>
        <ErrorBanner message={error} />
      </form>
    </Modal>
  )
}

export function RemoveDeviceDialog({
  device,
  onClose,
  onRemoved,
}: {
  device: Device
  onClose: () => void
  onRemoved?: () => void
}) {
  const hub = useHub()
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  async function remove() {
    if (busy) return
    setBusy(true)
    setError('')
    try {
      await api(`/devices/${device.id}`, { method: 'DELETE' })
      await hub.refresh()
      hub.notify('Machine removed from the workspace. Brew history is retained.')
      onClose()
      onRemoved?.()
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  return (
    <Modal
      title={`Remove ${device.name}?`}
      onClose={() => {
        if (!busy) onClose()
      }}
      actions={
        <>
          <Button variant="secondary" disabled={busy} onClick={onClose}>
            Cancel
          </Button>
          <Button variant="danger" disabled={busy} onClick={() => void remove()}>
            {busy ? <Loader2 size={16} className="spin" /> : <Trash2 size={16} />}
            Remove machine
          </Button>
        </>
      }
    >
      <p>
        The hub will stop monitoring {device.name}. Its recorded brew history stays in your
        workspace.
      </p>
      <p className="help-text">
        This removes the connection from the hub. It does not erase the brewer, change its recipes
        or stop an operation on the machine.
      </p>
      <ErrorBanner message={error} />
    </Modal>
  )
}
