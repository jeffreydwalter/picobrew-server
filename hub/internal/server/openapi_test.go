package server

import (
	"encoding/json"
	"net/http"
	"reflect"
	"sort"
	"strings"
	"testing"

	"github.com/go-chi/chi/v5"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

func TestOpenAPICatalogMatchesEveryRegisteredAPIRoute(t *testing.T) {
	s := &Server{}
	router := s.routes().(chi.Routes)
	actual := map[string]bool{}
	if err := chi.Walk(router, func(method, route string, _ http.Handler, _ ...func(http.Handler) http.Handler) error {
		if strings.HasPrefix(route, "/api/v1/") {
			actual[method+" "+strings.TrimPrefix(route, "/api/v1")] = true
		}
		return nil
	}); err != nil {
		t.Fatal(err)
	}
	document := openAPIDocument()
	documented := map[string]bool{}
	for path, item := range document["paths"].(map[string]any) {
		for method := range item.(map[string]any) {
			documented[strings.ToUpper(method)+" "+path] = true
		}
	}
	var missing, extra []string
	for route := range actual {
		if !documented[route] {
			missing = append(missing, route)
		}
	}
	for route := range documented {
		if !actual[route] {
			extra = append(extra, route)
		}
	}
	sort.Strings(missing)
	sort.Strings(extra)
	if len(missing) > 0 || len(extra) > 0 {
		t.Fatalf("API catalog differs from router\nmissing: %v\nextra: %v", missing, extra)
	}
}

func TestOpenAPIResponseStatusesAndQueries(t *testing.T) {
	paths := openAPIDocument()["paths"].(map[string]any)
	for _, test := range []struct {
		method, path string
		codes        []string
	}{
		{"post", "/devices", []string{"201", "400", "401", "403", "413", "415"}},
		{"post", "/recipes", []string{"201"}},
		{"post", "/recipes/import", []string{"201"}},
		{"post", "/devices/{id}/commands", []string{"200", "202", "409", "502"}},
		{"post", "/telemetry", []string{"202", "403"}},
		{"post", "/auth/logout", []string{"204"}},
		{"delete", "/recipes/{id}", []string{"204"}},
		{"delete", "/devices/{id}/recipes/{recipe_id}", []string{"204"}},
		{"put", "/profiles/{id}", []string{"200", "422"}},
		{"post", "/ai/assist", []string{"409", "429", "502"}},
	} {
		op := paths[test.path].(map[string]any)[test.method].(map[string]any)
		responses := op["responses"].(schemaMap)
		for _, code := range test.codes {
			if _, ok := responses[code]; !ok {
				t.Errorf("%s %s lacks status %s", test.method, test.path, code)
			}
		}
		if response, ok := responses["204"].(schemaMap); ok {
			if _, body := response["content"]; body {
				t.Fatal("204 response advertises a body")
			}
		}
	}
	for path, name := range map[string]string{"/ai/advice": "device_id", "/shopping-list": "recipes", "/recipes/{id}/export": "format", "/brews/{id}/export": "format"} {
		params := paths[path].(map[string]any)["get"].(map[string]any)["parameters"].([]any)
		found := false
		for _, value := range params {
			p := value.(schemaMap)
			if p["name"] == name && p["in"] == "query" {
				found = true
			}
		}
		if !found {
			t.Errorf("%s lacks query %s", path, name)
		}
	}
	params := paths["/devices/{id}/recipes/{recipe_id}"].(map[string]any)["delete"].(map[string]any)["parameters"].([]any)
	if len(params) != 2 || params[0].(schemaMap)["name"] != "id" || params[1].(schemaMap)["name"] != "recipe_id" {
		t.Fatalf("nested path variables: %#v", params)
	}
}

func TestOpenAPIReferencesResolveAndExamplesStayUsable(t *testing.T) {
	document := openAPIDocument()
	bytes, err := json.Marshal(document)
	if err != nil {
		t.Fatal(err)
	}
	var decoded map[string]any
	if err := json.Unmarshal(bytes, &decoded); err != nil {
		t.Fatal(err)
	}
	schemas := decoded["components"].(map[string]any)["schemas"].(map[string]any)
	var walk func(any)
	walk = func(value any) {
		switch v := value.(type) {
		case map[string]any:
			if ref, ok := v["$ref"].(string); ok {
				prefix := "#/components/schemas/"
				if !strings.HasPrefix(ref, prefix) || schemas[strings.TrimPrefix(ref, prefix)] == nil {
					t.Errorf("unresolved schema %q", ref)
				}
			}
			for _, child := range v {
				walk(child)
			}
		case []any:
			for _, child := range v {
				walk(child)
			}
		}
	}
	walk(decoded)
	for _, name := range []string{"DeviceView", "Recipe", "Ingredient", "MachineRecipe", "Profile", "Adapted", "Sample", "BrewDetail", "Error", "adviceRecord"} {
		if schemas[name] == nil {
			t.Errorf("missing domain schema %s", name)
		}
	}
	detail := schemas["BrewDetail"].(map[string]any)["properties"].(map[string]any)
	if detail["notes"] == nil || detail["samples"] == nil || detail["device_id"] == nil {
		t.Fatal("embedded brew fields not flattened")
	}
	paths := decoded["paths"].(map[string]any)
	for _, test := range []struct{ path, method string }{{"/recipes", "post"}, {"/recipes/{id}", "put"}} {
		example := paths[test.path].(map[string]any)[test.method].(map[string]any)["requestBody"].(map[string]any)["content"].(map[string]any)["application/json"].(map[string]any)["example"]
		data, _ := json.Marshal(example)
		var r domain.Recipe
		if err := json.Unmarshal(data, &r); err != nil {
			t.Fatal(err)
		}
		if err := recipe.Validate(r); err != nil {
			t.Fatalf("%s example is invalid: %v", test.path, err)
		}
	}
	request := paths["/recipes/import"].(map[string]any)["post"].(map[string]any)["requestBody"].(map[string]any)
	example := request["content"].(map[string]any)["application/json"].(map[string]any)["example"].(map[string]any)
	if _, err := recipe.Import([]byte(example["data"].(string)), example["format"].(string)); err != nil {
		t.Fatalf("import example is unusable: %v", err)
	}
}

func TestOpenAPISecurityAndNonJSONResponses(t *testing.T) {
	paths := openAPIDocument()["paths"].(map[string]any)
	for _, path := range []string{"/auth/setup", "/auth/login", "/telemetry"} {
		op := paths[path].(map[string]any)["post"].(map[string]any)
		if len(op["security"].([]any)) != 0 {
			t.Fatalf("public endpoint %s requires a session", path)
		}
	}
	mutation := paths["/devices/{id}/commands"].(map[string]any)["post"].(map[string]any)
	security := mutation["security"].([]any)[0].(map[string]any)
	if _, ok := security["session"]; !ok {
		t.Fatal("session scheme absent")
	}
	if _, ok := security["csrf"]; !ok {
		t.Fatal("CSRF scheme absent")
	}
	for path, media := range map[string]string{"/events": "text/event-stream", "/backup": "application/zip", "/brews/{id}/export": "text/csv", "/recipes/{id}/export": "application/xml"} {
		response := paths[path].(map[string]any)["get"].(map[string]any)["responses"].(schemaMap)["200"].(schemaMap)
		if response["content"].(schemaMap)[media] == nil {
			t.Errorf("%s lacks %s", path, media)
		}
	}
	// UI examples keep the established shape while no-body actions stay optional.
	body := paths["/auth/logout"].(map[string]any)["post"].(map[string]any)["requestBody"].(map[string]any)
	if !reflect.DeepEqual(body["required"], false) {
		t.Fatal("logout incorrectly requires JSON")
	}
}
