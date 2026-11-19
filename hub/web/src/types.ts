export interface Session {
  authenticated: boolean
  setup_required: boolean
  csrf_token: string
  version: string
  demo: boolean
}
export interface DeviceStatus {
  uptime_ms: number
  ready: boolean
  device_uid: string
  busy: boolean
  stopped: boolean
  command: string
  device_state: number
  phase: number
  controller_status: number
  compartment: number
  wort_f: number | null
  thermoblock_f: number | null
  target_f: number | null
  step_seconds: number
  current_step: number
  step_count: number
  recipe_name: string
  step_name: string
  session_id: string
  filter_inserted: boolean | null
  stop_reason: string
  encoder: { position: number; min: number; max: number }
}
export interface Capabilities {
  api_version: number
  firmware_version: string
  commands: string[]
  recipe_capacity: number
  step_capacity: number
  front_panel: boolean
  history_persistent: boolean
  telemetry_forwarding: boolean
  dedicated_pause_resume: boolean
  zero_minute_steps?: boolean
}
export interface Device {
  id: string
  name: string
  model: string
  adapter: string
  base_url: string
  location: string
  enabled: boolean
  key_configured: boolean
  key_reused?: boolean
  created_at: string
  online: boolean
  last_seen: string | null
  error?: string
  status: DeviceStatus | null
  capabilities: Capabilities | null
}
export interface Display {
  width: number
  height: number
  format: string
  data: string
}
export interface MachineStep {
  name: string
  location: number
  target_f: number
  minutes: number
  drain_minutes: number
}
export interface MachineRecipe {
  id: string
  name: string
  steps: MachineStep[]
}
export interface Ingredient {
  name: string
  kind: string
  amount: number
  unit: string
  use: string
  minutes: number
  alpha?: number
  type?: string
  form?: string
  yield?: number
  color?: number
}
export interface RecipeStep {
  name: string
  kind: string
  target_c: number
  minutes: number
  drain_minutes: number
  location: number
  type?: string
  infuse_liters?: number
  ramp_minutes?: number
}
export interface RecipeProvenance {
  catalog_id: string
  source_id: string
  repository: string
  commit: string
  path: string
  sha256: string
  source_url: string
  license: string
  warnings: string[]
}
export interface Recipe {
  id: string
  name: string
  style: string
  description: string
  batch_liters: number
  efficiency: number
  ingredients: Ingredient[]
  steps: RecipeStep[]
  tags: string[]
  notes: string
  revision: number
  created_at?: string
  updated_at?: string
  type?: string
  boil_liters?: number
  boil_minutes?: number
  water?: Record<string, number>
  fermentation?: { name: string; target_c: number; days: number }[]
  carbonation?: { method?: string; volumes?: number; temperature_c?: number }
  kind?: string
  source_model?: string
  native_program?: MachineRecipe
  provenance?: RecipeProvenance
}
export interface Profile {
  id: string
  name: string
  batch_liters: number
  efficiency: number
  max_steps: number
  max_target_c: number
  max_grain_kg: number
  hop_slots: number
  control_supported: boolean
  notes: string
  sources?: { label: string; url: string; description: string }[]
  user_configured?: boolean
}
export interface Adapted {
  recipe: Recipe
  machine_recipe: MachineRecipe | null
  profile: Profile
  warnings: string[]
  can_deploy: boolean
  source_revision: number
}
export interface Sample {
  elapsed_ms: number
  observed_at?: string
  wort_f: number | null
  thermoblock_f: number | null
  target_f: number | null
  step_seconds: number
  phase: number
  controller_status: number
  compartment: number
  current_step: number
  flags: number
  device_state: number
  source: string
}
export interface Brew {
  id: string
  device_id: string
  device_name: string
  model: string
  recipe_name: string
  recipe_id?: string
  recipe_revision?: number
  state: string
  command: string
  started_at: string | null
  observed_at: string
  duration_ms: number
  device_history_id: number
  gap_count: number
  truncated: boolean
  notes: string
  rating: number
  sample_count: number
  source: string
}
export interface BrewDetail extends Brew {
  samples: Sample[]
}
export interface DeviceSettings {
  temperature_unit: 'F' | 'C'
  language: string
  detailed_status: boolean
}
export interface Telemetry {
  enabled: boolean
  host: string
  port: number
  interval_ms?: number
  path?: string
  status?: { state: string; sent: number; failed: number; dropped: number }
}
export interface AISettings {
  provider: string
  model: string
  key_configured: boolean
  enabled: boolean
  monitor_enabled?: boolean
}
export interface AIModel {
  id: string
  name: string
}
export interface AIModels {
  provider: string
  models: AIModel[]
}
export interface AIResult {
  text: string
  recipe?: Recipe
  limitations: string[]
}
export type Unit = 'F' | 'C'

export interface CatalogSource {
  id: string
  name: string
  repository: string
  upstream: string
  commit: string
  license: string
  attribution: string
  notes: string
  record_count: number
  valid_count: number
  entry_count: number
  excluded_count: number
  excluded_reasons: Record<string, number>
  asset_count: number
  reference_count: number
}
export interface CatalogSummary {
  id: string
  name: string
  kind: string
  source_id: string
  source_model: string
  models: string[]
  deploy_models: string[]
  style: string
  author: string
  description: string
  abv?: number
  ibu?: number
  srm?: number
  image_url?: string
  image_kind?: string
  source_record_count: number
  can_copy: boolean
  can_deploy: boolean
  featured: boolean
  official: boolean
}
export interface CatalogDetail extends CatalogSummary {
  recipe?: Recipe
  native_program?: MachineRecipe
  warnings: string[]
  provenance: RecipeProvenance
  source_records: { id: string; path: string; sha256: string; bytes: number }[]
  source_url: string
  notes: string
  source_parameters?: unknown
}
export interface CatalogPage {
  items: CatalogSummary[]
  total: number
  offset: number
  limit: number
  sources: CatalogSource[]
  styles: string[]
}
export interface CatalogReferences {
  items: {
    id: string
    title: string
    description: string
    models: string[]
    source_id: string
    asset_path: string
    url: string
    content_type: string
    source_url: string
    bundled: boolean
    available: boolean
    download_url?: string
    sha256: string
  }[]
  assets: {
    path: string
    url: string
    title: string
    kind: string
    models: string[]
    source_id: string
    source_path: string
    sha256: string
    bytes: number
    content_type: string
    attribution: string
    license: string
    bundled: boolean
    download_url?: string
  }[]
}
export interface DeploymentItem {
  recipe_id: string
  source_revision: number
  batch_liters: number
  name: string
  status: string
  reason?: string
  update: boolean
  warnings: string[]
  native?: MachineRecipe
  previous?: MachineRecipe
}
export interface BulkDeployment {
  id: string
  device_id: string
  device_uid: string
  profile_id: string
  state: string
  created_at: string
  expires_at: string
  capacity: number
  used: number
  available: number
  planned_count: number
  new_count: number
  update_count: number
  verified_count: number
  started: boolean
  inventory_sha256: string
  items: DeploymentItem[]
}
