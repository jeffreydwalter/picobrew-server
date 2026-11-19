import { describe, it, expect } from 'vitest'
import { decodeDisplay } from './display'
describe('Pico OLED decoder', () => {
  it('decodes page-major LSB pixels and rotates both axes 180 degrees', () => {
    const pixels = decodeDisplay({ width: 2, height: 8, format: 'page-lsb-hex', data: '01 80' })
    expect(Array.from(pixels)).toEqual([1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1])
  })
  it('addresses successive vertical pages correctly', () => {
    const pixels = decodeDisplay({ width: 1, height: 16, format: 'page-lsb-hex', data: '0001' })
    expect(pixels[7]).toBe(1)
    expect(pixels.reduce((sum, pixel) => sum + pixel, 0)).toBe(1)
  })
  it('rejects truncated, malformed or oversized frames', () => {
    for (const data of ['', 'zz', '001122'])
      expect(() => decodeDisplay({ width: 1, height: 8, format: 'page-lsb-hex', data })).toThrow()
    expect(() =>
      decodeDisplay({ width: 10000, height: 8, format: 'page-lsb-hex', data: '00' })
    ).toThrow()
    expect(() => decodeDisplay({ width: 1, height: 8, format: 'unknown', data: '00' })).toThrow()
  })
})
