let csrfToken = ''
export function setCSRF(token: string) {
  csrfToken = token || ''
}
export function commandID() {
  if (typeof crypto.randomUUID === 'function') return crypto.randomUUID()
  // getRandomValues is available on HTTP LAN origins where randomUUID is not.
  const bytes = crypto.getRandomValues(new Uint8Array(16))
  bytes[6] = (bytes[6] & 15) | 64
  bytes[8] = (bytes[8] & 63) | 128
  const hex = Array.from(bytes, (value) => value.toString(16).padStart(2, '0')).join('')
  return `${hex.slice(0, 8)}-${hex.slice(8, 12)}-${hex.slice(12, 16)}-${hex.slice(16, 20)}-${hex.slice(20)}`
}
export class APIError extends Error {
  constructor(
    public status: number,
    public code: string,
    message: string
  ) {
    super(message)
    this.name = 'APIError'
  }
}
export interface RequestOptions {
  method?: string
  body?: unknown
  signal?: AbortSignal
  command?: boolean
  accept?: string
}
export async function apiResponse(path: string, options: RequestOptions = {}): Promise<Response> {
  const method = options.method || 'GET'
  const headers: Record<string, string> = { Accept: options.accept || 'application/json' }
  if (options.body !== undefined) headers['Content-Type'] = 'application/json'
  if (!['GET', 'HEAD', 'OPTIONS'].includes(method)) headers['X-CSRF-Token'] = csrfToken
  if (options.command) headers['Idempotency-Key'] = commandID()
  const response = await fetch(
    path.startsWith('/api/') || path === '/openapi.json' ? path : '/api/v1' + path,
    {
      method,
      headers,
      credentials: 'same-origin',
      body: options.body === undefined ? undefined : JSON.stringify(options.body),
      signal: options.signal,
    }
  )
  if (!response.ok) {
    const data = await response.json().catch(() => null)
    if (response.status === 401 && !path.includes('/auth/') && typeof window !== 'undefined')
      window.dispatchEvent(new Event('hub:unauthorized'))
    throw new APIError(
      response.status,
      data?.error?.code || 'request_failed',
      data?.error?.message ||
        data?.message ||
        (typeof data?.error === 'string' ? data.error : response.statusText) ||
        'Request failed'
    )
  }
  return response
}
export async function api<T>(path: string, options: RequestOptions = {}): Promise<T> {
  const response = await apiResponse(path, options)
  const text = await response.text()
  let data: any = null
  try {
    data = text ? JSON.parse(text) : null
  } catch {
    throw new Error('The server returned an unexpected response.')
  }
  return data as T
}
export async function download(path: string, filename: string) {
  const response = await fetch(path.startsWith('/api/') ? path : '/api/v1' + path, {
    credentials: 'same-origin',
  })
  if (!response.ok) {
    const data = await response.json().catch(() => null)
    throw new Error(data?.error?.message || 'The export could not be downloaded.')
  }
  saveBlob(await response.blob(), filename)
}
export function saveBlob(blob: Blob, filename: string) {
  const url = URL.createObjectURL(blob)
  const link = document.createElement('a')
  link.href = url
  link.download = filename
  link.click()
  setTimeout(() => URL.revokeObjectURL(url), 1000)
}
export function errorMessage(error: unknown) {
  return error instanceof Error ? error.message : 'Something went wrong. Please try again.'
}

export async function copyText(text: string) {
  if (navigator.clipboard?.writeText) {
    await navigator.clipboard.writeText(text)
    return
  }
  const input = document.createElement('textarea')
  input.value = text
  input.style.position = 'fixed'
  input.style.opacity = '0'
  document.body.append(input)
  input.select()
  const copied = document.execCommand('copy')
  input.remove()
  if (!copied)
    throw new Error(
      'Copy is unavailable in this browser. Select the request path and copy it manually.'
    )
}
