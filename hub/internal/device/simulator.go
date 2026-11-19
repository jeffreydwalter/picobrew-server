package device

import (
	"context"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"math"
	"sort"
	"strings"
	"sync"
	"sync/atomic"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

type simulatedBrew struct {
	meta    domain.HistorySession
	recipe  domain.MachineRecipe
	started time.Time
	samples []domain.Sample
}
type simulator struct {
	mu                  sync.Mutex
	id, model           string
	boot                time.Time
	status              domain.Status
	recipes             map[string]domain.MachineRecipe
	settings, telemetry map[string]any
	brews               []*simulatedBrew
	active              *simulatedBrew
	pending             *domain.MachineRecipe
	programStarted      time.Time
}

var simulationIDs atomic.Uint64

func init() { simulationIDs.Store(uint64(time.Now().UnixMilli()) & 0xffffffff) }

// NewSimulator creates an isolated, explicitly synthetic machine. It performs no
// network or hardware operations, including when the chosen model is Z or Pico C.
// Time progresses at sixty simulated seconds per real second for demonstration.
func NewSimulator(id, model string) Adapter {
	now := time.Now()
	s := &simulator{id: id, model: model, boot: now, recipes: make(map[string]domain.MachineRecipe), settings: map[string]any{"temperature_unit": "F", "language": "en", "detailed_status": false}, telemetry: map[string]any{"enabled": false, "host": "", "port": 8080}}
	s.status = domain.Status{Ready: true, DeviceUID: "simulator:" + id, Command: "idle", WortF: floatValue(72), ThermoblockF: floatValue(76), TargetF: floatValue(0), FilterInserted: boolValue(true), StopReason: "none"}
	s.status.Encoder.Max = 3
	porter := domain.MachineRecipe{ID: "studio-porter", Name: "Studio Porter", Steps: []domain.MachineStep{{Name: "Mash", Location: 1, TargetF: 152, Minutes: 60, DrainMinutes: 2}, {Name: "Boil", Location: 2, TargetF: 210, Minutes: 60, DrainMinutes: 2}}}
	pale := domain.MachineRecipe{ID: "weekend-pale", Name: "Weekend Pale", Steps: []domain.MachineStep{{Name: "Mash", Location: 1, TargetF: 149, Minutes: 60}, {Name: "Boil", Location: 2, TargetF: 210, Minutes: 60}}}
	s.recipes[porter.ID] = porter
	s.recipes[pale.ID] = pale
	if id == "demo-pro" {
		s.begin(porter, "brew", now.Add(-25*time.Second))
	}
	if id == "demo-z" {
		s.begin(pale, "brew", now.Add(-80*time.Second))
	}
	s.advance(now)
	return s
}

func floatValue(v float64) *float64 { return &v }
func boolValue(v bool) *bool        { return &v }
func cloneRecipe(v domain.MachineRecipe) domain.MachineRecipe {
	v.Steps = append([]domain.MachineStep(nil), v.Steps...)
	return v
}
func cloneMap(v map[string]any) map[string]any {
	data, _ := json.Marshal(v)
	var result map[string]any
	_ = json.Unmarshal(data, &result)
	return result
}
func cloneStatus(v domain.Status) domain.Status {
	if v.WortF != nil {
		v.WortF = floatValue(*v.WortF)
	}
	if v.ThermoblockF != nil {
		v.ThermoblockF = floatValue(*v.ThermoblockF)
	}
	if v.TargetF != nil {
		v.TargetF = floatValue(*v.TargetF)
	}
	if v.FilterInserted != nil {
		v.FilterInserted = boolValue(*v.FilterInserted)
	}
	return v
}
func cloneSamples(values []domain.Sample) []domain.Sample {
	out := append([]domain.Sample(nil), values...)
	for i := range out {
		if out[i].WortF != nil {
			out[i].WortF = floatValue(*out[i].WortF)
		}
		if out[i].ThermoblockF != nil {
			out[i].ThermoblockF = floatValue(*out[i].ThermoblockF)
		}
		if out[i].TargetF != nil {
			out[i].TargetF = floatValue(*out[i].TargetF)
		}
	}
	return out
}
func simulationContext(ctx context.Context) error {
	if ctx.Err() != nil {
		return &Error{Code: "cancelled", Message: "Simulator request was cancelled"}
	}
	return nil
}
func busyError() error {
	return &Error{StatusCode: 409, Code: "busy", Message: "Return to the idle main menu before changing this setting"}
}

func (s *simulator) begin(recipe domain.MachineRecipe, command string, started time.Time) {
	id := simulationIDs.Add(1) & 0xffffffff
	if id == 0 {
		id = simulationIDs.Add(1) & 0xffffffff
	}
	brew := &simulatedBrew{meta: domain.HistorySession{Available: true, ID: id, Command: command, RecipeName: recipe.Name, State: "recording"}, recipe: cloneRecipe(recipe), started: started}
	s.brews = append([]*simulatedBrew{brew}, s.brews...)
	if len(s.brews) > 8 {
		s.brews = s.brews[:8]
	}
	s.active = brew
	s.pending = nil
	s.status.Busy = true
	s.status.Command = command
	s.status.SessionID = fmt.Sprintf("simulation-%d", id)
	s.status.RecipeName = recipe.Name
	s.status.StepCount = len(recipe.Steps)
	s.status.DeviceState = 1
}
func duration(recipe domain.MachineRecipe) int64 {
	var result int64
	for _, step := range recipe.Steps {
		result += int64(step.Minutes+step.DrainMinutes) * 60000
	}
	return result
}
func sampleAt(brew *simulatedBrew, elapsed int64) domain.Sample {
	index := 0
	start := int64(0)
	for index < len(brew.recipe.Steps)-1 && elapsed >= start+int64(brew.recipe.Steps[index].Minutes+brew.recipe.Steps[index].DrainMinutes)*60000 {
		start += int64(brew.recipe.Steps[index].Minutes+brew.recipe.Steps[index].DrainMinutes) * 60000
		index++
	}
	step := brew.recipe.Steps[index]
	target := float64(step.TargetF)
	base := 72.0
	if index > 0 {
		base = float64(brew.recipe.Steps[index-1].TargetF)
		if base == 0 {
			base = 72
		}
	}
	wort := base
	if target != 0 {
		wort = base + (target-base)*math.Min(float64(elapsed-start)/720000, 1) + math.Sin(float64(elapsed)/180000)*0.4
	}
	return domain.Sample{ElapsedMS: elapsed, WortF: floatValue(math.Round(wort*10) / 10), ThermoblockF: floatValue(math.Round((wort+7)*10) / 10), TargetF: floatValue(target), StepSeconds: int((elapsed - start) / 1000), Phase: index + 1, ControllerStatus: 1, Compartment: step.Location, CurrentStep: index, Flags: 11, DeviceState: 1, Source: "simulation"}
}
func (s *simulator) advance(now time.Time) {
	s.status.UptimeMS = uint64(now.Sub(s.boot).Milliseconds()) & 0xffffffff
	if s.active == nil {
		if s.status.Busy && !s.programStarted.IsZero() && now.Sub(s.programStarted) > 12*time.Second {
			s.status.Busy = false
			s.status.Command = "idle"
			s.status.StepName = ""
			s.programStarted = time.Time{}
		}
		return
	}
	brew := s.active
	elapsed := int64(now.Sub(brew.started).Milliseconds()) * 60
	if elapsed < 0 {
		elapsed = 0
	}
	total := duration(brew.recipe)
	elapsed = min(elapsed, total)
	for next := int64(len(brew.samples)) * 15000; next <= elapsed && len(brew.samples) < 4224; next += 15000 {
		if !s.makeHistoryRoom() {
			brew.meta.Truncated = true
			break
		}
		brew.samples = append(brew.samples, sampleAt(brew, next))
	}
	if len(brew.samples) == 4224 && elapsed > brew.samples[len(brew.samples)-1].ElapsedMS {
		brew.meta.Truncated = true
	}
	brew.meta.DurationMS = elapsed
	brew.meta.RecordCount = len(brew.samples)
	row := sampleAt(brew, elapsed)
	s.status.WortF = row.WortF
	s.status.ThermoblockF = row.ThermoblockF
	s.status.TargetF = row.TargetF
	s.status.StepSeconds = row.StepSeconds
	s.status.Phase = row.Phase
	s.status.ControllerStatus = row.ControllerStatus
	s.status.Compartment = row.Compartment
	s.status.CurrentStep = row.CurrentStep
	s.status.StepName = brew.recipe.Steps[row.CurrentStep].Name
	if elapsed >= total {
		brew.meta.State = "completed"
		s.active = nil
		s.status.Busy = false
		s.status.Command = "idle"
		s.status.TargetF = floatValue(0)
		s.status.DeviceState = 0
	}
}

// Match the physical recorder's bounded shared capacity, evicting complete old
// brews before discarding any part of the active brew's beginning.
func (s *simulator) makeHistoryRoom() bool {
	for {
		total := 0
		for _, brew := range s.brews {
			total += len(brew.samples)
		}
		if total < 4224 {
			return true
		}
		if len(s.brews) < 2 || s.brews[len(s.brews)-1] == s.active {
			return false
		}
		s.brews = s.brews[:len(s.brews)-1]
	}
}

func (s *simulator) Discover(ctx context.Context) (domain.Capabilities, error) {
	if err := simulationContext(ctx); err != nil {
		return domain.Capabilities{}, err
	}
	return domain.Capabilities{APIVersion: 1, FirmwareVersion: "simulator", Commands: append([]string(nil), commands...), RecipeCapacity: 16, StepCapacity: 15, FrontPanel: true, HistoryPersistent: false, TelemetryForwarding: false, DedicatedPauseResume: false, ZeroMinuteSteps: true}, nil
}
func (s *simulator) Status(ctx context.Context) (domain.Status, error) {
	if err := simulationContext(ctx); err != nil {
		return domain.Status{}, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.advance(time.Now())
	return cloneStatus(s.status), nil
}
func (s *simulator) Recipes(ctx context.Context) ([]domain.MachineRecipe, error) {
	if err := simulationContext(ctx); err != nil {
		return nil, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	result := make([]domain.MachineRecipe, 0, len(s.recipes))
	for _, recipe := range s.recipes {
		result = append(result, cloneRecipe(recipe))
	}
	sort.Slice(result, func(i, j int) bool { return result[i].ID < result[j].ID })
	return result, nil
}
func (s *simulator) PutRecipe(ctx context.Context, recipe domain.MachineRecipe) error {
	if err := simulationContext(ctx); err != nil {
		return err
	}
	if err := validateRecipe(recipe); err != nil {
		return err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.advance(time.Now())
	if s.status.Busy {
		return busyError()
	}
	if _, ok := s.recipes[recipe.ID]; !ok && len(s.recipes) >= 16 {
		return &Error{StatusCode: 507, Code: "recipe_capacity", Message: "Simulator recipe capacity is full"}
	}
	s.recipes[recipe.ID] = cloneRecipe(recipe)
	return nil
}
func (s *simulator) DeleteRecipe(ctx context.Context, id string) error {
	if err := simulationContext(ctx); err != nil {
		return err
	}
	if !recipeID.MatchString(id) {
		return invalid("Recipe ID is invalid")
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.advance(time.Now())
	if s.status.Busy {
		return busyError()
	}
	if _, exists := s.recipes[id]; !exists {
		return &Error{StatusCode: 404, Code: "recipe_not_found", Message: "Recipe is not stored on this simulator"}
	}
	delete(s.recipes, id)
	return nil
}
func (s *simulator) Command(ctx context.Context, command domain.Command) error {
	if err := simulationContext(ctx); err != nil {
		return err
	}
	if err := validateCommand(command); err != nil {
		return err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	now := time.Now()
	s.advance(now)
	switch command.Command {
	case "stop":
		if s.active != nil {
			s.active.meta.State = "interrupted"
			s.active = nil
		}
		s.pending = nil
		s.status.Busy = false
		s.status.Stopped = true
		s.status.StopReason = "requested"
		s.status.Command = "idle"
		s.status.TargetF = floatValue(0)
		s.status.DeviceState = 0
		return nil
	case "clear-stop":
		if s.status.Busy {
			return busyError()
		}
		s.status.Stopped = false
		s.status.StopReason = "none"
		return nil
	case "encoder":
		s.status.Encoder.Position = max(s.status.Encoder.Min, min(s.status.Encoder.Max, s.status.Encoder.Position+command.Delta))
		if command.Click {
			if s.pending != nil {
				recipe := cloneRecipe(*s.pending)
				kind := s.status.Command
				s.begin(recipe, kind, now)
				s.advance(now)
			} else if s.status.Command == "utilities" || s.status.Command == "settings" {
				s.status.Busy = false
				s.status.Command = "idle"
				s.status.StepName = ""
			} else if s.status.Busy && s.active == nil {
				s.programStarted = now
				s.status.StepName = "Simulated program"
			}
		}
		return nil
	}
	if s.status.Stopped {
		return &Error{StatusCode: 409, Code: "stopped", Message: "Clear the latched stop before starting a program"}
	}
	if s.status.Busy {
		return busyError()
	}
	if command.Command == "brew" {
		recipe, ok := s.recipes[command.RecipeID]
		if !ok {
			return &Error{StatusCode: 404, Code: "recipe_not_found", Message: "Recipe is not stored on this simulator"}
		}
		copy := cloneRecipe(recipe)
		s.pending = &copy
		s.status.RecipeName = recipe.Name
		s.status.StepCount = len(recipe.Steps)
	} else if command.Command == "manual" {
		recipe := domain.MachineRecipe{ID: "manual", Name: "Manual brew", Steps: []domain.MachineStep{{Name: "Manual", Location: 1, TargetF: 152, Minutes: 30}}}
		s.pending = &recipe
		s.status.RecipeName = recipe.Name
	} else {
		s.status.RecipeName = ""
		s.pending = nil
	}
	s.status.Busy = true
	s.status.Command = command.Command
	s.status.StepName = "Press dial to begin"
	s.status.TargetF = floatValue(0)
	s.programStarted = time.Time{}
	return nil
}
func (s *simulator) Settings(ctx context.Context) (map[string]any, error) {
	if err := simulationContext(ctx); err != nil {
		return nil, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	return cloneMap(s.settings), nil
}
func (s *simulator) UpdateSettings(ctx context.Context, value map[string]any) error {
	if err := simulationContext(ctx); err != nil {
		return err
	}
	if err := validateSettings(value); err != nil {
		return err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.advance(time.Now())
	if s.status.Busy {
		return busyError()
	}
	for key, v := range value {
		s.settings[key] = v
	}
	return nil
}
func (s *simulator) Telemetry(ctx context.Context) (map[string]any, error) {
	if err := simulationContext(ctx); err != nil {
		return nil, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	result := cloneMap(s.telemetry)
	result["interval_ms"] = 15000
	result["path"] = "/api/v1/telemetry"
	result["mode"] = "simulator"
	result["status"] = map[string]any{"state": "disabled", "resolved": false, "sent": 0, "failed": 0, "dropped": 0, "last_success_ms": 0, "last_http_status": 0, "storage_error": false, "last_error": "none"}
	return result, nil
}
func (s *simulator) UpdateTelemetry(ctx context.Context, value map[string]any) error {
	if err := simulationContext(ctx); err != nil {
		return err
	}
	if err := validateTelemetry(value); err != nil {
		return err
	}
	if value["enabled"] == true {
		return &Error{StatusCode: 400, Code: "simulator_only", Message: "The simulator does not send network telemetry"}
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.advance(time.Now())
	if s.status.Busy {
		return busyError()
	}
	s.telemetry = cloneMap(value)
	return nil
}
func (s *simulator) History(ctx context.Context) (domain.History, error) {
	if err := simulationContext(ctx); err != nil {
		return domain.History{}, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.advance(time.Now())
	result := domain.History{Fields: append([]string(nil), historyFields...), Sessions: []domain.HistorySession{}}
	result.State = "empty"
	for _, brew := range s.brews {
		result.Sessions = append(result.Sessions, brew.meta)
	}
	if len(result.Sessions) > 0 {
		result.HistorySession = result.Sessions[0]
	}
	return result, nil
}
func (s *simulator) HistoryPage(ctx context.Context, id uint64, offset int) ([]domain.Sample, *int, error) {
	if err := simulationContext(ctx); err != nil {
		return nil, nil, err
	}
	s.mu.Lock()
	defer s.mu.Unlock()
	s.advance(time.Now())
	for _, brew := range s.brews {
		if brew.meta.ID == id {
			if offset < 0 || offset > len(brew.samples) {
				return nil, nil, invalid("History offset exceeds the retained recording")
			}
			end := min(offset+32, len(brew.samples))
			var next *int
			if end < len(brew.samples) {
				value := end
				next = &value
			}
			return cloneSamples(brew.samples[offset:end]), next, nil
		}
	}
	return nil, nil, &Error{StatusCode: 409, Code: "history_changed", Message: "This simulated brew is no longer retained"}
}

func (s *simulator) Display(ctx context.Context) (domain.Display, error) {
	status, err := s.Status(ctx)
	if err != nil {
		return domain.Display{}, err
	}
	pixels := make([]byte, 1024)
	draw := func(x, y int) {
		if x >= 0 && x < 128 && y >= 0 && y < 64 {
			rx, ry := 127-x, 63-y
			pixels[rx+(ry/8)*128] |= 1 << uint(ry&7)
		}
	}
	text := func(value string, y int) {
		for i, ch := range strings.ToUpper(value) {
			if i >= 21 {
				break
			}
			glyph := simFont[ch]
			for x, bits := range glyph {
				for row := range 7 {
					if bits&(1<<uint(row)) != 0 {
						draw(i*6+x, y+row)
					}
				}
			}
		}
	}
	text("SIMULATOR", 0)
	label := status.Command
	if status.Stopped {
		label = "STOPPED"
	}
	text(label, 14)
	text(status.StepName, 28)
	if status.WortF != nil {
		text(fmt.Sprintf("%.1f F", *status.WortF), 42)
	}
	for x := range 128 {
		draw(x, 61)
	}
	return domain.Display{Width: 128, Height: 64, Format: "page-lsb-hex", Data: hex.EncodeToString(pixels)}, nil
}

var simFont = map[rune][5]byte{
	'A': {0x7e, 0x11, 0x11, 0x11, 0x7e}, 'B': {0x7f, 0x49, 0x49, 0x49, 0x36}, 'C': {0x3e, 0x41, 0x41, 0x41, 0x22}, 'D': {0x7f, 0x41, 0x41, 0x22, 0x1c}, 'E': {0x7f, 0x49, 0x49, 0x49, 0x41}, 'F': {0x7f, 0x09, 0x09, 0x09, 0x01}, 'G': {0x3e, 0x41, 0x49, 0x49, 0x7a}, 'H': {0x7f, 0x08, 0x08, 0x08, 0x7f}, 'I': {0, 0x41, 0x7f, 0x41, 0}, 'J': {0x20, 0x40, 0x41, 0x3f, 0x01}, 'K': {0x7f, 0x08, 0x14, 0x22, 0x41}, 'L': {0x7f, 0x40, 0x40, 0x40, 0x40}, 'M': {0x7f, 0x02, 0x04, 0x02, 0x7f}, 'N': {0x7f, 0x04, 0x08, 0x10, 0x7f}, 'O': {0x3e, 0x41, 0x41, 0x41, 0x3e}, 'P': {0x7f, 0x09, 0x09, 0x09, 0x06}, 'Q': {0x3e, 0x41, 0x51, 0x21, 0x5e}, 'R': {0x7f, 0x09, 0x19, 0x29, 0x46}, 'S': {0x46, 0x49, 0x49, 0x49, 0x31}, 'T': {0x01, 0x01, 0x7f, 0x01, 0x01}, 'U': {0x3f, 0x40, 0x40, 0x40, 0x3f}, 'V': {0x1f, 0x20, 0x40, 0x20, 0x1f}, 'W': {0x7f, 0x20, 0x18, 0x20, 0x7f}, 'X': {0x63, 0x14, 0x08, 0x14, 0x63}, 'Y': {0x07, 0x08, 0x70, 0x08, 0x07}, 'Z': {0x61, 0x51, 0x49, 0x45, 0x43}, '0': {0x3e, 0x51, 0x49, 0x45, 0x3e}, '1': {0, 0x42, 0x7f, 0x40, 0}, '2': {0x42, 0x61, 0x51, 0x49, 0x46}, '3': {0x21, 0x41, 0x45, 0x4b, 0x31}, '4': {0x18, 0x14, 0x12, 0x7f, 0x10}, '5': {0x27, 0x45, 0x45, 0x45, 0x39}, '6': {0x3c, 0x4a, 0x49, 0x49, 0x30}, '7': {1, 0x71, 9, 5, 3}, '8': {0x36, 0x49, 0x49, 0x49, 0x36}, '9': {6, 0x49, 0x49, 0x29, 0x1e}, '.': {0, 0x60, 0x60, 0, 0}, '-': {8, 8, 8, 8, 8},
}
