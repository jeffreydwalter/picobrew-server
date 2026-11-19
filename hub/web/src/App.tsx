import { lazy, Suspense, useCallback, useEffect, useState, type FormEvent } from 'react'
import {
  LayoutDashboard,
  BookOpen,
  Library,
  History,
  Sparkles,
  Settings2,
  Code2,
  Wheat,
  Menu,
  X,
  LogOut,
  Check,
  TriangleAlert,
  ArrowRight,
  LockKeyhole,
  Radio,
} from 'lucide-react'
import { api, errorMessage, setCSRF } from './lib/api'
import type { Session, Device, Recipe, Brew, Profile, Unit } from './types'
import { HubContext } from './context'
import { Button, Field, ErrorBanner, Loading, IconButton } from './components/ui'
const Fleet = lazy(() => import('./pages/Fleet'))
const Machine = lazy(() => import('./pages/Machine'))
const Recipes = lazy(() => import('./pages/Recipes'))
const Catalog = lazy(() => import('./pages/Catalog'))
const HistoryPage = lazy(() => import('./pages/History'))
const Settings = lazy(() => import('./pages/Settings'))
const Assistant = lazy(() => import('./pages/Assistant'))
const Explorer = lazy(() => import('./pages/Explorer'))
const navigation = [
  { path: '/overview', label: 'Overview', icon: LayoutDashboard },
  { path: '/recipes', label: 'Recipe library', icon: BookOpen },
  { path: '/catalog', label: 'Community catalogue', icon: Library },
  { path: '/history', label: 'Brew journal', icon: History },
  { path: '/assistant', label: 'Brew assistant', icon: Sparkles },
  { path: '/settings', label: 'Settings', icon: Settings2 },
  { path: '/api', label: 'API explorer', icon: Code2 },
]
export default function App() {
  const [session, setSession] = useState<Session | null>(null)
  const [authError, setAuthError] = useState('')
  const [route, setRoute] = useState(location.hash.slice(1) || '/overview')
  const [mobile, setMobile] = useState(false)
  const [devices, setDevices] = useState<Device[]>([])
  const [recipes, setRecipes] = useState<Recipe[]>([])
  const [brews, setBrews] = useState<Brew[]>([])
  const [profiles, setProfiles] = useState<Profile[]>([])
  const [loading, setLoading] = useState(true)
  const [connected, setConnected] = useState(false)
  const [draft, setDraft] = useState<Recipe | null>(null)
  const [unit, setUnitState] = useState<Unit>(() =>
    localStorage.getItem('hub-temperature-unit') === 'C' ? 'C' : 'F'
  )
  const [toasts, setToasts] = useState<{ id: number; message: string; kind: string }[]>([])
  const notify = useCallback((message: string, kind: 'success' | 'error' = 'success') => {
    const id = Date.now() + Math.random()
    setToasts((items) => [...items, { id, message, kind }])
    setTimeout(() => setToasts((items) => items.filter((item) => item.id !== id)), 6000)
  }, [])
  const loadSession = useCallback(async () => {
    try {
      const value = await api<Session>('/auth/session')
      setSession(value)
      setCSRF(value.csrf_token)
      setAuthError('')
    } catch (error) {
      setAuthError(errorMessage(error))
    }
  }, [])
  const refresh = useCallback(async () => {
    const results = await Promise.allSettled([
      api<Device[]>('/devices'),
      api<Recipe[]>('/recipes'),
      api<Brew[]>('/brews'),
      api<Profile[]>('/profiles'),
    ])
    const setters = [setDevices, setRecipes, setBrews, setProfiles] as ((value: any[]) => void)[]
    results.forEach((result, i) => {
      if (result.status === 'fulfilled') setters[i](result.value || [])
      else notify(errorMessage(result.reason), 'error')
    })
    setLoading(false)
  }, [notify])
  useEffect(() => {
    void loadSession()
    const listener = () => {
      void loadSession()
    }
    window.addEventListener('hub:unauthorized', listener)
    return () => window.removeEventListener('hub:unauthorized', listener)
  }, [loadSession])
  useEffect(() => {
    const listener = () => {
      setRoute(location.hash.slice(1) || '/overview')
      setMobile(false)
      window.scrollTo(0, 0)
    }
    window.addEventListener('hashchange', listener)
    return () => window.removeEventListener('hashchange', listener)
  }, [])
  useEffect(() => {
    if (route === '/settings/new-device') {
      history.replaceState(null, '', '#/overview/new-device')
      setRoute('/overview/new-device')
    }
  }, [route])
  useEffect(() => {
    if (!session?.authenticated) return
    void refresh()
    const events = new EventSource('/api/v1/events', { withCredentials: true })
    events.onopen = () => setConnected(true)
    events.onerror = () => setConnected(false)
    events.addEventListener('fleet', (event) => {
      try {
        setDevices(JSON.parse((event as MessageEvent).data))
        setConnected(true)
      } catch {}
    })
    const interval = setInterval(() => {
      void refresh()
    }, 45000)
    return () => {
      events.close()
      clearInterval(interval)
    }
  }, [session?.authenticated, refresh])
  function navigate(path: string) {
    location.hash = path
    if (path === route) setMobile(false)
  }
  function setUnit(value: Unit) {
    setUnitState(value)
    localStorage.setItem('hub-temperature-unit', value)
  }
  if (!session)
    return (
      <div className="boot-screen">
        <Wheat size={38} />
        {authError ? (
          <ErrorBanner message={authError} retry={() => void loadSession()} />
        ) : (
          <Loading />
        )}
      </div>
    )
  if (!session.authenticated) return <AuthPage session={session} onComplete={loadSession} />
  const parts = route.split('?')[0].split('/').filter(Boolean)
  const section = parts[0] || 'overview'
  const selected = navigation.find((item) => item.path === `/${section}`) || navigation[0]
  return (
    <HubContext.Provider
      value={{
        session,
        devices,
        recipes,
        brews,
        profiles,
        loading,
        connected,
        unit,
        setUnit,
        refresh,
        notify,
        navigate,
        draft,
        setDraft,
      }}
    >
      <div className="app-shell">
        {mobile && <div className="sidebar-scrim" onClick={() => setMobile(false)} />}
        <aside className={`sidebar ${mobile ? 'open' : ''}`}>
          <a href="#/overview" className="brand">
            <span className="brand-mark">
              <Wheat size={24} />
            </span>
            <div>
              PicoBrew <strong>Hub</strong>
              <small>YOUR BREWERY, TOGETHER</small>
            </div>
          </a>
          <div className="sidebar-label">WORKSPACE</div>
          <nav aria-label="Main navigation">
            {navigation.map((item) => (
              <a
                key={item.path}
                href={`#${item.path}`}
                className={
                  section === item.path.slice(1) ||
                  (section === 'devices' && item.path === '/overview')
                    ? 'active'
                    : ''
                }
              >
                <item.icon size={19} />
                <span>{item.label}</span>
                {item.path === '/recipes' && <small>{recipes.length}</small>}
              </a>
            ))}
          </nav>
          <div className="sidebar-machines">
            <div className="sidebar-label">
              YOUR MACHINES <span>{devices.length}</span>
            </div>
            {devices.map((device) => (
              <a href={`#/devices/${device.id}`} key={device.id}>
                <i className={`online-dot ${device.online ? 'online' : ''}`} />
                <span>{device.name}</span>
                {device.status?.busy && <Radio size={13} />}
              </a>
            ))}
          </div>
          <div className="sidebar-bottom">
            <div className="local-status">
              <i className="online-dot online" />
              Independent by design
            </div>
            <p>
              Your machines. Your recipes.
              <br />
              Your brew history.
            </p>
            <small>Hub {session.version || '0.1.0'}</small>
          </div>
        </aside>
        <div className="main-shell">
          <header className="topbar">
            <div>
              <IconButton
                label="Open navigation"
                className="mobile-nav-button"
                onClick={() => setMobile(true)}
              >
                <Menu size={21} />
              </IconButton>
              <span className="breadcrumb">
                Brewery <span>/</span>{' '}
                <strong>{section === 'devices' ? 'Machine' : selected.label}</strong>
              </span>
            </div>
            <div className="topbar-right">
              {session.demo && <span className="simulation-badge">SIMULATION</span>}
              <span className={`connection-label ${connected ? 'connected' : ''}`}>
                <i />
                {connected ? 'Live connection' : 'Reconnecting…'}
              </span>
              <button className="unit-toggle" onClick={() => setUnit(unit === 'F' ? 'C' : 'F')}>
                °{unit}
              </button>
              <IconButton
                label="Sign out"
                onClick={async () => {
                  try {
                    await api('/auth/logout', { method: 'POST' })
                    await loadSession()
                  } catch (error) {
                    notify(errorMessage(error), 'error')
                  }
                }}
              >
                <LogOut size={17} />
              </IconButton>
            </div>
          </header>
          <main className="main-content">
            {session.demo && (
              <div className="demo-notice">
                <Radio size={15} />
                <span>
                  Simulation workspace · Controls operate virtual brewers. Your real machines are
                  untouched.
                </span>
              </div>
            )}
            <Suspense fallback={<Loading />}>
              {section === 'devices' ? (
                <Machine key={parts[1]} id={parts[1]} />
              ) : section === 'recipes' ? (
                <Recipes key={parts[1] || 'list'} id={parts[1]} />
              ) : section === 'catalog' ? (
                <Catalog key={parts[1] || 'list'} id={parts[1]} />
              ) : section === 'history' ? (
                <HistoryPage key={parts[1] || 'list'} id={parts[1]} />
              ) : section === 'settings' ? (
                <Settings />
              ) : section === 'assistant' ? (
                <Assistant />
              ) : section === 'api' ? (
                <Explorer />
              ) : (
                <Fleet addMachine={parts[1] === 'new-device'} />
              )}
            </Suspense>
          </main>
          <footer className="app-footer">
            <span>
              PicoBrew Hub <span>·</span> Crafted for your brew day
            </span>
            <span>
              {devices.filter((device) => device.online).length} of {devices.length} machines
              connected
            </span>
          </footer>
        </div>
        <div className="toasts" aria-live="polite">
          {toasts.map((toast) => (
            <div
              key={toast.id}
              className={`toast ${toast.kind}`}
              role={toast.kind === 'error' ? 'alert' : 'status'}
            >
              {toast.kind === 'error' ? <TriangleAlert size={18} /> : <Check size={18} />}
              <span>{toast.message}</span>
              <button
                aria-label="Dismiss notification"
                onClick={() => setToasts((items) => items.filter((item) => item.id !== toast.id))}
              >
                <X size={15} />
              </button>
            </div>
          ))}
        </div>
      </div>
    </HubContext.Provider>
  )
}
function AuthPage({ session, onComplete }: { session: Session; onComplete: () => Promise<void> }) {
  const [password, setPassword] = useState('')
  const [token, setToken] = useState('')
  const [busy, setBusy] = useState(false)
  const [error, setError] = useState('')
  async function submit(event: FormEvent) {
    event.preventDefault()
    setBusy(true)
    setError('')
    try {
      await api(session.setup_required ? '/auth/setup' : '/auth/login', {
        method: 'POST',
        body: session.setup_required ? { token, password } : { password },
      })
      await onComplete()
    } catch (error) {
      setError(errorMessage(error))
    } finally {
      setBusy(false)
    }
  }
  return (
    <div className="auth-page">
      <div className="auth-story">
        <div className="brand inverse">
          <span className="brand-mark">
            <Wheat size={27} />
          </span>
          <div>
            PicoBrew <strong>Hub</strong>
          </div>
        </div>
        <div>
          <p className="eyebrow">FROM RECIPE TO LAST POUR</p>
          <h1>
            A little more craft.
            <br />A lot more control.
          </h1>
          <p>Bring your brewers, recipes and brew days together in one place.</p>
          <div className="auth-lines">
            <span />
            <span />
            <span />
          </div>
        </div>
        <small>Local first. Made for the machines you love.</small>
      </div>
      <div className="auth-form">
        <LockKeyhole size={28} />
        <p className="eyebrow">YOUR BREWERY WORKSPACE</p>
        <h2>{session.setup_required ? 'Make yourself at home.' : 'Welcome back.'}</h2>
        <p>
          {session.setup_required
            ? 'Use the setup token printed by your hub and choose a workspace password.'
            : 'Sign in to monitor your machines and plan the next brew.'}
        </p>
        <form onSubmit={submit}>
          {session.setup_required && (
            <Field label="Setup token">
              <input
                value={token}
                onChange={(event) => setToken(event.target.value)}
                autoComplete="off"
                required
              />
            </Field>
          )}
          <Field label={session.setup_required ? 'Choose a password' : 'Password'}>
            <input
              type="password"
              value={password}
              onChange={(event) => setPassword(event.target.value)}
              autoComplete={session.setup_required ? 'new-password' : 'current-password'}
              required
              minLength={session.setup_required ? 12 : 1}
            />
          </Field>
          <ErrorBanner message={error} />
          <Button type="submit" disabled={busy}>
            {busy ? 'Connecting…' : session.setup_required ? 'Create workspace' : 'Sign in'}
            <ArrowRight size={17} />
          </Button>
        </form>
      </div>
    </div>
  )
}
