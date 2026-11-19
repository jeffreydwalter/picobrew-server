import { test, expect } from '@playwright/test'

test.describe.configure({ mode: 'serial' })
test.beforeEach(async ({ request }) => {
  const response = await request.get('/api/v1/auth/session')
  expect(
    (await response.json()).demo,
    'Browser mutation tests require an isolated simulation workspace'
  ).toBe(true)
})

test('fleet opens a machine and exposes real front-panel controls', async ({ page }) => {
  const errors: string[] = []
  page.on('pageerror', (error) => errors.push(error.message))
  await page.goto('/#/overview')
  await expect(page.getByRole('heading', { name: 'A good day to brew.' })).toBeVisible()
  await expect(page.locator('.machine-card')).toHaveCount(3)
  await expect(page.getByText('SIMULATION', { exact: true })).toBeVisible()
  await page.screenshot({ path: test.info().outputPath('overview-desktop.png'), fullPage: true })
  await page.locator('.machine-card').first().getByRole('button', { name: 'Open controls' }).click()
  await expect(
    page.getByRole('heading', { name: 'The front panel, wherever you are.' })
  ).toBeVisible()
  await expect(page.getByRole('button', { name: 'Move selection up' })).toBeVisible()
  await expect(page.getByRole('button', { name: 'Move selection down' })).toBeVisible()
  await expect(page.locator('canvas')).toBeVisible()
  await expect(page.getByRole('button', { name: 'Press dial' })).toBeEnabled()
  await page.getByRole('button', { name: 'Move selection down' }).click()
  await expect(page.getByRole('button', { name: 'Press dial' })).toBeEnabled()
  expect(errors).toEqual([])
})

test('create, revise and import a recipe through the working editor', async ({ page }) => {
  await page.goto('/#/recipes/new')
  await expect(page.getByRole('heading', { name: 'What will you brew next?' })).toBeVisible()
  await page.getByLabel('Recipe name', { exact: true }).fill('Browser test porter')
  await page.getByLabel('Style', { exact: true }).fill('Porter')
  await page.getByLabel('Tags', { exact: true }).fill('browser, porter')
  await page.getByRole('button', { name: 'Add ingredient' }).click()
  await page.getByLabel('Ingredient 1 name').fill('Pale malt')
  await page.getByRole('button', { name: 'Save recipe', exact: true }).click()
  await expect(
    page.getByRole('heading', { name: 'Browser test porter', exact: true, level: 1 })
  ).toBeVisible()
  await expect(page.getByText('Recipe saved to your library.')).toBeVisible()
  await page.getByLabel('Recipe notes').fill('Integration note: retained after save.')
  await page.getByRole('button', { name: 'Save recipe', exact: true }).click()
  await page.reload()
  await expect(page.getByLabel('Recipe notes')).toHaveValue(
    'Integration note: retained after save.'
  )
  await expect(page.getByLabel('Tags', { exact: true })).toHaveValue('browser, porter')
  await page.getByRole('button', { name: 'Revision history', exact: true }).click()
  await expect(page.getByRole('heading', { name: 'Recipe revision history' })).toBeVisible()
  await expect(page.getByText('Revision 1', { exact: true })).toBeVisible()
  await page.getByRole('button', { name: 'Close dialog' }).click()
  await page.goto('/#/recipes')
  await page.getByRole('button', { name: 'Import recipe', exact: true }).click()
  await page.getByLabel('Format', { exact: true }).selectOption('json')
  await page.getByLabel('Or paste the recipe').fill(
    JSON.stringify({
      name: 'Imported browser recipe',
      style: 'Pale ale',
      batch_liters: 5,
      efficiency: 55,
      ingredients: [
        {
          name: 'Pale malt',
          kind: 'fermentable',
          amount: 1.2,
          unit: 'kg',
          use: 'mash',
          minutes: 0,
        },
      ],
      steps: [
        { name: 'Mash', kind: 'mash', target_c: 67, minutes: 60, drain_minutes: 0, location: 1 },
      ],
      tags: [],
      notes: '',
    })
  )
  await page.getByRole('button', { name: 'Import recipes', exact: true }).click()
  await expect(
    page.getByRole('heading', { name: 'Imported browser recipe', exact: true, level: 1 })
  ).toBeVisible()
})

test('brew journal saves notes, exposes all sensor columns and exports', async ({ page }) => {
  await page.goto('/#/history/demo-history-demo-pro')
  await expect(page.getByRole('heading', { name: 'The complete temperature story' })).toBeVisible()
  await expect(
    page.getByRole('heading', { name: 'Phases, controller state & flags' })
  ).toBeVisible()
  await page
    .getByLabel('Brew notes', { exact: true })
    .fill('[25:30] Browser test annotation\nRoast aroma and stable mash.')
  await page.getByRole('button', { name: 'Rate 5 out of 5' }).click()
  await page.getByRole('button', { name: 'Save notes', exact: true }).click()
  await expect(page.getByText('Brew notes saved.')).toBeVisible()
  await page.reload()
  await expect(page.getByLabel('Brew notes', { exact: true })).toHaveValue(
    '[25:30] Browser test annotation\nRoast aroma and stable mash.'
  )
  await expect(page.getByRole('button', { name: 'Rate 5 out of 5' })).toHaveAttribute(
    'aria-pressed',
    'true'
  )
  await page.screenshot({ path: test.info().outputPath('history-desktop.png'), fullPage: true })
  await page.getByRole('button', { name: 'Show raw samples' }).click()
  await expect(page.getByRole('columnheader', { name: 'Flags', exact: true })).toBeVisible()
  const downloaded = page.waitForEvent('download')
  await page.getByRole('button', { name: 'CSV', exact: true }).click()
  expect((await downloaded).suggestedFilename()).toMatch(/\.csv$/)
})

test('settings, API explorer and contextual assistant are functional', async ({ page }) => {
  await page.goto('/#/settings')
  await expect(page.getByRole('heading', { name: 'Workspace settings.' })).toBeVisible()
  await expect(page.locator('.settings-row')).toHaveCount(6)
  await page.screenshot({ path: test.info().outputPath('settings-desktop.png'), fullPage: true })
  await page.getByLabel('Temperature unit', { exact: true }).selectOption('C')
  await page.reload()
  await expect(page.getByLabel('Temperature unit', { exact: true })).toHaveValue('C')
  await page.getByRole('button', { name: 'Explore the API' }).click()
  await expect(page.getByRole('heading', { name: 'Explore the hub API.' })).toBeVisible()
  await expect(page.locator('.endpoint-list>button')).not.toHaveCount(0)
  await page.getByRole('button', { name: 'Send request', exact: true }).click()
  await expect(page.locator('.api-response pre')).toContainText('demo-pro')
  await page.goto('/#/assistant?brew=demo-history-demo-pro')
  await expect(page.getByRole('heading', { name: 'The brew assistant.' })).toBeVisible()
  await expect(page.getByRole('tab', { name: 'After the last pour' })).toHaveAttribute(
    'aria-selected',
    'true'
  )
  await expect(page.getByLabel('Brew to analyze')).toHaveValue('demo-history-demo-pro')
})

test('mobile layouts contain wide recipe and history tables', async ({ page }) => {
  await page.setViewportSize({ width: 390, height: 844 })
  for (const route of [
    '/overview',
    '/recipes/studio-porter',
    '/history/demo-history-demo-pro',
    '/settings',
    '/api',
  ]) {
    await page.goto('/#' + route)
    await expect(page.locator('h1')).toBeVisible()
    await page.waitForTimeout(250)
    const bounds = await page.evaluate(() => ({
      scroll: document.documentElement.scrollWidth,
      width: innerWidth,
    }))
    expect(bounds.scroll, `${route} overflows`).toBeLessThanOrEqual(bounds.width + 1)
    if (route === '/overview' || route === '/settings')
      await page.screenshot({
        path: test.info().outputPath(route.slice(1) + '-mobile.png'),
        fullPage: true,
      })
  }
  await page.getByRole('button', { name: 'Open navigation' }).click()
  await expect(page.locator('.sidebar')).toHaveClass(/open/)
  await page
    .getByRole('navigation', { name: 'Main navigation' })
    .getByRole('link', { name: 'Recipe library' })
    .click()
  await expect(page.getByRole('heading', { name: 'Your recipe library.' })).toBeVisible()
})
