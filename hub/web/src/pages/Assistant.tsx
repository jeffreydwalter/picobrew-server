import { useEffect, useState } from 'react'
import {
  Sparkles,
  WandSparkles,
  Activity,
  BookOpen,
  ArrowRight,
  ArrowUpRight,
  Send,
  Loader2,
  Lightbulb,
  Radio,
} from 'lucide-react'
import { useHub, useResource } from '../context'
import { api, errorMessage } from '../lib/api'
import { dateLabel } from '../lib/utils'
import type { AISettings, AIResult } from '../types'
import {
  Badge,
  Button,
  Empty,
  ErrorBanner,
  Field,
  Loading,
  PageTitle,
  SectionHeading,
} from '../components/ui'
const modes = [
  {
    id: 'recipe',
    label: 'Create a recipe',
    icon: WandSparkles,
    description: 'Start with an idea and shape it into a brewable recipe.',
  },
  {
    id: 'live',
    label: 'During the brew',
    icon: Activity,
    description: 'Ask about the readings and progression of a connected machine.',
  },
  {
    id: 'analysis',
    label: 'After the last pour',
    icon: BookOpen,
    description: 'Reflect on a recorded brew and plan what to improve.',
  },
]
const prompts: Record<string, string[]> = {
  recipe: [
    'A balanced citrus pale ale for a 5-liter batch.',
    'Build a rich porter with chocolate notes and a dry finish.',
    'Help me adapt a low-alcohol recipe without losing body.',
  ],
  live: [
    'Review the current temperatures and phase progression.',
    'Are there signs of unstable heating or an unusual temperature trend?',
    'Explain what is happening in this stage of the brew.',
  ],
  analysis: [
    'Analyze this brew and suggest improvements for the next batch.',
    'Compare target and measured temperatures across the session.',
    'What should I note about phase transitions and heating behavior?',
  ],
}
export default function Assistant() {
  const hub = useHub()
  const initialBrew = new URLSearchParams(location.hash.split('?')[1] || '').get('brew') || ''
  const [mode, setMode] = useState(initialBrew ? 'analysis' : 'recipe')
  const [prompt, setPrompt] = useState('')
  const [recipeID, setRecipeID] = useState('')
  const [deviceID, setDeviceID] = useState(hub.devices.find((device) => device.online)?.id || '')
  const [brewID, setBrewID] = useState(initialBrew || hub.brews[0]?.id || '')
  const [busy, setBusy] = useState(false)
  const [result, setResult] = useState<AIResult | null>(null)
  const [error, setError] = useState('')
  const settings = useResource<AISettings>('/ai/settings')
  const advice = useResource<
    {
      id: string
      created_at: string
      device_id?: string
      mode: string
      status: string
      text: string
      error?: string
      limitations?: string[]
    }[]
  >('/ai/advice', 30000)
  async function submit() {
    setBusy(true)
    setError('')
    try {
      setResult(
        await api<AIResult>('/ai/assist', {
          method: 'POST',
          body: {
            mode,
            prompt,
            ...(recipeID ? { recipe_id: recipeID } : {}),
            ...(mode === 'live' ? { device_id: deviceID } : {}),
            ...(mode === 'analysis' ? { brew_id: brewID } : {}),
          },
        })
      )
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  const enabled = settings.data?.enabled && settings.data?.key_configured
  return (
    <>
      <PageTitle
        eyebrow="A SECOND PERSPECTIVE ON YOUR CRAFT"
        title="The brew assistant."
        description="Explore ideas, understand your readings and learn from each batch."
        actions={
          settings.data && (
            <Badge tone={enabled ? 'green' : 'neutral'}>
              {enabled ? `${settings.data.provider} · ${settings.data.model}` : 'Not configured'}
            </Badge>
          )
        }
      />
      {!settings.loading && !enabled && (
        <div className="assistant-setup">
          <span className="assistant-symbol">
            <Sparkles size={25} />
          </span>
          <div>
            <h3>Bring your own brewing companion.</h3>
            <p>
              Add an OpenAI or Anthropic API key to enable the assistant. Your workspace works fully
              without it.
            </p>
          </div>
          <Button variant="secondary" onClick={() => hub.navigate('/settings')}>
            Configure assistant
            <ArrowUpRight size={16} />
          </Button>
        </div>
      )}
      <div className="assistant-layout">
        <div>
          <div className="assistant-modes" role="tablist" aria-label="Assistant task">
            {modes.map((item) => (
              <button
                role="tab"
                aria-selected={mode === item.id}
                key={item.id}
                className={mode === item.id ? 'active' : ''}
                onClick={() => {
                  setMode(item.id)
                  setResult(null)
                  setError('')
                }}
              >
                <item.icon size={19} />
                <strong>{item.label}</strong>
              </button>
            ))}
          </div>
          <section className="panel assistant-compose">
            <SectionHeading
              title={modes.find((item) => item.id === mode)?.label || ''}
              description={modes.find((item) => item.id === mode)?.description}
            />
            <div className="form-grid">
              {mode === 'live' && (
                <Field label="Connected machine">
                  <select value={deviceID} onChange={(event) => setDeviceID(event.target.value)}>
                    <option value="">Choose a machine</option>
                    {hub.devices.map((device) => (
                      <option key={device.id} value={device.id}>
                        {device.name}
                        {device.online ? '' : ' (offline)'}
                      </option>
                    ))}
                  </select>
                </Field>
              )}
              {mode === 'analysis' && (
                <Field label="Brew to analyze">
                  <select value={brewID} onChange={(event) => setBrewID(event.target.value)}>
                    <option value="">Choose a brew session</option>
                    {hub.brews.map((brew) => (
                      <option key={brew.id} value={brew.id}>
                        {brew.recipe_name || brew.command} · {brew.device_name}
                      </option>
                    ))}
                  </select>
                </Field>
              )}
              <Field
                label={
                  mode === 'recipe'
                    ? 'Existing recipe to build on (optional)'
                    : 'Recipe context (optional)'
                }
              >
                <select value={recipeID} onChange={(event) => setRecipeID(event.target.value)}>
                  <option value="">No recipe selected</option>
                  {hub.recipes.map((recipe) => (
                    <option key={recipe.id} value={recipe.id}>
                      {recipe.name}
                    </option>
                  ))}
                </select>
              </Field>
            </div>
            <Field label="What would you like to explore?">
              <textarea
                rows={6}
                value={prompt}
                onChange={(event) => setPrompt(event.target.value)}
                placeholder={
                  mode === 'recipe'
                    ? 'Describe the style, batch size, ingredients or character you have in mind…'
                    : mode === 'live'
                      ? 'Ask about the current brew. Recent readings are included as context.'
                      : 'What would you like to understand about this brew?'
                }
              />
            </Field>
            <ErrorBanner message={error || settings.error} />
            <div className="assistant-compose-footer">
              <small>Uses your configured provider. Responses may take a moment.</small>
              <Button
                disabled={
                  !enabled ||
                  !prompt.trim() ||
                  busy ||
                  (mode === 'live' && !deviceID) ||
                  (mode === 'analysis' && !brewID)
                }
                onClick={() => void submit()}
              >
                {busy ? <Loader2 size={16} className="spin" /> : <Sparkles size={16} />}{' '}
                {busy ? 'Thinking through your brew…' : 'Ask the assistant'}
              </Button>
            </div>
          </section>
          {result && (
            <section className="panel assistant-response">
              <SectionHeading
                title="A perspective to brew on."
                actions={<Badge tone="copper">AI SUGGESTION</Badge>}
              />
              <div className="assistant-response-body">
                {result.text.split(/\n\n+/).map((paragraph, index) => (
                  <p key={index}>{paragraph.replace(/^#{1,6}\s+/, '')}</p>
                ))}
              </div>
              {result.recipe && (
                <div className="ai-recipe-draft">
                  <div>
                    <p className="eyebrow">RECIPE DRAFT</p>
                    <h3>{result.recipe.name}</h3>
                    <p>
                      {result.recipe.style} · {result.recipe.batch_liters} L ·{' '}
                      {result.recipe.ingredients?.length || 0} ingredients ·{' '}
                      {result.recipe.steps?.length || 0} steps
                    </p>
                  </div>
                  <Button
                    onClick={() => {
                      hub.setDraft({ ...result.recipe!, id: '', revision: 0 })
                      hub.navigate('/recipes/new')
                    }}
                  >
                    Review in recipe editor
                    <ArrowRight size={16} />
                  </Button>
                </div>
              )}
              {result.limitations?.length > 0 && (
                <div className="ai-limitations">
                  <h4>Keep in mind</h4>
                  <ul>
                    {result.limitations.map((limitation, index) => (
                      <li key={index}>{limitation}</li>
                    ))}
                  </ul>
                </div>
              )}
            </section>
          )}
          {!!advice.data?.length && (
            <section className="panel advice-feed">
              <SectionHeading
                title="From your brew watch"
                description="Recent advice and analysis retained by the hub."
              />
              {advice.data.slice(0, 8).map((item) => (
                <details key={item.id}>
                  <summary>
                    <span>
                      <strong>
                        {item.device_id
                          ? hub.devices.find((device) => device.id === item.device_id)?.name ||
                            'Machine observation'
                          : item.mode === 'recipe'
                            ? 'Recipe exploration'
                            : 'Brew analysis'}
                      </strong>
                      <small>{dateLabel(item.created_at)}</small>
                    </span>
                    <Badge tone={item.status === 'error' ? 'amber' : 'green'}>
                      {item.status === 'error' ? 'Unavailable' : 'Advice'}
                    </Badge>
                  </summary>
                  <p>{item.status === 'error' ? item.error : item.text}</p>
                </details>
              ))}
            </section>
          )}
        </div>
        <aside className="assistant-aside">
          <div className="assistant-note">
            <Lightbulb size={27} strokeWidth={1.3} />
            <h3>A useful place to start.</h3>
            <div className="prompt-suggestions">
              {prompts[mode].map((suggestion) => (
                <button key={suggestion} onClick={() => setPrompt(suggestion)}>
                  {suggestion}
                  <ArrowUpRight size={14} />
                </button>
              ))}
            </div>
          </div>
          <div className="assistant-boundaries">
            <h3>You stay at the controls.</h3>
            <p>
              The assistant can suggest recipes and observations. It cannot start programs, move the
              dial or change heating and pumping.
            </p>
            <p>
              Review a draft in the recipe editor, save it, and preview its machine adaptation
              before sending it.
            </p>
            {settings.data?.monitor_enabled && (
              <p className="monitor-note">
                <Radio size={15} />
                Periodic brew advice is enabled in settings.
              </p>
            )}
          </div>
        </aside>
      </div>
    </>
  )
}
