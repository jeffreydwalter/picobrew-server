import { spawn } from 'node:child_process'
import { setTimeout as delay } from 'node:timers/promises'
import { startDemo, stopProcess, waitForExit } from './start-demo.mjs'

const controller = new AbortController()
let demo
let tests
let testsClosed
const interrupt = () => {
  controller.abort()
  if (tests) void stopProcess(tests, testsClosed)
}
process.once('SIGTERM', interrupt)
process.once('SIGINT', interrupt)
try {
  demo = await startDemo({ port: '18081', signal: controller.signal })
  const deadline = Date.now() + 30000
  let ready = false
  while (Date.now() < deadline) {
    controller.signal.throwIfAborted()
    if (demo.hasExited()) {
      const result = await demo.closed
      throw result.error || new Error(`The demo server exited during startup (${result.code}).`)
    }
    try {
      const response = await fetch('http://127.0.0.1:18081/healthz', {
        signal: AbortSignal.any([
          controller.signal,
          AbortSignal.timeout(Math.max(1, Math.min(1000, deadline - Date.now()))),
        ]),
      })
      ready = response.ok
      await response.body?.cancel()
    } catch {}
    if (ready) break
    await delay(250, undefined, { signal: controller.signal })
  }
  if (!ready) throw new Error('The compiled demo server did not become ready within 30 seconds.')
  controller.signal.throwIfAborted()
  tests = spawn(
    process.execPath,
    ['node_modules/vitest/vitest.mjs', 'run', '--config', 'vitest.integration.config.ts'],
    { stdio: 'inherit', env: process.env }
  )
  testsClosed = waitForExit(tests)
  const timeout = setTimeout(() => {
    console.error('DOM integration tests exceeded the 180-second limit.')
    tests.kill('SIGKILL')
  }, 180000)
  try {
    const result = await testsClosed
    if (result.error) throw result.error
    process.exitCode = result.code
  } finally {
    clearTimeout(timeout)
  }
} catch (error) {
  console.error(error.stderr || error.message)
  process.exitCode = 1
} finally {
  if (tests) await stopProcess(tests, testsClosed)
  await demo?.stop()
  process.off('SIGTERM', interrupt)
  process.off('SIGINT', interrupt)
}
