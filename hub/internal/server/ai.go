package server

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"net/http"
	"sort"
	"strings"
	"time"
	"unicode"

	"github.com/jeffreydwalter/picobrew-hub/internal/ai"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

type adviceRecord struct {
	ID          string          `json:"id"`
	CreatedAt   time.Time       `json:"created_at"`
	DeviceID    string          `json:"device_id,omitempty"`
	BrewID      string          `json:"brew_id,omitempty"`
	Mode        string          `json:"mode"`
	Status      string          `json:"status"`
	Text        string          `json:"text"`
	Recipe      json.RawMessage `json:"recipe,omitempty"`
	Limitations []string        `json:"limitations,omitempty"`
	Error       string          `json:"error,omitempty"`
}

func invalidProviderKey(key string) bool {
	return len(key) > 1024 || strings.IndexFunc(key, func(r rune) bool {
		return unicode.IsControl(r) || unicode.IsSpace(r)
	}) >= 0
}

func (s *Server) aiSettings(w http.ResponseWriter, r *http.Request) {
	s.configMu.Lock()
	defer s.configMu.Unlock()
	cfg := ai.Config{Provider: "openai"}
	if _, err := s.db.Get("config", "ai", &cfg); err != nil && !errors.Is(err, store.ErrNotFound) {
		s.internal(w, err)
		return
	}
	if r.Method == http.MethodPut {
		var in struct {
			ai.Config
			APIKey string `json:"api_key"`
		}
		if !s.decode(w, r, &in) {
			return
		}
		in.Model = strings.TrimSpace(in.Model)
		if in.Provider != "openai" && in.Provider != "anthropic" {
			s.fail(w, 400, "provider", "Choose OpenAI or Anthropic")
			return
		}
		if len(in.Model) > 512 || invalidProviderKey(in.APIKey) {
			s.fail(w, 400, "config", "Invalid model or API key")
			return
		}
		key := in.APIKey
		if key == "" {
			key, _ = s.readSecret("ai:" + in.Provider)
		}
		in.KeyConfigured = key != ""
		if in.Enabled && (!in.KeyConfigured || in.Model == "") {
			s.fail(w, 400, "config", "Set an API key and model before enabling AI")
			return
		}
		cfg = in.Config
		var encrypted []byte
		if in.APIKey != "" {
			encrypted = s.vault.seal("ai:"+in.Provider, in.APIKey)
		}
		if _, err := s.db.PutWithSecret("config", "ai", cfg, -1, "ai:"+in.Provider, encrypted); err != nil {
			s.internal(w, err)
			return
		}
	}
	key, _ := s.readSecret("ai:" + cfg.Provider)
	cfg.KeyConfigured = key != ""
	s.json(w, 200, cfg)
}

func (s *Server) aiConfiguration() (ai.Config, string, error) {
	s.configMu.Lock()
	defer s.configMu.Unlock()
	var cfg ai.Config
	if _, err := s.db.Get("config", "ai", &cfg); err != nil {
		return cfg, "", errors.New("configure AI in Settings first")
	}
	key, err := s.readSecret("ai:" + cfg.Provider)
	if err != nil || key == "" || cfg.Model == "" || !cfg.Enabled {
		return cfg, "", errors.New("enable AI with a provider API key and model in Settings first")
	}
	return cfg, key, nil
}

func (s *Server) aiContext(input ai.Request) (map[string]any, error) {
	profiles, err := s.calibratedProfiles()
	if err != nil {
		return nil, err
	}
	data := map[string]any{"available_profiles": profiles}
	if input.RecipeID != "" {
		var v domain.Recipe
		if _, err := s.db.Get("recipe", input.RecipeID, &v); err != nil {
			return nil, err
		}
		data["recipe"] = v
	}
	if input.BrewID != "" {
		v, err := s.brewDetail(input.BrewID)
		if err != nil {
			return nil, err
		}
		if len(v.Samples) > 600 {
			stride := (len(v.Samples) + 598) / 599
			reduced := make([]domain.Sample, 0, 600)
			for i := 0; i < len(v.Samples)-1; i += stride {
				reduced = append(reduced, v.Samples[i])
			}
			reduced = append(reduced, v.Samples[len(v.Samples)-1])
			v.Samples = reduced
			data["sampling_note"] = "Time-series context was downsampled; full-fidelity export remains available."
		}
		data["brew"] = v
	}
	if input.DeviceID != "" {
		found := false
		for _, v := range s.fleet() {
			if v.ID == input.DeviceID {
				data["device"] = v
				found = true
				break
			}
		}
		if !found {
			return nil, store.ErrNotFound
		}
	}
	return data, nil
}

func (s *Server) performAI(ctx context.Context, cfg ai.Config, key string, input ai.Request, data map[string]any) (ai.Result, error) {
	result, err := s.aiClient.Assist(ctx, cfg, key, input, data)
	if err != nil {
		return result, err
	}
	if len(result.Recipe) > 0 {
		var draft domain.Recipe
		if err = json.Unmarshal(result.Recipe, &draft); err != nil {
			return ai.Result{}, errors.New("generated recipe is not valid JSON")
		}
		if err = recipe.Validate(draft); err != nil {
			return ai.Result{}, fmt.Errorf("generated recipe failed validation: %w", err)
		}
	}
	return result, nil
}

func (s *Server) recordAdvice(input ai.Request, result ai.Result, err error) {
	record := adviceRecord{ID: randomID(), CreatedAt: time.Now().UTC(), DeviceID: input.DeviceID, BrewID: input.BrewID, Mode: input.Mode, Status: "complete", Text: result.Text, Recipe: result.Recipe, Limitations: result.Limitations}
	if err != nil {
		record.Status = "error"
		record.Error = safeMessage(err.Error())
	}
	if _, e := s.db.Put("ai-advice", record.ID, record, 0); e != nil {
		s.cfg.Logger.Error("Could not save AI advice", "error", e)
		return
	}
	// Provider outputs can be large; retain a bounded recent advisory log.
	rows, e := s.db.List("ai-advice")
	if e != nil || len(rows) <= 500 {
		return
	}
	records := make([]adviceRecord, 0, len(rows))
	for _, raw := range rows {
		var v adviceRecord
		if json.Unmarshal(raw, &v) == nil {
			records = append(records, v)
		}
	}
	sort.Slice(records, func(i, j int) bool { return records[i].CreatedAt.After(records[j].CreatedAt) })
	for _, v := range records[500:] {
		_ = s.db.Delete("ai-advice", v.ID)
	}
}

func (s *Server) aiAssist(w http.ResponseWriter, r *http.Request) {
	var input ai.Request
	if !s.decode(w, r, &input) {
		return
	}
	if input.Mode != "recipe" && input.Mode != "live" && input.Mode != "analysis" {
		s.fail(w, 400, "mode", "Choose recipe, live, or analysis")
		return
	}
	if input.Mode == "live" && input.DeviceID == "" {
		s.fail(w, 400, "device", "Choose a machine for live advice")
		return
	}
	if input.Mode == "analysis" && input.BrewID == "" {
		s.fail(w, 400, "brew", "Choose a recorded brew to analyze")
		return
	}
	if len(input.Prompt) > 16000 {
		s.fail(w, 400, "prompt", "Prompts must not exceed 16000 characters")
		return
	}
	cfg, key, err := s.aiConfiguration()
	if err != nil {
		s.fail(w, 409, "ai_not_configured", err.Error())
		return
	}
	if input.Mode == "live" {
		input.BrewID = s.activeBrewID(r.Context(), input.DeviceID)
	}
	data, err := s.aiContext(input)
	if err != nil {
		s.storeError(w, err)
		return
	}
	if !s.aiMu.TryLock() {
		s.fail(w, 429, "ai_busy", "An AI request is already in progress. Try again after it finishes.")
		return
	}
	defer s.aiMu.Unlock()
	result, err := s.performAI(r.Context(), cfg, key, input, data)
	s.recordAdvice(input, result, err)
	if err != nil {
		s.fail(w, 502, "ai_provider", err.Error())
		return
	}
	s.json(w, 200, result)
}

func (s *Server) aiAdvice(w http.ResponseWriter, r *http.Request) {
	rows, err := s.db.List("ai-advice")
	if err != nil {
		s.internal(w, err)
		return
	}
	out := make([]adviceRecord, 0, len(rows))
	for _, raw := range rows {
		var v adviceRecord
		if json.Unmarshal(raw, &v) == nil && (r.URL.Query().Get("device_id") == "" || r.URL.Query().Get("device_id") == v.DeviceID) {
			out = append(out, v)
		}
	}
	sort.Slice(out, func(i, j int) bool { return out[i].CreatedAt.After(out[j].CreatedAt) })
	s.json(w, 200, out)
}

func (s *Server) monitorAI(ctx context.Context) {
	ticker := time.NewTicker(30 * time.Second)
	defer ticker.Stop()
	last := map[string]time.Time{}
	for {
		select {
		case <-ctx.Done():
			return
		case <-ticker.C:
			s.monitorTick(ctx, last)
		}
	}
}

func (s *Server) monitorTick(ctx context.Context, last map[string]time.Time) {
	cfg, key, err := s.aiConfiguration()
	if err != nil || !cfg.MonitorEnabled {
		return
	}
	for _, v := range s.fleet() {
		if ctx.Err() != nil {
			return
		}
		if !v.Enabled || !v.Online || v.Status == nil || !v.Status.Busy || time.Since(last[v.ID]) < 5*time.Minute {
			continue
		}
		if !s.aiMu.TryLock() {
			return
		}
		last[v.ID] = time.Now()
		input := ai.Request{Mode: "live", DeviceID: v.ID, Prompt: "Review the active brew's recent sensor readings and phases. Identify meaningful temperature deviations, telemetry gaps or stalled progress. Give concise observations and actions the brewer can review. If data is insufficient, say so. Do not issue or imply machine commands."}
		input.BrewID = s.activeBrewID(ctx, v.ID)
		data, e := s.aiContext(input)
		var result ai.Result
		if e == nil {
			result, e = s.performAI(ctx, cfg, key, input, data)
		}
		s.recordAdvice(input, result, e)
		s.aiMu.Unlock()
	}
}

// Read the currently selected native history ID. A cached "recording" flag can
// belong to a previous brew between imports and must not be used as live context.
func (s *Server) activeBrewID(ctx context.Context, deviceID string) string {
	s.mu.RLock()
	runtime := s.devices[deviceID]
	s.mu.RUnlock()
	if runtime == nil || runtime.adapter == nil {
		return ""
	}
	history, err := runtime.adapter.History(ctx)
	if err != nil || !history.Available || history.State != "recording" {
		return ""
	}
	id := fmt.Sprintf("%s-%d", deviceID, history.ID)
	var brew domain.Brew
	if _, err = s.db.Get("brew", id, &brew); err != nil || brew.DeviceHistoryID != history.ID {
		return ""
	}
	return id
}
