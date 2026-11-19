import { test, expect } from '@playwright/test'

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
  await page
    .locator('.machine-card')
    .first()
    .getByRole('link', { name: /^Open .+ controls$/ })
    .click()
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

test('machine management stays on the Overview and works offline on mobile', async ({
  page,
  request,
}) => {
  const longName = 'Mobile brewing bench with a long machine name for the shared family workspace'
  let id = ''
  try {
    await page.setViewportSize({ width: 390, height: 844 })
    await page.goto('/#/overview')
    await page.getByRole('button', { name: 'Add machine', exact: true }).click()
    let dialog = page.getByRole('dialog', { name: 'Add machine', exact: true })
    await expect(dialog).toBeVisible()
    await expect(page).toHaveURL(/#\/overview$/)
    await dialog.getByLabel('Machine name').fill(longName)
    await dialog.getByLabel('Location').fill('Upstairs brew studio')
    await page.screenshot({
      path: test.info().outputPath('add-machine-mobile.png'),
      fullPage: true,
    })
    await dialog.getByRole('button', { name: 'Add machine', exact: true }).click()
    await expect(dialog).not.toBeVisible()
    const created = (await (await request.get('/api/v1/devices')).json()).find(
      (device: { name: string }) => device.name === longName
    )
    expect(created.adapter).toBe('simulator')
    id = created.id
    let card = page
      .locator('.machine-card')
      .filter({ has: page.getByRole('heading', { name: longName, exact: true }) })
    await expect(card).toBeVisible()
    const bounds = await page.evaluate(() => ({
      scroll: document.documentElement.scrollWidth,
      width: innerWidth,
    }))
    expect(bounds.scroll).toBeLessThanOrEqual(bounds.width + 1)
    await page.screenshot({
      path: test.info().outputPath('machine-actions-mobile.png'),
      fullPage: true,
    })

    await card.getByRole('button', { name: `Edit ${longName}`, exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Edit machine connection' })
    await expect(dialog).toBeVisible()
    await expect(page).toHaveURL(/#\/overview$/)
    await expect(dialog.getByLabel('Machine model')).toBeDisabled()
    await expect(dialog.getByLabel('Connection type')).toBeDisabled()
    await dialog.getByLabel('Machine name').fill('Browser managed brewer')
    await dialog.getByLabel('Monitor this machine').uncheck()
    await dialog.getByRole('button', { name: 'Save machine' }).click()
    await expect(dialog).not.toBeVisible()
    card = page
      .locator('.machine-card')
      .filter({ has: page.getByRole('heading', { name: 'Browser managed brewer', exact: true }) })
    const controls = card.getByRole('link', { name: 'Open Browser managed brewer controls' })
    await controls.focus()
    await controls.press('Enter')
    await expect(
      page.getByRole('heading', { name: 'Browser managed brewer', exact: true, level: 1 })
    ).toBeVisible()
    await expect(page.getByRole('button', { name: 'Edit connection' })).toBeEnabled()
    await expect(page.getByRole('button', { name: 'Remove machine', exact: true })).toBeEnabled()
    await expect(page.getByRole('button', { name: 'Brewer preferences' })).toBeDisabled()
    await page.getByRole('button', { name: 'Edit connection' }).click()
    dialog = page.getByRole('dialog', { name: 'Edit machine connection' })
    await dialog.getByLabel('Location').fill('Updated while offline')
    await dialog.getByRole('button', { name: 'Save machine' }).click()
    await expect(dialog).not.toBeVisible()
    expect(await (await request.get(`/api/v1/devices/${id}`)).json()).toMatchObject({
      enabled: false,
      online: false,
      location: 'Updated while offline',
    })

    await page.getByRole('button', { name: 'Remove machine', exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Remove Browser managed brewer?' })
    await expect(dialog.getByText(/brew history/i)).toBeVisible()
    await dialog.getByRole('button', { name: 'Cancel' }).click()
    await expect(dialog).not.toBeVisible()
    expect((await request.get(`/api/v1/devices/${id}`)).status()).toBe(200)
    await page.goto('/#/overview')
    await page.getByRole('button', { name: 'Remove Browser managed brewer', exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Remove Browser managed brewer?' })
    await dialog.getByRole('button', { name: 'Remove machine', exact: true }).click()
    await expect(dialog).not.toBeVisible()
    expect((await request.get(`/api/v1/devices/${id}`)).status()).toBe(404)
    await expect(
      page.getByRole('link', { name: 'Open Browser managed brewer controls' })
    ).toHaveCount(0)

    await page.goto('/#/settings/new-device')
    dialog = page.getByRole('dialog', { name: 'Add machine', exact: true })
    await expect(dialog).toBeVisible()
    await expect(page).toHaveURL(/#\/overview\/new-device$/)
    await dialog.getByRole('button', { name: 'Cancel' }).click()
    await expect(dialog).not.toBeVisible()
    await expect(page).toHaveURL(/#\/overview$/)
  } finally {
    if (id) await request.delete(`/api/v1/devices/${id}`, { headers: { 'X-CSRF-Token': 'demo' } })
  }
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
  await expect(page.locator('.settings-row')).toHaveCount(4)
  await expect(page.getByRole('heading', { name: 'Your machines' })).toHaveCount(0)
  await expect(page.getByRole('button', { name: 'Add machine' })).toHaveCount(0)
  await page.screenshot({ path: test.info().outputPath('settings-desktop.png'), fullPage: true })
  await page.getByLabel('Temperature unit', { exact: true }).selectOption('C')
  await page.reload()
  await expect(page.getByLabel('Temperature unit', { exact: true })).toHaveValue('C')
  await page.getByRole('link', { name: 'API explorer', exact: true }).click()
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
