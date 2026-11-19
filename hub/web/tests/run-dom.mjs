import { spawn } from 'node:child_process'
import { setTimeout as delay } from 'node:timers/promises'

const server = spawn(process.execPath, ['tests/start-demo.mjs'], {
  stdio: 'inherit',
  env: { ...process.env, HUB_TEST_PORT: '18081' },
})
let exited = false
server.on('exit', () => {
  exited = true
})
let tests
try {
  let ready = false
  for (let attempt = 0; attempt < 120; attempt++) {
    if (exited) throw new Error('The demo server exited during startup.')
    try {
      ready = (await fetch('http://127.0.0.1:18081/healthz')).ok
    } catch {}
    if (ready) break
    await delay(250)
  }
  if (!ready) throw new Error('The demo server did not become ready.')
  tests = spawn(
    process.execPath,
    ['node_modules/vitest/vitest.mjs', 'run', '--config', 'vitest.integration.config.ts'],
    { stdio: 'inherit', env: process.env }
  )
  const code = await new Promise((resolve) => tests.on('exit', resolve))
  process.exitCode = code || 0
} catch (error) {
  console.error(error.message)
  process.exitCode = 1
} finally {
  tests?.kill('SIGTERM')
  server.kill('SIGTERM')
  await new Promise((resolve) => {
    if (exited) resolve()
    else server.on('exit', resolve)
  })
}
