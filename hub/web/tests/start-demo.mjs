import { execFile, spawn } from 'node:child_process'
import { mkdtemp, rm } from 'node:fs/promises'
import { tmpdir } from 'node:os'
import path from 'node:path'
import { fileURLToPath } from 'node:url'
import { promisify } from 'node:util'

const root = path.resolve(path.dirname(fileURLToPath(import.meta.url)), '../..')
const temporary = await mkdtemp(path.join(tmpdir(), 'picobrew-hub-browser-'))
const binary = path.join(temporary, process.platform === 'win32' ? 'hub.exe' : 'hub')
try {
  await promisify(execFile)(
    process.env.GO_BINARY || 'go',
    ['build', '-o', binary, './cmd/picobrew-hub'],
    { cwd: root }
  )
} catch (error) {
  console.error(error.stderr || error.message)
  await rm(temporary, { recursive: true, force: true })
  process.exit(1)
}
const child = spawn(
  binary,
  [
    '--demo',
    '--listen',
    `127.0.0.1:${process.env.HUB_TEST_PORT || '18080'}`,
    '--data',
    path.join(temporary, 'data'),
    '--web',
    path.join(root, 'web/dist'),
  ],
  { cwd: root, stdio: 'inherit' }
)
let stopping = false
function stop() {
  if (stopping) return
  stopping = true
  child.kill('SIGTERM')
  setTimeout(() => child.kill('SIGKILL'), 12000).unref()
}
process.on('SIGTERM', stop)
process.on('SIGINT', stop)
child.on('error', (error) => console.error(error.message))
child.on('close', async (code) => {
  await rm(temporary, { recursive: true, force: true })
  process.exit(stopping ? 0 : code || 0)
})
