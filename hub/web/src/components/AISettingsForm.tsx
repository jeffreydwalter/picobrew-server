import { useCallback, useEffect, useRef, useState, type FormEvent } from 'react'
import { RefreshCw } from 'lucide-react'
import { useHub, useResource } from '../context'
import { api, APIError, errorMessage } from '../lib/api'
import type { AIModel, AIModels, AISettings } from '../types'
import { Button, ErrorBanner, Field, Loading } from './ui'

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
  const [models, setModels] = useState<AIModel[]>([])
  const [modelsState, setModelsState] = useState<'idle' | 'loading' | 'ready' | 'error'>('idle')
  const [modelsError, setModelsError] = useState('')
  const [manualModel, setManualModel] = useState(false)
  const modelRequest = useRef<{ sequence: number; controller?: AbortController }>({ sequence: 0 })
  const savedKeys = useRef<Record<string, boolean>>({})

  const cancelModels = useCallback(() => {
    modelRequest.current.sequence += 1
    modelRequest.current.controller?.abort()
  }, [])

  const loadModels = useCallback(
    async (provider: string, apiKey = '') => {
      cancelModels()
      const sequence = modelRequest.current.sequence
      const controller = new AbortController()
      modelRequest.current.controller = controller
      const current = () => sequence === modelRequest.current.sequence && !controller.signal.aborted
      setModelsState('loading')
      setModelsError('')
      try {
        const result = await api<AIModels>(
          apiKey ? '/ai/models' : `/ai/models?provider=${encodeURIComponent(provider)}`,
          {
            signal: controller.signal,
            ...(apiKey ? { method: 'POST', body: { provider, api_key: apiKey } } : {}),
          }
        )
        if (!current()) return
        if (result.provider !== provider)
          throw new Error('The provider returned an unexpected model list.')
        setModels(result.models)
        setModelsState('ready')
        if (!apiKey) {
          savedKeys.current[provider] = true
          setForm((value) =>
            value.provider === provider ? { ...value, key_configured: true } : value
          )
        }
      } catch (error) {
        if (!current()) return
        setModels([])
        if (error instanceof APIError && error.code === 'ai_key_missing') {
          savedKeys.current[provider] = false
          setForm((value) =>
            value.provider === provider ? { ...value, key_configured: false } : value
          )
          setModelsState('idle')
        } else {
          // A provider rejection still confirms the server found this provider's saved key.
          // Keep manual entry usable when that key cannot list models (for example, restricted scopes).
          if (!apiKey && error instanceof APIError && error.code === 'ai_provider') {
            savedKeys.current[provider] = true
            setForm((value) =>
              value.provider === provider ? { ...value, key_configured: true } : value
            )
          }
          setModelsState('error')
          setModelsError(errorMessage(error))
        }
      }
    },
    [cancelModels]
  )

  useEffect(() => () => cancelModels(), [cancelModels])

  useEffect(() => {
    if (!resource.data) return
    cancelModels()
    const saved = resource.data
    savedKeys.current[saved.provider] = saved.key_configured
    setForm(saved)
    setKey('')
    setModels([])
    setModelsState('idle')
    setModelsError('')
    if (saved.key_configured) void loadModels(saved.provider)
  }, [resource.data, cancelModels, loadModels])

  function chooseProvider(provider: string) {
    cancelModels()
    setForm({ ...form, provider, model: '', key_configured: savedKeys.current[provider] || false })
    setKey('')
    setModels([])
    setModelsState('idle')
    setModelsError('')
    setManualModel(false)
    setError('')
    // A provider may have its own stored key even when another provider was last selected.
    void loadModels(provider)
  }

  function changeKey(value: string) {
    cancelModels()
    setKey(value)
    setModels([])
    setModelsState('idle')
    setModelsError('')
    setError('')
  }

  async function save(event: FormEvent) {
    event.preventDefault()
    setBusy(true)
    setError('')
    try {
      const saved = await api<AISettings>('/ai/settings', {
        method: 'PUT',
        body: {
          provider: form.provider,
          model: form.model.trim(),
          enabled: form.enabled,
          monitor_enabled: form.monitor_enabled,
          ...(key ? { api_key: key } : {}),
        },
      })
      setKey('')
      resource.setData(saved)
      hub.notify('Brew assistant settings saved.')
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }

  if (resource.loading) return <Loading label="Loading assistant settings…" />
  const loadingModels = modelsState === 'loading'
  const currentNotListed = form.model && !models.some((model) => model.id === form.model)
  const canLoadModels = Boolean(key || form.key_configured)
  return (
    <form onSubmit={save}>
      <label className="check-field">
        <input
          type="checkbox"
          checked={form.enabled}
          disabled={busy}
          onChange={(event) => setForm({ ...form, enabled: event.target.checked })}
        />
        Enable the brew assistant
      </label>
      <Field label="Provider">
        <select
          value={form.provider}
          disabled={busy}
          onChange={(event) => chooseProvider(event.target.value)}
        >
          <option value="openai">OpenAI</option>
          <option value="anthropic">Anthropic / Claude</option>
        </select>
      </Field>
      <Field
        label="Provider API key"
        hint={
          form.key_configured
            ? 'A key is saved for this provider. Leave this blank to keep it.'
            : 'Use a provider API key; a ChatGPT or Claude subscription login is not an API key.'
        }
      >
        <input
          type="password"
          autoComplete="new-password"
          required={form.enabled && !form.key_configured}
          disabled={busy}
          value={key}
          onChange={(event) => changeKey(event.target.value)}
          placeholder={form.key_configured ? '•••••••• · key configured' : ''}
        />
      </Field>
      <div className="inline-actions">
        <Button
          type="button"
          variant="secondary"
          disabled={busy || loadingModels || !canLoadModels}
          onClick={() => void loadModels(form.provider, key)}
        >
          <RefreshCw size={15} className={loadingModels ? 'spin' : ''} />
          {loadingModels
            ? 'Loading models…'
            : key || !form.key_configured
              ? 'Connect and load models'
              : 'Refresh models'}
        </Button>
      </div>
      <p className="help-text" role="status" aria-live="polite">
        {loadingModels
          ? 'Checking models available to this provider API key…'
          : modelsState === 'ready'
            ? models.length
              ? `Connected. ${models.length} model${models.length === 1 ? '' : 's'} available. Choose a model and save your settings.`
              : 'Connected, but this key did not return any models. Check its model permissions or enter a model ID manually.'
            : 'Connect your provider to load its models. You can also enter a model ID manually.'}
      </p>
      <ErrorBanner message={modelsError} />
      <Field
        label="Model"
        hint="Choose a model that supports the provider’s chat API. The available list comes from your provider account."
      >
        {manualModel ? (
          <input
            value={form.model}
            required={form.enabled}
            disabled={busy}
            onChange={(event) => setForm({ ...form, model: event.target.value })}
            placeholder="Enter a model ID"
          />
        ) : (
          <select
            value={form.model}
            required={form.enabled}
            disabled={busy || loadingModels}
            onChange={(event) => setForm({ ...form, model: event.target.value })}
          >
            <option value="">Choose a model</option>
            {currentNotListed && (
              <option value={form.model}>{form.model} · current selection</option>
            )}
            {models.map((model) => (
              <option key={model.id} value={model.id}>
                {model.name && model.name !== model.id ? `${model.name} (${model.id})` : model.id}
              </option>
            ))}
          </select>
        )}
      </Field>
      <label className="check-field">
        <input
          type="checkbox"
          checked={manualModel}
          disabled={busy}
          onChange={(event) => setManualModel(event.target.checked)}
        />
        Enter model ID manually
      </label>
      <label className="check-field">
        <input
          type="checkbox"
          checked={form.monitor_enabled || false}
          disabled={busy}
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
      <Button type="submit" disabled={busy || loadingModels}>
        {busy ? 'Saving…' : 'Save assistant settings'}
      </Button>
    </form>
  )
}
