package server

import (
	"encoding/json"
	"net/http"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

func (s *Server) listRecipes(w http.ResponseWriter, r *http.Request) {
	rows, e := s.db.List("recipe")
	if e != nil {
		s.internal(w, e)
		return
	}
	s.json(w, 200, rows)
}
func (s *Server) getRecipe(w http.ResponseWriter, r *http.Request) {
	var v domain.Recipe
	if _, e := s.db.Get("recipe", chi.URLParam(r, "id"), &v); e != nil {
		s.storeError(w, e)
		return
	}
	s.json(w, 200, v)
}
func (s *Server) saveRecipe(w http.ResponseWriter, r *http.Request) {
	var v domain.Recipe
	if !s.decode(w, r, &v) {
		return
	}
	if e := recipe.Validate(v); e != nil {
		s.fail(w, 400, "invalid_recipe", e.Error())
		return
	}
	s.recipeMu.Lock()
	defer s.recipeMu.Unlock()
	id := chi.URLParam(r, "id")
	expected := int64(0)
	code := 201
	now := time.Now().UTC()
	if id != "" {
		var old domain.Recipe
		version, e := s.db.Get("recipe", id, &old)
		if e != nil {
			s.storeError(w, e)
			return
		}
		if v.Revision != old.Revision {
			s.fail(w, 409, "revision_conflict", "The recipe was changed by another editor. Reload it before saving.")
			return
		}
		expected = version
		v.CreatedAt = old.CreatedAt
		code = 200
	} else {
		id = randomID()
		v.CreatedAt = now
	}
	v.ID = id
	v.Revision = expected + 1
	v.UpdatedAt = now
	if _, e := s.db.Put("recipe", id, v, expected); e != nil {
		s.storeError(w, e)
		return
	}
	if _, e := s.db.Put("recipe-revision", id+":"+strconv.FormatInt(v.Revision, 10), v, 0); e != nil {
		s.cfg.Logger.Error("Recipe revision archive failed", "id", id, "error", e)
	}
	s.json(w, code, v)
}
func (s *Server) deleteRecipe(w http.ResponseWriter, r *http.Request) {
	if e := s.db.Delete("recipe", chi.URLParam(r, "id")); e != nil {
		s.storeError(w, e)
		return
	}
	w.WriteHeader(204)
}
func (s *Server) adaptRecipe(w http.ResponseWriter, r *http.Request) {
	var in struct {
		ProfileID   string  `json:"profile_id"`
		BatchLiters float64 `json:"batch_liters"`
	}
	if !s.decode(w, r, &in) {
		return
	}
	var v domain.Recipe
	if _, e := s.db.Get("recipe", chi.URLParam(r, "id"), &v); e != nil {
		s.storeError(w, e)
		return
	}
	p, ok := s.selectProfile(in.ProfileID)
	if !ok {
		s.fail(w, 400, "profile", "Unknown machine profile")
		return
	}
	adapted, e := recipe.Adapt(v, p, in.BatchLiters)
	if e != nil {
		s.fail(w, 422, "adaptation", e.Error())
		return
	}
	s.json(w, 200, adapted)
}
func (s *Server) deploy(w http.ResponseWriter, r *http.Request) {
	d, ok := s.runtime(w, r)
	if !ok {
		return
	}
	var in struct {
		RecipeID       string  `json:"recipe_id"`
		ProfileID      string  `json:"profile_id"`
		BatchLiters    float64 `json:"batch_liters"`
		SourceRevision int64   `json:"source_revision"`
	}
	if !s.decode(w, r, &in) {
		return
	}
	var v domain.Recipe
	if _, e := s.db.Get("recipe", in.RecipeID, &v); e != nil {
		s.storeError(w, e)
		return
	}
	if in.SourceRevision != v.Revision {
		s.fail(w, 409, "revision_conflict", "Review adaptation again for the current recipe revision")
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
	p, ok := s.selectProfile(in.ProfileID)
	if !ok {
		s.fail(w, 400, "profile", "Unknown machine profile")
		return
	}
	a, e := recipe.Adapt(v, p, in.BatchLiters)
	if e != nil {
		s.fail(w, 422, "adaptation", e.Error())
		return
	}
	if !a.CanDeploy || a.MachineRecipe == nil {
		s.fail(w, 422, "unsupported_deployment", "This profile currently supports planning and export; live control is not available")
		return
	}
	status, e := s.verifiedStatus(r.Context(), d)
	if e != nil {
		s.deviceError(w, e)
		return
	}
	if status.Busy || status.Stopped || !status.Ready {
		s.fail(w, 409, "machine_not_idle", "The machine must be ready at its main menu before recipe deployment")
		return
	}
	native := *a.MachineRecipe
	existing, e := d.adapter.Recipes(r.Context())
	if e != nil {
		s.deviceError(w, e)
		return
	}
	deployment := map[string]any{"device_id": view.ID, "recipe_id": v.ID, "revision": v.Revision, "native": native, "at": time.Now().UTC(), "state": "prepared"}
	for _, previous := range existing {
		if previous.ID == native.ID {
			deployment["previous"] = previous
		}
	}
	recordID := randomID()
	if _, e = s.db.Put("deployment", recordID, deployment, 0); e != nil {
		s.internal(w, e)
		return
	}
	if e = d.adapter.PutRecipe(r.Context(), native); e != nil {
		deployment["state"] = "unconfirmed"
		_, _ = s.db.Put("deployment", recordID, deployment, -1)
		s.deviceError(w, e)
		return
	}
	deployment["state"] = "verified"
	if _, e = s.db.Put("deployment", recordID, deployment, -1); e != nil {
		s.fail(w, 500, "receipt_error", "The recipe was deployed, but the deployment receipt could not be saved. Inspect the machine before retrying.")
		return
	}
	s.json(w, 200, map[string]any{"deployed": true, "started": false, "device_id": view.ID, "recipe": native, "warnings": a.Warnings, "deployment_id": recordID})
}
func (s *Server) importRecipes(w http.ResponseWriter, r *http.Request) {
	var in struct {
		Format string `json:"format"`
		Data   string `json:"data"`
	}
	if !s.decode(w, r, &in) {
		return
	}
	items, e := recipe.Import([]byte(in.Data), in.Format)
	if e != nil {
		s.fail(w, 400, "invalid_import", e.Error())
		return
	}
	if len(items) == 0 || len(items) > 100 {
		s.fail(w, 400, "import_count", "Import between 1 and 100 recipes at a time")
		return
	}
	for _, v := range items {
		if e = recipe.Validate(v); e != nil {
			s.fail(w, 400, "invalid_recipe", e.Error())
			return
		}
	}
	now := time.Now().UTC()
	for i := range items {
		items[i].ID = randomID()
		items[i].Revision = 1
		items[i].CreatedAt = now
		items[i].UpdatedAt = now
		if _, e = s.db.Put("recipe", items[i].ID, items[i], 0); e != nil {
			s.internal(w, e)
			return
		}
		_, _ = s.db.Put("recipe-revision", items[i].ID+":1", items[i], 0)
	}
	s.json(w, 201, items)
}
func (s *Server) exportRecipe(w http.ResponseWriter, r *http.Request) {
	var v domain.Recipe
	if _, e := s.db.Get("recipe", chi.URLParam(r, "id"), &v); e != nil {
		s.storeError(w, e)
		return
	}
	format := r.URL.Query().Get("format")
	if format == "" {
		format = "json"
	}
	data, e := recipe.Export(v, format)
	if e != nil {
		s.fail(w, 400, "format", e.Error())
		return
	}
	extension := "json"
	ct := "application/json"
	if format == "beerxml" {
		extension = "xml"
		ct = "application/xml"
	}
	w.Header().Set("Content-Disposition", `attachment; filename="recipe-`+v.ID+`.`+extension+`"`)
	w.Header().Set("Content-Type", ct)
	_, _ = w.Write(data)
}
func (s *Server) recipeRevisions(w http.ResponseWriter, r *http.Request) {
	id := chi.URLParam(r, "id")
	rows, e := s.db.List("recipe-revision")
	if e != nil {
		s.internal(w, e)
		return
	}
	out := []domain.Recipe{}
	for _, raw := range rows {
		var v domain.Recipe
		if json.Unmarshal(raw, &v) == nil && v.ID == id {
			out = append(out, v)
		}
	}
	sort.Slice(out, func(i, j int) bool { return out[i].Revision > out[j].Revision })
	s.json(w, 200, out)
}
func (s *Server) shoppingList(w http.ResponseWriter, r *http.Request) {
	ids := strings.Split(r.URL.Query().Get("recipes"), ",")
	out := map[string]domain.Ingredient{}
	for _, id := range ids {
		if id == "" {
			continue
		}
		var v domain.Recipe
		if _, e := s.db.Get("recipe", id, &v); e != nil {
			s.storeError(w, e)
			return
		}
		for _, x := range v.Ingredients {
			k := strings.ToLower(x.Name) + ":" + x.Unit
			old := out[k]
			if old.Name == "" {
				old = x
				old.Amount = 0
			}
			old.Amount += x.Amount
			out[k] = old
		}
	}
	list := make([]domain.Ingredient, 0, len(out))
	for _, v := range out {
		list = append(list, v)
	}
	sort.Slice(list, func(i, j int) bool { return list[i].Name < list[j].Name })
	s.json(w, 200, list)
}
