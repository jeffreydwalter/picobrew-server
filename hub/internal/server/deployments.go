package server

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"errors"
	"net/http"
	"reflect"
	"sort"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/jeffreydwalter/picobrew-hub/internal/device"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

// Bulk transfers always have a persisted preview and a receipt. They never
// delete recipes or start a program. A transport failure stops the batch; an
// uncertain write is not retried, even after a Hub restart.
type DeploymentSelection struct {
	RecipeID       string  `json:"recipe_id"`
	SourceRevision int64   `json:"source_revision"`
	BatchLiters    float64 `json:"batch_liters"`
}

type DeploymentInput struct {
	ProfileID         string                `json:"profile_id"`
	OverwriteExisting bool                  `json:"overwrite_existing"`
	Items             []DeploymentSelection `json:"items"`
}

type DeploymentItem struct {
	DeploymentSelection
	Name     string                `json:"name"`
	Status   string                `json:"status"`
	Reason   string                `json:"reason,omitempty"`
	Update   bool                  `json:"update"`
	Warnings []string              `json:"warnings"`
	Native   *domain.MachineRecipe `json:"native,omitempty"`
	Previous *domain.MachineRecipe `json:"previous,omitempty"`
}

type BulkDeployment struct {
	ID                   string           `json:"id"`
	DeviceID             string           `json:"device_id"`
	DeviceUID            string           `json:"device_uid"`
	ProfileID            string           `json:"profile_id"`
	State                string           `json:"state"`
	CreatedAt            time.Time        `json:"created_at"`
	ExpiresAt            time.Time        `json:"expires_at"`
	Capacity             int              `json:"capacity"`
	Used                 int              `json:"used"`
	Available            int              `json:"available"`
	PlannedCount         int              `json:"planned_count"`
	NewCount             int              `json:"new_count"`
	UpdateCount          int              `json:"update_count"`
	VerifiedCount        int              `json:"verified_count"`
	Started              bool             `json:"started"`
	InventoryFingerprint string           `json:"inventory_sha256"`
	ProfileFingerprint   string           `json:"profile_sha256"`
	Items                []DeploymentItem `json:"items"`
}

func profileFingerprint(profile domain.Profile) string {
	b, _ := json.Marshal(profile)
	sum := sha256.Sum256(b)
	return hex.EncodeToString(sum[:])
}

// A persisted sending state can outlive the HTTP request or the Hub process.
// Expose that uncertainty after restart instead of leaving a receipt running
// forever or automatically repeating a write that may already have succeeded.
func (s *Server) recoverDeployments() error {
	rows, err := s.db.List("batch-deployment")
	if err != nil {
		return err
	}
	for _, row := range rows {
		var plan BulkDeployment
		if err := json.Unmarshal(row, &plan); err != nil {
			return err
		}
		if plan.State != "running" {
			continue
		}
		plan.State = "interrupted"
		for i := range plan.Items {
			item := &plan.Items[i]
			switch item.Status {
			case "sending":
				item.Status = "unconfirmed"
				item.Reason = "The Hub restarted before this write could be confirmed. Refresh the machine inventory before preparing another transfer."
			case "ready":
				item.Status = "not_sent"
				item.Reason = "The Hub restarted before this recipe was sent"
			}
		}
		if _, err := s.db.Put("batch-deployment", plan.ID, plan, -1); err != nil {
			return err
		}
	}
	return nil
}

func inventoryFingerprint(items []domain.MachineRecipe) string {
	ordered := append([]domain.MachineRecipe(nil), items...)
	sort.Slice(ordered, func(i, j int) bool { return ordered[i].ID < ordered[j].ID })
	b, _ := json.Marshal(ordered)
	sum := sha256.Sum256(b)
	return hex.EncodeToString(sum[:])
}

func hasZeroMinuteSteps(v domain.MachineRecipe) bool {
	for _, step := range v.Steps {
		if step.Minutes == 0 {
			return true
		}
	}
	return false
}

func (s *Server) planDeployment(w http.ResponseWriter, r *http.Request) {
	d, ok := s.runtime(w, r)
	if !ok {
		return
	}
	var in DeploymentInput
	if !s.decode(w, r, &in) {
		return
	}
	if len(in.Items) < 1 || len(in.Items) > 1000 {
		s.fail(w, 400, "selection_count", "Choose between 1 and 1000 library recipes or programs")
		return
	}
	d.commandMu.Lock()
	defer d.commandMu.Unlock()
	if !s.currentDevice(d) {
		s.fail(w, 409, "connection_changed", "The machine connection changed; reload before continuing")
		return
	}
	d.mu.RLock()
	view := d.view
	d.mu.RUnlock()
	if in.ProfileID == "" {
		in.ProfileID = view.Model
	}
	if in.ProfileID != view.Model {
		s.fail(w, 422, "profile_mismatch", "Choose the profile that matches the target machine")
		return
	}
	profile, ok := s.selectProfile(in.ProfileID)
	if !ok {
		s.fail(w, 400, "profile", "Unknown machine profile")
		return
	}
	status, err := s.verifiedStatus(r.Context(), d)
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if status.Busy || status.Stopped || !status.Ready {
		s.fail(w, 409, "machine_not_idle", "Return the machine to its ready main menu before preparing a transfer")
		return
	}
	caps, err := d.adapter.Discover(r.Context())
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if caps.RecipeCapacity < 1 || caps.RecipeCapacity > 1024 || caps.StepCapacity < 1 {
		s.fail(w, 422, "recipe_storage_unavailable", "The machine did not advertise usable recipe storage")
		return
	}
	existing, err := d.adapter.Recipes(r.Context())
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if len(existing) > caps.RecipeCapacity {
		s.fail(w, 502, "invalid_inventory", "The machine inventory exceeds its advertised recipe capacity")
		return
	}
	byID := make(map[string]domain.MachineRecipe, len(existing))
	for _, v := range existing {
		if _, duplicate := byID[v.ID]; duplicate {
			s.fail(w, 502, "invalid_inventory", "The machine returned duplicate recipe identifiers")
			return
		}
		byID[v.ID] = v
	}
	now := time.Now().UTC()
	plan := BulkDeployment{ID: randomID(), DeviceID: view.ID, DeviceUID: status.DeviceUID,
		ProfileID: in.ProfileID, State: "planned", CreatedAt: now, ExpiresAt: now.Add(15 * time.Minute),
		Capacity: caps.RecipeCapacity, Used: len(existing), Available: caps.RecipeCapacity - len(existing),
		InventoryFingerprint: inventoryFingerprint(existing), ProfileFingerprint: profileFingerprint(profile), Items: make([]DeploymentItem, 0, len(in.Items))}
	seen := map[string]bool{}
	for _, selection := range in.Items {
		item := DeploymentItem{DeploymentSelection: selection, Status: "skipped", Warnings: []string{}}
		var source domain.Recipe
		if _, err := s.db.Get("recipe", selection.RecipeID, &source); err != nil {
			item.Reason = "Library recipe is unavailable"
		} else if selection.SourceRevision != source.Revision {
			item.Name = source.Name
			item.Reason = "Recipe changed; reload its current revision"
		} else {
			item.Name = source.Name
			adapted, err := recipe.Adapt(source, profile, selection.BatchLiters)
			if err != nil {
				item.Reason = safeMessage(err.Error())
			} else if !adapted.CanDeploy || adapted.MachineRecipe == nil {
				item.Reason = "This recipe and model combination supports planning only"
				item.Warnings = adapted.Warnings
			} else {
				native := *adapted.MachineRecipe
				item.Native, item.Warnings = &native, adapted.Warnings
				previous, exists := byID[native.ID]
				switch {
				case seen[native.ID]:
					item.Reason = "Another selected recipe uses this machine recipe ID"
				case len(native.Steps) > caps.StepCapacity:
					item.Reason = "Recipe exceeds the machine's step capacity"
				case hasZeroMinuteSteps(native) && !caps.ZeroMinuteSteps:
					item.Reason = "Update the machine firmware to support zero-minute heating steps"
				case exists && reflect.DeepEqual(previous, native):
					item.Reason = "The same recipe is already on this machine"
				case exists && !in.OverwriteExisting:
					item.Reason = "This ID already exists; enable updating matching recipes to replace it"
				case !exists && plan.NewCount >= plan.Available:
					item.Reason = "No recipe slots remain"
				default:
					item.Status = "ready"
					item.Update = exists
					seen[native.ID] = true
					plan.PlannedCount++
					if exists {
						item.Previous = &previous
						plan.UpdateCount++
					} else {
						plan.NewCount++
					}
				}
			}
		}
		plan.Items = append(plan.Items, item)
	}
	if _, err := s.db.Put("batch-deployment", plan.ID, plan, 0); err != nil {
		s.internal(w, err)
		return
	}
	s.json(w, 201, plan)
}

func (s *Server) getDeployment(w http.ResponseWriter, r *http.Request) {
	var plan BulkDeployment
	if _, err := s.db.Get("batch-deployment", chi.URLParam(r, "deployment_id"), &plan); err != nil {
		s.storeError(w, err)
		return
	}
	if plan.DeviceID != chi.URLParam(r, "id") {
		s.fail(w, 404, "not_found", "Transfer not found")
		return
	}
	s.json(w, 200, plan)
}

func (s *Server) executeDeployment(w http.ResponseWriter, r *http.Request) {
	d, ok := s.runtime(w, r)
	if !ok {
		return
	}
	d.commandMu.Lock()
	defer d.commandMu.Unlock()
	var plan BulkDeployment
	version, err := s.db.Get("batch-deployment", chi.URLParam(r, "deployment_id"), &plan)
	if err != nil {
		s.storeError(w, err)
		return
	}
	if plan.DeviceID != chi.URLParam(r, "id") {
		s.fail(w, 404, "not_found", "Transfer not found")
		return
	}
	if plan.State == "complete" {
		s.json(w, 200, plan)
		return
	}
	if plan.State != "planned" {
		s.fail(w, 409, "transfer_already_attempted", "This transfer was already attempted. Read its receipt and prepare a fresh preview from the machine's current inventory.")
		return
	}
	if !s.currentDevice(d) || time.Now().After(plan.ExpiresAt) {
		s.fail(w, 409, "preview_expired", "The connection changed or the preview expired; prepare it again")
		return
	}
	d.mu.RLock()
	model := d.view.Model
	d.mu.RUnlock()
	status, err := s.verifiedStatus(r.Context(), d)
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if model != plan.ProfileID || status.DeviceUID != plan.DeviceUID {
		s.fail(w, 409, "connection_changed", "The target machine changed; prepare the transfer again")
		return
	}
	if status.Busy || status.Stopped || !status.Ready {
		s.fail(w, 409, "machine_not_idle", "Return the machine to its ready main menu before sending recipes")
		return
	}
	profile, known := s.selectProfile(plan.ProfileID)
	if !known || profileFingerprint(profile) != plan.ProfileFingerprint {
		s.fail(w, 409, "profile_changed", "The machine profile changed after this preview; prepare it again")
		return
	}
	caps, err := d.adapter.Discover(r.Context())
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if caps.RecipeCapacity < plan.Used+plan.NewCount || caps.StepCapacity < 1 {
		s.fail(w, 409, "capabilities_changed", "The machine's recipe capacity changed after this preview; prepare it again")
		return
	}
	existing, err := d.adapter.Recipes(r.Context())
	if err != nil {
		s.deviceError(w, err)
		return
	}
	if inventoryFingerprint(existing) != plan.InventoryFingerprint {
		s.fail(w, 409, "inventory_changed", "Recipes on the machine changed after this preview; prepare it again")
		return
	}
	for _, item := range plan.Items {
		if item.Status != "ready" {
			continue
		}
		if item.Native == nil || len(item.Native.Steps) > caps.StepCapacity || hasZeroMinuteSteps(*item.Native) && !caps.ZeroMinuteSteps {
			s.fail(w, 409, "capabilities_changed", "The machine's step capabilities changed after this preview; prepare it again")
			return
		}
		var source domain.Recipe
		if _, err := s.db.Get("recipe", item.RecipeID, &source); err != nil || source.Revision != item.SourceRevision {
			s.fail(w, 409, "revision_conflict", "A selected recipe changed after this preview; prepare it again")
			return
		}
	}
	plan.State = "running"
	version, err = s.db.Put("batch-deployment", plan.ID, plan, version)
	if err != nil {
		s.storeError(w, err)
		return
	}
	for i := range plan.Items {
		item := &plan.Items[i]
		if item.Status != "ready" {
			continue
		}
		// Physical controls or another client can start a program between writes.
		status, err = s.verifiedStatus(r.Context(), d)
		if err != nil || status.Busy || status.Stopped || !status.Ready {
			item.Status = "not_sent"
			item.Reason = "The machine is no longer confirmed ready; the remaining transfer was stopped"
			plan.State = "partial"
			break
		}
		item.Status = "sending"
		version, err = s.db.Put("batch-deployment", plan.ID, plan, version)
		if err != nil {
			s.fail(w, 500, "receipt_error", "Transfer stopped because its receipt could not be saved. Inspect the current machine inventory before trying again.")
			return
		}
		if err = d.adapter.PutRecipe(r.Context(), *item.Native); err != nil {
			item.Status = "unconfirmed"
			item.Reason = "The recipe write was not verified. Refresh the machine inventory before preparing another transfer."
			var failure *device.Error
			if errors.As(err, &failure) && !failure.UnknownOutcome {
				item.Status = "not_sent"
				item.Reason = safeMessage(failure.Message)
			}
			plan.State = "partial"
			break
		}
		item.Status = "verified"
		plan.VerifiedCount++
		version, err = s.db.Put("batch-deployment", plan.ID, plan, version)
		if err != nil {
			s.fail(w, 500, "receipt_error", "A recipe was verified on the machine, but its receipt could not be saved. Inspect the machine before trying again.")
			return
		}
	}
	if plan.State == "running" {
		plan.State = "complete"
	} else {
		for i := range plan.Items {
			if plan.Items[i].Status == "ready" {
				plan.Items[i].Status = "not_sent"
				plan.Items[i].Reason = "Transfer stopped before this recipe was sent"
			}
		}
	}
	if _, err := s.db.Put("batch-deployment", plan.ID, plan, version); err != nil {
		s.fail(w, 500, "receipt_error", "Transfer finished but its final receipt could not be saved. Inspect the current machine inventory before trying again.")
		return
	}
	s.json(w, 200, plan)
}
