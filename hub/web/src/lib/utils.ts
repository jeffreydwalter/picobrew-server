import type { Device, Recipe, Sample, Unit } from '../types'
export function temperature(value: number | null | undefined, unit: Unit = 'F') {
  return value == null
    ? '—'
    : `${(unit === 'C' ? ((value - 32) * 5) / 9 : value).toFixed(1)}°${unit}`
}
export function convertTemp(value: number | null, unit: Unit) {
  return value == null ? null : unit === 'C' ? Math.round(((value - 32) * 50) / 9) / 10 : value
}
export function duration(milliseconds: number) {
  const total = Math.max(0, Math.floor(milliseconds / 1000))
  const h = Math.floor(total / 3600)
  const m = Math.floor((total % 3600) / 60)
  const s = total % 60
  return h ? `${h}h ${m.toString().padStart(2, '0')}m` : `${m}:${s.toString().padStart(2, '0')}`
}
export function clockTime(milliseconds: number) {
  const total = Math.max(0, Math.floor(milliseconds / 1000))
  return `${Math.floor(total / 60)}:${(total % 60).toString().padStart(2, '0')}`
}
export function dateLabel(value: string | null | undefined) {
  return value
    ? new Intl.DateTimeFormat(undefined, {
        month: 'short',
        day: 'numeric',
        hour: 'numeric',
        minute: '2-digit',
      }).format(new Date(value))
    : 'Not recorded'
}
export function deviceState(device: Device) {
  return !device.online
    ? 'Offline'
    : device.status?.stopped
      ? 'Stopped'
      : device.status?.busy
        ? 'Brewing'
        : device.status?.ready
          ? 'Ready'
          : 'Connected'
}
export function slug(value: string) {
  return (
    value
      .toLowerCase()
      .replace(/[^a-z0-9]+/g, '-')
      .replace(/^-|-$/g, '') || 'export'
  )
}
export function newRecipe(): Recipe {
  return {
    id: '',
    name: '',
    style: '',
    description: '',
    batch_liters: 5,
    efficiency: 70,
    ingredients: [],
    steps: [
      { name: 'Mash', kind: 'mash', target_c: 67, minutes: 60, drain_minutes: 2, location: 0 },
    ],
    tags: [],
    notes: '',
    revision: 0,
  }
}
export function transitions(samples: Sample[]) {
  return samples.filter(
    (sample, i) =>
      i === 0 ||
      sample.phase !== samples[i - 1].phase ||
      sample.current_step !== samples[i - 1].current_step ||
      sample.flags !== samples[i - 1].flags ||
      sample.device_state !== samples[i - 1].device_state
  )
}
export function annotations(notes: string) {
  return Array.from(notes.matchAll(/^\[(\d+):(\d{2})\]\s*(.+)$/gm), (m) => ({
    elapsed_ms: (Number(m[1]) * 60 + Number(m[2])) * 1000,
    label: m[3],
  }))
}

export function ingredientUnits(kind: string) {
  return kind === 'fermentable'
    ? ['kg']
    : kind === 'hop'
      ? ['g']
      : kind === 'water'
        ? ['L']
        : ['g', 'mL', 'count']
}
