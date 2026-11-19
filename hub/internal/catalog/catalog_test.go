package catalog

import (
	"encoding/json"
	"errors"
	"reflect"
	"strings"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

func TestPinnedArchiveIntegrityAndCopyCoverage(t *testing.T) {
	a, err := Open()
	if err != nil {
		t.Fatal(err)
	}
	if len(a.entries) != 5282 {
		t.Fatalf("unexpected catalogue size: %d", len(a.entries))
	}
	kinds := map[string]int{}
	copyable, deployable, originalRecords := 0, 0, 0
	for _, summary := range a.entries {
		detail, err := a.Detail(summary.ID)
		if err != nil {
			t.Fatal(err)
		}
		if !reflect.DeepEqual(summary.Summary, detail.Summary) {
			t.Fatalf("index/detail mismatch: %s", summary.ID)
		}
		kinds[detail.Kind]++
		for _, record := range detail.SourceRecords {
			data, got, err := a.Original(detail.ID, record.ID)
			if err != nil || got != record || !json.Valid(data) {
				t.Fatalf("invalid original %s/%s: %v", detail.ID, record.ID, err)
			}
			originalRecords++
		}
		if !detail.CanCopy {
			if detail.Recipe != nil || detail.CanDeploy {
				t.Fatalf("reference-only entry became executable: %s", detail.ID)
			}
			continue
		}
		copyable++
		if detail.Recipe == nil {
			t.Fatalf("copyable entry has no recipe: %s", detail.ID)
		}
		if err := recipe.Validate(*detail.Recipe); err != nil {
			t.Fatalf("invalid copied recipe %s: %v", detail.ID, err)
		}
		if detail.Recipe.Provenance == nil || !reflect.DeepEqual(*detail.Recipe.Provenance, detail.Provenance) {
			t.Fatalf("provenance mismatch: %s", detail.ID)
		}
		if recipe.IsNative(*detail.Recipe) {
			if detail.Recipe.BatchLiters != 0 || detail.Recipe.Efficiency != 0 || len(detail.Recipe.Ingredients) != 0 || len(detail.Recipe.Steps) != 0 || !reflect.DeepEqual(detail.NativeProgram, detail.Recipe.NativeProgram) {
				t.Fatalf("invented plan for native program %s", detail.ID)
			}
		}
		if detail.CanDeploy {
			deployable++
			if detail.NativeProgram == nil {
				t.Fatalf("deployable source has no program: %s", detail.ID)
			}
			if _, err := recipe.NativeJSON(*detail.NativeProgram); err != nil {
				t.Fatalf("invalid advertised deployment %s: %v", detail.ID, err)
			}
		}
	}
	if copyable != 5272 || deployable != 3755 || originalRecords != 11164 {
		t.Fatalf("coverage changed: %d copyable, %d deployable, %d originals", copyable, deployable, originalRecords)
	}
	if !reflect.DeepEqual(kinds, map[string]int{"recipe": 1320, "native-program": 3959, "maintenance": 3}) {
		t.Fatalf("kinds: %v", kinds)
	}
	for _, asset := range a.meta.Assets {
		if !asset.Bundled {
			continue
		}
		if _, _, err := a.AssetData(asset.Path); err != nil {
			t.Fatalf("asset %s: %v", asset.Path, err)
		}
	}
	var excluded []struct {
		SourceID string `json:"source_id"`
		Path     string `json:"path"`
		SHA256   string `json:"sha256"`
		Bytes    int64  `json:"bytes"`
	}
	if err := a.decode("excluded.json", &excluded); err != nil {
		t.Fatal(err)
	}
	if len(excluded) != 354 {
		t.Fatalf("excluded count %d", len(excluded))
	}
	for _, record := range excluded {
		data, err := a.read("originals/" + record.SourceID + "/" + record.Path)
		if err != nil || Verify(data, record.Bytes, record.SHA256) != nil {
			t.Fatalf("excluded original lost: %s", record.Path)
		}
	}
}

func TestNativeSchedulesPreserveSourceInstructions(t *testing.T) {
	a, err := Open()
	if err != nil {
		t.Fatal(err)
	}
	for _, item := range a.entries {
		if item.SourceID != "picopak-library" {
			continue
		}
		detail, err := a.Detail(item.ID)
		if err != nil {
			t.Fatal(err)
		}
		data, _, err := a.Original(item.ID, "")
		if err != nil {
			t.Fatal(err)
		}
		var source struct {
			Name  string
			Steps []struct {
				Name                        string
				Location, Temp, Time, Drain int
			}
		}
		if err := json.Unmarshal(data, &source); err != nil {
			t.Fatal(err)
		}
		program := detail.NativeProgram
		if program == nil || program.Name != source.Name || len(program.Steps) != len(source.Steps) {
			t.Fatalf("native schedule replaced: %s", item.ID)
		}
		for i, step := range source.Steps {
			got := program.Steps[i]
			if step.Name != got.Name || step.Location != got.Location || step.Temp != got.TargetF || step.Time != got.Minutes || step.Drain != got.DrainMinutes {
				t.Fatalf("native step changed: %s step %d", item.ID, i)
			}
		}
	}
}

func TestSearchPaginationAndIndependentResults(t *testing.T) {
	a, err := Open()
	if err != nil {
		t.Fatal(err)
	}
	first := a.List(Query{Limit: 2})
	next := a.List(Query{Offset: 2, Limit: 2})
	if first.Total != 5282 || len(first.Items) != 2 || first.Items[0].ID == next.Items[0].ID || !first.Items[0].Featured {
		t.Fatalf("bad pages: %+v %+v", first, next)
	}
	for _, query := range []Query{{Query: "DEAD GUY"}, {Source: "public-recipes", Kind: "recipe", Model: "zymatic"}, {Kind: "maintenance", Model: "zymatic"}} {
		page := a.List(query)
		if page.Total == 0 {
			t.Fatalf("no matches for %+v", query)
		}
		for _, item := range page.Items {
			if query.Kind != "" && item.Kind != query.Kind || query.Source != "" && item.SourceID != query.Source {
				t.Fatalf("filter mismatch: %+v", item)
			}
		}
	}
	if a.List(Query{Kind: "maintenance", Model: "pico-pro"}).Total != 0 {
		t.Fatal("Zymatic maintenance advertised for Pico")
	}
	first.Items[0].Models[0] = "corrupt"
	first.Sources[1].ExcludedReasons["corrupt"] = 1
	again := a.List(Query{Limit: 2})
	if again.Items[0].Models[0] == "corrupt" || again.Sources[1].ExcludedReasons["corrupt"] != 0 {
		t.Fatal("caller mutated immutable metadata")
	}
	detail, _ := a.Detail("picopak-64")
	detail.Recipe.NativeProgram.Steps[0].Minutes = 17
	fresh, _ := a.Detail("picopak-64")
	if fresh.Recipe.NativeProgram.Steps[0].Minutes == 17 {
		t.Fatal("caller mutated source program")
	}
	if _, _, err := a.Original("picopak-64", "../../manifest.json"); !errors.Is(err, ErrNotFound) {
		t.Fatal("arbitrary source path accepted", err)
	}
	if _, _, err := a.AssetData("../manifest.json"); !errors.Is(err, ErrNotFound) {
		t.Fatal("arbitrary asset path accepted", err)
	}
	if _, err := a.Detail("../../manifest"); !errors.Is(err, ErrNotFound) {
		t.Fatal("arbitrary detail path accepted", err)
	}
	for _, item := range a.List(Query{Source: "public-recipes", Limit: 100}).Items {
		if item.ImageURL != "" && !strings.HasPrefix(item.ImageURL, "/api/v1/catalog/assets/") {
			t.Fatal("remote image dependency", item.ID)
		}
	}
}
