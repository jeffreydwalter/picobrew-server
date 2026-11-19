import {
  cloneElement,
  isValidElement,
  useEffect,
  useId,
  useRef,
  type ReactNode,
  type ButtonHTMLAttributes,
} from 'react'
import { createPortal } from 'react-dom'
import { ArrowLeft, Loader2, TriangleAlert, X, Plus, Search, Check } from 'lucide-react'
export function Button({
  variant = 'primary',
  className = '',
  children,
  ...props
}: ButtonHTMLAttributes<HTMLButtonElement> & {
  variant?: 'primary' | 'secondary' | 'ghost' | 'danger' | 'copper'
}) {
  return (
    <button className={`button ${variant} ${className}`} {...props}>
      {children}
    </button>
  )
}
export function IconButton({
  label,
  children,
  className = '',
  ...props
}: ButtonHTMLAttributes<HTMLButtonElement> & { label: string }) {
  return (
    <button className={`icon-button ${className}`} title={label} aria-label={label} {...props}>
      {children}
    </button>
  )
}
export function Badge({ children, tone = 'neutral' }: { children: ReactNode; tone?: string }) {
  return (
    <span className={`badge ${tone}`}>
      <span className="badge-dot" />
      {children}
    </span>
  )
}
export function PageTitle({
  eyebrow,
  title,
  description,
  actions,
  back,
  className = '',
}: {
  eyebrow?: string
  title: string
  description?: string
  actions?: ReactNode
  back?: () => void
  className?: string
}) {
  return (
    <header className={`page-heading ${className}`}>
      <div>
        {back && (
          <button className="back-link" onClick={back}>
            <ArrowLeft size={16} />
            Back
          </button>
        )}
        {eyebrow && <p className="eyebrow">{eyebrow}</p>}
        <h1>{title}</h1>
        {description && <p className="page-description">{description}</p>}
      </div>
      {actions && <div className="page-actions">{actions}</div>}
    </header>
  )
}
export function Field({
  label,
  hint,
  children,
  className = '',
}: {
  label: string
  hint?: string
  children: ReactNode
  className?: string
}) {
  const generatedID = useId()
  const hintID = useId()
  const element = isValidElement<{ id?: string; 'aria-describedby'?: string }>(children)
    ? children
    : null
  const controlID = element ? element.props.id || generatedID : undefined
  const control = element
    ? cloneElement(element, {
        id: controlID,
        ...(hint
          ? {
              'aria-describedby': [element.props['aria-describedby'], hintID]
                .filter(Boolean)
                .join(' '),
            }
          : {}),
      })
    : children
  return (
    <div className={`field ${className}`}>
      <label className="field-label" htmlFor={controlID}>
        <span>{label}</span>
      </label>
      {control}
      {hint && <small id={hintID}>{hint}</small>}
    </div>
  )
}
export function Empty({
  title,
  description,
  action,
  icon,
}: {
  title: string
  description: string
  action?: ReactNode
  icon?: ReactNode
}) {
  return (
    <div className="empty-state">
      <div className="empty-symbol">{icon || <Plus size={26} />}</div>
      <h3>{title}</h3>
      <p>{description}</p>
      {action}
    </div>
  )
}
export function Loading({ label = 'Loading your brewery…' }: { label?: string }) {
  return (
    <div className="loading-state" role="status">
      <Loader2 size={22} className="spin" />
      {label}
    </div>
  )
}
export function ErrorBanner({ message, retry }: { message: string; retry?: () => void }) {
  if (!message) return null
  return (
    <div className="error-banner" role="alert">
      <TriangleAlert size={18} />
      <span>{message}</span>
      {retry && (
        <Button variant="ghost" onClick={retry}>
          Try again
        </Button>
      )}
    </div>
  )
}
export function SearchBox({
  value,
  onChange,
  placeholder = 'Search',
}: {
  value: string
  onChange: (value: string) => void
  placeholder?: string
}) {
  return (
    <div className="search-box">
      <Search size={17} />
      <input
        aria-label={placeholder}
        value={value}
        onChange={(event) => onChange(event.target.value)}
        placeholder={placeholder}
      />
    </div>
  )
}
export function Modal({
  title,
  children,
  onClose,
  actions,
  wide = false,
}: {
  title: string
  children: ReactNode
  onClose: () => void
  actions?: ReactNode
  wide?: boolean
}) {
  const ref = useRef<HTMLDialogElement>(null)
  const id = useId()
  useEffect(() => {
    const previous = document.activeElement as HTMLElement | null
    ref.current?.showModal()
    return () => previous?.focus()
  }, [])
  return createPortal(
    <dialog
      className={`modal ${wide ? 'wide' : ''}`}
      ref={ref}
      aria-labelledby={id}
      onCancel={(event) => {
        event.preventDefault()
        onClose()
      }}
      onClick={(event) => {
        if (event.target === event.currentTarget) onClose()
      }}
    >
      <div className="modal-heading">
        <h2 id={id}>{title}</h2>
        <IconButton label="Close dialog" onClick={onClose}>
          <X size={20} />
        </IconButton>
      </div>
      <div className="modal-content">{children}</div>
      {actions && <footer className="modal-actions">{actions}</footer>}
    </dialog>,
    document.body
  )
}
export function Confirm({
  title,
  description,
  onConfirm,
  onClose,
  busy = false,
  danger = false,
  label = 'Confirm',
}: {
  title: string
  description: string
  onConfirm: () => void
  onClose: () => void
  busy?: boolean
  danger?: boolean
  label?: string
}) {
  return (
    <Modal
      title={title}
      onClose={onClose}
      actions={
        <>
          <Button variant="secondary" onClick={onClose} disabled={busy}>
            Cancel
          </Button>
          <Button variant={danger ? 'danger' : 'primary'} onClick={onConfirm} disabled={busy}>
            {busy ? <Loader2 className="spin" size={16} /> : <Check size={16} />} {label}
          </Button>
        </>
      }
    >
      <p>{description}</p>
    </Modal>
  )
}
export function Metric({
  label,
  value,
  note,
  accent = false,
}: {
  label: string
  value: ReactNode
  note?: string
  accent?: boolean
}) {
  return (
    <div className={`metric ${accent ? 'accent' : ''}`}>
      <span>{label}</span>
      <strong>{value}</strong>
      {note && <small>{note}</small>}
    </div>
  )
}
export function SectionHeading({
  title,
  description,
  actions,
}: {
  title: string
  description?: string
  actions?: ReactNode
}) {
  return (
    <div className="section-heading">
      <div>
        <h2>{title}</h2>
        {description && <p>{description}</p>}
      </div>
      {actions}
    </div>
  )
}
