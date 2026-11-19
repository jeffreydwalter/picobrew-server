package recipe

import (
	"encoding/json"
	"errors"
	"fmt"
	"strings"
	"unicode/utf8"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

// IsNative separates a machine program from a complete ingredient recipe.
// Empty Kind continues to mean the canonical recipe used by earlier Hub builds.
func IsNative(r domain.Recipe) bool {
	return r.Kind == "native-program" || r.Kind == "maintenance"
}

func validateOrigin(r domain.Recipe) error {
	if len(r.SourceModel) > 64 || !utf8.ValidString(r.SourceModel) || strings.ContainsRune(r.SourceModel, '\x00') {
		return errors.New("invalid source model")
	}
	if r.Provenance == nil {
		return nil
	}
	p := r.Provenance
	for _, value := range []string{p.CatalogID, p.SourceID, p.Repository, p.Commit, p.Path, p.SHA256, p.SourceURL, p.License} {
		if len(value) > 2048 || !utf8.ValidString(value) || strings.ContainsRune(value, '\x00') {
			return errors.New("invalid recipe provenance")
		}
	}
	if len(p.Warnings) > 128 {
		return errors.New("too many source conversion warnings")
	}
	for _, warning := range p.Warnings {
		if len(warning) > 8192 || !utf8.ValidString(warning) || strings.ContainsRune(warning, '\x00') {
			return errors.New("invalid source conversion warning")
		}
	}
	return nil
}

// Library programs retain full source labels, including labels that cannot fit
// a brewer. This allows explicit editing instead of silently truncating them.
// Device limits are always checked again by NativeJSON before deployment.
func validateNativeLibrary(r domain.Recipe) error {
	switch r.SourceModel {
	case "pico", "pico-s", "pico-pro", "pico-c", "z", "zymatic":
	default:
		return errors.New("native program requires an explicit supported source_model")
	}
	if r.NativeProgram == nil {
		return errors.New("native program data is required")
	}
	if len(r.Steps) != 0 || len(r.Ingredients) != 0 || len(r.Fermentation) != 0 || len(r.Water) != 0 || r.BoilLiters != 0 || r.BoilMinutes != 0 || r.Type != "" || r.Carbonation != (domain.Carbonation{}) {
		return errors.New("program-only records cannot also contain a canonical ingredient/step plan; keep source notes or create a separate complete recipe")
	}
	if r.BatchLiters != 0 || r.Efficiency != 0 {
		return errors.New("program-only records have no inferred batch volume or efficiency")
	}
	if len(r.Description) > 65536 || len(r.Notes) > 65536 || len(r.Tags) > 100 {
		return errors.New("recipe metadata is too large")
	}
	p := r.NativeProgram
	if !nativeID.MatchString(p.ID) || p.Name != r.Name {
		return errors.New("native program requires a valid ID and the same name as its library record")
	}
	if len(p.Steps) < 1 || len(p.Steps) > 256 {
		return errors.New("native program requires 1–256 source steps")
	}
	for i, step := range p.Steps {
		if !text(step.Name, 512) || step.Location < 0 || step.Location > 6 || step.TargetF < 0 || step.TargetF > 212 || step.Minutes < 0 || step.Minutes > 43200 || step.DrainMinutes < 0 || step.DrainMinutes > 1440 {
			return fmt.Errorf("native source step %d has invalid data", i+1)
		}
		if strings.HasPrefix(r.SourceModel, "pico") && (step.Location == 0 || step.TargetF != 0 && step.TargetF < 60) {
			return fmt.Errorf("native Pico step %d requires verified locations 1–6 and target 0 or 60–212F", i+1)
		}
	}
	return nil
}

func nativeModelMatches(sourceModel, targetModel string) bool {
	if sourceModel == "pico" {
		return targetModel == "pico-s" || targetModel == "pico-pro" || targetModel == "pico-c"
	}
	return sourceModel == targetModel
}

func adaptNative(r domain.Recipe, profile, builtIn domain.Profile, batchLiters float64) (domain.Adapted, error) {
	if batchLiters != 0 {
		return domain.Adapted{}, errors.New("native machine programs cannot be scaled by batch volume; their exact source schedule is retained")
	}
	if !nativeModelMatches(r.SourceModel, profile.ID) {
		return domain.Adapted{}, fmt.Errorf("native %s routing cannot be used on %s", r.SourceModel, profile.ID)
	}
	// A complete copy keeps the original immutable even if a caller edits the
	// preview, its steps or source warnings before confirming deployment.
	data, err := json.Marshal(r)
	if err != nil {
		return domain.Adapted{}, err
	}
	var copy domain.Recipe
	if err := json.Unmarshal(data, &copy); err != nil {
		return domain.Adapted{}, err
	}
	plan := domain.Adapted{Recipe: copy, Profile: profile, SourceRevision: r.Revision, Warnings: []string{
		"Native machine program: exact source temperatures, timings and routing are retained. No ingredient or batch scaling is performed.",
	}}
	if r.Provenance != nil {
		plan.Warnings = append(plan.Warnings, r.Provenance.Warnings...)
	}
	if !builtIn.ControlSupported || !profile.ControlSupported {
		plan.Profile.ControlSupported = false
		plan.Warnings = append(plan.Warnings, "Reference/export only: this source model has no enabled Hub control adapter.")
		return plan, nil
	}
	if profile.MaxSteps > 0 && len(copy.NativeProgram.Steps) > profile.MaxSteps {
		return domain.Adapted{}, errors.New("native program exceeds the configured step capacity")
	}
	for i, step := range copy.NativeProgram.Steps {
		if step.TargetF != 0 && float64(step.TargetF-32)*5/9 > profile.MaxTargetC {
			return domain.Adapted{}, fmt.Errorf("native step %d exceeds the configured temperature limit", i+1)
		}
	}
	if _, err := NativeJSON(*copy.NativeProgram); err != nil {
		return domain.Adapted{}, fmt.Errorf("source program needs an explicit edit before deployment: %w", err)
	}
	program := *copy.NativeProgram
	program.Steps = append([]domain.MachineStep(nil), copy.NativeProgram.Steps...)
	plan.MachineRecipe = &program
	plan.CanDeploy = true
	return plan, nil
}
