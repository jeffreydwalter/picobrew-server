import { useEffect, useState } from 'react'
import {
  ArrowDown,
  ArrowUp,
  ArrowUpRight,
  RefreshCw,
  Square,
  Play,
  SlidersHorizontal,
  Settings2,
  Radio,
  BookOpen,
  Clock3,
  Copy,
  Loader2,
  Trash2,
  Pencil,
} from 'lucide-react'
import { useHub, useResource } from '../context'
import { api, errorMessage } from '../lib/api'
import type { Display, MachineRecipe, DeviceSettings, Telemetry, BrewDetail } from '../types'
import {
  Badge,
  Button,
  Confirm,
  Empty,
  ErrorBanner,
  Field,
  Loading,
  Metric,
  Modal,
  PageTitle,
  SectionHeading,
} from '../components/ui'
import OLED from '../components/OLED'
import { TransferDialog } from '../components/TransferDialog'
import { InstalledProgramDialog } from '../components/InstalledProgramDialog'
import { DeviceDialog, DeviceKeyNotice, RemoveDeviceDialog } from '../components/DeviceDialogs'
import { TemperatureChart } from '../components/Charts'
import { deviceState, duration, temperature } from '../lib/utils'
const programNames: Record<string, string> = {
  'sous-vide': 'Sous vide',
  'cold-brew': 'Cold brew',
  manual: 'Manual brew',
  rinse: 'Rinse',
  'first-rinse': 'First rinse',
  descale: 'Descale',
  circulate: 'Circulate',
  'drain-reservoir': 'Drain reservoir',
  'drain-filter': 'Drain filter',
  'rack-beer': 'Rack beer',
  utilities: 'Utilities',
  settings: 'Settings',
}
export default function Machine({ id }: { id: string }) {
  const hub = useHub()
  const device = hub.devices.find((device) => device.id === id)
  const display = useResource<Display>(device?.online ? `/devices/${id}/display` : null, 2500)
  const native = useResource<MachineRecipe[] | { recipes: MachineRecipe[] }>(
    device?.online ? `/devices/${id}/recipes` : null
  )
  const [program, setProgram] = useState('rinse')
  const [recipeID, setRecipeID] = useState('')
  const [pending, setPending] = useState('')
  const [confirm, setConfirm] = useState<{
    command: string
    recipe_id?: string
    label: string
  } | null>(null)
  const [settings, setSettings] = useState(false)
  const [editing, setEditing] = useState(false)
  const [removing, setRemoving] = useState(false)
  const [testingConnection, setTestingConnection] = useState(false)
  const [removeRecipe, setRemoveRecipe] = useState(false)
  const [editRecipe, setEditRecipe] = useState<MachineRecipe | null>(null)
  const [transferRecipes, setTransferRecipes] = useState(false)
  const [error, setError] = useState('')
  const history =
    hub.brews.find((brew) => brew.device_id === id && brew.state === 'recording') ||
    hub.brews.find((brew) => brew.device_id === id)
  const trace = useResource<BrewDetail>(
    history ? `/brews/${history.id}` : null,
    device?.status?.busy ? 10000 : 0
  )
  const recipes = Array.isArray(native.data) ? native.data : native.data?.recipes || []
  async function command(command: string, extra: Record<string, unknown> = {}) {
    setPending(command)
    setError('')
    try {
      await api(`/devices/${id}/commands`, {
        method: 'POST',
        body: { command, ...extra },
        command: true,
      })
      if (command !== 'encoder')
        hub.notify(
          command === 'stop'
            ? 'Stop command acknowledged.'
            : command === 'clear-stop'
              ? 'Stop latch cleared.'
              : 'Command acknowledged by the brewer.'
        )
      setConfirm(null)
      display.reload()
      await hub.refresh()
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setPending('')
    }
  }
  async function sync() {
    setPending('sync')
    try {
      await api(`/devices/${id}/sync-history`, { method: 'POST' })
      await hub.refresh()
      hub.notify('Retained brew history imported.')
    } catch (error) {
      hub.notify(errorMessage(error), 'error')
    } finally {
      setPending('')
    }
  }
  async function testConnection() {
    setTestingConnection(true)
    try {
      await api(`/devices/${id}/test`, { method: 'POST' })
      await hub.refresh()
      hub.notify('Machine responded successfully.')
    } catch (error) {
      hub.notify(errorMessage(error), 'error')
    } finally {
      setTestingConnection(false)
    }
  }
  if (hub.loading) return <Loading />
  if (!device)
    return (
      <Empty
        title="Machine not found"
        description="This machine may have been removed from your workspace."
        action={<Button onClick={() => hub.navigate('/overview')}>Return to overview</Button>}
      />
    )
  const state = deviceState(device)
  const status = device.status
  const commands = device.capabilities?.commands || []
  const programs = Object.keys(programNames).filter((name) => commands.includes(name))
  const allowed = (name: string) => device.online && commands.includes(name)
  return (
    <>
      <PageTitle
        className="machine-heading"
        back={() => hub.navigate('/overview')}
        eyebrow={`${device.model.replace(/-/g, ' ')} · ${device.location || 'YOUR BREWERY'}`}
        title={device.name}
        description={device.base_url || 'Virtual brewer in your simulation workspace'}
        actions={
          <>
            <Badge
              tone={
                !device.online
                  ? 'neutral'
                  : status?.stopped
                    ? 'danger'
                    : status?.busy
                      ? 'copper'
                      : 'green'
              }
            >
              {state}
            </Badge>
            <Button
              variant="ghost"
              disabled={testingConnection || !device.enabled}
              onClick={() => void testConnection()}
            >
              <RefreshCw size={16} className={testingConnection ? 'spin' : ''} />
              Test connection
            </Button>
            <Button variant="secondary" onClick={() => setEditing(true)}>
              <Pencil size={16} />
              Edit connection
            </Button>
            <Button variant="secondary" disabled={!device.online} onClick={() => setSettings(true)}>
              <SlidersHorizontal size={16} />
              Brewer preferences
            </Button>
            <Button variant="ghost" onClick={() => setRemoving(true)}>
              <Trash2 size={16} />
              Remove machine
            </Button>
          </>
        }
      />
      {device.error && <ErrorBanner message={device.error} />}
      {device.key_reused && <DeviceKeyNotice />}
      <ErrorBanner message={error} />
      <div className="machine-metrics">
        <Metric
          label="Wort temperature"
          value={temperature(status?.wort_f, hub.unit)}
          note="Current reading"
          accent
        />
        <Metric
          label="Thermoblock"
          value={temperature(status?.thermoblock_f, hub.unit)}
          note="Heating circuit"
        />
        <Metric
          label="Target"
          value={status?.target_f ? temperature(status.target_f, hub.unit) : 'Unheated'}
          note={status?.step_name || 'No active step'}
        />
        <Metric
          label="Step time"
          value={duration((status?.step_seconds || 0) * 1000)}
          note={
            status?.step_count ? `Step ${status.current_step + 1} of ${status.step_count}` : 'Idle'
          }
        />
      </div>
      <div className="machine-main">
        <section className="panel front-panel">
          <SectionHeading
            title="The front panel, wherever you are."
            description="Turn the dial, follow the prompts, stay in control."
            actions={
              status?.filter_inserted === false && <Badge tone="amber">Filter removed</Badge>
            }
          />
          <OLED display={display.data} error={!device.online ? 'Machine offline' : display.error} />
          <div className="dial-controls">
            <Button
              variant="secondary"
              aria-label="Move selection up"
              disabled={!allowed('encoder') || !!pending}
              onClick={() => void command('encoder', { delta: -1 })}
            >
              <ArrowUp size={20} />
            </Button>
            <Button
              disabled={!allowed('encoder') || !!pending}
              onClick={() => void command('encoder', { click: true })}
            >
              Press dial
            </Button>
            <Button
              variant="secondary"
              aria-label="Move selection down"
              disabled={!allowed('encoder') || !!pending}
              onClick={() => void command('encoder', { delta: 1 })}
            >
              <ArrowDown size={20} />
            </Button>
          </div>
          <p className="help-text">
            The display mirrors the physical brewer. Use its prompts to confirm preparation and
            advance through each program.
          </p>
        </section>
        <section className="panel program-panel">
          <SectionHeading
            title="What's brewing?"
            description={
              status?.busy
                ? status.recipe_name || 'A program is in progress.'
                : 'Choose a recipe or a maintenance program.'
            }
          />
          <Field label="Recipe on this machine">
            <select value={recipeID} onChange={(event) => setRecipeID(event.target.value)}>
              <option value="">Choose a saved recipe</option>
              {recipes.map((recipe) => (
                <option key={recipe.id} value={recipe.id}>
                  {recipe.name}
                </option>
              ))}
            </select>
          </Field>
          <Button
            className="full-width"
            disabled={!recipeID || !allowed('brew') || !!pending || !!status?.busy}
            onClick={() =>
              setConfirm({
                command: 'brew',
                recipe_id: recipeID,
                label: recipes.find((recipe) => recipe.id === recipeID)?.name || 'this recipe',
              })
            }
          >
            <Play size={16} />
            Start recipe
          </Button>
          <button className="text-button recipe-link" onClick={() => setTransferRecipes(true)}>
            <BookOpen size={15} />
            Send recipes from the library
            <ArrowUpRight size={14} />
          </button>
          <ErrorBanner message={native.error} retry={native.reload} />
          <div className="native-library">
            <small>
              {recipes.length} / {device.capabilities?.recipe_capacity || '—'} recipe slots used
            </small>
            <Button
              variant="ghost"
              disabled={
                !recipeID ||
                !device.online ||
                !status?.ready ||
                !!status?.busy ||
                !!status?.stopped ||
                !!pending
              }
              onClick={() =>
                setEditRecipe(recipes.find((recipe) => recipe.id === recipeID) || null)
              }
            >
              <Pencil size={13} />
              Edit saved program
            </Button>
            <Button
              variant="ghost"
              className="danger-text"
              disabled={
                !recipeID || !device.online || !!status?.busy || !!status?.stopped || !!pending
              }
              onClick={() => setRemoveRecipe(true)}
            >
              <Trash2 size={13} />
              Remove selected copy
            </Button>
          </div>
          <div className="divider" />
          <Field label="Other programs">
            <select value={program} onChange={(event) => setProgram(event.target.value)}>
              {programs.map((command) => (
                <option key={command} value={command}>
                  {programNames[command]}
                </option>
              ))}
            </select>
          </Field>
          <Button
            variant="secondary"
            className="full-width"
            disabled={!allowed(program) || !!pending || !!status?.busy}
            onClick={() =>
              setConfirm({ command: program, label: programNames[program] || program })
            }
          >
            Open program
            <ArrowUpRight size={16} />
          </Button>
          <div className="stop-row">
            <div>
              <strong>{status?.stopped ? 'Outputs are stopped' : 'Stop machine outputs'}</strong>
              <small>{status?.stop_reason || 'Immediately latch heating and pumping off.'}</small>
            </div>
            <Button
              variant="danger"
              disabled={!allowed('stop') || pending === 'stop'}
              onClick={() => void command('stop')}
            >
              <Square size={14} fill="currentColor" />
              Stop
            </Button>
          </div>
          {status?.stopped && (
            <Button
              variant="secondary"
              className="full-width"
              disabled={!allowed('clear-stop') || !!pending}
              onClick={() => setConfirm({ command: 'clear-stop', label: 'clear the stop latch' })}
            >
              Clear stop latch
            </Button>
          )}
        </section>
      </div>
      <section className="panel">
        <SectionHeading
          title="Readings through the brew"
          description={
            history
              ? `${history.recipe_name || history.command} · ${history.sample_count.toLocaleString()} recorded samples`
              : 'The hub records available telemetry while it is connected.'
          }
          actions={
            <div className="inline-actions">
              <Button
                variant="ghost"
                disabled={!device.online || !!pending}
                onClick={() => void sync()}
              >
                <RefreshCw size={15} />
                Import retained history
              </Button>
              {history && (
                <Button variant="secondary" onClick={() => hub.navigate(`/history/${history.id}`)}>
                  Open journal
                  <ArrowUpRight size={15} />
                </Button>
              )}
            </div>
          }
        />
        {trace.data?.samples?.length ? (
          <TemperatureChart samples={trace.data.samples} unit={hub.unit} />
        ) : (
          <Empty
            icon={<Radio size={24} />}
            title="Waiting for brew readings"
            description="Run a program to begin recording temperatures, phases and controller state."
          />
        )}
      </section>
      <div className="machine-footnotes">
        <span>Firmware {device.capabilities?.firmware_version || '—'}</span>
        <span>API v{device.capabilities?.api_version || '—'}</span>
        <span>UID {status?.device_uid || 'Not reported'}</span>
        <span>Uptime {duration(status?.uptime_ms || 0)}</span>
      </div>
      {confirm && (
        <Confirm
          title={
            confirm.command === 'clear-stop' ? 'Clear the stop latch?' : `Start ${confirm.label}?`
          }
          description={
            confirm.command === 'clear-stop'
              ? 'This re-enables machine controls. Confirm that the operation has ended and the brewer is ready.'
              : `This sends a command to ${device.name}. Confirm the brewer is prepared, then follow any prompts on its front panel.`
          }
          label={confirm.command === 'clear-stop' ? 'Clear stop' : 'Start program'}
          busy={!!pending}
          onClose={() => !pending && setConfirm(null)}
          onConfirm={() =>
            void command(confirm.command, {
              ...(confirm.recipe_id ? { recipe_id: confirm.recipe_id } : {}),
              confirm: true,
            })
          }
        />
      )}
      {removeRecipe && (
        <Confirm
          title="Remove this recipe from the machine?"
          description={`This frees a saved-recipe slot on ${device.name}. The shared recipe library remains available.`}
          label="Remove machine copy"
          danger
          busy={pending === 'remove-recipe'}
          onClose={() => setRemoveRecipe(false)}
          onConfirm={async () => {
            setPending('remove-recipe')
            setError('')
            try {
              await api(`/devices/${id}/recipes/${encodeURIComponent(recipeID)}`, {
                method: 'DELETE',
              })
              setRecipeID('')
              native.reload()
              setRemoveRecipe(false)
              hub.notify('Saved machine recipe removed.')
            } catch (error) {
              setError(errorMessage(error))
              setRemoveRecipe(false)
            } finally {
              setPending('')
            }
          }}
        />
      )}{' '}
      {settings && <MachineSettings id={id} onClose={() => setSettings(false)} />}
      {transferRecipes && (
        <TransferDialog
          recipes={hub.recipes}
          initialDeviceID={id}
          onClose={() => {
            setTransferRecipes(false)
            native.reload()
          }}
          onComplete={native.reload}
        />
      )}
      {editRecipe && (
        <InstalledProgramDialog
          deviceID={id}
          initial={editRecipe}
          onClose={() => setEditRecipe(null)}
          onSaved={native.reload}
        />
      )}
      {editing && <DeviceDialog device={device} onClose={() => setEditing(false)} />}
      {removing && (
        <RemoveDeviceDialog
          device={device}
          onClose={() => setRemoving(false)}
          onRemoved={() => hub.navigate('/overview')}
        />
      )}
    </>
  )
}
function MachineSettings({ id, onClose }: { id: string; onClose: () => void }) {
  const hub = useHub()
  const canForward = !!hub.devices.find((device) => device.id === id)?.capabilities
    ?.telemetry_forwarding
  const settings = useResource<DeviceSettings>(`/devices/${id}/settings`)
  const forwarding = useResource<Telemetry>(canForward ? `/devices/${id}/telemetry` : null)
  const [form, setForm] = useState<DeviceSettings | null>(null)
  const [telemetry, setTelemetry] = useState<Telemetry | null>(null)
  const [busy, setBusy] = useState('')
  const [error, setError] = useState('')
  useEffect(() => {
    if (settings.data) setForm(settings.data)
  }, [settings.data])
  useEffect(() => {
    if (forwarding.data) setTelemetry(forwarding.data)
  }, [forwarding.data])
  async function save(part: 'settings' | 'telemetry') {
    if (part === 'telemetry' && !canForward) return
    setBusy(part)
    setError('')
    try {
      await api(`/devices/${id}/${part}`, {
        method: part === 'settings' ? 'PATCH' : 'PUT',
        body:
          part === 'settings'
            ? form
            : { enabled: telemetry?.enabled, host: telemetry?.host, port: telemetry?.port },
      })
      hub.notify(
        part === 'settings' ? 'Machine preferences saved.' : 'Telemetry forwarding updated.'
      )
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy('')
    }
  }
  return (
    <Modal title="Brewer preferences" onClose={onClose} wide>
      <ErrorBanner message={error || settings.error || forwarding.error} />
      {!form ? (
        <Loading />
      ) : (
        <>
          <div className="settings-row compact">
            <div>
              <h3>Display preferences</h3>
              <p>Saved on the machine.</p>
            </div>
            <div className="settings-fields">
              <div className="form-grid">
                <Field label="Temperature unit">
                  <select
                    value={form.temperature_unit}
                    onChange={(event) =>
                      setForm({ ...form, temperature_unit: event.target.value as 'F' | 'C' })
                    }
                  >
                    <option value="F">Fahrenheit (°F)</option>
                    <option value="C">Celsius (°C)</option>
                  </select>
                </Field>
                <Field label="Front-panel language">
                  <select
                    value={form.language}
                    onChange={(event) => setForm({ ...form, language: event.target.value })}
                  >
                    <option value="en">English</option>
                    <option value="es">Español</option>
                    <option value="fr">Français</option>
                  </select>
                </Field>
              </div>
              <label className="check-field">
                <input
                  type="checkbox"
                  checked={form.detailed_status}
                  onChange={(event) => setForm({ ...form, detailed_status: event.target.checked })}
                />
                Show detailed status
              </label>
              <Button disabled={!!busy} onClick={() => void save('settings')}>
                Save preferences
              </Button>
            </div>
          </div>
          {!canForward && (
            <div className="settings-row compact">
              <div>
                <h3>Telemetry forwarding</h3>
                <p>
                  Unavailable for this machine adapter. The hub still collects available readings
                  while connected.
                </p>
              </div>
              <div className="settings-fields">
                <label className="check-field">
                  <input type="checkbox" disabled checked={false} readOnly />
                  Send readings to a receiver
                </label>
              </div>
            </div>
          )}
          {canForward && telemetry && (
            <div className="settings-row compact">
              <div>
                <h3>Telemetry forwarding</h3>
                <p>
                  Optionally send live readings to another receiver. Local history stays on the
                  machine.
                </p>
              </div>
              <div className="settings-fields">
                <label className="check-field">
                  <input
                    type="checkbox"
                    checked={telemetry.enabled}
                    onChange={(event) =>
                      setTelemetry({ ...telemetry, enabled: event.target.checked })
                    }
                  />
                  Send readings to a receiver
                </label>
                <div className="form-grid">
                  <Field label="Host name or IPv4 address">
                    <input
                      value={telemetry.host}
                      onChange={(event) => setTelemetry({ ...telemetry, host: event.target.value })}
                      placeholder="brewbox.local"
                    />
                  </Field>
                  <Field label="Port">
                    <input
                      type="number"
                      min="1"
                      max="65535"
                      value={telemetry.port}
                      onChange={(event) =>
                        setTelemetry({ ...telemetry, port: Number(event.target.value) })
                      }
                    />
                  </Field>
                </div>
                <Button disabled={!!busy} onClick={() => void save('telemetry')}>
                  Save forwarding
                </Button>
                {telemetry.status && (
                  <small className="help-text">
                    {telemetry.status.state} · Sent {telemetry.status.sent} · Failed{' '}
                    {telemetry.status.failed}
                  </small>
                )}
              </div>
            </div>
          )}
        </>
      )}
    </Modal>
  )
}
