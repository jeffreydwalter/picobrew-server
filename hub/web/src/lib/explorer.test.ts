import { afterEach, describe, expect, it, vi } from 'vitest'
import { inspectAPI } from './explorer'

afterEach(() => vi.restoreAllMocks())
describe('API explorer response handling', () => {
  it('previews CSV exports as text instead of requiring JSON', async () => {
    vi.spyOn(globalThis, 'fetch').mockResolvedValue(
      new Response('elapsed_ms,wort_f\n0,70', { headers: { 'Content-Type': 'text/csv' } })
    )
    const response = await inspectAPI('/brews/brew/export?format=csv')
    expect(response.text).toContain('elapsed_ms,wort_f\n0,70')
    expect(response.download).toBeUndefined()
  })
  it('prepares a binary backup for download without interpreting bytes as JSON', async () => {
    vi.spyOn(globalThis, 'fetch').mockResolvedValue(
      new Response(new Uint8Array([80, 75, 3, 4]), {
        headers: {
          'Content-Type': 'application/zip',
          'Content-Disposition': 'attachment; filename="backup.zip"',
        },
      })
    )
    const response = await inspectAPI('/backup')
    expect(response.download?.filename).toBe('backup.zip')
    expect(response.download?.blob.size).toBe(4)
  })
  it('cancels a live event stream after a complete preview event', async () => {
    const cancel = vi.fn()
    const stream = new ReadableStream({
      start(controller) {
        controller.enqueue(new TextEncoder().encode('event: fleet\ndata: []\n\n'))
      },
      cancel,
    })
    vi.spyOn(globalThis, 'fetch').mockResolvedValue(
      new Response(stream, { headers: { 'Content-Type': 'text/event-stream' } })
    )
    const response = await inspectAPI('/events')
    expect(response.text).toContain('data: []')
    expect(response.text).toContain('Stream preview ended')
    expect(cancel).toHaveBeenCalledOnce()
  })
})
