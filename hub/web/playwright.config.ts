import { defineConfig } from '@playwright/test'
const external = process.env.HUB_TEST_URL
export default defineConfig({
  testDir: 'tests',
  timeout: 30000,
  workers: 1,
  reporter: 'list',
  use: {
    baseURL: external || 'http://127.0.0.1:18080',
    headless: true,
    trace: 'retain-on-failure',
    screenshot: 'only-on-failure',
    launchOptions: process.env.CHROMIUM_PATH
      ? { executablePath: process.env.CHROMIUM_PATH, args: ['--no-sandbox'] }
      : {},
  },
  webServer: external
    ? undefined
    : {
        command: 'node tests/start-demo.mjs',
        url: 'http://127.0.0.1:18080/healthz',
        // Allow the bounded cold build, server startup, and shutdown grace period.
        timeout: 180000,
        reuseExistingServer: false,
      },
})
