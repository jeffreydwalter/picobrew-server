package recipe_test

import (
	"bytes"
	"encoding/json"
	"math"
	"reflect"
	"strings"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

func exchangeRecipe() domain.Recipe {
	r := source()
	r.Name = "Café & Ale"
	r.Type = "All Grain"
	r.BoilLiters = 12
	r.BoilMinutes = 30
	r.Style = "Pale Ale"
	r.Notes = "Preserve notes\nand timing."
	r.Ingredients[0].Type = "Grain"
	r.Ingredients[0].Yield = 80
	r.Ingredients[0].Color = 2.5
	r.Ingredients[1].Minutes = 12.5
	r.Ingredients[1].Amount = 22.75
	r.Ingredients[1].Form = "Pellet"
	r.Ingredients = append(r.Ingredients,
		domain.Ingredient{Name: "Dry yeast", Kind: "yeast", Type: "Ale", Form: "Dry", Amount: 11.5, Unit: "g", Use: "fermentation"},
		domain.Ingredient{Name: "Spice", Kind: "misc", Type: "Spice", Amount: 2.25, Unit: "g", Use: "boil", Minutes: 7.5},
		domain.Ingredient{Name: "Flavor extract", Kind: "misc", Type: "Flavor", Amount: 2.5, Unit: "mL", Use: "bottling", Minutes: 0},
		domain.Ingredient{Name: "Water", Kind: "water", Amount: 12, Unit: "L", Use: "mash"})
	r.Water = map[string]float64{"calcium": 50, "chloride": 70, "sulfate": 100, "ph": 5.4}
	r.Fermentation = []domain.FermentationStep{{Name: "Primary", TargetC: 18, Days: 7.5}, {Name: "Secondary", TargetC: 3, Days: 2}}
	r.Carbonation = domain.Carbonation{Method: "forced", Volumes: 2.4, TemperatureC: 4}
	r.Steps[0].Type = "Infusion"
	r.Steps[0].InfuseLiters = 5.5
	r.Steps[0].RampMinutes = 2.5
	return r
}

func TestBeerXMLRoundTripPreservesUnitsTimingAndPlanning(t *testing.T) {
	want := exchangeRecipe()
	data, err := recipe.Export(want, "beerxml")
	if err != nil {
		t.Fatal(err)
	}
	if !bytes.Contains(data, []byte("<AMOUNT>0.02275</AMOUNT>")) || !bytes.Contains(data, []byte("<TIME>12.5</TIME>")) {
		t.Fatalf("BeerXML kilograms/fractional minutes missing: %s", data)
	}
	recipes, err := recipe.Import(data, "xml")
	if err != nil {
		t.Fatal(err)
	}
	if len(recipes) != 1 {
		t.Fatal("recipe count")
	}
	got := recipes[0]
	if got.Name != want.Name || got.Notes != want.Notes || got.Type != want.Type || got.BatchLiters != want.BatchLiters || got.BoilLiters != want.BoilLiters || got.BoilMinutes != want.BoilMinutes {
		t.Fatalf("metadata changed: %+v", got)
	}
	if !reflect.DeepEqual(got.Ingredients, want.Ingredients) || !reflect.DeepEqual(got.Steps, want.Steps) || !reflect.DeepEqual(got.Water, want.Water) || !reflect.DeepEqual(got.Fermentation, want.Fermentation) || got.Carbonation != want.Carbonation {
		t.Fatalf("planning values changed\ngot %+v\nwant %+v", got, want)
	}
	plan, err := recipe.Adapt(got, recipe.Profiles()[4], 5)
	if err != nil {
		t.Fatal("planning adaptation should retain infusion details", err)
	}
	if plan.Recipe.Steps[0].InfuseLiters != 2.75 || plan.Recipe.Water["calcium"] != 50 || plan.Recipe.Fermentation[0].Days != 7.5 {
		t.Fatal("volume scaling changed concentration or fermentation timing")
	}
	if _, err := recipe.Adapt(got, recipe.Profiles()[0], 5); err == nil {
		t.Fatal("unmapped infusion was silently deployed")
	}
}

func TestExternalBeerXMLWithoutExtensionHasUnmappedLocations(t *testing.T) {
	r := exchangeRecipe()
	r.Steps[0].Type = "Temperature"
	r.Steps[0].InfuseLiters = 0
	r.Steps[0].RampMinutes = 0
	data, err := recipe.Export(r, "beerxml")
	if err != nil {
		t.Fatal(err)
	}
	start := bytes.Index(data, []byte("  <HUB_STEPS>"))
	end := bytes.Index(data, []byte("</HUB_STEPS>"))
	if start < 0 || end < 0 {
		t.Fatal("missing extension")
	}
	data = append(append([]byte{}, data[:start]...), data[end+len("</HUB_STEPS>"):]...)
	values, err := recipe.Import(data, "beerxml")
	if err != nil {
		t.Fatal(err)
	}
	for _, step := range values[0].Steps {
		if step.Location != 0 {
			t.Fatal("import invented chamber location")
		}
	}
	if _, err := recipe.Adapt(values[0], recipe.Profiles()[0], 5); err == nil {
		t.Fatal("unmapped external recipe is deployable")
	}
}

func TestJSONRoundTripAndAmbiguousInputs(t *testing.T) {
	want := exchangeRecipe()
	want.Description = "Detailed description"
	want.Tags = []string{"test"}
	data, err := recipe.Export(want, "json")
	if err != nil {
		t.Fatal(err)
	}
	got, err := recipe.Import(data, "json")
	if err != nil || !reflect.DeepEqual(got, []domain.Recipe{want}) {
		t.Fatalf("roundtrip %+v %v", got, err)
	}
	for _, bad := range [][]byte{[]byte(`{"name":"one","name":"two"}`), append(append([]byte{}, data...), data...), []byte(`null`), []byte(`[]`), bytes.Repeat([]byte(" "), recipe.MaxImportBytes+1)} {
		if _, err := recipe.Import(bad, "json"); err == nil {
			t.Fatal("ambiguous/empty/oversized input accepted")
		}
	}
	unknown := bytes.Replace(data, []byte(`"name":`), []byte(`"unexpected":true,"name":`), 1)
	if _, err := recipe.Import(unknown, "json"); err == nil {
		t.Fatal("unknown JSON field silently dropped")
	}
}

func TestBeerXMLRejectsDTDIncompleteAndUnrepresentableInputs(t *testing.T) {
	data, err := recipe.Export(exchangeRecipe(), "xml")
	if err != nil {
		t.Fatal(err)
	}
	cases := [][]byte{
		[]byte(`<!DOCTYPE RECIPES [<!ENTITY x SYSTEM "file:///etc/passwd">]><RECIPES>&x;</RECIPES>`),
		[]byte(`<RECIPES><RECIPE><NAME>Incomplete</NAME><VERSION>1</VERSION></RECIPE></RECIPES>`),
		bytes.Replace(data, []byte("<BATCH_SIZE>10</BATCH_SIZE>"), []byte("<BATCH_SIZE>NaN</BATCH_SIZE>"), 1),
		bytes.Replace(data, []byte("<STEP_TIME>60</STEP_TIME>"), []byte("<STEP_TIME>60.5</STEP_TIME>"), 1),
		append(append([]byte{}, data...), []byte(`<RECIPES></RECIPES>`)...),
		[]byte(`<RECIPES></RECIPES>`),
	}
	for i, bad := range cases {
		if _, err := recipe.Import(bad, "beerxml"); err == nil {
			t.Fatalf("invalid XML case %d accepted", i)
		}
	}
	r := exchangeRecipe()
	r.Type = ""
	if _, err := recipe.Export(r, "beerxml"); err == nil {
		t.Fatal("recipe type was guessed")
	}
	r = exchangeRecipe()
	r.BoilLiters = 0
	if _, err := recipe.Export(r, "beerxml"); err == nil {
		t.Fatal("boil volume was guessed")
	}
	r = exchangeRecipe()
	r.Ingredients[2].Unit = "count"
	if _, err := recipe.Export(r, "beerxml"); err == nil {
		t.Fatal("yeast packet mass was guessed")
	}
	r = exchangeRecipe()
	r.Water["calcium"] = math.NaN()
	if err := recipe.Validate(r); err == nil {
		t.Fatal("nonfinite water metadata accepted")
	}
}

func TestISO88591BeerXMLAndExtractScaling(t *testing.T) {
	r := exchangeRecipe()
	data, err := recipe.Export(r, "xml")
	if err != nil {
		t.Fatal(err)
	}
	data = bytes.Replace(data, []byte(`encoding="UTF-8"`), []byte(`encoding="ISO-8859-1"`), 1)
	data = bytes.ReplaceAll(data, []byte("é"), []byte{0xe9})
	values, err := recipe.Import(data, "xml")
	if err != nil || values[0].Name != r.Name {
		t.Fatalf("Latin-1: %+v %v", values, err)
	}
	r = source()
	r.Ingredients[0].Type = "Extract"
	p := recipe.Profiles()[0]
	p.Efficiency = 25
	plan, err := recipe.Adapt(r, p, 5)
	if err != nil {
		t.Fatal(err)
	}
	if plan.Recipe.Ingredients[0].Amount != 1 {
		t.Fatal("extract should scale by volume without mash efficiency penalty")
	}
}

func TestRecipePlanPreservesWholeInputInJSON(t *testing.T) {
	r := exchangeRecipe()
	encoded, _ := json.Marshal([]domain.Recipe{r, r})
	got, err := recipe.Import(encoded, "application/json")
	if err != nil || len(got) != 2 {
		t.Fatalf("array import: %v", err)
	}
	if _, err := recipe.Import([]byte(strings.Repeat("[", 65)+"0"+strings.Repeat("]", 65)), "json"); err == nil {
		t.Fatal("excessive nesting accepted")
	}
}
