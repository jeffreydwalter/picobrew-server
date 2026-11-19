import {
  ResponsiveContainer,
  LineChart,
  Line,
  XAxis,
  YAxis,
  CartesianGrid,
  Tooltip,
  ReferenceLine,
  AreaChart,
  Area,
} from 'recharts'
import type { Sample, Unit } from '../types'
import { annotations, clockTime, convertTemp } from '../lib/utils'
export function TemperatureChart({
  samples,
  unit,
  notes = '',
  height = 280,
  onSelect,
}: {
  samples: Sample[]
  unit: Unit
  notes?: string
  height?: number
  onSelect?: (elapsed: number) => void
}) {
  const data = samples.map((sample) => ({
    ...sample,
    wort: convertTemp(sample.wort_f, unit),
    block: convertTemp(sample.thermoblock_f, unit),
    target: convertTemp(sample.target_f, unit),
  }))
  return (
    <div
      className="chart-frame"
      role="img"
      aria-label={`Wort, thermoblock and target temperatures in degrees ${unit === 'F' ? 'Fahrenheit' : 'Celsius'} over the brew`}
    >
      <ResponsiveContainer width="100%" height={height}>
        <LineChart
          data={data}
          margin={{ top: 12, right: 16, bottom: 0, left: -18 }}
          onClick={(event: any) => {
            const value = event?.activeLabel
            if (value !== undefined && value !== null && Number.isFinite(Number(value)))
              onSelect?.(Number(value))
          }}
        >
          <CartesianGrid vertical={false} stroke="#e7e1d8" strokeDasharray="3 4" />
          <XAxis
            dataKey="elapsed_ms"
            tickFormatter={clockTime}
            minTickGap={55}
            stroke="#8d8b81"
            tickLine={false}
            axisLine={false}
            fontSize={11}
          />
          <YAxis
            domain={['auto', 'auto']}
            stroke="#8d8b81"
            tickLine={false}
            axisLine={false}
            fontSize={11}
            tickFormatter={(value) => `${value}°`}
          />
          <Tooltip
            labelFormatter={(value) => clockTime(Number(value))}
            formatter={(value: any, name: any) => [
              value == null ? '—' : `${Number(value).toFixed(1)}°${unit}`,
              name,
            ]}
            contentStyle={{ border: '1px solid #e5ddd2', borderRadius: 10, fontSize: 12 }}
          />
          <Line
            name="Wort"
            type="monotone"
            dataKey="wort"
            stroke="#b46b40"
            strokeWidth={2.5}
            dot={false}
            isAnimationActive={false}
          />
          <Line
            name="Thermoblock"
            type="monotone"
            dataKey="block"
            stroke="#597b6b"
            strokeWidth={1.7}
            dot={false}
            isAnimationActive={false}
          />
          <Line
            name="Target"
            type="stepAfter"
            dataKey="target"
            stroke="#afa38c"
            strokeDasharray="5 5"
            strokeWidth={1.5}
            dot={false}
            isAnimationActive={false}
          />
          {annotations(notes).map((note, index) => (
            <ReferenceLine
              key={index}
              x={note.elapsed_ms}
              stroke="#565775"
              strokeDasharray="3 3"
              label={{ value: note.label, position: 'insideTopLeft', fontSize: 10 }}
            />
          ))}
        </LineChart>
      </ResponsiveContainer>
      <div className="chart-legend">
        <span>
          <i className="wort" />
          Wort
        </span>
        <span>
          <i className="block" />
          Thermoblock
        </span>
        <span>
          <i className="target" />
          Target
        </span>
        <small>{samples.length.toLocaleString()} samples</small>
      </div>
    </div>
  )
}
export function StateChart({ samples }: { samples: Sample[] }) {
  return (
    <ResponsiveContainer width="100%" height={190}>
      <LineChart data={samples} margin={{ top: 10, right: 16, bottom: 0, left: -20 }}>
        <CartesianGrid vertical={false} stroke="#e7e1d8" />
        <XAxis dataKey="elapsed_ms" tickFormatter={clockTime} minTickGap={55} fontSize={11} />
        <YAxis allowDecimals={false} fontSize={11} />
        <Tooltip labelFormatter={(value) => clockTime(Number(value))} />
        <Line
          name="Phase"
          type="stepAfter"
          dataKey="phase"
          stroke="#b46b40"
          dot={false}
          isAnimationActive={false}
        />
        <Line
          name="Step"
          type="stepAfter"
          dataKey="current_step"
          stroke="#597b6b"
          dot={false}
          isAnimationActive={false}
        />
        <Line
          name="Controller"
          type="stepAfter"
          dataKey="controller_status"
          stroke="#77769a"
          dot={false}
          isAnimationActive={false}
        />
        <Line
          name="Flags"
          type="stepAfter"
          dataKey="flags"
          stroke="#b89a4c"
          dot={false}
          isAnimationActive={false}
        />
        <Line
          name="Compartment"
          type="stepAfter"
          dataKey="compartment"
          stroke="#8e9aa5"
          dot={false}
          isAnimationActive={false}
        />
        <Line
          name="Device state"
          type="stepAfter"
          dataKey="device_state"
          stroke="#343e43"
          dot={false}
          isAnimationActive={false}
        />
      </LineChart>
    </ResponsiveContainer>
  )
}
export function Sparkline({ values }: { values: { value: number | null }[] }) {
  return (
    <ResponsiveContainer width="100%" height={40}>
      <AreaChart data={values}>
        <Area
          dataKey="value"
          type="monotone"
          stroke="#b46b40"
          fill="#f0e2d7"
          strokeWidth={2}
          isAnimationActive={false}
        />
      </AreaChart>
    </ResponsiveContainer>
  )
}
