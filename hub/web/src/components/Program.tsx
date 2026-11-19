import { ArrowDown, ArrowUp, Plus, Trash2 } from 'lucide-react'
import type { MachineRecipe, MachineStep, RecipeProvenance } from '../types'
import { externalURL, programTiming } from '../lib/programs'
import { Button, Field, IconButton, SectionHeading } from './ui'

export function ProgramTable({ program }: { program: MachineRecipe }) {
  return (
    <div className="table-scroll">
      <table className="program-table">
        <thead>
          <tr>
            <th>Step</th>
            <th>Compartment</th>
            <th>Target</th>
            <th>Hold</th>
            <th>Drain</th>
          </tr>
        </thead>
        <tbody>
          {program.steps.map((step, index) => (
            <tr key={index}>
              <td>
                <span className="muted">{index + 1}. </span>
                {step.name}
              </td>
              <td>{step.location}</td>
              <td>{step.target_f === 0 ? 'Unheated' : `${step.target_f} °F`}</td>
              <td>
                {step.minutes === 0
                  ? step.target_f === 0
                    ? 'Advance immediately'
                    : 'Reach target'
                  : `${step.minutes} min`}
              </td>
              <td>{step.drain_minutes} min</td>
            </tr>
          ))}
        </tbody>
      </table>
      <p className="help-text">
        {program.steps.length} steps · {programTiming(program)}. Heating time depends on the brewer.
      </p>
    </div>
  )
}

export function ProgramFields({
  program,
  onChange,
  device = false,
}: {
  program: MachineRecipe
  onChange: (program: MachineRecipe) => void
  device?: boolean
}) {
  function updateStep(index: number, patch: Partial<MachineStep>) {
    onChange({
      ...program,
      steps: program.steps.map((step, i) => (i === index ? { ...step, ...patch } : step)),
    })
  }
  function move(index: number, delta: number) {
    const steps = [...program.steps]
    ;[steps[index], steps[index + delta]] = [steps[index + delta], steps[index]]
    onChange({ ...program, steps })
  }
  return (
    <>
      <SectionHeading
        title="Machine program"
        description="Temperatures, timings and compartments are stored exactly as entered. A zero-minute hold reaches the target, then advances."
        actions={
          <Button
            variant="secondary"
            disabled={program.steps.length >= (device ? 15 : 256)}
            onClick={() =>
              onChange({
                ...program,
                steps: [
                  ...program.steps,
                  { name: 'New step', location: 1, target_f: 152, minutes: 10, drain_minutes: 0 },
                ],
              })
            }
          >
            <Plus size={15} />
            Add program step
          </Button>
        }
      />
      <Field
        label="Machine recipe ID"
        hint="This ID identifies the saved slot. Changing it in a library copy creates a separate program when transferred."
      >
        <input
          value={program.id}
          readOnly={device}
          onChange={(event) => onChange({ ...program, id: event.target.value })}
          maxLength={20}
        />
      </Field>
      <div className="recipe-steps">
        {program.steps.map((step, index) => (
          <div className="recipe-step" key={index}>
            <span className="step-number">{String(index + 1).padStart(2, '0')}</span>
            <div className="step-fields">
              <Field label={`Program step ${index + 1} name`}>
                <input
                  value={step.name}
                  maxLength={device ? 21 : 512}
                  onChange={(event) => updateStep(index, { name: event.target.value })}
                />
              </Field>
              <div className="form-grid quarters">
                <Field label={`Step ${index + 1} compartment`}>
                  <input
                    type="number"
                    min={device ? '1' : '0'}
                    max="6"
                    step="1"
                    value={step.location}
                    onChange={(event) =>
                      updateStep(index, { location: Number(event.target.value) })
                    }
                  />
                </Field>
                <Field label={`Step ${index + 1} target (°F)`} hint="0 = unheated">
                  <input
                    type="number"
                    min="0"
                    max="212"
                    step="1"
                    value={step.target_f}
                    onChange={(event) =>
                      updateStep(index, { target_f: Number(event.target.value) })
                    }
                  />
                </Field>
                <Field label={`Step ${index + 1} hold (min)`} hint="0 = reach target">
                  <input
                    type="number"
                    min="0"
                    max={device ? 599 : 43200}
                    step="1"
                    value={step.minutes}
                    onChange={(event) => updateStep(index, { minutes: Number(event.target.value) })}
                  />
                </Field>
                <Field label={`Step ${index + 1} drain (min)`}>
                  <input
                    type="number"
                    min="0"
                    max={device ? 120 : 1440}
                    step="1"
                    value={step.drain_minutes}
                    onChange={(event) =>
                      updateStep(index, { drain_minutes: Number(event.target.value) })
                    }
                  />
                </Field>
              </div>
            </div>
            <div className="step-actions">
              <IconButton
                label={`Move program step ${index + 1} up`}
                disabled={!index}
                onClick={() => move(index, -1)}
              >
                <ArrowUp size={15} />
              </IconButton>
              <IconButton
                label={`Move program step ${index + 1} down`}
                disabled={index === program.steps.length - 1}
                onClick={() => move(index, 1)}
              >
                <ArrowDown size={15} />
              </IconButton>
              <IconButton
                label={`Remove program step ${index + 1}`}
                disabled={program.steps.length <= 1}
                onClick={() =>
                  onChange({ ...program, steps: program.steps.filter((_, i) => i !== index) })
                }
              >
                <Trash2 size={15} />
              </IconButton>
            </div>
          </div>
        ))}
      </div>
    </>
  )
}

export function Provenance({ value }: { value?: RecipeProvenance }) {
  if (!value) return null
  const url = externalURL(value.source_url)
  return (
    <details className="source-provenance">
      <summary>Source & attribution</summary>
      <dl>
        <div>
          <dt>Archive</dt>
          <dd>{value.source_id}</dd>
        </div>
        <div>
          <dt>Original record</dt>
          <dd>
            {url ? (
              <a href={url} target="_blank" rel="noreferrer">
                {value.path}
              </a>
            ) : (
              value.path
            )}
          </dd>
        </div>
        <div>
          <dt>Source revision</dt>
          <dd>
            <code>{value.commit}</code>
          </dd>
        </div>
        <div>
          <dt>License</dt>
          <dd>{value.license}</dd>
        </div>
        <div>
          <dt>Source SHA-256</dt>
          <dd>
            <code>{value.sha256}</code>
          </dd>
        </div>
      </dl>
      {value.warnings?.length > 0 && (
        <ul>
          {value.warnings.map((warning, index) => (
            <li key={index}>{warning}</li>
          ))}
        </ul>
      )}
    </details>
  )
}
