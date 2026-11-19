import { apiResponse, type RequestOptions } from './api'

export async function inspectAPI(path: string, options: RequestOptions = {}) {
  const controller = new AbortController()
  const timeout = setTimeout(() => controller.abort(), 15000)
  try {
    const response = await apiResponse(path, {
      ...options,
      accept: '*/*',
      signal: controller.signal,
    })
    const contentType = response.headers.get('Content-Type') || ''
    const heading = `${response.status} ${response.statusText}\n${contentType || 'No Content-Type'}\n`
    if (response.status === 204) return { text: `${heading}\nNo response body` }
    if (contentType.includes('text/event-stream')) {
      const reader = response.body?.getReader()
      if (!reader) return { text: `${heading}\nNo stream body` }
      const decoder = new TextDecoder()
      let text = ''
      try {
        while (text.length < 65536) {
          const chunk = await reader.read()
          if (chunk.done) break
          text += decoder.decode(chunk.value, { stream: true })
          if (/\r?\n\r?\n/.test(text)) break
        }
      } finally {
        await reader.cancel().catch(() => {})
      }
      return {
        text: `${heading}\n${text.slice(0, 65536)}\nStream preview ended after the first event (maximum 64 KiB).`,
      }
    }
    if (/^(text\/|application\/(?:json|[^;]+\+json|xml|[^;]+\+xml))/i.test(contentType)) {
      const text = await response.text()
      let formatted = text
      if (contentType.includes('json')) {
        try {
          formatted = JSON.stringify(JSON.parse(text), null, 2)
        } catch {}
      }
      return {
        text: `${heading}\n${formatted.slice(0, 1048576)}${formatted.length > 1048576 ? '\n[Preview truncated at 1 MiB]' : ''}`,
      }
    }
    const blob = await response.blob()
    const disposition = response.headers.get('Content-Disposition') || ''
    const proposed = /filename="?([^";]+)"?/i.exec(disposition)?.[1]
    const filename = (
      proposed || (contentType.includes('zip') ? 'picobrew-export.zip' : 'api-response.bin')
    ).replace(/[\\/]/g, '_')
    return {
      text: `${heading}\nBinary response: ${blob.size.toLocaleString()} bytes. Download prepared as ${filename}.`,
      download: { blob, filename },
    }
  } catch (error) {
    if (controller.signal.aborted)
      throw new Error(
        'The request timed out after 15 seconds. Check the current state before retrying a change.'
      )
    throw error
  } finally {
    clearTimeout(timeout)
    controller.abort()
  }
}
