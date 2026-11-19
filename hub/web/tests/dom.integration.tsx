import { afterAll, afterEach, beforeAll, beforeEach, describe, expect, it, vi } from 'vitest'
import { cleanup, render, screen, waitFor } from '@testing-library/react'
import userEvent from '@testing-library/user-event'
import { EventSource as NodeEventSource } from 'eventsource'
import App from '../src/App'

const origin = 'http://127.0.0.1:18081'
const originalFetch = globalThis.fetch
const painted = vi.fn()
let user: ReturnType<typeof userEvent.setup>
beforeAll(async () => {
  const response = await originalFetch(origin + '/api/v1/auth/session')
  expect((await response.json()).demo).toBe(true)
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
})
async function open(route: string) {
  window.location.hash = route
  render(<App />)
  await screen.findByText('SIMULATION', { exact: true })
}

describe('UI interactions against the real isolated Go simulator', () => {
  it('loads fleet data, streams updates and decodes an actual OLED response', async () => {
    await open('/overview')
    await screen.findByRole('heading', { name: 'A good day to brew.' })
    await waitFor(() => expect(document.querySelectorAll('.machine-card').length).toBe(3))
    await user.click(screen.getAllByRole('button', { name: 'Open controls' })[0])
    await screen.findByRole('heading', { name: 'The front panel, wherever you are.' })
    await waitFor(() => expect(painted).toHaveBeenCalled())
    const down = screen.getByRole('button', { name: 'Move selection down' }) as HTMLButtonElement
    await waitFor(() => expect(down.disabled).toBe(false))
    await user.click(down)
    await waitFor(() => expect(down.disabled).toBe(false))
    expect(screen.getByRole('button', { name: 'Stop' })).toBeTruthy()
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
    await open('/history/demo-history-demo-pro')
    await screen.findByRole('heading', { name: 'The complete temperature story' })
    const notes = screen.getByLabelText('Brew notes', { exact: true })
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
    await waitFor(() =>
      expect((screen.getByLabelText('Equipment model') as HTMLSelectElement).value).not.toBe('')
    )
    const efficiency = screen.getByLabelText('Measured efficiency (%)')
    await user.clear(efficiency)
    await user.type(efficiency, '68')
    await user.click(screen.getByRole('button', { name: 'Save calibration' }))
    await screen.findByText('Equipment calibration saved.')
    const profile = (screen.getByLabelText('Equipment model') as HTMLSelectElement).value
    const profiles = await (await originalFetch(origin + '/api/v1/profiles')).json()
    expect(profiles.find((item: any) => item.id === profile).efficiency).toBe(68)
    await user.click(screen.getByRole('button', { name: 'Explore the API' }))
    await screen.findByRole('heading', { name: 'Explore the hub API.' })
    await user.click(await screen.findByRole('button', { name: 'Send request' }))
    await waitFor(() =>
      expect(document.querySelector('.api-response pre')?.textContent).toContain('demo-pro')
    )
  })
})
