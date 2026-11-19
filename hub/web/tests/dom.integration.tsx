import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it, vi } from 'vitest'
import { cleanup, render, screen, waitFor, within } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { EventSource as NodeEventSource } from 'eventsource'
import App from '../src/App'

const origin = 'http://127.0.0.1:18081'
const originalFetch = globalThis.fetch
const painted = vi.fn()
let user: ReturnType<typeof userEvent.setup>
let csrfToken = ''
let dialogPolyfilled = false
beforeAll(async () => {
  const response = await originalFetch(origin + '/api/v1/auth/session')
  const session = await response.json()
  expect(session.demo).toBe(true)
  csrfToken = session.csrf_token
  // jsdom has no native modal implementation; browser CI exercises the real dialog.
  if (typeof HTMLDialogElement.prototype.showModal !== 'function') {
    Object.defineProperty(HTMLDialogElement.prototype, 'showModal', {
      configurable: true,
      value(this: HTMLDialogElement) {
        this.open = true
      },
    })
    dialogPolyfilled = true
  }
  vi.stubGlobal('fetch', ((input: any, init?: RequestInit) =>
    originalFetch(
      typeof input === 'string' ? new URL(input, origin) : input,
      init
    )) as typeof fetch)
  vi.stubGlobal(
    'EventSource',
    class extends NodeEventSource {
      constructor(url: string, options?: EventSourceInit) {
        super(new URL(url, origin), options)
      }
    }
  )
  vi.stubGlobal(
    'ResizeObserver',
    class {
      callback: ResizeObserverCallback
      constructor(callback: ResizeObserverCallback) {
        this.callback = callback
      }
      observe(target: Element) {
        this.callback(
          [
            {
              target,
              contentRect: { width: 800, height: 300 },
              borderBoxSize: [{ inlineSize: 800, blockSize: 300 }],
            } as unknown as ResizeObserverEntry,
          ],
          this as unknown as ResizeObserver
        )
      }
      unobserve() {}
      disconnect() {}
    }
  )
  vi.spyOn(window, 'scrollTo').mockImplementation(() => {})
  vi.spyOn(HTMLCanvasElement.prototype, 'getContext').mockImplementation(
    () =>
      ({
        createImageData: (width: number, height: number) => ({
          data: new Uint8ClampedArray(width * height * 4),
        }),
        putImageData: painted,
      }) as unknown as CanvasRenderingContext2D
  )
})
beforeEach(() => {
  localStorage.clear()
  user = userEvent.setup()
})
afterEach(() => cleanup())
afterAll(() => {
  vi.restoreAllMocks()
  vi.unstubAllGlobals()
  if (dialogPolyfilled) Reflect.deleteProperty(HTMLDialogElement.prototype, 'showModal')
})
async function open(route: string) {
  window.location.hash = route
  render(<App />)
  await screen.findByText('SIMULATION', { exact: true })
}
async function simulationMutation(path: string, body: unknown = {}) {
  const response = await originalFetch(origin + '/api/v1' + path, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'X-CSRF-Token': csrfToken,
      'Idempotency-Key': crypto.randomUUID(),
    },
    body: JSON.stringify(body),
  })
  const data = await response.json()
  expect(response.ok, JSON.stringify(data)).toBe(true)
  return data
}

describe('UI interactions against the real isolated Go simulator', () => {
  it('loads fleet data, streams updates and decodes an actual OLED response', async () => {
    await open('/overview')
    await screen.findByRole('heading', { name: 'A good day to brew.' })
    await waitFor(() => expect(document.querySelectorAll('.machine-card').length).toBe(3))
    await user.click(screen.getAllByRole('link', { name: /^Open .+ controls$/ })[0])
    await screen.findByRole('heading', { name: 'The front panel, wherever you are.' })
    await waitFor(() => expect(painted).toHaveBeenCalled())
    const down = screen.getByRole('button', { name: 'Move selection down' }) as HTMLButtonElement
    await waitFor(() => expect(down.disabled).toBe(false))
    await user.click(down)
    await waitFor(() => expect(down.disabled).toBe(false))
    expect(screen.getByRole('button', { name: 'Stop' })).toBeTruthy()
  })
  it('manages an offline machine from Overview and its detail page while retaining brew history', async () => {
    let id = ''
    try {
      await open('/overview')
      await user.click(await screen.findByRole('button', { name: 'Add machine' }))
      let dialog = await screen.findByRole('dialog', { name: 'Add machine' })
      expect(window.location.hash).toBe('#/overview')
      await user.type(within(dialog).getByLabelText('Machine name'), 'DOM management brewer')
      await user.type(within(dialog).getByLabelText('Location'), 'Test bench')
      await user.click(within(dialog).getByRole('button', { name: 'Add machine' }))
      await waitFor(() => expect(screen.queryByRole('dialog')).toBeNull())
      const devices = await (await originalFetch(origin + '/api/v1/devices')).json()
      const created = devices.find((device: any) => device.name === 'DOM management brewer')
      expect(created.adapter).toBe('simulator')
      id = created.id

      await user.click(screen.getByRole('button', { name: 'Edit DOM management brewer' }))
      dialog = await screen.findByRole('dialog', { name: 'Edit machine connection' })
      expect((within(dialog).getByLabelText('Connection type') as HTMLSelectElement).disabled).toBe(
        true
      )
      expect((within(dialog).getByLabelText('Machine model') as HTMLSelectElement).disabled).toBe(
        true
      )
      await user.clear(within(dialog).getByLabelText('Machine name'))
      await user.type(within(dialog).getByLabelText('Machine name'), 'DOM renamed brewer')
      await user.click(within(dialog).getByLabelText('Monitor this machine'))
      await user.click(within(dialog).getByRole('button', { name: 'Save machine' }))
      await waitFor(() => expect(screen.queryByRole('dialog')).toBeNull())
      const offline = await (await originalFetch(origin + `/api/v1/devices/${id}`)).json()
      expect(offline).toMatchObject({ name: 'DOM renamed brewer', enabled: false, online: false })
      expect(window.location.hash).toBe('#/overview')

      await user.click(screen.getByRole('button', { name: 'Remove DOM renamed brewer' }))
      dialog = await screen.findByRole('dialog', { name: 'Remove DOM renamed brewer?' })
      expect(within(dialog).getByText(/brew history/i)).toBeTruthy()
      await user.click(within(dialog).getByRole('button', { name: 'Cancel' }))
      await waitFor(() => expect(screen.queryByRole('dialog')).toBeNull())
      expect((await originalFetch(origin + `/api/v1/devices/${id}`)).status).toBe(200)

      await user.click(screen.getByRole('link', { name: 'Open DOM renamed brewer controls' }))
      await screen.findByRole('heading', { name: 'DOM renamed brewer', level: 1 })
      expect(
        (screen.getByRole('button', { name: 'Edit connection' }) as HTMLButtonElement).disabled
      ).toBe(false)
      expect(
        (screen.getByRole('button', { name: 'Brewer preferences' }) as HTMLButtonElement).disabled
      ).toBe(true)
      await user.click(screen.getByRole('button', { name: 'Edit connection' }))
      dialog = await screen.findByRole('dialog', { name: 'Edit machine connection' })
      await user.clear(within(dialog).getByLabelText('Location'))
      await user.type(within(dialog).getByLabelText('Location'), 'Offline edit saved')
      await user.click(within(dialog).getByLabelText('Monitor this machine'))
      await user.click(within(dialog).getByRole('button', { name: 'Save machine' }))
      await waitFor(() => expect(screen.queryByRole('dialog')).toBeNull())
      const edited = await (await originalFetch(origin + `/api/v1/devices/${id}`)).json()
      expect(edited.location).toBe('Offline edit saved')

      // Create a real archived trace on this isolated simulator, not on a seeded machine.
      await simulationMutation(`/devices/${id}/test`)
      await simulationMutation(`/devices/${id}/commands`, {
        command: 'brew',
        recipe_id: 'studio-porter',
        confirm: true,
      })
      await simulationMutation(`/devices/${id}/commands`, { command: 'encoder', click: true })
      await simulationMutation(`/devices/${id}/commands`, { command: 'stop' })
      await simulationMutation(`/devices/${id}/test`)
      await simulationMutation(`/devices/${id}/sync-history`)
      const brews = await (await originalFetch(origin + '/api/v1/brews')).json()
      const brew = brews.find((item: any) => item.device_id === id)
      expect(brew.sample_count).toBeGreaterThan(0)

      await user.click(screen.getByRole('button', { name: 'Edit connection' }))
      dialog = await screen.findByRole('dialog', { name: 'Edit machine connection' })
      await user.click(within(dialog).getByLabelText('Monitor this machine'))
      await user.click(within(dialog).getByRole('button', { name: 'Save machine' }))
      await waitFor(() => expect(screen.queryByRole('dialog')).toBeNull())
      await user.click(screen.getByRole('button', { name: 'Remove machine' }))
      dialog = await screen.findByRole('dialog', { name: 'Remove DOM renamed brewer?' })
      await user.click(within(dialog).getByRole('button', { name: 'Remove machine' }))
      await screen.findByRole('heading', { name: 'A good day to brew.' })
      expect(window.location.hash).toBe('#/overview')
      expect((await originalFetch(origin + `/api/v1/devices/${id}`)).status).toBe(404)
      const retained = await (await originalFetch(origin + `/api/v1/brews/${brew.id}`)).json()
      expect(retained.device_id).toBe(id)
      expect(retained.samples.length).toBeGreaterThan(0)
    } finally {
      if (id)
        await originalFetch(origin + `/api/v1/devices/${id}`, {
          method: 'DELETE',
          headers: { 'X-CSRF-Token': csrfToken },
        })
    }
  })
  it('creates and saves a canonical recipe through the editor', async () => {
    await open('/recipes/new')
    await screen.findByRole('heading', { name: 'What will you brew next?' })
    await user.type(screen.getByLabelText('Recipe name', { exact: true }), 'DOM test porter')
    await user.type(screen.getByLabelText('Tags', { exact: true }), 'dom, porter')
    await user.click(screen.getByRole('button', { name: 'Add ingredient' }))
    await user.type(screen.getByLabelText('Ingredient 1 name'), 'Pale malt')
    await user.click(screen.getByRole('button', { name: 'Save recipe' }))
    await screen.findByRole('heading', { name: 'DOM test porter', level: 1 })
    const recipes = await (await originalFetch(origin + '/api/v1/recipes')).json()
    const saved = recipes.find((recipe: any) => recipe.name === 'DOM test porter')
    expect(saved.tags).toEqual(['dom', 'porter'])
    expect(saved.ingredients[0]).toMatchObject({
      name: 'Pale malt',
      kind: 'fermentable',
      unit: 'kg',
    })
    await user.type(screen.getByLabelText('Recipe notes'), 'Actual backend persistence.')
    await user.click(screen.getByRole('button', { name: 'Save recipe' }))
    await waitFor(async () => {
      const stored = await (await originalFetch(origin + `/api/v1/recipes/${saved.id}`)).json()
      expect(stored.notes).toBe('Actual backend persistence.')
      expect(stored.revision).toBe(2)
    })
  })
  it('persists journal annotations and ratings and exports every sensor column', async () => {
    const initial = await (
      await originalFetch(origin + '/api/v1/brews/demo-history-demo-pro')
    ).json()
    await open('/history/demo-history-demo-pro')
    await screen.findByRole('heading', { name: 'The complete temperature story' })
    const notes = screen.getByLabelText('Brew notes', { exact: true }) as HTMLTextAreaElement
    await waitFor(() => expect(notes.value).toBe(initial.notes || ''))
    await user.clear(notes)
    await user.paste('[25:30] Stable mash from DOM integration')
    await user.click(screen.getByRole('button', { name: 'Rate 5 out of 5' }))
    await user.click(screen.getByRole('button', { name: 'Save notes' }))
    await screen.findByText('Brew notes saved.')
    const saved = await (await originalFetch(origin + '/api/v1/brews/demo-history-demo-pro')).json()
    expect(saved.notes).toBe('[25:30] Stable mash from DOM integration')
    expect(saved.rating).toBe(5)
    await user.click(screen.getByRole('button', { name: 'Show raw samples' }))
    expect(screen.getByRole('columnheader', { name: 'Flags' })).toBeTruthy()
    const csv = await (
      await originalFetch(origin + '/api/v1/brews/demo-history-demo-pro/export?format=csv')
    ).text()
    expect(csv).toContain('thermoblock_f')
    expect(csv).toContain('controller_status')
  })
  it('persists calibration and exercises the authenticated API explorer', async () => {
    await open('/settings')
    await screen.findByRole('heading', { name: 'Workspace settings.' })
    expect(screen.queryByRole('heading', { name: 'Your machines' })).toBeNull()
    expect(screen.queryByRole('button', { name: 'Add machine' })).toBeNull()
    await waitFor(() =>
      expect((screen.getByLabelText('Equipment model') as HTMLSelectElement).value).not.toBe('')
    )
    const efficiency = screen.getByLabelText('Planning efficiency (%)')
    await user.clear(efficiency)
    await user.type(efficiency, '68')
    await user.click(screen.getByRole('button', { name: 'Save calibration' }))
    await screen.findByText('Equipment calibration saved.')
    const profile = (screen.getByLabelText('Equipment model') as HTMLSelectElement).value
    const profiles = await (await originalFetch(origin + '/api/v1/profiles')).json()
    expect(profiles.find((item: any) => item.id === profile).efficiency).toBe(68)
    await user.click(screen.getByRole('link', { name: 'API explorer' }))
    await screen.findByRole('heading', { name: 'Explore the hub API.' })
    await user.click(await screen.findByRole('button', { name: 'Send request' }))
    await waitFor(() =>
      expect(document.querySelector('.api-response pre')?.textContent).toContain('demo-pro')
    )
  })
})
