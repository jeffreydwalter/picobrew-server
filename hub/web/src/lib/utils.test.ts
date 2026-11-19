import { describe, it, expect } from 'vitest'
import { annotations, clockTime, convertTemp, duration, temperature, transitions } from './utils'
import type { Sample } from '../types'
describe('telemetry presentation', () => {
  it('preserves missing sensors and converts Fahrenheit accurately', () => {
    expect(temperature(null, 'C')).toBe('—')
    expect(convertTemp(null, 'C')).toBeNull()
    expect(temperature(212, 'C')).toBe('100.0°C')
    expect(convertTemp(32, 'C')).toBe(0)
  })
  it('formats elapsed time without interpreting it as wall-clock time', () => {
    expect(clockTime(7800000)).toBe('130:00')
    expect(duration(7800000)).toBe('2h 10m')
    expect(duration(65000)).toBe('1:05')
  })
  it('parses timestamp annotations as plain text', () => {
    expect(annotations('[25:30] Mash settled\nA tasting note\n[130:00] Finished')).toEqual([
      { elapsed_ms: 1530000, label: 'Mash settled' },
      { elapsed_ms: 7800000, label: 'Finished' },
    ])
  })
  it('retains phase, step, flags and device-state transitions', () => {
    const base = { elapsed_ms: 0, phase: 1, current_step: 0, flags: 1, device_state: 1 } as Sample
    const samples = [
      base,
      { ...base, elapsed_ms: 1 },
      { ...base, elapsed_ms: 2, flags: 3 },
      { ...base, elapsed_ms: 3, flags: 3, current_step: 1 },
    ]
    expect(transitions(samples).map((sample) => sample.elapsed_ms)).toEqual([0, 2, 3])
  })
})
