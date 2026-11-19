import type { MachineRecipe, Recipe } from '../types'

export function isNative(recipe: Recipe) {
  return recipe.kind === 'native-program' || recipe.kind === 'maintenance'
}
export function kindLabel(kind?: string) {
  return kind === 'native-program'
    ? 'Machine program'
    : kind === 'maintenance'
      ? 'Cleaning program'
      : 'Ingredient recipe'
}
export function programTiming(program: MachineRecipe) {
  const minutes = program.steps.reduce((sum, step) => sum + step.minutes + step.drain_minutes, 0)
  return `${minutes} min${program.steps.some((step) => step.target_f > 0) ? ' + heating' : ''}`
}
export function externalURL(value?: string) {
  if (!value) return undefined
  try {
    const url = new URL(value)
    return ['https:', 'http:'].includes(url.protocol) ? url.href : undefined
  } catch {
    return undefined
  }
}
export function catalogAssetURL(value?: string) {
  return value?.startsWith('/api/v1/catalog/assets/') && !value.includes('..') ? value : undefined
}
export function copyProgram(program: MachineRecipe): MachineRecipe {
  return { ...program, steps: program.steps.map((step) => ({ ...step })) }
}
