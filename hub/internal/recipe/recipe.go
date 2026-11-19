// Package recipe validates canonical recipes and produces explicit, reviewable
// device plans. It never infers a valve/chamber location from a step's name.
package recipe

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"fmt"
	"math"
	"regexp"
	"strings"
	"unicode/utf8"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

const MaxImportBytes = 1 << 20

var nativeID = regexp.MustCompile(`^[A-Za-z0-9_-]{1,20}$`)
var stepKinds = map[string]bool{"mash": true, "mash-out": true, "boil": true, "hop": true, "heat": true, "hold": true, "rest": true, "unheated": true, "cool": true, "fermentation": true}

// Profiles returns independent planning defaults, not hardware calibration.
// Only S/Pro running the standalone REST firmware have a control adapter. The Z
// entry describes one brewing module, not a combined Z2/Z4 program.
func Profiles() []domain.Profile {
	const assumptions = "70% efficiency is a planning assumption; replace it with measured efficiency. Locations must be supplied and checked against the actual machine."
	const picoAssumptions = "5 L is a nominal batch size. 50% efficiency is Small Brew Companion's community planning estimate, not a measurement of your machine. Replace it with your measured efficiency. Grain capacity is unverified; check your actual grain container. Locations must be supplied and checked against the actual machine."
	picoSource := domain.ProfileSource{
		Label:       "Small Brew Companion: Pico C/S/Pro recipe scaling",
		URL:         "https://www.smallbrewcompanion.com/docs/scale-recipe-to-pico-c-s-pro-manual-steps/",
		Description: "Community starting points: 5 L batches and approximately 50% efficiency. Does not establish a maximum grain capacity.",
	}
	return []domain.Profile{
		{
			ID: "pico-pro", Name: "Pico Pro (standalone)", BatchLiters: 5, Efficiency: 50,
			MaxSteps: 15, MaxTargetC: 100, ControlSupported: true, Notes: picoAssumptions,
			Sources: []domain.ProfileSource{picoSource, {
				Label:       "PicoBrew: Pico Pro specifications",
				URL:         "https://picobrew.com/store/products/pico",
				Description: "Manufacturer nominal batch size: 5 L. The separately listed 6.6 L keg volume is not the batch size.",
			}},
		},
		{
			ID: "pico-s", Name: "Pico S (standalone)", BatchLiters: 5, Efficiency: 50,
			MaxSteps: 15, MaxTargetC: 100, ControlSupported: true, Notes: picoAssumptions,
			Sources: []domain.ProfileSource{picoSource},
		},
		{
			ID: "pico-c", Name: "Pico C (planning only)", BatchLiters: 5, Efficiency: 50,
			MaxTargetC: 100, Notes: picoAssumptions + " No hub control adapter is implemented for C firmware.",
			Sources: []domain.ProfileSource{picoSource, {
				Label:       "PicoBrew: Pico C specifications",
				URL:         "https://picobrew.com/store/products/picoc",
				Description: "Manufacturer nominal batch size: 5 L. This specification does not establish grain capacity or efficiency.",
			}},
		},
		{ID: "z", Name: "PicoBrew Z, one module (planning only)", BatchLiters: 9.46352946, Efficiency: 70, MaxTargetC: 100, MaxGrainKG: 4.08233133, HopSlots: 4, Notes: assumptions + " Nominal batch is 2.5 US gallons; grain capacity is 9 lb. No control adapter is implemented."},
		{ID: "zymatic", Name: "Zymatic (planning only)", BatchLiters: 9.46352946, Efficiency: 70, MaxTargetC: 100, MaxGrainKG: 4.08233133, HopSlots: 4, Notes: assumptions + " Nominal batch is 2.5 US gallons; grain capacity is 9 lb. No control adapter is implemented."},
	}
}

func finite(value float64) bool { return !math.IsNaN(value) && !math.IsInf(value, 0) }
func text(value string, max int) bool {
	return utf8.ValidString(value) && strings.TrimSpace(value) != "" && len(value) <= max && !strings.ContainsRune(value, '\x00')
}

// Validate accepts canonical plans, including unmapped location zero. Native
// timing, label and routing constraints are checked by Adapt/NativeJSON instead.
// Canonical units: fermentable kg; hop g; water L; yeast/misc g, mL or count.
func Validate(r domain.Recipe) error {
	if !text(r.Name, 512) {
		return errors.New("recipe name is required and must be valid text of at most 512 bytes")
	}
	if len(r.ID) > 256 || strings.ContainsRune(r.ID, '\x00') {
		return errors.New("invalid recipe id")
	}
	if !finite(r.BatchLiters) || r.BatchLiters <= 0 || r.BatchLiters > 10000 {
		return errors.New("batch_liters must be positive and at most 10000")
	}
	if !finite(r.Efficiency) || r.Efficiency <= 0 || r.Efficiency > 100 {
		return errors.New("efficiency must be a percentage greater than zero and at most 100")
	}
	if r.Type != "" {
		if _, err := enum(r.Type, "All Grain", "Partial Mash", "Extract"); err != nil {
			return err
		}
	}
	if !finite(r.BoilLiters) || r.BoilLiters < 0 || r.BoilLiters > 10000 || !finite(r.BoilMinutes) || r.BoilMinutes < 0 || r.BoilMinutes > 43200 {
		return errors.New("invalid boil volume or duration")
	}
	if len(r.Water) > 32 || len(r.Fermentation) > 32 {
		return errors.New("too many water parameters or fermentation phases")
	}
	for name, value := range r.Water {
		if !text(name, 64) || !finite(value) || value < 0 || name == "ph" && value > 14 {
			return errors.New("invalid water chemistry parameter")
		}
	}
	for _, phase := range r.Fermentation {
		if !text(phase.Name, 512) || !finite(phase.TargetC) || phase.TargetC < -10 || phase.TargetC > 60 || !finite(phase.Days) || phase.Days <= 0 || phase.Days > 3650 {
			return errors.New("invalid fermentation phase")
		}
	}
	if !finite(r.Carbonation.Volumes) || r.Carbonation.Volumes < 0 || r.Carbonation.Volumes > 10 || !finite(r.Carbonation.TemperatureC) || r.Carbonation.TemperatureC < -10 || r.Carbonation.TemperatureC > 60 {
		return errors.New("invalid carbonation plan")
	}
	if r.Carbonation.Method != "" {
		if _, err := enum(r.Carbonation.Method, "forced", "natural"); err != nil {
			return err
		}
	}
	if len(r.Ingredients) > 512 || len(r.Steps) == 0 || len(r.Steps) > 256 {
		return errors.New("a recipe requires 1–256 steps and at most 512 ingredients")
	}
	if len(r.Description) > 65536 || len(r.Notes) > 65536 || len(r.Tags) > 100 {
		return errors.New("recipe metadata is too large")
	}
	for i, in := range r.Ingredients {
		if !text(in.Name, 512) || !finite(in.Amount) || in.Amount <= 0 || !finite(in.Minutes) || in.Minutes < 0 || !finite(in.Alpha) || in.Alpha < 0 || in.Alpha > 100 {
			return fmt.Errorf("ingredient %d has invalid name, amount, timing or alpha percentage", i+1)
		}
		if !finite(in.Yield) || in.Yield < 0 || in.Yield > 100 || !finite(in.Color) || in.Color < 0 {
			return fmt.Errorf("ingredient %d has invalid yield or color", i+1)
		}
		valid := false
		switch in.Kind {
		case "fermentable":
			valid = in.Unit == "kg"
		case "hop":
			valid = in.Unit == "g"
		case "water":
			valid = in.Unit == "L"
		case "yeast", "misc":
			valid = in.Unit == "g" || in.Unit == "mL" || in.Unit == "count"
		}
		if !valid {
			return fmt.Errorf("ingredient %d has unsupported kind/unit %q/%q", i+1, in.Kind, in.Unit)
		}
	}
	for i, step := range r.Steps {
		if !text(step.Name, 512) || !stepKinds[step.Kind] {
			return fmt.Errorf("step %d has a missing name or unsupported kind %q", i+1, step.Kind)
		}
		if !finite(step.TargetC) || step.TargetC < 0 || step.TargetC > 100 {
			return fmt.Errorf("step %d target_c must be 0–100 Celsius", i+1)
		}
		if step.Minutes < 1 || step.Minutes > 43200 || step.DrainMinutes < 0 || step.DrainMinutes > 1440 {
			return fmt.Errorf("step %d has invalid duration", i+1)
		}
		if step.Location < 0 || step.Location > 6 {
			return fmt.Errorf("step %d location must be 0 (unmapped) or an explicit 1–6", i+1)
		}
		if step.Kind == "unheated" && step.TargetC != 0 {
			return fmt.Errorf("step %d unheated target must be zero", i+1)
		}
		if step.Type != "" {
			if _, err := enum(step.Type, "Temperature", "Infusion", "Decoction"); err != nil {
				return fmt.Errorf("step %d: %w", i+1, err)
			}
		}
		if !finite(step.InfuseLiters) || step.InfuseLiters < 0 || !finite(step.RampMinutes) || step.RampMinutes < 0 {
			return fmt.Errorf("step %d has invalid infusion volume or ramp time", i+1)
		}
	}
	return nil
}

func supportedProfile(id string) (domain.Profile, bool) {
	for _, profile := range Profiles() {
		if profile.ID == id {
			return profile, true
		}
	}
	return domain.Profile{}, false
}

// Adapt scales grain/unspecified fermentables by volume and source/target
// efficiency. Known extracts/sugars, hops and other ingredients scale by volume.
// It preserves source data and returns a new plan.
// It does not solve hop utilization, mash water, losses or yeast cell counts.
func Adapt(r domain.Recipe, profile domain.Profile, batchLiters float64) (domain.Adapted, error) {
	if err := Validate(r); err != nil {
		return domain.Adapted{}, err
	}
	builtIn, known := supportedProfile(profile.ID)
	if !known {
		return domain.Adapted{}, fmt.Errorf("unknown machine profile %q", profile.ID)
	}
	if !finite(profile.BatchLiters) || profile.BatchLiters <= 0 || !finite(profile.Efficiency) || profile.Efficiency <= 0 || profile.Efficiency > 100 || !finite(profile.MaxTargetC) || profile.MaxTargetC <= 0 || profile.MaxTargetC > 100 || !finite(profile.MaxGrainKG) || profile.MaxGrainKG < 0 || profile.MaxSteps < 0 || profile.HopSlots < 0 {
		return domain.Adapted{}, errors.New("profile has invalid capacity, temperature or efficiency")
	}
	if batchLiters == 0 {
		batchLiters = profile.BatchLiters
	}
	if !finite(batchLiters) || batchLiters <= 0 || batchLiters > profile.BatchLiters+1e-9 {
		return domain.Adapted{}, errors.New("requested batch exceeds the configured profile batch limit or is invalid")
	}
	if profile.MaxSteps > 0 && len(r.Steps) > profile.MaxSteps {
		return domain.Adapted{}, fmt.Errorf("recipe has %d steps; configured profile allows %d", len(r.Steps), profile.MaxSteps)
	}
	plan := domain.Adapted{Recipe: r, Profile: profile, SourceRevision: r.Revision, Warnings: []string{}}
	plan.Recipe.Ingredients = append([]domain.Ingredient(nil), r.Ingredients...)
	plan.Recipe.Steps = append([]domain.RecipeStep(nil), r.Steps...)
	plan.Recipe.Tags = append([]string(nil), r.Tags...)
	plan.Recipe.Fermentation = append([]domain.FermentationStep(nil), r.Fermentation...)
	if r.Water != nil {
		plan.Recipe.Water = make(map[string]float64, len(r.Water))
		for name, value := range r.Water {
			plan.Recipe.Water[name] = value
		}
	}
	plan.Recipe.BatchLiters = batchLiters
	plan.Recipe.Efficiency = profile.Efficiency
	ratio := batchLiters / r.BatchLiters
	plan.Recipe.BoilLiters *= ratio
	for i := range plan.Recipe.Steps {
		plan.Recipe.Steps[i].InfuseLiters *= ratio
	}
	grain := 0.0
	hops := false
	yeast := false
	timings := map[string]bool{}
	for i := range plan.Recipe.Ingredients {
		in := &plan.Recipe.Ingredients[i]
		factor := ratio
		isExtract := strings.EqualFold(in.Type, "Extract") || strings.EqualFold(in.Type, "Dry Extract") || strings.EqualFold(in.Type, "Sugar")
		if in.Kind == "fermentable" && !isExtract {
			factor *= r.Efficiency / profile.Efficiency
		}
		in.Amount *= factor
		if !finite(in.Amount) || in.Amount <= 0 {
			return domain.Adapted{}, errors.New("ingredient scaling overflow")
		}
		if in.Kind == "fermentable" && !isExtract {
			grain += in.Amount
		}
		if in.Kind == "hop" {
			hops = true
			if in.Use == "boil" || in.Use == "first-wort" {
				timings[fmt.Sprintf("%s:%g", in.Use, in.Minutes)] = true
			}
		}
		if in.Kind == "yeast" {
			yeast = true
		}
	}
	if profile.MaxGrainKG > 0 && grain > profile.MaxGrainKG+1e-9 {
		return domain.Adapted{}, fmt.Errorf("scaled fermentables %.3f kg exceed configured grain limit %.3f kg; separate extract/sugar additions require explicit review", grain, profile.MaxGrainKG)
	}
	if profile.HopSlots > 0 && len(timings) > profile.HopSlots {
		return domain.Adapted{}, fmt.Errorf("%d timed hop additions exceed the configured %d slots", len(timings), profile.HopSlots)
	}
	if hops {
		plan.Warnings = append(plan.Warnings, "Hops scale by batch volume only; utilization, gravity, boil-off and IBU changes are not modeled. Ingredient timings do not create machine steps or assign chambers.")
	}
	if yeast {
		plan.Warnings = append(plan.Warnings, "Yeast scales by volume only; verify cell count, viability and pitch rate separately.")
	}
	if len(r.Fermentation) > 0 || r.Carbonation.Volumes > 0 {
		plan.Warnings = append(plan.Warnings, "Fermentation and carbonation remain in the recipe plan; the hot-side machine program does not control them.")
	}
	plan.Warnings = append(plan.Warnings, "Verify the profile efficiency and actual grain/water capacity. Water absorption, dead space and boil-off are not modeled.")
	if profile.MaxGrainKG == 0 {
		plan.Warnings = append(plan.Warnings, "No verified grain capacity is configured for this profile; this plan does not prove the grain load fits.")
	}
	for i, step := range r.Steps {
		if step.TargetC > profile.MaxTargetC {
			return domain.Adapted{}, fmt.Errorf("step %d exceeds configured maximum temperature", i+1)
		}
	}
	if !builtIn.ControlSupported || !profile.ControlSupported {
		plan.Profile.ControlSupported = false
		plan.Warnings = append(plan.Warnings, "Planning/export only: this profile has no enabled hub control adapter. No machine program was generated.")
		return plan, nil
	}
	if len(r.Steps) > 15 {
		return domain.Adapted{}, errors.New("standalone firmware accepts at most 15 steps")
	}
	machine := &domain.MachineRecipe{ID: r.ID, Steps: make([]domain.MachineStep, 0, len(r.Steps))}
	if !nativeID.MatchString(machine.ID) {
		basis := r.ID
		if basis == "" {
			data, _ := json.Marshal(r)
			basis = string(data)
		}
		sum := sha256.Sum256([]byte(basis))
		machine.ID = "hub-" + hex.EncodeToString(sum[:8])
		plan.Warnings = append(plan.Warnings, "Native recipe ID is "+machine.ID+" because firmware IDs allow only 1–20 ASCII letters, digits, '_' or '-'.")
	}
	machine.Name = label(r.Name, 31, "Recipe", &plan.Warnings)
	for i, step := range r.Steps {
		if step.InfuseLiters > 0 || step.RampMinutes > 0 || step.Type != "" && !strings.EqualFold(step.Type, "Temperature") {
			return domain.Adapted{}, fmt.Errorf("step %d has infusion, decoction or timed-ramp instructions with no verified native mapping; prepare an explicit machine program first", i+1)
		}
		if step.Kind == "cool" || step.Kind == "fermentation" {
			return domain.Adapted{}, fmt.Errorf("step %d kind %q has no verified standalone firmware mapping", i+1, step.Kind)
		}
		if step.Location < 1 || step.Location > 6 {
			return domain.Adapted{}, fmt.Errorf("step %d requires an explicitly verified chamber location 1–6; no routing is inferred", i+1)
		}
		if step.Minutes > 599 || step.DrainMinutes > 120 {
			return domain.Adapted{}, fmt.Errorf("step %d exceeds firmware duration limits (599 active / 120 drain minutes)", i+1)
		}
		target := 0
		if step.TargetC != 0 {
			exact := step.TargetC*9/5 + 32
			if exact < 60 || exact > 212 {
				return domain.Adapted{}, fmt.Errorf("step %d is outside the firmware range of 60–212F", i+1)
			}
			target = int(math.Round(exact))
			if math.Abs(exact-float64(target)) > 1e-9 {
				plan.Warnings = append(plan.Warnings, fmt.Sprintf("Step %d temperature rounds from %.3fC to %dF for the firmware integer target.", i+1, step.TargetC, target))
			}
		}
		machine.Steps = append(machine.Steps, domain.MachineStep{Name: label(step.Name, 21, fmt.Sprintf("Step %d", i+1), &plan.Warnings), Location: step.Location, TargetF: target, Minutes: step.Minutes, DrainMinutes: step.DrainMinutes})
	}
	if _, err := NativeJSON(*machine); err != nil {
		return domain.Adapted{}, err
	}
	plan.MachineRecipe = machine
	plan.CanDeploy = true
	return plan, nil
}

func label(original string, max int, fallback string, warnings *[]string) string {
	var b strings.Builder
	for _, r := range original {
		if r >= 32 && r <= 126 {
			b.WriteRune(r)
		} else {
			b.WriteByte('?')
		}
	}
	value := strings.TrimSpace(b.String())
	if value == "" {
		value = fallback
	}
	if len(value) > max {
		value = value[:max]
	}
	if value != original {
		*warnings = append(*warnings, fmt.Sprintf("Device label %q becomes %q (printable ASCII, at most %d bytes); the canonical label is unchanged.", original, value, max))
	}
	return value
}

// NativeJSON validates the exact standalone API wire schema and its 4095-byte
// request cap. Its bounds come from firmware docs/standalone-api.md and
// src/local_api.c, not a stock PicoBrew cloud protocol.
func NativeJSON(r domain.MachineRecipe) ([]byte, error) {
	validLabel := func(s string, n int) bool {
		if len(s) < 1 || len(s) > n {
			return false
		}
		for _, c := range []byte(s) {
			if c < 32 || c > 126 {
				return false
			}
		}
		return true
	}
	if !nativeID.MatchString(r.ID) || !validLabel(r.Name, 31) || len(r.Steps) < 1 || len(r.Steps) > 15 {
		return nil, errors.New("invalid native recipe id, name or step count")
	}
	for i, s := range r.Steps {
		if !validLabel(s.Name, 21) || s.Location < 1 || s.Location > 6 || (s.TargetF != 0 && (s.TargetF < 60 || s.TargetF > 212)) || s.Minutes < 1 || s.Minutes > 599 || s.DrainMinutes < 0 || s.DrainMinutes > 120 {
			return nil, fmt.Errorf("invalid native step %d", i+1)
		}
	}
	data, err := json.Marshal(r)
	if err != nil {
		return nil, err
	}
	if len(data) > 4095 {
		return nil, errors.New("native recipe exceeds the firmware 4095-byte request limit")
	}
	return data, nil
}
