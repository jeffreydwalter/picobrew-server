import { useEffect, useState, type FormEvent } from 'react'
import {
  Plus,
  Trash2,
  Pencil,
  RefreshCw,
  Download,
  KeyRound,
  Wifi,
  Check,
  Globe,
  ArrowUpRight,
  Loader2,
} from 'lucide-react'
import { useHub, useResource } from '../context'
import { api, download, errorMessage } from '../lib/api'
import type { Device, AISettings, Profile } from '../types'
import {
  Badge,
  Button,
  Confirm,
  Empty,
  ErrorBanner,
  Field,
  IconButton,
  Loading,
  Modal,
  PageTitle,
  SectionHeading,
} from '../components/ui'
import { deviceState } from '../lib/utils'
export default function Settings() {
  const hub = useHub()
  const [editing, setEditing] = useState<Device | 'new' | null>(() =>
    location.hash.includes('new-device') ? 'new' : null
  )
  const [remove, setRemove] = useState<Device | null>(null)
  const [busy, setBusy] = useState('')
  const [error, setError] = useState('')
  useEffect(() => {
    if (location.hash.includes('new-device')) setEditing('new')
  }, [location.hash])
  async function test(device: Device) {
    setBusy(device.id)
    try {
      await api(`/devices/${device.id}/test`, { method: 'POST' })
      await hub.refresh()
      hub.notify(`${device.name} responded successfully.`)
    } catch (error) {
      hub.notify(errorMessage(error), 'error')
    } finally {
      setBusy('')
    }
  }
  return (
    <>
      <PageTitle
        eyebrow="MAKE IT YOUR BREWERY"
        title="Workspace settings."
        description="Connect your machines and choose how your hub works for you."
      />
      <div className="settings-sheet">
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">01</span>
            <h2>Your machines</h2>
            <p>
              Each machine keeps its local interface. The hub brings them together without replacing
              it.
            </p>
          </div>
          <div className="settings-fields">
            <div className="settings-section-action">
              <Button variant="secondary" onClick={() => setEditing('new')}>
                <Plus size={16} />
                Add machine
              </Button>
            </div>
            {hub.devices.length ? (
              <div className="connection-list">
                {hub.devices.map((device) => (
                  <div className="connection-item" key={device.id}>
                    <div>
                      <div className="connection-item-name">
                        <strong>{device.name}</strong>
                        <Badge tone={device.online ? 'green' : 'neutral'}>
                          {deviceState(device)}
                        </Badge>
                      </div>
                      <small>
                        {device.model} · {device.location || 'No location'} ·{' '}
                        {device.adapter === 'simulator' ? 'Virtual machine' : device.base_url}
                      </small>
                      {device.key_configured && (
                        <small>
                          <KeyRound size={11} />
                          Device key configured
                        </small>
                      )}
                    </div>
                    <div className="inline-actions">
                      <IconButton
                        label={`Test ${device.name}`}
                        disabled={busy === device.id}
                        onClick={() => void test(device)}
                      >
                        <RefreshCw size={16} className={busy === device.id ? 'spin' : ''} />
                      </IconButton>
                      <IconButton label={`Edit ${device.name}`} onClick={() => setEditing(device)}>
                        <Pencil size={16} />
                      </IconButton>
                      <IconButton label={`Remove ${device.name}`} onClick={() => setRemove(device)}>
                        <Trash2 size={16} />
                      </IconButton>
                    </div>
                  </div>
                ))}
              </div>
            ) : (
              <Empty
                title="A space for your brewers."
                description="Add a Pico S or Pro running standalone firmware, or a simulator to explore the hub."
              />
            )}
            <p className="help-text">
              For machines at another site, use an address reachable from the hub over your private
              network or VPN. Keep machine APIs on that trusted network.
            </p>
          </div>
        </section>
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">02</span>
            <h2>Equipment calibration</h2>
            <p>Replace planning assumptions with measurements from your own equipment.</p>
          </div>
          <div className="settings-fields">
            <CalibrationForm />
          </div>
        </section>
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">03</span>
            <h2>Display preferences</h2>
            <p>Choose how readings appear in this browser.</p>
          </div>
          <div className="settings-fields">
            <Field label="Temperature unit">
              <select
                className="short-field"
                value={hub.unit}
                onChange={(event) => hub.setUnit(event.target.value as 'C' | 'F')}
              >
                <option value="F">Fahrenheit (°F)</option>
                <option value="C">Celsius (°C)</option>
              </select>
            </Field>
            <p className="help-text">
              This changes hub charts and readings. Individual machine preferences are available
              from each machine's control page.
            </p>
          </div>
        </section>
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">04</span>
            <h2>Brew assistant</h2>
            <p>Optional AI help with recipe creation, live observations and post-brew analysis.</p>
          </div>
          <div className="settings-fields">
            <AISettingsForm />
          </div>
        </section>
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">05</span>
            <h2>Your data</h2>
            <p>A portable record of your recipes, equipment and brewing history.</p>
          </div>
          <div className="settings-fields">
            <h3>Export the workspace</h3>
            <p>
              Download recipes, revisions, machine records, brew sessions and their sensor samples
              as a ZIP archive.
            </p>
            <Button
              variant="secondary"
              onClick={async () => {
                try {
                  await download('/backup', 'picobrew-hub-export.zip')
                } catch (error) {
                  hub.notify(errorMessage(error), 'error')
                }
              }}
            >
              <Download size={16} />
              Export workspace
            </Button>
            <p className="help-text">
              Account passwords and API credentials are excluded. Complete disaster recovery uses a
              private backup of the server's data directory, including its encryption key.
            </p>
          </div>
        </section>
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">06</span>
            <h2>Connection & API</h2>
            <p>Build other clients on the same hub.</p>
          </div>
          <div className="settings-fields">
            <div className="connection-summary">
              <Badge tone={hub.connected ? 'green' : 'amber'}>
                {hub.connected ? 'Live stream connected' : 'Reconnecting'}
              </Badge>
              <code>/api/v1</code>
            </div>
            <p>
              Session authentication and a CSRF token protect changes. Commands carry unique
              identifiers to avoid accidental repeated delivery.
            </p>
            <Button variant="secondary" onClick={() => hub.navigate('/api')}>
              Explore the API
              <ArrowUpRight size={16} />
            </Button>
          </div>
        </section>
      </div>
      <ErrorBanner message={error} />
      {editing && (
        <DeviceDialog
          device={editing === 'new' ? undefined : editing}
          onClose={() => {
            setEditing(null)
            if (location.hash.includes('new-device')) hub.navigate('/settings')
          }}
        />
      )}
      {remove && (
        <Confirm
          title={`Remove ${remove.name}?`}
          description="The hub will stop monitoring this machine. Its own firmware, recipes and local interface remain available; existing hub brew history is retained."
          label="Remove machine"
          danger
          busy={busy === 'remove'}
          onClose={() => setRemove(null)}
          onConfirm={async () => {
            setBusy('remove')
            try {
              await api(`/devices/${remove.id}`, { method: 'DELETE' })
              await hub.refresh()
              hub.notify('Machine removed from the workspace.')
              setRemove(null)
            } catch (error) {
              setError(errorMessage(error))
              setRemove(null)
            } finally {
              setBusy('')
            }
          }}
        />
      )}
    </>
  )
}
function DeviceDialog({ device, onClose }: { device?: Device; onClose: () => void }) {
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
    setBusy(true)
    setError('')
    try {
      const saved = await api<Device>(device ? `/devices/${device.id}` : '/devices', {
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
      title={device ? 'Edit machine connection' : 'A new brewer on the bench.'}
      onClose={onClose}
      actions={
        <>
          <Button variant="secondary" onClick={onClose}>
            Cancel
          </Button>
          <Button form="device-form" type="submit" disabled={busy}>
            {busy ? <Loader2 size={16} className="spin" /> : <Plus size={16} />}{' '}
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
              value={form.name}
              onChange={(event) => setForm({ ...form, name: event.target.value })}
              placeholder="Pico Pro · Brew studio"
            />
          </Field>
          <Field label="Location">
            <input
              value={form.location}
              onChange={(event) => setForm({ ...form, location: event.target.value })}
              placeholder="Brew studio"
            />
          </Field>
          <Field label="Connection type">
            <select
              value={form.adapter}
              onChange={(event) =>
                setForm({ ...form, adapter: event.target.value, model: 'pico-pro' })
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
                value={form.base_url}
                onChange={(event) => setForm({ ...form, base_url: event.target.value })}
                placeholder="http://picobrew.local"
              />
            </Field>
            <Field
              label="Machine API key"
              hint={
                device?.key_configured
                  ? 'Leave empty to keep the existing key.'
                  : 'Use the key provisioned with your standalone firmware.'
              }
            >
              <input
                type="password"
                autoComplete="new-password"
                required={!device?.key_configured}
                value={form.api_key}
                onChange={(event) => setForm({ ...form, api_key: event.target.value })}
              />
            </Field>
          </>
        )}
        <label className="check-field">
          <input
            type="checkbox"
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
export function AISettingsForm() {
  const hub = useHub()
  const resource = useResource<AISettings>('/ai/settings')
  const [form, setForm] = useState<AISettings>({
    provider: 'openai',
    model: '',
    enabled: false,
    key_configured: false,
    monitor_enabled: false,
  })
  const [key, setKey] = useState('')
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  useEffect(() => {
    if (resource.data) setForm(resource.data)
  }, [resource.data])
  async function save(event: FormEvent) {
    event.preventDefault()
    setBusy(true)
    setError('')
    try {
      await api('/ai/settings', {
        method: 'PUT',
        body: {
          provider: form.provider,
          model: form.model,
          enabled: form.enabled,
          monitor_enabled: form.monitor_enabled,
          ...(key ? { api_key: key } : {}),
        },
      })
      setKey('')
      resource.reload()
      hub.notify('Brew assistant settings saved.')
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  if (resource.loading) return <Loading label="Loading assistant settings…" />
  return (
    <form onSubmit={save}>
      <label className="check-field">
        <input
          type="checkbox"
          checked={form.enabled}
          onChange={(event) => setForm({ ...form, enabled: event.target.checked })}
        />
        Enable the brew assistant
      </label>
      <div className="form-grid">
        <Field label="Provider">
          <select
            value={form.provider}
            onChange={(event) => setForm({ ...form, provider: event.target.value, model: '' })}
          >
            <option value="openai">OpenAI</option>
            <option value="anthropic">Anthropic / Claude</option>
          </select>
        </Field>
        <Field label="Model" hint="Use a model available to your provider account.">
          <input
            value={form.model}
            onChange={(event) => setForm({ ...form, model: event.target.value })}
            placeholder={
              form.provider === 'anthropic' ? 'Your Claude model ID' : 'Your OpenAI model ID'
            }
          />
        </Field>
      </div>
      <Field
        label="Provider API key"
        hint={
          form.key_configured
            ? 'A key is configured. Leave this blank to keep it.'
            : 'Use a provider API key; a ChatGPT or Claude subscription login is not an API key.'
        }
      >
        <input
          type="password"
          autoComplete="new-password"
          value={key}
          onChange={(event) => setKey(event.target.value)}
          placeholder={form.key_configured ? '•••••••• · key configured' : ''}
        />
      </Field>
      <label className="check-field">
        <input
          type="checkbox"
          checked={form.monitor_enabled || false}
          onChange={(event) => setForm({ ...form, monitor_enabled: event.target.checked })}
        />
        Check active brews every 5 minutes
      </label>
      <p className="help-text">
        Selected recipes and readings are sent to your chosen provider. Suggestions are advisory;
        the assistant cannot start a machine or change its outputs. You review recipe drafts and
        send changes yourself.
      </p>
      <ErrorBanner message={error || resource.error} />
      <Button type="submit" disabled={busy}>
        {busy ? 'Saving…' : 'Save assistant settings'}
      </Button>
    </form>
  )
}

function CalibrationForm() {
  const hub = useHub()
  const [id, setID] = useState('')
  const [form, setForm] = useState({ batch_liters: 5, efficiency: 70, max_grain_kg: 0 })
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  const selected = hub.profiles.find((profile) => profile.id === id)
  function choose(profile: Profile) {
    setID(profile.id)
    setForm({
      batch_liters: profile.batch_liters,
      efficiency: profile.efficiency,
      max_grain_kg: profile.max_grain_kg,
    })
    setError('')
  }
  useEffect(() => {
    if (!id && hub.profiles[0]) choose(hub.profiles[0])
  }, [hub.profiles, id])
  async function save(reset = false) {
    setBusy(true)
    setError('')
    try {
      const profile = await api<Profile>(`/profiles/${id}`, {
        method: reset ? 'DELETE' : 'PUT',
        ...(!reset ? { body: form } : {}),
      })
      choose(profile)
      await hub.refresh()
      hub.notify(reset ? 'Planning defaults restored.' : 'Equipment calibration saved.')
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  return (
    <>
      <Field label="Equipment model">
        <select
          value={id}
          onChange={(event) => {
            const profile = hub.profiles.find((profile) => profile.id === event.target.value)
            if (profile) choose(profile)
          }}
        >
          {hub.profiles.map((profile) => (
            <option key={profile.id} value={profile.id}>
              {profile.name}
            </option>
          ))}
        </select>
      </Field>
      <div className="form-grid thirds">
        <Field label="Usable batch size (L)">
          <input
            type="number"
            min="0.1"
            step="0.1"
            value={form.batch_liters}
            onChange={(event) => setForm({ ...form, batch_liters: Number(event.target.value) })}
          />
        </Field>
        <Field label="Measured efficiency (%)">
          <input
            type="number"
            min="1"
            max="100"
            step="0.1"
            value={form.efficiency}
            onChange={(event) => setForm({ ...form, efficiency: Number(event.target.value) })}
          />
        </Field>
        <Field label="Grain capacity (kg)" hint="0 means unverified.">
          <input
            type="number"
            min="0"
            step="0.1"
            value={form.max_grain_kg}
            onChange={(event) => setForm({ ...form, max_grain_kg: Number(event.target.value) })}
          />
        </Field>
      </div>
      <p className="help-text">
        Shared model defaults apply to every machine of this model. Starting values are planning
        assumptions; use measured values before relying on scaled recipes.
      </p>
      {selected?.notes && (
        <details className="profile-notes">
          <summary>Profile notes</summary>
          <p>{selected.notes}</p>
        </details>
      )}
      <ErrorBanner message={error} />
      <div className="inline-actions">
        <Button disabled={!id || busy} onClick={() => void save()}>
          Save calibration
        </Button>
        <Button variant="ghost" disabled={!id || busy} onClick={() => void save(true)}>
          Restore planning defaults
        </Button>
      </div>
    </>
  )
}
