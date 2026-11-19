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

test('community catalogue preserves source schedules through copy, bulk transfer and machine editing', async ({
  page,
  request,
}) => {
  test.setTimeout(60000)
  const headers = { 'X-CSRF-Token': 'demo' }
  let deviceID = ''
  let recipeID = ''
  const errors: string[] = []
  page.on('pageerror', (error) => errors.push(error.message))
  try {
    const created = await request.post('/api/v1/devices', {
      headers,
      data: {
        name: 'Catalogue transfer brewer',
        adapter: 'simulator',
        model: 'pico-pro',
        enabled: true,
      },
    })
    expect(created.ok()).toBe(true)
    deviceID = (await created.json()).id
    await request.post(`/api/v1/devices/${deviceID}/test`, { headers, data: {} })
    const original = await (await request.get('/api/v1/catalog/picopak-64')).json()
    expect(
      original.native_program.steps.some((step: { minutes: number }) => step.minutes === 0)
    ).toBe(true)
    const before = await (await request.get(`/api/v1/devices/${deviceID}/recipes`)).json()

    await page.goto('/#/catalog')
    await expect(page.getByRole('heading', { name: 'A whole library to explore.' })).toBeVisible()
    await page.getByLabel('Catalogue type', { exact: true }).selectOption('native-program')
    await page.getByLabel('Search community recipes and programs').fill('Pico Pale')
    await expect(
      page
        .locator('.catalog-card')
        .filter({ has: page.getByRole('button', { name: 'Pico Pale', exact: true }) })
    ).toHaveCount(1)
    await page.screenshot({
      path: test.info().outputPath('community-catalogue-desktop.png'),
      fullPage: true,
    })
    await page.getByRole('button', { name: 'Pico Pale', exact: true }).click()
    await expect(page.getByRole('heading', { name: 'The original machine schedule' })).toBeVisible()
    await expect(
      page.getByRole('cell', { name: 'Reach target', exact: true }).first()
    ).toBeVisible()
    await page.getByRole('button', { name: 'Copy to my library', exact: true }).click()
    await expect(page.getByRole('button', { name: 'Save program', exact: true })).toBeVisible()
    recipeID = page.url().split('/recipes/')[1]
    expect(recipeID).toBeTruthy()
    await expect(page.getByLabel('Batch size (L)', { exact: true })).toHaveCount(0)
    await expect(page.getByRole('button', { name: 'Add ingredient', exact: true })).toHaveCount(0)
    await expect(page.getByLabel('Step 1 hold (min)', { exact: true })).toHaveValue('0')
    await page
      .getByLabel('Program notes', { exact: true })
      .fill('Copied source schedule, reviewed in browser.')
    await page.getByRole('button', { name: 'Save program', exact: true }).click()
    await expect(page.getByText('Machine program saved to your library.')).toBeVisible()
    const saved = await (await request.get(`/api/v1/recipes/${recipeID}`)).json()
    expect(saved.native_program).toEqual(original.native_program)
    expect(saved.batch_liters).toBe(0)
    expect(saved.ingredients).toEqual([])
    expect(saved.provenance.catalog_id).toBe('picopak-64')

    await page.getByRole('button', { name: 'Preview & send', exact: true }).click()
    let dialog = page.getByRole('dialog', { name: 'Send recipes & programs', exact: true })
    await dialog.getByLabel('Transfer to machine', { exact: true }).selectOption(deviceID)
    await dialog.getByRole('button', { name: 'Preview transfer', exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Review this transfer', exact: true })
    await expect(dialog.getByText(`${before.recipes.length} / 16`, { exact: true })).toBeVisible()
    await expect(dialog.getByText('Ready to send', { exact: true })).toBeVisible()
    await dialog.locator('summary').filter({ hasText: 'Review machine schedule' }).click()
    await page.screenshot({
      path: test.info().outputPath('transfer-preview-desktop.png'),
      fullPage: true,
    })
    await dialog.getByRole('button', { name: 'Send 1 program', exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Transfer receipt', exact: true })
    await expect(dialog.getByText('Verified on machine', { exact: true })).toBeVisible()
    await expect(dialog.getByText('Transfer complete.', { exact: true })).toBeVisible()
    const after = await (await request.get(`/api/v1/devices/${deviceID}/recipes`)).json()
    expect(
      after.recipes.find((program: { id: string }) => program.id === original.native_program.id)
    ).toEqual(original.native_program)
    for (const existing of before.recipes) expect(after.recipes).toContainEqual(existing)
    const device = await (await request.get(`/api/v1/devices/${deviceID}`)).json()
    expect(device.status.busy).toBe(false)
    await dialog.getByRole('button', { name: 'Done', exact: true }).click()

    await page.goto(`/#/devices/${deviceID}`)
    await page
      .getByLabel('Recipe on this machine', { exact: true })
      .selectOption(original.native_program.id)
    await page.getByRole('button', { name: 'Edit saved program', exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Edit the machine’s saved program', exact: true })
    await dialog.getByLabel('Saved program name', { exact: true }).fill('Pico Pale bench edit')
    await dialog.getByRole('button', { name: 'Save to machine', exact: true }).click()
    await expect(dialog).not.toBeVisible()
    const changed = await (await request.get(`/api/v1/devices/${deviceID}/recipes`)).json()
    expect(
      changed.recipes.find((program: { id: string }) => program.id === original.native_program.id)
    ).toMatchObject({ name: 'Pico Pale bench edit', steps: original.native_program.steps })
    expect((await (await request.get(`/api/v1/recipes/${recipeID}`)).json()).name).toBe('Pico Pale')

    // A second preview preserves the machine edit unless the user elects to update matching IDs.
    await page.goto(`/#/recipes/${recipeID}`)
    await page.getByRole('button', { name: 'Preview & send', exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Send recipes & programs', exact: true })
    await dialog.getByLabel('Transfer to machine', { exact: true }).selectOption(deviceID)
    await dialog.getByRole('button', { name: 'Preview transfer', exact: true }).click()
    dialog = page.getByRole('dialog', { name: 'Review this transfer', exact: true })
    await expect(dialog.getByText('Skipped', { exact: true })).toBeVisible()
    await expect(dialog.getByText(/enable updating matching recipes/)).toBeVisible()
    await expect(
      dialog.getByRole('button', { name: 'Send 0 programs', exact: true })
    ).toBeDisabled()
    expect(errors).toEqual([])
  } finally {
    if (deviceID) await request.delete(`/api/v1/devices/${deviceID}`, { headers })
    if (recipeID) await request.delete(`/api/v1/recipes/${recipeID}`, { headers })
  }
})

test('catalogue images, machine guides and program editor fit a mobile screen', async ({
  page,
  request,
}) => {
  await page.setViewportSize({ width: 390, height: 844 })
  const errors: string[] = []
  page.on('pageerror', (error) => errors.push(error.message))
  const externalImages: string[] = []
  page.on('request', (req) => {
    if (req.resourceType() === 'image' && !req.url().startsWith(new URL(page.url()).origin))
      externalImages.push(req.url())
  })
  await page.goto('/#/catalog?kind=recipe')
  await expect(page.locator('.catalog-card').first()).toBeVisible()
  const image = page.locator('.catalog-card img').first()
  await expect(image).toBeVisible()
  await expect
    .poll(() => image.evaluate((node: HTMLImageElement) => node.naturalWidth))
    .toBeGreaterThan(0)
  await page.getByRole('button', { name: 'Machine guides & artwork', exact: true }).click()
  const dialog = page.getByRole('dialog', { name: 'Machine guides & artwork', exact: true })
  await expect(dialog.locator('.machine-artwork img').first()).toBeVisible()
  await page.screenshot({
    path: test.info().outputPath('machine-guides-mobile.png'),
    fullPage: true,
  })
  await dialog.getByRole('button', { name: 'Close dialog', exact: true }).click()
  for (const route of ['/catalog', '/catalog/picopak-64', '/catalog?kind=maintenance']) {
    await page.goto('/#' + route)
    await expect(page.locator('h1')).toBeVisible()
    const bounds = await page.evaluate(() => ({
      scroll: document.documentElement.scrollWidth,
      width: innerWidth,
    }))
    expect(bounds.scroll, `${route} overflows`).toBeLessThanOrEqual(bounds.width + 1)
  }
  const catalog = await (await request.get('/api/v1/catalog?kind=maintenance')).json()
  expect(catalog.items.length).toBeGreaterThan(0)
  await expect(page.getByText('Cleaning program', { exact: true }).first()).toBeVisible()
  await page.screenshot({
    path: test.info().outputPath('cleaning-catalogue-mobile.png'),
    fullPage: true,
  })
  const copied = await request.post('/api/v1/catalog/picopak-64/copy', {
    headers: { 'X-CSRF-Token': 'demo' },
    data: {},
  })
  expect(copied.ok()).toBe(true)
  const recipeID = (await copied.json()).id
  try {
    await page.goto(`/#/recipes/${recipeID}`)
    await expect(page.getByRole('button', { name: 'Save program', exact: true })).toBeVisible()
    const bounds = await page.evaluate(() => ({
      scroll: document.documentElement.scrollWidth,
      width: innerWidth,
    }))
    expect(bounds.scroll, 'Native program editor overflows').toBeLessThanOrEqual(bounds.width + 1)
    await page.screenshot({
      path: test.info().outputPath('native-program-editor-mobile.png'),
      fullPage: true,
    })
  } finally {
    await request.delete(`/api/v1/recipes/${recipeID}`, { headers: { 'X-CSRF-Token': 'demo' } })
  }
  expect(externalImages).toEqual([])
  expect(errors).toEqual([])
})
