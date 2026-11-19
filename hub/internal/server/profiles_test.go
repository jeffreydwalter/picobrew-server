package server

import (
	"bytes"
	"encoding/json"
	"reflect"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

func TestCalibrationAffectsAdaptationWithoutChangingNativeCapabilities(t *testing.T) {
	_, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	status, p := request(t, c, "POST", h.URL+"/api/v1/recipes", csrf, validRecipe(), nil)
	if status != 201 {
		t.Fatalf("recipe: %d %s", status, p)
	}
	var source domain.Recipe
	_ = json.Unmarshal(p, &source)
	status, p = request(t, c, "PUT", h.URL+"/api/v1/profiles/pico-pro", csrf, map[string]any{"batch_liters": 5, "efficiency": 55, "max_grain_kg": 2}, nil)
	if status != 200 {
		t.Fatalf("calibrate: %d %s", status, p)
	}
	var profile domain.Profile
	_ = json.Unmarshal(p, &profile)
	if profile.MaxSteps != 15 || !profile.ControlSupported || profile.MaxTargetC != 100 {
		t.Fatal("calibration changed native capabilities")
	}
	if !profile.UserConfigured || len(profile.Sources) == 0 {
		t.Fatal("saved calibration lost its provenance")
	}
	status, p = request(t, c, "POST", h.URL+"/api/v1/recipes/"+source.ID+"/adapt", csrf, map[string]any{"profile_id": "pico-pro", "batch_liters": 5}, nil)
	if status != 200 {
		t.Fatalf("adapt: %d %s", status, p)
	}
	var adapted domain.Adapted
	_ = json.Unmarshal(p, &adapted)
	if adapted.Recipe.Ingredients[0].Amount != source.Ingredients[0].Amount {
		t.Fatal("measured efficiency ignored")
	}
	status, _ = request(t, c, "PUT", h.URL+"/api/v1/profiles/pico-pro", csrf, map[string]any{"batch_liters": 5, "efficiency": 110, "max_grain_kg": 2}, nil)
	if status != 422 {
		t.Fatal("invalid efficiency accepted")
	}
	status, p = request(t, c, "DELETE", h.URL+"/api/v1/profiles/pico-pro", csrf, nil, nil)
	if status != 200 {
		t.Fatal(status, string(p))
	}
}

func TestPublishedProfileDefaultsPreserveExistingCalibrationAndRecipes(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	// Existing installations store only these numbers. Adding published defaults
	// must not replace an older saved calibration or require new metadata.
	legacy := profileCalibration{BatchLiters: 4.8, Efficiency: 70, MaxGrainKG: 1.5}
	if _, err := s.db.Put("profile", "pico-pro", legacy, 0); err != nil {
		t.Fatal(err)
	}
	status, recipeBefore := request(t, c, "POST", h.URL+"/api/v1/recipes", csrf, validRecipe(), nil)
	if status != 201 {
		t.Fatalf("recipe: %d %s", status, recipeBefore)
	}
	var savedRecipe domain.Recipe
	if err := json.Unmarshal(recipeBefore, &savedRecipe); err != nil {
		t.Fatal(err)
	}
	readProfiles := func() map[string]domain.Profile {
		t.Helper()
		status, p := request(t, c, "GET", h.URL+"/api/v1/profiles", csrf, nil, nil)
		if status != 200 {
			t.Fatalf("profiles: %d %s", status, p)
		}
		var values []domain.Profile
		if err := json.Unmarshal(p, &values); err != nil {
			t.Fatal(err)
		}
		out := make(map[string]domain.Profile, len(values))
		for _, v := range values {
			out[v.ID] = v
		}
		return out
	}
	profiles := readProfiles()
	pro := profiles["pico-pro"]
	if pro.BatchLiters != legacy.BatchLiters || pro.Efficiency != 70 || pro.MaxGrainKG != 1.5 || !pro.UserConfigured || len(pro.Sources) == 0 {
		t.Fatalf("existing calibration was replaced or source metadata lost: %+v", pro)
	}
	for _, id := range []string{"pico-s", "pico-c"} {
		p := profiles[id]
		if p.BatchLiters != 5 || p.Efficiency != 50 || p.MaxGrainKG != 0 || p.UserConfigured || len(p.Sources) == 0 {
			t.Fatalf("incorrect published starting point for %s: %+v", id, p)
		}
	}
	for _, id := range []string{"z", "zymatic"} {
		if profiles[id].Efficiency != 70 {
			t.Fatalf("unrelated %s defaults changed", id)
		}
	}
	// Provenance is server-generated and cannot be forged by a calibration PUT.
	for _, metadata := range []map[string]any{
		{"user_configured": false},
		{"sources": []domain.ProfileSource{{Label: "Fake", URL: "https://example.com"}}},
	} {
		metadata["batch_liters"] = 5
		metadata["efficiency"] = 80
		metadata["max_grain_kg"] = 2
		status, _ := request(t, c, "PUT", h.URL+"/api/v1/profiles/pico-pro", csrf, metadata, nil)
		if status != 400 {
			t.Fatalf("caller-supplied provenance accepted: %d", status)
		}
	}
	if after := readProfiles()["pico-pro"]; !reflect.DeepEqual(pro, after) {
		t.Fatalf("rejected metadata changed calibration: %+v", after)
	}
	status, p := request(t, c, "DELETE", h.URL+"/api/v1/profiles/pico-pro", csrf, nil, nil)
	if status != 200 {
		t.Fatalf("reset: %d %s", status, p)
	}
	var reset domain.Profile
	if err := json.Unmarshal(p, &reset); err != nil {
		t.Fatal(err)
	}
	if reset.Efficiency != 50 || reset.BatchLiters != 5 || reset.MaxGrainKG != 0 || reset.UserConfigured || !reflect.DeepEqual(reset.Sources, pro.Sources) {
		t.Fatalf("reset did not restore sourced defaults: %+v", reset)
	}
	if persisted := readProfiles()["pico-pro"]; !reflect.DeepEqual(reset, persisted) {
		t.Fatalf("reset response differs from persisted result: %+v", persisted)
	}
	status, recipeAfter := request(t, c, "GET", h.URL+"/api/v1/recipes/"+savedRecipe.ID, csrf, nil, nil)
	if status != 200 || !bytes.Equal(recipeBefore, recipeAfter) {
		t.Fatalf("profile defaults changed the stored recipe: %d %s", status, recipeAfter)
	}
}
