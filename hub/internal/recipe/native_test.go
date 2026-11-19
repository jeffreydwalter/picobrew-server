package recipe_test

import (
	"encoding/json"
	"reflect"
	"strings"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

func archivedProgram() domain.Recipe {
	return domain.Recipe{ID: "library-pale", Name: "Pico Pale", Kind: "native-program", SourceModel: "pico", Revision: 1,
		NativeProgram: &domain.MachineRecipe{ID: "pak-64", Name: "Pico Pale", Steps: []domain.MachineStep{
			{Name: "Heat Mash", Location: 1, TargetF: 152, Minutes: 0},
			{Name: "Mash", Location: 1, TargetF: 152, Minutes: 60, DrainMinutes: 3},
		}}, Provenance: &domain.RecipeProvenance{CatalogID: "pico-64", Warnings: []string{"No ingredient bill is present in this source"}}}
}

func TestNativeProgramRetainsExactScheduleAndIndependentProvenance(t *testing.T) {
	r := archivedProgram()
	before, _ := json.Marshal(r)
	a, err := recipe.Adapt(r, recipe.Profiles()[0], 0)
	if err != nil || !a.CanDeploy || !reflect.DeepEqual(a.MachineRecipe, r.NativeProgram) {
		t.Fatalf("native adaptation: %+v %v", a, err)
	}
	if a.Recipe.BatchLiters != 0 || a.Recipe.Efficiency != 0 || len(a.Recipe.Ingredients) != 0 || len(a.Recipe.Steps) != 0 {
		t.Fatal("native program acquired invented recipe metadata")
	}
	a.MachineRecipe.Steps[0].Minutes = 10
	a.Recipe.NativeProgram.Steps[0].Name = "Edited"
	a.Recipe.Provenance.Warnings[0] = "Edited"
	after, _ := json.Marshal(r)
	if string(before) != string(after) {
		t.Fatal("editing a preview altered its original program")
	}
	if _, err := recipe.Adapt(r, recipe.Profiles()[0], 5); err == nil {
		t.Fatal("native program was scaled using an invented batch volume")
	}
	data, err := recipe.Export(r, "json")
	if err != nil {
		t.Fatal(err)
	}
	items, err := recipe.Import(data, "json")
	if err != nil || len(items) != 1 || !reflect.DeepEqual(items[0], r) {
		t.Fatalf("native JSON roundtrip: %v %+v", err, items)
	}
	if _, err := recipe.Export(r, "beerxml"); err == nil {
		t.Fatal("program-only record exported as a complete BeerXML recipe")
	}
	native, err := recipe.Export(r, "native-json")
	var actual domain.MachineRecipe
	if err != nil || json.Unmarshal(native, &actual) != nil || !reflect.DeepEqual(actual, *r.NativeProgram) {
		t.Fatalf("native export changed routing/timings: %s %v", native, err)
	}
}

func TestNativeProgramsRespectModelAndDeviceLimits(t *testing.T) {
	for _, change := range []func(*domain.Recipe){
		func(r *domain.Recipe) { r.BatchLiters = 5 },
		func(r *domain.Recipe) { r.Ingredients = source().Ingredients },
		func(r *domain.Recipe) { r.NativeProgram = nil },
		func(r *domain.Recipe) { r.NativeProgram.Name = "Different" },
		func(r *domain.Recipe) { r.NativeProgram.Steps[0].Location = 0 },
		func(r *domain.Recipe) { r.NativeProgram.Steps[0].Minutes = -1 },
	} {
		r := archivedProgram()
		change(&r)
		if err := recipe.Validate(r); err == nil {
			t.Fatalf("invalid native library record accepted: %+v", r)
		}
	}
	r := archivedProgram()
	r.Name = strings.Repeat("Long source label ", 3)
	r.NativeProgram.Name = r.Name
	if err := recipe.Validate(r); err != nil {
		t.Fatal("source label should remain editable", err)
	}
	if _, err := recipe.Adapt(r, recipe.Profiles()[0], 0); err == nil {
		t.Fatal("overlong native label silently truncated")
	}
	r = archivedProgram()
	r.SourceModel = "zymatic"
	if _, err := recipe.Adapt(r, recipe.Profiles()[0], 0); err == nil {
		t.Fatal("Zymatic routing sent to Pico")
	}
	p := recipe.Profiles()[4]
	p.ControlSupported = true
	a, err := recipe.Adapt(r, p, 0)
	if err != nil || a.CanDeploy || a.MachineRecipe != nil {
		t.Fatal("Zymatic source acquired an unsupported live adapter", err)
	}
	r = archivedProgram()
	r.Kind = "maintenance"
	if _, err := recipe.Adapt(r, recipe.Profiles()[0], 0); err != nil {
		t.Fatal("valid maintenance program could not use normal capacity-checked deployment", err)
	}
}

func TestCanonicalZeroMinuteHeatingRemainsAHeatingStep(t *testing.T) {
	r := source()
	r.Steps[0].Kind, r.Steps[0].Minutes = "heat", 0
	a, err := recipe.Adapt(r, recipe.Profiles()[0], 0)
	if err != nil || a.MachineRecipe.Steps[0].Minutes != 0 || a.MachineRecipe.Steps[0].TargetF != 149 {
		t.Fatalf("zero-minute heat changed: %+v %v", a, err)
	}
	r.Steps[0].Kind = "mash"
	if err := recipe.Validate(r); err == nil {
		t.Fatal("zero-duration mash silently accepted as heating")
	}
}
