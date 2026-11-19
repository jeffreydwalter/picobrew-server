package server

import (
	"net/http"
	"reflect"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

type MachineRecipeUpdate struct {
	Recipe   domain.MachineRecipe `json:"recipe"`
	Expected domain.MachineRecipe `json:"expected"`
}

// updateMachineRecipe edits the real appliance slot the user opened. A stale
// editor cannot replace a change already observed on that slot. The firmware
// remains authoritative and checks its idle state again when accepting a write.
func (s *Server) updateMachineRecipe(w http.ResponseWriter, r *http.Request) {
	d, ok := s.runtime(w, r)
	if !ok {
		return
	}
	var in MachineRecipeUpdate
	if !s.decode(w, r, &in) {
		return
	}
	id := chi.URLParam(r, "recipe_id")
	if in.Recipe.ID != id || in.Expected.ID != id {
		s.fail(w, 400, "recipe_id", "Keep the installed recipe ID unchanged when editing its slot")
		return
	}
	if _, err := recipe.NativeJSON(in.Recipe); err != nil {
		s.fail(w, 400, "invalid_recipe", err.Error())
		return
	}
	d.commandMu.Lock()
	defer d.commandMu.Unlock()
	if !s.currentDevice(d) {
		s.fail(w, 409, "connection_changed", "The machine connection changed; reload before continuing")
		return
	}
	status, err := s.verifiedStatus(r.Context(), d)
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if status.Busy || status.Stopped || !status.Ready {
		s.fail(w, 409, "machine_not_idle", "Return the machine to its main menu before editing recipes")
		return
	}
	existing, err := d.adapter.Recipes(r.Context())
	if err != nil {
		s.deviceError(w, err)
		return
	}
	found := false
	for _, current := range existing {
		if current.ID != id {
			continue
		}
		found = true
		if !reflect.DeepEqual(current, in.Expected) {
			s.fail(w, 409, "recipe_changed", "This machine recipe changed after you opened it. Reload before saving.")
			return
		}
	}
	if !found {
		s.fail(w, 409, "recipe_changed", "This machine recipe was removed after you opened it. Reload before saving.")
		return
	}
	if reflect.DeepEqual(in.Recipe, in.Expected) {
		s.json(w, 200, map[string]any{"updated": false, "started": false, "recipe": in.Recipe})
		return
	}
	caps, err := d.adapter.Discover(r.Context())
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if len(in.Recipe.Steps) > caps.StepCapacity || hasZeroMinuteSteps(in.Recipe) && !caps.ZeroMinuteSteps {
		s.fail(w, 422, "unsupported_recipe", "The edited recipe exceeds this firmware's advertised step capabilities")
		return
	}
	receiptID := randomID()
	receipt := map[string]any{"device_id": chi.URLParam(r, "id"), "native": in.Recipe, "previous": in.Expected, "at": time.Now().UTC(), "state": "prepared", "kind": "installed-edit"}
	if _, err = s.db.Put("deployment", receiptID, receipt, 0); err != nil {
		s.internal(w, err)
		return
	}
	if err = d.adapter.PutRecipe(r.Context(), in.Recipe); err != nil {
		receipt["state"] = "unconfirmed"
		_, _ = s.db.Put("deployment", receiptID, receipt, -1)
		s.deviceError(w, err)
		return
	}
	receipt["state"] = "verified"
	if _, err = s.db.Put("deployment", receiptID, receipt, -1); err != nil {
		s.fail(w, 500, "receipt_error", "The recipe was verified on the machine but its receipt could not be saved. Reload the machine inventory before trying again.")
		return
	}
	s.json(w, 200, map[string]any{"updated": true, "started": false, "recipe": in.Recipe})
}
