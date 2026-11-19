package server

import (
	"encoding/json"
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
