// @vitest-environment jsdom
import { createElement } from 'react'
import { afterEach, beforeEach, describe, expect, it, vi } from 'vitest'
import { act, cleanup, render, screen, waitFor } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { HubContext, type HubState } from '../context'
import { setCSRF } from '../lib/api'
import type { AISettings } from '../types'
import { AISettingsForm } from './AISettingsForm'

const initial: AISettings = {
  provider: 'openai',
  model: '',
  key_configured: false,
  enabled: false,
  monitor_enabled: false,
}
const notify = vi.fn()
const fetchMock = vi.fn<typeof fetch>()

function json(data: unknown, status = 200) {
  return new Response(JSON.stringify(data), {
    status,
    headers: { 'Content-Type': 'application/json' },
  })
}

function mount() {
  return render(
    createElement(
      HubContext.Provider,
      { value: { notify } as unknown as HubState },
      createElement(AISettingsForm)
    )
  )
}

function modelCalls() {
  return fetchMock.mock.calls.filter(([path]) => String(path).startsWith('/api/v1/ai/models'))
}

beforeEach(() => {
  vi.stubGlobal('fetch', fetchMock)
  setCSRF('test-csrf')
  notify.mockReset()
  fetchMock.mockReset()
})
afterEach(() => {
  cleanup()
  vi.unstubAllGlobals()
})

describe('AI provider model discovery', () => {
  it('loads saved credentials automatically, preserves an unlisted model and refreshes on demand', async () => {
    fetchMock.mockImplementation(async (path) =>
      String(path) === '/api/v1/ai/settings'
        ? json({ ...initial, key_configured: true, model: 'private-model', enabled: true })
        : json({ provider: 'openai', models: [{ id: 'text-model', name: 'Text model' }] })
    )
    const user = userEvent.setup()
    mount()
    await screen.findByRole('option', { name: 'Text model (text-model)' })
    expect(modelCalls()).toHaveLength(1)
    expect(modelCalls()[0][0]).toBe('/api/v1/ai/models?provider=openai')
    expect((screen.getByLabelText('Model') as HTMLSelectElement).value).toBe('private-model')
    expect(screen.getByRole('option', { name: 'private-model · current selection' })).toBeTruthy()
    await user.click(screen.getByRole('button', { name: 'Refresh models' }))
    await screen.findByRole('button', { name: 'Refresh models' })
    expect(modelCalls()).toHaveLength(2)
    expect((screen.getByLabelText('Model') as HTMLSelectElement).value).toBe('private-model')
  })

  it('connects an unsaved key before choosing a model and refreshes after explicit save', async () => {
    let saved = { ...initial }
    fetchMock.mockImplementation(async (path, options) => {
      if (String(path) === '/api/v1/ai/settings') {
        if (options?.method === 'PUT') {
          const body = JSON.parse(String(options.body))
          saved = { ...initial, ...body, key_configured: true }
        }
        return json(saved)
      }
      return json({ provider: 'openai', models: [{ id: 'text-model', name: 'Text model' }] })
    })
    const user = userEvent.setup()
    mount()
    const key = (await screen.findByLabelText('Provider API key')) as HTMLInputElement
    await user.type(key, 'test-provider-secret')
    expect(modelCalls()).toHaveLength(0)
    await user.click(screen.getByRole('button', { name: 'Connect and load models' }))
    await screen.findByRole('option', { name: 'Text model (text-model)' })
    const [, connection] = modelCalls()[0]
    expect(connection?.method).toBe('POST')
    expect(connection?.headers).toMatchObject({ 'X-CSRF-Token': 'test-csrf' })
    expect(JSON.parse(String(connection?.body))).toEqual({
      provider: 'openai',
      api_key: 'test-provider-secret',
    })
    expect((screen.getByLabelText('Model') as HTMLSelectElement).value).toBe('')
    expect(fetchMock.mock.calls.some(([, options]) => options?.method === 'PUT')).toBe(false)
    await user.selectOptions(screen.getByLabelText('Model'), 'text-model')
    await user.click(screen.getByLabelText('Enable the brew assistant'))
    await user.click(screen.getByRole('button', { name: 'Save assistant settings' }))
    await waitFor(() => expect(notify).toHaveBeenCalledWith('Brew assistant settings saved.'))
    await waitFor(() => expect(modelCalls()).toHaveLength(2))
    expect(modelCalls()[1][0]).toBe('/api/v1/ai/models?provider=openai')
    expect(key.value).toBe('')
    const update = fetchMock.mock.calls.find(([, options]) => options?.method === 'PUT')!
    expect(JSON.parse(String(update[1]?.body))).toMatchObject({
      provider: 'openai',
      model: 'text-model',
      enabled: true,
      api_key: 'test-provider-secret',
    })
  })

  it('does not reuse another provider’s key status or display its late model response', async () => {
    let resolveOld!: (response: Response) => void
    fetchMock.mockImplementation(async (path) => {
      if (String(path) === '/api/v1/ai/settings')
        return json({ ...initial, key_configured: true, model: 'old-model' })
      if (String(path).endsWith('provider=openai'))
        return new Promise<Response>((resolve) => {
          resolveOld = resolve
        })
      return json({ error: { code: 'ai_key_missing', message: 'Configure a provider key.' } }, 409)
    })
    const user = userEvent.setup()
    mount()
    await waitFor(() => expect(modelCalls()).toHaveLength(1))
    const oldSignal = modelCalls()[0][1]?.signal
    await user.selectOptions(screen.getByLabelText('Provider'), 'anthropic')
    await waitFor(() => expect(modelCalls()).toHaveLength(2))
    const button = await screen.findByRole('button', { name: 'Connect and load models' })
    expect((button as HTMLButtonElement).disabled).toBe(true)
    expect((screen.getByLabelText('Provider API key') as HTMLInputElement).placeholder).toBe('')
    expect(oldSignal?.aborted).toBe(true)
    await act(async () => {
      resolveOld(json({ provider: 'openai', models: [{ id: 'late-model', name: 'Late model' }] }))
    })
    expect(screen.queryByRole('option', { name: /Late model/ })).toBeNull()
    expect((screen.getByLabelText('Model') as HTMLSelectElement).value).toBe('')
    expect((screen.getByLabelText('Provider') as HTMLSelectElement).value).toBe('anthropic')
  })

  it('keeps manual model entry available when discovery fails and never saves the key on connect', async () => {
    fetchMock.mockImplementation(async (path) =>
      String(path) === '/api/v1/ai/settings'
        ? json({ ...initial, key_configured: true, model: 'kept-model' })
        : json({ error: { code: 'ai_provider', message: 'Model access is unavailable.' } }, 502)
    )
    const user = userEvent.setup()
    mount()
    await screen.findByRole('alert')
    expect(screen.getByRole('alert').textContent).toContain('Model access is unavailable.')
    expect((screen.getByLabelText('Model') as HTMLSelectElement).value).toBe('kept-model')
    await user.click(screen.getByLabelText('Enter model ID manually'))
    const model = screen.getByRole('textbox', { name: 'Model' }) as HTMLInputElement
    expect(model.value).toBe('kept-model')
    await user.clear(model)
    await user.type(model, 'private-finetuned-model')
    expect(model.value).toBe('private-finetuned-model')
    await user.selectOptions(screen.getByLabelText('Provider'), 'anthropic')
    await screen.findByRole('alert')
    await waitFor(() =>
      expect((screen.getByLabelText('Provider API key') as HTMLInputElement).placeholder).toContain(
        'key configured'
      )
    )
    expect(fetchMock.mock.calls.some(([, options]) => options?.method === 'PUT')).toBe(false)
  })

  it('cancels model discovery when the key changes and when the settings form unmounts', async () => {
    let resolveOld!: (response: Response) => void
    fetchMock.mockImplementation(async (path) => {
      if (String(path) === '/api/v1/ai/settings') return json({ ...initial, key_configured: true })
      return new Promise<Response>((resolve) => {
        resolveOld = resolve
      })
    })
    const user = userEvent.setup()
    const view = mount()
    await waitFor(() => expect(modelCalls()).toHaveLength(1))
    const firstSignal = modelCalls()[0][1]?.signal
    await user.type(screen.getByLabelText('Provider API key'), 'new-key')
    expect(firstSignal?.aborted).toBe(true)
    await act(async () => {
      resolveOld(
        json({ provider: 'openai', models: [{ id: 'old-key-model', name: 'Old key model' }] })
      )
    })
    expect(screen.queryByRole('option', { name: /Old key model/ })).toBeNull()
    expect(modelCalls()).toHaveLength(1)
    await user.click(screen.getByRole('button', { name: 'Connect and load models' }))
    await waitFor(() => expect(modelCalls()).toHaveLength(2))
    const secondSignal = modelCalls()[1][1]?.signal
    view.unmount()
    expect(secondSignal?.aborted).toBe(true)
  })
})
