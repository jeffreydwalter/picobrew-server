import { createContext, useContext, useEffect, useState } from 'react'
import type { Session, Device, Recipe, Brew, Profile, Unit } from './types'
import { api, errorMessage } from './lib/api'
export interface HubState {
  session: Session
  devices: Device[]
  recipes: Recipe[]
  brews: Brew[]
  profiles: Profile[]
  loading: boolean
  connected: boolean
  unit: Unit
  setUnit: (value: Unit) => void
  refresh: () => Promise<void>
  notify: (message: string, kind?: 'success' | 'error') => void
  navigate: (path: string) => void
  draft: Recipe | null
  setDraft: (recipe: Recipe | null) => void
}
export const HubContext = createContext<HubState | null>(null)
export function useHub() {
  const hub = useContext(HubContext)
  if (!hub) throw new Error('Hub context is unavailable')
  return hub
}
export function useResource<T>(path: string | null, poll = 0) {
  const [data, setData] = useState<T | null>(null)
  const [error, setError] = useState('')
  const [loading, setLoading] = useState(true)
  const [version, setVersion] = useState(0)
  useEffect(() => {
    if (!path) {
      setData(null)
      setLoading(false)
      return
    }
    const controller = new AbortController()
    setLoading(true)
    setError('')
    const load = () =>
      api<T>(path, { signal: controller.signal })
        .then((value) => {
          setData(value)
          setError('')
          setLoading(false)
        })
        .catch((error) => {
          if (error.name !== 'AbortError') {
            setError(errorMessage(error))
            setLoading(false)
          }
        })
    void load()
    const timer = poll ? setInterval(load, poll) : undefined
    return () => {
      controller.abort()
      clearInterval(timer)
    }
  }, [path, poll, version])
  return { data, setData, error, loading, reload: () => setVersion((value) => value + 1) }
}
