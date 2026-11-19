import { useEffect, useRef, useState } from 'react'
import {
  ArrowUpRight,
  ArrowRight,
  Plus,
  Clock3,
  FlaskConical,
  BookOpen,
  Radio,
  WifiOff,
  Pencil,
  Trash2,
  KeyRound,
} from 'lucide-react'
import { useHub, useResource } from '../context'
import type { BrewDetail, Device } from '../types'
import {
  Badge,
  Button,
  Empty,
  IconButton,
  Loading,
  Metric,
  PageTitle,
  SectionHeading,
} from '../components/ui'
import { DeviceDialog, RemoveDeviceDialog } from '../components/DeviceDialogs'
import { Sparkline, TemperatureChart } from '../components/Charts'
import { dateLabel, deviceState, duration, temperature } from '../lib/utils'
export default function Fleet({ addMachine = false }: { addMachine?: boolean }) {
  const hub = useHub()
  const { devices, brews, recipes, unit, navigate, loading } = hub
  const [editing, setEditing] = useState<Device | 'new' | null>(addMachine ? 'new' : null)
  const [removing, setRemoving] = useState<Device | null>(null)
  useEffect(() => {
    if (addMachine) setEditing('new')
  }, [addMachine])
  function closeEditor() {
    setEditing(null)
    if (addMachine) navigate('/overview')
  }
  const active = devices.filter((device) => device.online && device.status?.busy)
  const focus = active[0]
  const lastBrew = focus ? brews.find((brew) => brew.device_id === focus.id) : brews[0]
  const trace = useResource<BrewDetail>(
    lastBrew ? `/brews/${lastBrew.id}` : null,
    focus ? 10000 : 0
  )
  const [traces, setTraces] = useState<Record<string, { value: number | null }[]>>({})
  const previous = useRef('')
  useEffect(() => {
    const signature = JSON.stringify(devices.map((device) => [device.id, device.status?.uptime_ms]))
    if (signature === previous.current) return
    previous.current = signature
    setTraces((old) => {
      const next = { ...old }
      for (const device of devices)
        next[device.id] = [
          ...(old[device.id] || []),
          { value: device.status?.wort_f ?? null },
        ].slice(-40)
      return next
    })
  }, [devices])
  if (loading) return <Loading />
  return (
    <>
      <PageTitle
        eyebrow="YOUR BREWERY AT A GLANCE"
        title="A good day to brew."
        description="Every machine, every recipe, every moment of the brew."
        actions={
          <>
            <Button variant="secondary" onClick={() => setEditing('new')}>
              <Plus size={16} />
              Add machine
            </Button>
            <Button onClick={() => navigate('/recipes/new')}>
              <Plus size={16} />
              Create recipe
            </Button>
          </>
        }
      />
      <div className="overview-stats">
        <Metric
          label="Machines connected"
          value={
            <>
              {devices.filter((d) => d.online).length}
              <small> / {devices.length}</small>
            </>
          }
          note="Across your brewery"
        />
        <Metric
          label="Brews in progress"
          value={active.length}
          note={active.length ? 'Keeping an eye on every step' : 'Ready when you are'}
          accent
        />
        <Metric
          label="In your recipe library"
          value={recipes.length}
          note="One collection, all your machines"
        />
        <Metric
          label="Brew days recorded"
          value={brews.length}
          note="Your craft, captured over time"
        />
      </div>
      <SectionHeading
        title="On the brew bench"
        description={
          devices.length
            ? `${devices.length} ${devices.length === 1 ? 'machine' : 'machines'} · readings update automatically`
            : 'Connect a machine to start your workspace'
        }
        actions={
          <span className="live-label">
            <i />
            LIVE READINGS
          </span>
        }
      />
      {devices.length ? (
        <div className="fleet-grid">
          {devices.map((device) => (
            <MachineCard
              key={device.id}
              device={device}
              values={traces[device.id] || []}
              onEdit={() => setEditing(device)}
              onRemove={() => setRemoving(device)}
            />
          ))}
        </div>
      ) : (
        <div className="panel">
          <Empty
            title="Make room for your first brewer."
            description="Connect a Pico running standalone firmware, then manage it here alongside your other machines."
            action={
              <Button onClick={() => setEditing('new')}>
                <Plus size={16} />
                Connect a machine
              </Button>
            }
          />
        </div>
      )}
      <div className="overview-bottom">
        <section className="panel trace-panel">
          <SectionHeading
            title={
              focus
                ? 'Brew day, in motion.'
                : lastBrew
                  ? 'A look at your last brew.'
                  : 'The story is in the readings.'
            }
            description={
              focus
                ? `${focus.name} · ${focus.status?.recipe_name || focus.status?.step_name || 'Program in progress'}`
                : lastBrew
                  ? `${lastBrew.recipe_name} · ${lastBrew.device_name}`
                  : 'Temperatures and phases will appear as you brew.'
            }
            actions={
              lastBrew && (
                <Button variant="ghost" onClick={() => navigate(`/history/${lastBrew.id}`)}>
                  Open journal
                  <ArrowUpRight size={15} />
                </Button>
              )
            }
          />
          {trace.data?.samples?.length ? (
            <TemperatureChart samples={trace.data.samples} unit={unit} height={255} />
          ) : (
            <Empty
              icon={<Radio size={25} />}
              title={focus ? 'Listening to your brew.' : 'Ready for a fresh trace.'}
              description={
                focus
                  ? 'The first sensor samples will appear here shortly.'
                  : 'Start a brew and capture its temperatures and phases from beginning to end.'
              }
            />
          )}
        </section>
        <section className="panel recent-panel">
          <SectionHeading
            title="From your brew journal"
            actions={
              <button className="text-button" onClick={() => navigate('/history')}>
                <ArrowRight size={18} />
              </button>
            }
          />
          {brews.length ? (
            <div className="recent-list">
              {brews.slice(0, 4).map((brew) => (
                <button key={brew.id} onClick={() => navigate(`/history/${brew.id}`)}>
                  <span className="brew-icon">
                    <FlaskConical size={19} />
                  </span>
                  <span>
                    <strong>{brew.recipe_name || brew.command || 'Brew session'}</strong>
                    <small>
                      {brew.device_name} · {dateLabel(brew.started_at || brew.observed_at)}
                    </small>
                  </span>
                  <ArrowUpRight size={16} />
                </button>
              ))}
            </div>
          ) : (
            <Empty
              icon={<BookOpen size={23} />}
              title="Your first chapter awaits."
              description="Completed and live brews are collected here automatically."
            />
          )}
        </section>
      </div>
      {editing && (
        <DeviceDialog device={editing === 'new' ? undefined : editing} onClose={closeEditor} />
      )}
      {removing && <RemoveDeviceDialog device={removing} onClose={() => setRemoving(null)} />}
    </>
  )
}
function MachineCard({
  device,
  values,
  onEdit,
  onRemove,
}: {
  device: Device
  values: { value: number | null }[]
  onEdit: () => void
  onRemove: () => void
}) {
  const { unit } = useHub()
  const status = device.status
  const state = deviceState(device)
  const progress = status?.step_count
    ? Math.min(100, ((status.current_step + 1) / status.step_count) * 100)
    : 0
  return (
    <article className={`machine-card ${status?.busy ? 'brewing' : ''}`}>
      <div className="machine-card-top">
        <div>
          <p className="eyebrow">
            {device.model.replace(/-/g, ' ')}
            {device.adapter === 'simulator' ? ' · VIRTUAL' : ''}
          </p>
          <h3>{device.name}</h3>
          <p className="machine-location">{device.location || 'Your brewery'}</p>
        </div>
        <div className="machine-card-status">
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
          {device.key_reused && (
            <span
              className="machine-key-notice"
              title="The hub has the same saved API key for another machine."
            >
              <KeyRound size={12} />
              Shared API key
            </span>
          )}
        </div>
      </div>
      <div className="machine-card-body">
        <BrewerIllustration active={!!status?.busy} />
        <div className="machine-readings">
          <span>WORT TEMPERATURE</span>
          <strong>{device.online ? temperature(status?.wort_f, unit) : '—'}</strong>
          <small>Target {temperature(status?.target_f, unit)}</small>
          {values.length > 1 ? (
            <Sparkline values={values} />
          ) : (
            <div className="trace-wait">
              {device.online ? 'Collecting a live trace' : 'Awaiting connection'}
            </div>
          )}
        </div>
      </div>
      <div className="machine-phase">
        <div>
          <span>
            {!device.online ? (
              <>
                <WifiOff size={14} />
                Connection unavailable
              </>
            ) : status?.stopped ? (
              'Outputs stopped'
            ) : status?.busy ? (
              status.step_name || `Phase ${status.phase}`
            ) : (
              'Ready for the next recipe'
            )}
          </span>
          {status?.busy && (
            <small>
              <Clock3 size={12} />
              {duration(status.step_seconds * 1000)}
            </small>
          )}
        </div>
        <div className="progress-track">
          <span style={{ width: `${progress}%` }} />
        </div>
      </div>
      <footer>
        <span>
          {status?.recipe_name ||
            (device.capabilities?.firmware_version &&
              `Firmware ${device.capabilities.firmware_version}`) ||
            'Standalone brewer'}
        </span>
        <a
          className="machine-card-link"
          href={`#/devices/${device.id}`}
          aria-label={`Open ${device.name} controls`}
        >
          Open controls <ArrowUpRight size={16} />
        </a>
        <div className="machine-card-actions">
          <IconButton label={`Edit ${device.name}`} onClick={onEdit}>
            <Pencil size={16} />
          </IconButton>
          <IconButton label={`Remove ${device.name}`} onClick={onRemove}>
            <Trash2 size={16} />
          </IconButton>
        </div>
      </footer>
    </article>
  )
}
function BrewerIllustration({ active }: { active: boolean }) {
  return (
    <svg className="brewer-illustration" viewBox="0 0 140 160" aria-hidden="true">
      <ellipse cx="72" cy="148" rx="51" ry="6" fill="#dfd9cf" />
      <path d="M30 25Q30 17 38 17H105Q113 17 113 25V140H30Z" fill="#d9d8d0" />
      <path d="M30 25Q30 17 38 17H105Q113 17 113 25V49H30Z" fill="#45483f" />
      <path d="M38 52H105V136H38Z" fill="#ecebe5" />
      <rect x="45" y="28" width="39" height="12" rx="2" fill="#1c2b23" />
      <path d="M50 34H77M50 37H69" stroke={active ? '#c4dbab' : '#79876d'} strokeWidth="1.5" />
      <circle cx="98" cy="34" r="5" fill="#c4c6bc" />
      <path d="M43 64H102V75H43Z" fill="#cbc9bf" />
      <path d="M48 78H98V119Q98 125 92 125H54Q48 125 48 119Z" fill="#9d8770" />
      <path d="M53 81H93V118H53Z" fill={active ? '#b66c3c' : '#c1aa8f'} opacity=".75" />
      <rect x="62" y="58" width="22" height="5" rx="2" fill="#6e7167" />
      <rect x="35" y="140" width="12" height="5" rx="2" fill="#45483f" />
      <rect x="96" y="140" width="12" height="5" rx="2" fill="#45483f" />
      {active && (
        <>
          <path
            d="M58 11Q53 6 58 1M72 11Q67 6 72 1M86 11Q81 6 86 1"
            fill="none"
            stroke="#bca68f"
            strokeWidth="1.5"
            strokeLinecap="round"
          />
        </>
      )}
    </svg>
  )
}
