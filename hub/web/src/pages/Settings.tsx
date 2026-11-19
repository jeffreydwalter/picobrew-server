import { useEffect, useState } from 'react'
import { Download } from 'lucide-react'
import { useHub } from '../context'
import { api, download, errorMessage } from '../lib/api'
import type { Profile } from '../types'
import { AISettingsForm } from '../components/AISettingsForm'
import { Button, ErrorBanner, Field, PageTitle } from '../components/ui'
export default function Settings() {
  const hub = useHub()
  return (
    <>
      <PageTitle
        eyebrow="MAKE IT YOUR BREWERY"
        title="Workspace settings."
        description="Choose how your hub plans, displays and records your brews."
      />
      <div className="settings-sheet">
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">01</span>
            <h2>Equipment calibration</h2>
            <p>Replace planning assumptions with measurements from your own equipment.</p>
          </div>
          <div className="settings-fields">
            <CalibrationForm />
          </div>
        </section>
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">02</span>
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
            <span className="section-index">03</span>
            <h2>Brew assistant</h2>
            <p>Optional AI help with recipe creation, live observations and post-brew analysis.</p>
          </div>
          <div className="settings-fields">
            <AISettingsForm />
          </div>
        </section>
        <section className="settings-row">
          <div className="settings-description">
            <span className="section-index">04</span>
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
      </div>
    </>
  )
}

function CalibrationForm() {
  const hub = useHub()
  const [id, setID] = useState('')
  const [form, setForm] = useState({
    batch_liters: hub.profiles[0]?.batch_liters ?? 0,
    efficiency: hub.profiles[0]?.efficiency ?? 0,
    max_grain_kg: hub.profiles[0]?.max_grain_kg ?? 0,
  })
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
        <Field
          label="Planning efficiency (%)"
          hint="Use your measured efficiency when available; the default is an estimate."
        >
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
        {selected?.user_configured
          ? 'Using your saved calibration values.'
          : 'Using starting defaults; efficiency is an estimate.'}{' '}
        These values apply to every machine of this model. Use measured values before relying on
        scaled recipes.
      </p>
      {selected && (selected.notes || (selected.sources?.length ?? 0) > 0) && (
        <details className="profile-notes">
          <summary>Profile notes</summary>
          {selected.notes && <p>{selected.notes}</p>}
          {Boolean(selected.sources?.length) && (
            <>
              <h4>Published starting points</h4>
              <ul>
                {selected.sources!.map((source) => (
                  <li key={source.url}>
                    <a href={source.url} target="_blank" rel="noreferrer">
                      {source.label}
                    </a>
                    <p>{source.description}</p>
                  </li>
                ))}
              </ul>
            </>
          )}
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
