import { useEffect, useMemo, useState } from 'react'
import { Play, Search, Code2, Copy, ArrowUpRight, LockKeyhole, ChevronRight } from 'lucide-react'
import { useHub, useResource } from '../context'
import { copyText, errorMessage, saveBlob } from '../lib/api'
import { inspectAPI } from '../lib/explorer'
import {
  Badge,
  Button,
  Confirm,
  ErrorBanner,
  Field,
  Loading,
  PageTitle,
  SearchBox,
  SectionHeading,
} from '../components/ui'
type Operation = { path: string; method: string; summary: string; definition: any }
export default function Explorer() {
  const hub = useHub()
  const resource = useResource<any>('/api/v1/openapi.json')
  const [query, setQuery] = useState('')
  const [selected, setSelected] = useState<Operation | null>(null)
  const [path, setPath] = useState('')
  const [body, setBody] = useState('{}')
  const [response, setResponse] = useState('')
  const [error, setError] = useState('')
  const [busy, setBusy] = useState(false)
  const [writes, setWrites] = useState(false)
  const [confirm, setConfirm] = useState(false)
  const operations = useMemo(
    () =>
      Object.entries(resource.data?.paths || {}).flatMap(([path, entry]) =>
        Object.entries(entry as Record<string, any>)
          .filter(([method]) => ['get', 'post', 'put', 'patch', 'delete'].includes(method))
          .map(([method, definition]) => ({
            path,
            method: method.toUpperCase(),
            summary: definition.summary || definition.description || '',
            definition,
          }))
      ),
    [resource.data]
  )
  const filtered = operations.filter((operation) =>
    `${operation.method} ${operation.path} ${operation.summary}`
      .toLowerCase()
      .includes(query.toLowerCase())
  )
  function select(operation: Operation) {
    setSelected(operation)
    const firstID = operation.path.includes('/devices/')
      ? hub.devices[0]?.id
      : operation.path.includes('/recipes/')
        ? hub.recipes[0]?.id
        : operation.path.includes('/brews/')
          ? hub.brews[0]?.id
          : ''
    setPath(operation.path.replace('{id}', firstID || '{id}'))
    const example = operation.definition.requestBody?.content?.['application/json']?.example
    setBody(
      JSON.stringify(
        example || operation.path.includes('/commands')
          ? example || { command: 'encoder', delta: 1 }
          : example || {},
        null,
        2
      )
    )
    setResponse('')
    setError('')
  }
  useEffect(() => {
    if (!selected && operations.length)
      select(
        operations.find(
          (operation) => operation.method === 'GET' && operation.path.endsWith('/devices')
        ) || operations[0]
      )
  }, [operations])
  async function execute() {
    if (!selected) return
    setBusy(true)
    setError('')
    setConfirm(false)
    const started = performance.now()
    try {
      const target = path.startsWith('/api/v1/')
        ? path
        : `/api/v1${path.startsWith('/') ? '' : '/'}${path}`
      if (!target.startsWith('/api/v1/') || target.includes('..') || target.includes('://'))
        throw new Error('Only hub API paths can be used here.')
      const result = await inspectAPI(target, {
        method: selected.method,
        ...(selected.method !== 'GET' ? { body: body.trim() ? JSON.parse(body) : undefined } : {}),
        command: target.includes('/commands'),
      })
      setResponse(`${Math.round(performance.now() - started)} ms\n\n${result.text}`)
      if (result.download) saveBlob(result.download.blob, result.download.filename)
      if (selected.method !== 'GET') await hub.refresh()
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  return (
    <>
      <PageTitle
        eyebrow="THE SAME API, YOUR OWN IDEAS"
        title="Explore the hub API."
        description="Browse every endpoint and try authenticated requests from your workspace."
        actions={
          <Button
            variant="secondary"
            onClick={() => window.open('/api/v1/openapi.json', '_blank', 'noopener')}
          >
            OpenAPI document
            <ArrowUpRight size={16} />
          </Button>
        }
      />
      <div className="api-auth-note">
        <LockKeyhole size={17} />
        <span>
          Uses your signed-in session. CSRF tokens and command identifiers are added automatically.
        </span>
      </div>
      {resource.loading ? (
        <Loading label="Loading API definition…" />
      ) : resource.error ? (
        <ErrorBanner message={resource.error} retry={resource.reload} />
      ) : (
        <div className="explorer-layout">
          <aside className="endpoint-sidebar">
            <SearchBox value={query} onChange={setQuery} placeholder="Find an endpoint" />
            <div className="endpoint-list">
              {filtered.map((operation) => (
                <button
                  key={`${operation.method} ${operation.path}`}
                  className={
                    selected?.path === operation.path && selected?.method === operation.method
                      ? 'active'
                      : ''
                  }
                  onClick={() => select(operation)}
                >
                  <span className={`http-method method-${operation.method.toLowerCase()}`}>
                    {operation.method}
                  </span>
                  <code>{operation.path}</code>
                </button>
              ))}
            </div>
          </aside>
          <section className="panel endpoint-detail">
            {selected && (
              <>
                <div className="endpoint-heading">
                  <span className={`http-method method-${selected.method.toLowerCase()}`}>
                    {selected.method}
                  </span>
                  <h2>{selected.summary || selected.path}</h2>
                </div>
                <Field label="Request path">
                  <input
                    className="code-input"
                    value={path}
                    onChange={(event) => setPath(event.target.value)}
                  />
                </Field>
                {selected.definition.parameters?.length > 0 && (
                  <div className="api-parameters">
                    <h3>Parameters</h3>
                    {selected.definition.parameters.map((parameter: any, index: number) => (
                      <p key={index}>
                        <code>{parameter.name}</code>{' '}
                        <small>
                          {parameter.in} {parameter.required ? '· required' : ''}
                        </small>{' '}
                        {parameter.description}
                      </p>
                    ))}
                  </div>
                )}
                {selected.method !== 'GET' && (
                  <>
                    <Field label="JSON request body">
                      <textarea
                        className="code-input"
                        rows={8}
                        value={body}
                        onChange={(event) => setBody(event.target.value)}
                      />
                    </Field>
                    <label className="check-field">
                      <input
                        type="checkbox"
                        checked={writes}
                        onChange={(event) => setWrites(event.target.checked)}
                      />
                      Allow requests that change data or control a machine
                    </label>
                  </>
                )}
                <div className="inline-actions">
                  <Button
                    disabled={busy || (selected.method !== 'GET' && !writes) || path.includes('{')}
                    onClick={() => (selected.method === 'GET' ? void execute() : setConfirm(true))}
                  >
                    <Play size={15} />
                    {busy ? 'Sending…' : 'Send request'}
                  </Button>
                  <Button
                    variant="ghost"
                    onClick={async () => {
                      try {
                        await copyText(
                          `${location.origin}${path.startsWith('/api/v1/') ? path : '/api/v1' + path}`
                        )
                        hub.notify('Endpoint URL copied.')
                      } catch (error) {
                        hub.notify(errorMessage(error), 'error')
                      }
                    }}
                  >
                    <Copy size={15} />
                    Copy URL
                  </Button>
                </div>
                <ErrorBanner message={error} />
                {response && (
                  <div className="api-response">
                    <SectionHeading title="Response" />
                    <pre>{response}</pre>
                  </div>
                )}
              </>
            )}
          </section>
        </div>
      )}
      {confirm && (
        <Confirm
          title="Send this API request?"
          description={`${selected?.method} ${path} may change workspace data or operate a machine. Review the path and body before continuing.`}
          label="Send request"
          onClose={() => setConfirm(false)}
          onConfirm={() => void execute()}
        />
      )}
    </>
  )
}
