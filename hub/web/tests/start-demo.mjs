import { execFile, spawn } from 'node:child_process'
import { mkdtemp, rm } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { promisify } from 'node:util'

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..')
const buildTimeout = 120000
const shutdownTimeout = 12000

export function waitForExit(child) {
  return new Promise((resolve) => {
    child.once('error', (error) => resolve({ code: 1, error }))
    child.once('close', (code, signal) => resolve({ code: code ?? 1, signal }))
  })
}

export async function stopProcess(child, closed) {
  if (child.pid && child.exitCode === null && child.signalCode === null) {
    const force = setTimeout(() => child.kill('SIGKILL'), shutdownTimeout)
    try {
      child.kill('SIGTERM')
      await closed
    } finally {
      clearTimeout(force)
    }
  } else {
    await closed
  }
}

// Compilation finishes before callers start their server-readiness deadline.
export async function startDemo({ port = process.env.HUB_TEST_PORT || '18080', signal } = {}) {
  const temporary = await mkdtemp(path.join(tmpdir(), 'picobrew-hub-browser-'))
  const binary = path.join(temporary, process.platform === 'win32' ? 'hub.exe' : 'hub')
  let child
  let closed
  let exited = false
  let stopping
  const stop = () => {
    if (!stopping)
      stopping = (async () => {
        signal?.removeEventListener('abort', onAbort)
        try {
          if (child) await stopProcess(child, closed)
        } finally {
          await rm(temporary, { recursive: true, force: true })
        }
      })()
    return stopping
  }
  const onAbort = () => void stop().catch((error) => console.error(error.message))
  try {
    signal?.throwIfAborted()
    console.log('[demo] Building isolated Go simulator (120-second build limit)...')
    await promisify(execFile)(
      process.env.GO_BINARY || 'go',
      ['build', '-o', binary, './cmd/picobrew-hub'],
      { cwd: root, timeout: buildTimeout, killSignal: 'SIGKILL', signal }
    )
    signal?.throwIfAborted()
    console.log(`[demo] Build complete; starting simulator on 127.0.0.1:${port}.`)
    child = spawn(
      binary,
      [
        '--demo',
        '--listen',
        `127.0.0.1:${port}`,
        '--data',
        path.join(temporary, 'data'),
        '--web',
        path.join(root, 'web/dist'),
      ],
      { cwd: root, stdio: 'inherit' }
    )
    closed = waitForExit(child)
    closed.then(() => {
      exited = true
    })
    signal?.addEventListener('abort', onAbort, { once: true })
    if (signal?.aborted) {
      await stop()
      signal.throwIfAborted()
    }
    return { child, closed, stop, hasExited: () => exited }
  } catch (error) {
    await stop()
    throw error
  }
}

if (process.argv[1] && path.resolve(process.argv[1]) === fileURLToPath(import.meta.url)) {
  const controller = new AbortController()
  const interrupt = () => controller.abort()
  process.once('SIGTERM', interrupt)
  process.once('SIGINT', interrupt)
  let demo
  try {
    demo = await startDemo({ signal: controller.signal })
    const result = await demo.closed
    if (result.error) throw result.error
    process.exitCode = controller.signal.aborted ? 0 : result.code
  } catch (error) {
    if (!controller.signal.aborted) {
      console.error(error.stderr || error.message)
      process.exitCode = 1
    }
  } finally {
    await demo?.stop()
    process.off('SIGTERM', interrupt)
    process.off('SIGINT', interrupt)
  }
}
