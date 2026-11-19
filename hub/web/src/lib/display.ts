import type { Display } from '../types'
// Pico's framebuffer is page-major, bit 0 at the top of each 8-pixel page.
// The physical OLED orientation is 180 degrees from the raw framebuffer.
export function decodeDisplay(display: Display): Uint8Array {
  const { width, height } = display
  if (
    !Number.isInteger(width) ||
    !Number.isInteger(height) ||
    width < 1 ||
    height < 1 ||
    width > 512 ||
    height > 512
  )
    throw new Error('Invalid OLED dimensions')
  if (display.format !== 'page-lsb-hex')
    throw new Error(`Unsupported display format: ${display.format}`)
  const hex = display.data.replace(/\s/g, '')
  const size = width * Math.ceil(height / 8)
  if (!/^[0-9a-f]*$/i.test(hex) || hex.length !== size * 2) throw new Error('Incomplete OLED frame')
  const bytes = Uint8Array.from({ length: size }, (_, i) =>
    parseInt(hex.slice(i * 2, i * 2 + 2), 16)
  )
  const pixels = new Uint8Array(width * height)
  for (let y = 0; y < height; y++)
    for (let x = 0; x < width; x++)
      pixels[(height - 1 - y) * width + (width - 1 - x)] =
        (bytes[(y >> 3) * width + x] >> (y & 7)) & 1
  return pixels
}
