package recipe_test

import (
	"encoding/json"
	"math"
	"reflect"
	"strings"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

func source() domain.Recipe {
	return domain.Recipe{ID: "house-ale", Name: "House Ale", BatchLiters: 10, Efficiency: 75, Revision: 4,
		Ingredients: []domain.Ingredient{{Name: "Pale malt", Kind: "fermentable", Amount: 2, Unit: "kg", Use: "mash"}, {Name: "Cascade", Kind: "hop", Amount: 20, Unit: "g", Use: "boil", Minutes: 30, Alpha: 5.5}},
		Steps:       []domain.RecipeStep{{Name: "Mash", Kind: "mash", TargetC: 65, Minutes: 60, DrainMinutes: 2, Location: 1}, {Name: "Boil", Kind: "boil", TargetC: 100, Minutes: 30, Location: 3}}}
}

func TestAdaptScalesWithExplicitMappingsWithoutMutatingSource(t *testing.T) {
	original := source()
	before, _ := json.Marshal(original)
	p := recipe.Profiles()[0]
	p.Efficiency = 60
	plan, err := recipe.Adapt(original, p, 5)
	if err != nil {
		t.Fatal(err)
	}
	if !plan.CanDeploy || plan.MachineRecipe == nil || plan.SourceRevision != 4 {
		t.Fatalf("incomplete plan: %+v", plan)
	}
	if plan.Recipe.Ingredients[0].Amount != 1.25 || plan.Recipe.Ingredients[1].Amount != 10 {
		t.Fatalf("wrong scaling: %+v", plan.Recipe.Ingredients)
	}
	if plan.Recipe.Steps[0].TargetC != 65 || plan.MachineRecipe.Steps[0].TargetF != 149 || plan.MachineRecipe.Steps[1].Location != 3 {
		t.Fatalf("units/routing altered: %+v", plan)
	}
	if !strings.Contains(strings.Join(plan.Warnings, " "), "utilization") {
		t.Fatal("hop limitation omitted")
	}
	plan.Recipe.Ingredients[0].Name = "mutated"
	plan.Recipe.Steps[0].Name = "mutated"
	after, _ := json.Marshal(original)
	if string(before) != string(after) {
		t.Fatal("adaptation shares mutable source slices")
	}
}

func TestUnsupportedModelsCannotBePromotedToControl(t *testing.T) {
	for _, p := range recipe.Profiles()[2:] {
		p.ControlSupported = true // a caller cannot manufacture an adapter this way.
		plan, err := recipe.Adapt(source(), p, 5)
		if err != nil {
			t.Fatalf("%s: %v", p.ID, err)
		}
		if plan.CanDeploy || plan.MachineRecipe != nil || plan.Profile.ControlSupported {
			t.Fatalf("%s became deployable", p.ID)
		}
	}
	p := recipe.Profiles()[0]
	p.ID = "unknown"
	if _, err := recipe.Adapt(source(), p, 5); err == nil {
		t.Fatal("unknown profile accepted")
	}
}

func TestNativeLimitsCannotBeRelaxedByProfile(t *testing.T) {
	profile := recipe.Profiles()[0]
	profile.MaxSteps = 100
	cases := []struct {
		name   string
		change func(*domain.Recipe)
	}{
		{"unmapped", func(r *domain.Recipe) { r.Steps[0].Location = 0 }},
		{"unknown step", func(r *domain.Recipe) { r.Steps[0].Kind = "magic-valve" }},
		{"unsupported cooling", func(r *domain.Recipe) { r.Steps[0].Kind = "cool" }},
		{"too cold", func(r *domain.Recipe) { r.Steps[0].TargetC = 10 }},
		{"too long", func(r *domain.Recipe) { r.Steps[0].Minutes = 600 }},
		{"drain too long", func(r *domain.Recipe) { r.Steps[0].DrainMinutes = 121 }},
		{"too many steps", func(r *domain.Recipe) {
			for len(r.Steps) < 16 {
				r.Steps = append(r.Steps, r.Steps[0])
			}
		}},
	}
	for _, test := range cases {
		t.Run(test.name, func(t *testing.T) {
			r := source()
			test.change(&r)
			if _, err := recipe.Adapt(r, profile, 5); err == nil {
				t.Fatal("invalid program accepted")
			}
		})
	}
	if _, err := recipe.Adapt(source(), profile, 6); err == nil {
		t.Fatal("profile batch limit ignored")
	}
	profile.MaxGrainKG = 0.1
	if _, err := recipe.Adapt(source(), profile, 5); err == nil {
		t.Fatal("grain limit ignored")
	}
}

func TestDeviceLabelChangesAreExplicitAndStable(t *testing.T) {
	r := source()
	r.ID = "a-canonical-identifier-longer-than-twenty"
	r.Name = "Très long recipe name which exceeds the display limit"
	r.Steps[0].Name = "Ä very long mash step name exceeds the display"
	first, err := recipe.Adapt(r, recipe.Profiles()[0], 5)
	if err != nil {
		t.Fatal(err)
	}
	second, err := recipe.Adapt(r, recipe.Profiles()[0], 5)
	if err != nil {
		t.Fatal(err)
	}
	if first.MachineRecipe.ID != second.MachineRecipe.ID || len(first.MachineRecipe.ID) != 20 {
		t.Fatal("native id is unstable")
	}
	if first.Recipe.Name != r.Name || len(first.MachineRecipe.Name) > 31 || len(first.MachineRecipe.Steps[0].Name) > 21 {
		t.Fatal("label limits/source preservation")
	}
	if !strings.Contains(strings.Join(first.Warnings, " "), "canonical label is unchanged") {
		t.Fatal("transformation was silent")
	}
	bytes, err := recipe.NativeJSON(*first.MachineRecipe)
	if err != nil || len(bytes) > 4095 {
		t.Fatalf("wire payload: %d %v", len(bytes), err)
	}
}

func TestCanonicalValidationAndNativeBoundaryValues(t *testing.T) {
	r := source()
	r.Steps[0].Location = 0
	if err := recipe.Validate(r); err != nil {
		t.Fatalf("unmapped canonical plan should remain editable: %v", err)
	}
	r.Ingredients[0].Unit = "lb"
	if err := recipe.Validate(r); err == nil {
		t.Fatal("noncanonical mass silently accepted")
	}
	r = source()
	r.BatchLiters = math.NaN()
	if err := recipe.Validate(r); err == nil {
		t.Fatal("NaN accepted")
	}
	machine := domain.MachineRecipe{ID: "a", Name: strings.Repeat("<", 31), Steps: []domain.MachineStep{{Name: strings.Repeat("&", 21), Location: 6, TargetF: 60, Minutes: 599, DrainMinutes: 120}}}
	for len(machine.Steps) < 15 {
		machine.Steps = append(machine.Steps, machine.Steps[0])
	}
	data, err := recipe.NativeJSON(machine)
	if err != nil {
		t.Fatal(err)
	}
	var roundtrip domain.MachineRecipe
	if err := json.Unmarshal(data, &roundtrip); err != nil || !reflect.DeepEqual(machine, roundtrip) {
		t.Fatal("ASCII JSON escapes changed payload")
	}
	machine.Steps[0].TargetF = 59
	if _, err := recipe.NativeJSON(machine); err == nil {
		t.Fatal("native gap 1–59F accepted")
	}
}
