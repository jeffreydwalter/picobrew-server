package domain

import "time"

type Device struct {
	ID            string    `json:"id"`
	Name          string    `json:"name"`
	Model         string    `json:"model"`
	Adapter       string    `json:"adapter"`
	BaseURL       string    `json:"base_url"`
	Location      string    `json:"location"`
	Enabled       bool      `json:"enabled"`
	KeyConfigured bool      `json:"key_configured"`
	CreatedAt     time.Time `json:"created_at"`
}
type Status struct {
	UptimeMS         uint64   `json:"uptime_ms"`
	Ready            bool     `json:"ready"`
	DeviceUID        string   `json:"device_uid"`
	Busy             bool     `json:"busy"`
	Stopped          bool     `json:"stopped"`
	Command          string   `json:"command"`
	DeviceState      int      `json:"device_state"`
	Phase            int      `json:"phase"`
	ControllerStatus int      `json:"controller_status"`
	Compartment      int      `json:"compartment"`
	WortF            *float64 `json:"wort_f"`
	ThermoblockF     *float64 `json:"thermoblock_f"`
	TargetF          *float64 `json:"target_f"`
	StepSeconds      int      `json:"step_seconds"`
	CurrentStep      int      `json:"current_step"`
	StepCount        int      `json:"step_count"`
	RecipeName       string   `json:"recipe_name"`
	StepName         string   `json:"step_name"`
	SessionID        string   `json:"session_id"`
	FilterInserted   *bool    `json:"filter_inserted"`
	StopReason       string   `json:"stop_reason"`
	Encoder          struct {
		Position int `json:"position"`
		Min      int `json:"min"`
		Max      int `json:"max"`
	} `json:"encoder"`
}
type Capabilities struct {
	APIVersion            int      `json:"api_version"`
	FirmwareVersion       string   `json:"firmware_version"`
	Commands              []string `json:"commands"`
	RecipeCapacity        int      `json:"recipe_capacity"`
	StepCapacity          int      `json:"step_capacity"`
	FrontPanel            bool     `json:"front_panel"`
	HistoryPersistent     bool     `json:"history_persistent"`
	TelemetryForwarding   bool     `json:"telemetry_forwarding"`
	DedicatedPauseResume  bool     `json:"dedicated_pause_resume"`
	ZeroMinuteSteps       bool     `json:"zero_minute_steps"`
	RestoreDefaultRecipes bool     `json:"restore_default_recipes"`
	StarterRecipeCount    int      `json:"starter_recipe_count"`
}
type DeviceView struct {
	Device
	Online       bool          `json:"online"`
	KeyReused    bool          `json:"key_reused"`
	LastSeen     *time.Time    `json:"last_seen"`
	Error        string        `json:"error,omitempty"`
	Status       *Status       `json:"status"`
	Capabilities *Capabilities `json:"capabilities"`
}
type Display struct {
	Width  int    `json:"width"`
	Height int    `json:"height"`
	Format string `json:"format"`
	Data   string `json:"data"`
}
type Command struct {
	Command  string `json:"command"`
	RecipeID string `json:"recipe_id,omitempty"`
	Delta    int    `json:"delta,omitempty"`
	Click    bool   `json:"click,omitempty"`
}
type MachineStep struct {
	Name         string `json:"name"`
	Location     int    `json:"location"`
	TargetF      int    `json:"target_f"`
	Minutes      int    `json:"minutes"`
	DrainMinutes int    `json:"drain_minutes"`
}
type MachineRecipe struct {
	ID    string        `json:"id"`
	Name  string        `json:"name"`
	Steps []MachineStep `json:"steps"`
}
type Ingredient struct {
	Name    string  `json:"name"`
	Kind    string  `json:"kind"`
	Amount  float64 `json:"amount"`
	Unit    string  `json:"unit"`
	Use     string  `json:"use"`
	Minutes float64 `json:"minutes"`
	Alpha   float64 `json:"alpha,omitempty"`
	Type    string  `json:"type,omitempty"`
	Form    string  `json:"form,omitempty"`
	Yield   float64 `json:"yield,omitempty"`
	Color   float64 `json:"color,omitempty"`
}
type RecipeStep struct {
	Name         string  `json:"name"`
	Kind         string  `json:"kind"`
	TargetC      float64 `json:"target_c"`
	Minutes      int     `json:"minutes"`
	DrainMinutes int     `json:"drain_minutes"`
	Location     int     `json:"location"`
	Type         string  `json:"type,omitempty"`
	InfuseLiters float64 `json:"infuse_liters,omitempty"`
	RampMinutes  float64 `json:"ramp_minutes,omitempty"`
}
type FermentationStep struct {
	Name    string  `json:"name"`
	TargetC float64 `json:"target_c"`
	Days    float64 `json:"days"`
}
type Carbonation struct {
	Method       string  `json:"method,omitempty"`
	Volumes      float64 `json:"volumes,omitempty"`
	TemperatureC float64 `json:"temperature_c,omitempty"`
}

// RecipeProvenance identifies the immutable original record from which an
// editable library copy was made. Warnings describe incomplete conversions.
type RecipeProvenance struct {
	CatalogID  string   `json:"catalog_id"`
	SourceID   string   `json:"source_id"`
	Repository string   `json:"repository"`
	Commit     string   `json:"commit"`
	Path       string   `json:"path"`
	SHA256     string   `json:"sha256"`
	SourceURL  string   `json:"source_url"`
	License    string   `json:"license"`
	Warnings   []string `json:"warnings"`
}
type Recipe struct {
	ID            string             `json:"id"`
	Name          string             `json:"name"`
	Style         string             `json:"style"`
	Description   string             `json:"description"`
	BatchLiters   float64            `json:"batch_liters"`
	Efficiency    float64            `json:"efficiency"`
	Ingredients   []Ingredient       `json:"ingredients"`
	Steps         []RecipeStep       `json:"steps"`
	Tags          []string           `json:"tags"`
	Notes         string             `json:"notes"`
	Revision      int64              `json:"revision"`
	CreatedAt     time.Time          `json:"created_at"`
	UpdatedAt     time.Time          `json:"updated_at"`
	Type          string             `json:"type,omitempty"`
	BoilLiters    float64            `json:"boil_liters,omitempty"`
	BoilMinutes   float64            `json:"boil_minutes,omitempty"`
	Water         map[string]float64 `json:"water,omitempty"`
	Fermentation  []FermentationStep `json:"fermentation,omitempty"`
	Carbonation   Carbonation        `json:"carbonation,omitempty"`
	Kind          string             `json:"kind,omitempty"`
	SourceModel   string             `json:"source_model,omitempty"`
	NativeProgram *MachineRecipe     `json:"native_program,omitempty"`
	Provenance    *RecipeProvenance  `json:"provenance,omitempty"`
}
type Profile struct {
	ID               string          `json:"id"`
	Name             string          `json:"name"`
	BatchLiters      float64         `json:"batch_liters"`
	Efficiency       float64         `json:"efficiency"`
	MaxSteps         int             `json:"max_steps"`
	MaxTargetC       float64         `json:"max_target_c"`
	MaxGrainKG       float64         `json:"max_grain_kg"`
	HopSlots         int             `json:"hop_slots"`
	ControlSupported bool            `json:"control_supported"`
	Notes            string          `json:"notes"`
	Sources          []ProfileSource `json:"sources,omitempty"`
	UserConfigured   bool            `json:"user_configured"`
}
type ProfileSource struct {
	Label       string `json:"label"`
	URL         string `json:"url"`
	Description string `json:"description"`
}
type Adapted struct {
	Recipe         Recipe         `json:"recipe"`
	MachineRecipe  *MachineRecipe `json:"machine_recipe"`
	Profile        Profile        `json:"profile"`
	Warnings       []string       `json:"warnings"`
	CanDeploy      bool           `json:"can_deploy"`
	SourceRevision int64          `json:"source_revision"`
}
type Sample struct {
	ElapsedMS        int64      `json:"elapsed_ms"`
	ObservedAt       *time.Time `json:"observed_at,omitempty"`
	WortF            *float64   `json:"wort_f"`
	ThermoblockF     *float64   `json:"thermoblock_f"`
	TargetF          *float64   `json:"target_f"`
	StepSeconds      int        `json:"step_seconds"`
	Phase            int        `json:"phase"`
	ControllerStatus int        `json:"controller_status"`
	Compartment      int        `json:"compartment"`
	CurrentStep      int        `json:"current_step"`
	Flags            int        `json:"flags"`
	DeviceState      int        `json:"device_state"`
	Source           string     `json:"source"`
}
type HistorySession struct {
	Available             bool   `json:"available"`
	ID                    uint64 `json:"id"`
	Command               string `json:"command"`
	RecipeName            string `json:"recipe_name"`
	State                 string `json:"state"`
	DurationMS            int64  `json:"duration_ms"`
	RecordCount           int    `json:"record_count"`
	PersistedRecordCount  int    `json:"persisted_record_count"`
	FirstElapsedMS        int64  `json:"first_elapsed_ms"`
	GapCount              int    `json:"gap_count"`
	Truncated             bool   `json:"truncated"`
	StorageError          bool   `json:"storage_error"`
	RecoveredAfterRestart bool   `json:"recovered_after_restart"`
}
type History struct {
	HistorySession
	Sessions []HistorySession `json:"sessions"`
	Fields   []string         `json:"fields"`
}
type Brew struct {
	ID              string     `json:"id"`
	DeviceID        string     `json:"device_id"`
	DeviceName      string     `json:"device_name"`
	Model           string     `json:"model"`
	RecipeName      string     `json:"recipe_name"`
	RecipeID        string     `json:"recipe_id,omitempty"`
	RecipeRevision  int64      `json:"recipe_revision,omitempty"`
	State           string     `json:"state"`
	Command         string     `json:"command"`
	StartedAt       *time.Time `json:"started_at"`
	ObservedAt      time.Time  `json:"observed_at"`
	DurationMS      int64      `json:"duration_ms"`
	DeviceHistoryID uint64     `json:"device_history_id"`
	GapCount        int        `json:"gap_count"`
	Truncated       bool       `json:"truncated"`
	Notes           string     `json:"notes"`
	Rating          int        `json:"rating"`
	SampleCount     int        `json:"sample_count"`
	Source          string     `json:"source"`
}
type BrewDetail struct {
	Brew
	Samples []Sample `json:"samples"`
}
