import { useEffect, useMemo, useState } from 'react'
import {
  ArrowUpRight,
  Download,
  History,
  Star,
  Save,
  MessageSquarePlus,
  Filter,
  Activity,
  Clock3,
  FlaskConical,
  Sparkles,
  RefreshCw,
} from 'lucide-react'
import { useHub, useResource } from '../context'
import { api, download, errorMessage } from '../lib/api'
import type { BrewDetail } from '../types'
import { dateLabel, duration, clockTime, slug, temperature, transitions } from '../lib/utils'
import {
  Badge,
  Button,
  Empty,
  ErrorBanner,
  Field,
  Loading,
  Metric,
  PageTitle,
  SearchBox,
  SectionHeading,
} from '../components/ui'
import { TemperatureChart, StateChart } from '../components/Charts'
export default function HistoryPage({ id }: { id?: string }) {
  return id ? <BrewPage id={id} /> : <BrewList />
}
function BrewList() {
  const hub = useHub()
  const [query, setQuery] = useState('')
  const [device, setDevice] = useState('')
  const [state, setState] = useState('')
  const items = hub.brews.filter(
    (brew) =>
      (!device || brew.device_id === device) &&
      (!state || brew.state === state) &&
      `${brew.recipe_name} ${brew.device_name} ${brew.notes}`
        .toLowerCase()
        .includes(query.toLowerCase())
  )
  return (
    <>
      <PageTitle
        eyebrow="EVERY BREW TELLS A STORY"
        title="The brew journal."
        description="Live traces, completed sessions and the notes that make your next batch better."
        actions={
          <Button variant="secondary" onClick={() => void hub.refresh()}>
            <RefreshCw size={16} />
            Refresh history
          </Button>
        }
      />
      <div className="list-toolbar">
        <SearchBox value={query} onChange={setQuery} placeholder="Search brews or tasting notes" />
        <div className="filter-controls">
          <select
            aria-label="Filter by machine"
            value={device}
            onChange={(event) => setDevice(event.target.value)}
          >
            <option value="">All machines</option>
            {hub.devices.map((device) => (
              <option key={device.id} value={device.id}>
                {device.name}
              </option>
            ))}
          </select>
          <select
            aria-label="Filter by brew state"
            value={state}
            onChange={(event) => setState(event.target.value)}
          >
            <option value="">All sessions</option>
            {Array.from(new Set(hub.brews.map((brew) => brew.state))).map((state) => (
              <option key={state} value={state}>
                {state}
              </option>
            ))}
          </select>
        </div>
      </div>
      {items.length ? (
        <div className="panel history-list">
          <div className="table-scroll">
            <table className="data-table brew-table">
              <thead>
                <tr>
                  <th>Brew</th>
                  <th>Machine</th>
                  <th>Started</th>
                  <th>Duration</th>
                  <th>Samples</th>
                  <th>State</th>
                  <th>Rating</th>
                  <th />
                </tr>
              </thead>
              <tbody>
                {items.map((brew) => (
                  <tr key={brew.id}>
                    <td>
                      <button
                        className="table-link"
                        onClick={() => hub.navigate(`/history/${brew.id}`)}
                      >
                        <span className="brew-icon">
                          <FlaskConical size={17} />
                        </span>
                        <strong>{brew.recipe_name || brew.command || 'Brew session'}</strong>
                      </button>
                    </td>
                    <td>{brew.device_name}</td>
                    <td>{dateLabel(brew.started_at || brew.observed_at)}</td>
                    <td>{duration(brew.duration_ms)}</td>
                    <td>{brew.sample_count.toLocaleString()}</td>
                    <td>
                      <Badge
                        tone={
                          brew.state === 'completed'
                            ? 'green'
                            : brew.state === 'active' || brew.state === 'running'
                              ? 'copper'
                              : 'neutral'
                        }
                      >
                        {brew.state}
                      </Badge>
                    </td>
                    <td>
                      <span className="rating-small">
                        {brew.rating ? (
                          <>
                            <Star size={13} fill="currentColor" />
                            {brew.rating}
                          </>
                        ) : (
                          '—'
                        )}
                      </span>
                    </td>
                    <td>
                      <button
                        className="icon-button"
                        aria-label={`Open ${brew.recipe_name || brew.command} history`}
                        onClick={() => hub.navigate(`/history/${brew.id}`)}
                      >
                        <ArrowUpRight size={17} />
                      </button>
                    </td>
                  </tr>
                ))}
              </tbody>
            </table>
          </div>
        </div>
      ) : (
        <div className="panel">
          <Empty
            icon={<History size={26} />}
            title={
              query || device || state
                ? 'No brews match these filters.'
                : 'Your brewing story starts here.'
            }
            description={
              query || device || state
                ? 'Adjust your search to see more sessions.'
                : 'The hub records connected machines and imports their retained brew history automatically.'
            }
          />
        </div>
      )}
    </>
  )
}
function BrewPage({ id }: { id: string }) {
  const hub = useHub()
  const resource = useResource<BrewDetail>(`/brews/${id}`, 10000)
  const brew = resource.data
  const [notes, setNotes] = useState('')
  const [rating, setRating] = useState(0)
  const [dirty, setDirty] = useState(false)
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  const [marker, setMarker] = useState<number | null>(null)
  const [showSamples, setShowSamples] = useState(false)
  const [samplePage, setSamplePage] = useState(0)
  useEffect(() => {
    if (brew && !dirty) {
      setNotes(brew.notes || '')
      setRating(brew.rating || 0)
    }
  }, [brew, dirty])
  const events = useMemo(() => transitions(brew?.samples || []), [brew?.samples])
  async function save() {
    setBusy(true)
    setError('')
    try {
      await api(`/brews/${id}`, { method: 'PATCH', body: { notes, rating } })
      resource.setData((current) => (current ? { ...current, notes, rating } : current))
      setDirty(false)
      resource.reload()
      await hub.refresh()
      hub.notify('Brew notes saved.')
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  async function exportBrew(format: string) {
    try {
      await download(
        `/brews/${id}/export?format=${format}`,
        `${slug(brew?.recipe_name || 'brew')}-${id.slice(0, 8)}.${format}`
      )
    } catch (error) {
      hub.notify(errorMessage(error), 'error')
    }
  }
  if (resource.loading && !brew) return <Loading label="Opening the brew journal…" />
  if (!brew) return <ErrorBanner message={resource.error} retry={resource.reload} />
  const samples = brew.samples || []
  const last = samples.at(-1)
  return (
    <>
      <PageTitle
        back={() => hub.navigate('/history')}
        eyebrow={`${brew.device_name} · ${dateLabel(brew.started_at || brew.observed_at)}`}
        title={brew.recipe_name || brew.command || 'Brew session'}
        description={`${brew.model} · ${brew.source} · ${brew.recipe_revision ? `Recipe revision ${brew.recipe_revision}` : 'Recipe revision not recorded'}`}
        actions={
          <>
            <Badge tone={brew.state === 'completed' ? 'green' : 'copper'}>{brew.state}</Badge>
            <Button variant="secondary" onClick={() => void exportBrew('csv')}>
              <Download size={16} />
              CSV
            </Button>
            <Button variant="secondary" onClick={() => void exportBrew('json')}>
              <Download size={16} />
              JSON
            </Button>
          </>
        }
      />
      <ErrorBanner message={error || resource.error} />
      {(brew.gap_count > 0 || brew.truncated) && (
        <div className="notice">
          {brew.gap_count > 0 ? `${brew.gap_count} gaps were reported in this session. ` : ''}
          {brew.truncated ? 'The retained machine history was truncated. ' : ''}The chart shows the
          samples available to the hub.
        </div>
      )}
      <div className="machine-metrics">
        <Metric label="Brew duration" value={duration(brew.duration_ms)} note="Recorded session" />
        <Metric
          label="Sensor samples"
          value={samples.length.toLocaleString()}
          note={`${events.length} phase or state transitions`}
        />
        <Metric
          label="Last wort reading"
          value={temperature(last?.wort_f, hub.unit)}
          note="At the end of the available trace"
          accent
        />
        <Metric
          label="Your rating"
          value={
            rating ? (
              <>
                {rating}
                <small> / 5</small>
              </>
            ) : (
              'Unrated'
            )
          }
          note="A little hindsight helps"
        />
      </div>
      <section className="panel">
        <SectionHeading
          title="The complete temperature story"
          description="Select a point on the graph to add a timestamped note."
          actions={
            marker !== null && (
              <Button
                variant="secondary"
                onClick={() => {
                  setNotes((value) => `${value}${value ? '\n' : ''}[${clockTime(marker)}] `)
                  setDirty(true)
                  document.getElementById('brew-notes')?.focus()
                }}
              >
                <MessageSquarePlus size={15} />
                Note at {clockTime(marker)}
              </Button>
            )
          }
        />
        {samples.length ? (
          <TemperatureChart
            samples={samples}
            unit={hub.unit}
            notes={notes}
            height={340}
            onSelect={setMarker}
          />
        ) : (
          <Empty
            icon={<Activity size={24} />}
            title="No sensor samples are available."
            description="This session may have been discovered before a complete history could be imported."
          />
        )}
      </section>
      <section className="panel">
        <SectionHeading
          title="Phases, controller state & flags"
          description="Raw machine state values are retained alongside temperatures for comparison and diagnosis."
        />
        {samples.length > 0 && <StateChart samples={samples} />}
        <div className="chart-legend state-legend">
          <span>Phase</span>
          <span>Step</span>
          <span>Controller</span>
          <span>Flags</span>
          <span>Compartment</span>
          <span>Device state</span>
        </div>
      </section>
      <div className="journal-bottom">
        <section className="panel">
          <SectionHeading
            title="What did you notice?"
            description="Capture brew-day observations, tasting notes and what you would change."
            actions={
              <Button disabled={!dirty || busy} onClick={() => void save()}>
                <Save size={15} />
                {busy ? 'Saving…' : 'Save notes'}
              </Button>
            }
          />
          <div className="rating-control">
            <span>Rate this brew</span>
            {[1, 2, 3, 4, 5].map((value) => (
              <button
                aria-label={`Rate ${value} out of 5`}
                aria-pressed={rating === value}
                key={value}
                onClick={() => {
                  setRating(rating === value ? 0 : value)
                  setDirty(true)
                }}
              >
                <Star
                  size={23}
                  fill={value <= rating ? 'currentColor' : 'none'}
                  className={value <= rating ? 'filled' : ''}
                />
              </button>
            ))}
          </div>
          <Field
            label="Brew notes"
            hint="Timestamped annotations use [minutes:seconds] at the beginning of a line, for example [25:30] Stable mash temperature."
          >
            <textarea
              id="brew-notes"
              rows={8}
              value={notes}
              onChange={(event) => {
                setNotes(event.target.value)
                setDirty(true)
              }}
              placeholder="Aroma, clarity, flavor, process changes…"
            />
          </Field>
          <Button variant="ghost" onClick={() => hub.navigate(`/assistant?brew=${id}`)}>
            <Sparkles size={15} />
            Analyze this brew with AI
            <ArrowUpRight size={15} />
          </Button>
        </section>
        <section className="panel">
          <SectionHeading
            title="Session timeline"
            description="Changes observed in the machine's state."
          />
          <div className="timeline">
            {events.slice(0, 30).map((sample, index) => (
              <div key={index}>
                <span className="timeline-dot" />
                <time>{clockTime(sample.elapsed_ms)}</time>
                <div>
                  <strong>
                    Phase {sample.phase} · Step {sample.current_step + 1}
                  </strong>
                  <small>
                    Controller {sample.controller_status} · Compartment {sample.compartment} · Flags
                    0x{sample.flags.toString(16).padStart(2, '0')}
                  </small>
                  <small>
                    {temperature(sample.wort_f, hub.unit)} wort · Device state {sample.device_state}
                  </small>
                </div>
              </div>
            ))}
            {events.length > 30 && (
              <p className="help-text">
                {events.length - 30} more transitions are included in the raw samples and exports.
              </p>
            )}
          </div>
        </section>
      </div>
      <section className="panel">
        <SectionHeading
          title="Every recorded reading"
          description="Elapsed time, all available sensor values and machine state."
          actions={
            <Button variant="secondary" onClick={() => setShowSamples(!showSamples)}>
              {showSamples ? 'Hide' : 'Show'} raw samples
            </Button>
          }
        />
        {showSamples && (
          <>
            <div className="table-scroll">
              <table className="data-table sample-table">
                <thead>
                  <tr>
                    {[
                      'Time',
                      'Wort',
                      'Thermoblock',
                      'Target',
                      'Step time',
                      'Phase',
                      'Controller',
                      'Compartment',
                      'Step',
                      'Flags',
                      'State',
                      'Source',
                    ].map((name) => (
                      <th key={name}>{name}</th>
                    ))}
                  </tr>
                </thead>
                <tbody>
                  {samples.slice(samplePage * 100, (samplePage + 1) * 100).map((sample, index) => (
                    <tr key={index}>
                      <td>{clockTime(sample.elapsed_ms)}</td>
                      <td>{temperature(sample.wort_f, hub.unit)}</td>
                      <td>{temperature(sample.thermoblock_f, hub.unit)}</td>
                      <td>{temperature(sample.target_f, hub.unit)}</td>
                      <td>{duration(sample.step_seconds * 1000)}</td>
                      <td>{sample.phase}</td>
                      <td>{sample.controller_status}</td>
                      <td>{sample.compartment}</td>
                      <td>{sample.current_step}</td>
                      <td>0x{sample.flags.toString(16)}</td>
                      <td>{sample.device_state}</td>
                      <td>{sample.source}</td>
                    </tr>
                  ))}
                </tbody>
              </table>
            </div>
            <div className="pagination">
              <Button
                variant="secondary"
                disabled={!samplePage}
                onClick={() => setSamplePage((page) => page - 1)}
              >
                Previous
              </Button>
              <span>
                Page {samplePage + 1} of {Math.max(1, Math.ceil(samples.length / 100))}
              </span>
              <Button
                variant="secondary"
                disabled={(samplePage + 1) * 100 >= samples.length}
                onClick={() => setSamplePage((page) => page + 1)}
              >
                Next
              </Button>
            </div>
          </>
        )}
      </section>
    </>
  )
}
