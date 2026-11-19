import { afterEach, describe, it, expect, vi } from 'vitest'
import { api, APIError, commandID, setCSRF } from './api'
afterEach(() => {
  vi.restoreAllMocks()
  vi.unstubAllGlobals()
})
describe('hub API transport', () => {
  it('keeps session credentials and sends CSRF on mutations', async () => {
    const fetch = vi
      .spyOn(globalThis, 'fetch')
      .mockResolvedValue(new Response('{"saved":true}', { status: 200 }))
    setCSRF('session-token')
    await api('/recipes', { method: 'POST', body: { name: 'Porter' } })
    const [path, request] = fetch.mock.calls[0]
    expect(path).toBe('/api/v1/recipes')
    expect(request?.credentials).toBe('same-origin')
    expect((request?.headers as Record<string, string>)['X-CSRF-Token']).toBe('session-token')
    expect(JSON.parse(request?.body as string)).toEqual({ name: 'Porter' })
  })
  it('gives separate intended commands unique idempotency keys', async () => {
    const fetch = vi
      .spyOn(globalThis, 'fetch')
      .mockImplementation(async () => new Response('{}', { status: 202 }))
    await api('/devices/pico/commands', {
      method: 'POST',
      body: { command: 'stop' },
      command: true,
    })
    await api('/devices/pico/commands', {
      method: 'POST',
      body: { command: 'stop' },
      command: true,
    })
    const keys = fetch.mock.calls.map(
      (call) => (call[1]?.headers as Record<string, string>)['Idempotency-Key']
    )
    expect(keys[0]).toMatch(/^[\da-f-]{36}$/)
    expect(keys[0]).not.toBe(keys[1])
  })
  it('does not automatically retry an uncertain machine command', async () => {
    const fetch = vi
      .spyOn(globalThis, 'fetch')
      .mockResolvedValue(
        new Response(
          '{"error":{"code":"command_outcome_unknown","message":"Inspect the machine"}}',
          { status: 502 }
        )
      )
    await expect(
      api('/devices/pico/commands', { method: 'POST', body: { command: 'brew' }, command: true })
    ).rejects.toMatchObject({
      status: 502,
      code: 'command_outcome_unknown',
      message: 'Inspect the machine',
    })
    expect(fetch).toHaveBeenCalledTimes(1)
  })
  it('creates cryptographic UUIDs on plain HTTP origins without randomUUID', () => {
    const getRandomValues = crypto.getRandomValues.bind(crypto)
    vi.stubGlobal('crypto', { getRandomValues })
    expect(commandID()).toMatch(/^[\da-f]{8}-[\da-f]{4}-4[\da-f]{3}-[89ab][\da-f]{3}-[\da-f]{12}$/)
  })
  it('accepts empty successful responses', async () => {
    vi.spyOn(globalThis, 'fetch').mockResolvedValue(new Response(null, { status: 204 }))
    expect(await api('/auth/logout', { method: 'POST' })).toBeNull()
  })
})
