package server

import (
	"math"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

func (s *Server) seedDemo() error {
	existing, e := s.db.List("device")
	if e != nil {
		return e
	}
	if len(existing) > 0 {
		return nil
	}
	now := time.Now().UTC()
	devices := []domain.Device{{ID: "demo-pro", Name: "Pico Pro · Studio", Model: "pico-pro", Adapter: "simulator", Location: "Brew studio", Enabled: true, CreatedAt: now}, {ID: "demo-s", Name: "Pico S · Test bench", Model: "pico-s", Adapter: "simulator", Location: "Test bench", Enabled: true, CreatedAt: now}, {ID: "demo-z", Name: "Z · Pilot batch", Model: "z", Adapter: "simulator", Location: "Pilot room", Enabled: true, CreatedAt: now}}
	for _, d := range devices {
		if _, e = s.db.Put("device", d.ID, d, 0); e != nil {
			return e
		}
	}
	recipes := []domain.Recipe{{ID: "studio-porter", Name: "Studio Porter", Style: "Robust Porter", Type: "All Grain", BoilLiters: 6.5, BoilMinutes: 60, Description: "Cocoa, toasted bread and a clean finish.", BatchLiters: 5, Efficiency: 55, Ingredients: []domain.Ingredient{{Name: "Pale malt", Kind: "fermentable", Type: "Grain", Amount: 1.2, Unit: "kg", Use: "mash"}, {Name: "Chocolate malt", Kind: "fermentable", Type: "Grain", Amount: 0.1, Unit: "kg", Use: "mash"}, {Name: "East Kent Goldings", Kind: "hop", Amount: 18, Unit: "g", Use: "boil", Minutes: 60, Alpha: 5}, {Name: "English ale yeast", Kind: "yeast", Type: "Ale", Form: "Dry", Amount: 5, Unit: "g", Use: "primary"}}, Steps: []domain.RecipeStep{{Name: "Mash", Kind: "mash", TargetC: 67, Minutes: 60, Location: 1}, {Name: "Mash out", Kind: "mash", TargetC: 76, Minutes: 10, Location: 1}, {Name: "Boil", Kind: "boil", TargetC: 100, Minutes: 60, Location: 2}}, Tags: []string{"dark", "studio"}, Notes: "Example profile: verify your physical chamber assignments before using a real machine.", Revision: 1, CreatedAt: now, UpdatedAt: now}, {ID: "weekend-pale", Name: "Weekend Pale", Style: "American Pale Ale", Type: "All Grain", BoilLiters: 6.5, BoilMinutes: 60, Description: "Bright citrus with a soft malt backbone.", BatchLiters: 5, Efficiency: 55, Ingredients: []domain.Ingredient{{Name: "Pale malt", Kind: "fermentable", Type: "Grain", Amount: 1.2, Unit: "kg", Use: "mash"}, {Name: "Cascade", Kind: "hop", Amount: 20, Unit: "g", Use: "boil", Minutes: 30, Alpha: 6}}, Steps: []domain.RecipeStep{{Name: "Mash", Kind: "mash", TargetC: 65, Minutes: 60, Location: 1}, {Name: "Boil", Kind: "boil", TargetC: 100, Minutes: 60, Location: 2}}, Tags: []string{"pale", "weekend"}, Revision: 1, CreatedAt: now, UpdatedAt: now}}
	for _, v := range recipes {
		if _, e = s.db.Put("recipe", v.ID, v, 0); e != nil {
			return e
		}
		_, _ = s.db.Put("recipe-revision", v.ID+":1", v, 0)
	}
	for j, d := range devices {
		start := now.Add(-time.Duration(j+1) * 24 * time.Hour)
		b := domain.Brew{ID: "demo-history-" + d.ID, DeviceID: d.ID, DeviceName: d.Name, Model: d.Model, RecipeName: recipes[j%2].Name, RecipeID: recipes[j%2].ID, RecipeRevision: 1, State: "completed", Command: "brew", StartedAt: &start, ObservedAt: start, DurationMS: 7800000, SampleCount: 521, Source: "simulation", Notes: "Simulated brew history for exploring graphs and exports.", Rating: 4}
		samples := make([]domain.Sample, 0, 521)
		for i := 0; i <= 520; i++ {
			elapsed := i * 15000
			minute := float64(elapsed) / 60000
			target := 152.0
			phase := 1
			wort := 72 + math.Min(minute/18, 1)*80 + math.Sin(minute)*0.6
			if minute > 70 {
				target = 210
				phase = 2
				wort = 152 + math.Min((minute-70)/15, 1)*58 + math.Sin(minute)*0.4
			}
			thermo := wort + 6 + math.Sin(minute*0.4)*2
			at := start.Add(time.Duration(elapsed) * time.Millisecond)
			samples = append(samples, domain.Sample{ElapsedMS: int64(elapsed), ObservedAt: &at, WortF: floatPtr(wort), ThermoblockF: floatPtr(thermo), TargetF: floatPtr(target), StepSeconds: i * 15, Phase: phase, Compartment: phase, CurrentStep: phase - 1, Flags: 11, Source: "simulation"})
		}
		if _, e = s.db.Put("brew", b.ID, b, 0); e != nil {
			return e
		}
		if e = s.db.AppendSamples(b.ID, samples); e != nil {
			return e
		}
	}
	return nil
}
func floatPtr(v float64) *float64 { return &v }
