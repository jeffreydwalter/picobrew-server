import { useEffect, useRef, useState } from 'react'
import type { Display } from '../types'
import { decodeDisplay } from '../lib/display'
export default function OLED({ display, error }: { display: Display | null; error?: string }) {
  const ref = useRef<HTMLCanvasElement>(null)
  const [invalid, setInvalid] = useState('')
  useEffect(() => {
    if (!display || !ref.current) return
    try {
      const pixels = decodeDisplay(display)
      const ctx = ref.current.getContext('2d')
      if (!ctx) return
      const data = ctx.createImageData(display.width, display.height)
      pixels.forEach((on, i) => {
        const color = on ? [196, 219, 171] : [28, 43, 35]
        data.data.set([...color, 255], i * 4)
      })
      ctx.putImageData(data, 0, 0)
      setInvalid('')
    } catch (error) {
      setInvalid((error as Error).message)
    }
  }, [display])
  return (
    <div className="oled">
      {display && !invalid ? (
        <canvas
          ref={ref}
          width={display.width}
          height={display.height}
          aria-label="Live Pico OLED display"
        />
      ) : (
        <div className="oled-placeholder">{error || invalid || 'Waiting for the front panel…'}</div>
      )}
      <span>LIVE OLED · {display ? `${display.width} × ${display.height}` : '128 × 64'}</span>
    </div>
  )
}
