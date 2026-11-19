package server

import (
	"errors"
	"net/http"

	"github.com/jeffreydwalter/picobrew-hub/internal/ai"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

type aiModelsInput struct {
	Provider string `json:"provider"`
	APIKey   string `json:"api_key,omitempty"`
}

type aiModelList struct {
	Provider string     `json:"provider"`
	Models   []ai.Model `json:"models"`
}

// Model discovery is available before enabling the assistant or selecting a
// model. A supplied key is used for this request only; saving settings remains
// an explicit, separate action so a failed connection cannot replace it.
func (s *Server) aiModels(w http.ResponseWriter, r *http.Request) {
	in := aiModelsInput{Provider: r.URL.Query().Get("provider")}
	if r.Method == http.MethodPost {
		in = aiModelsInput{}
		if !s.decode(w, r, &in) {
			return
		}
	} else if in.Provider == "" {
		cfg := ai.Config{Provider: "openai"}
		s.configMu.Lock()
		_, err := s.db.Get("config", "ai", &cfg)
		s.configMu.Unlock()
		if err != nil && !errors.Is(err, store.ErrNotFound) {
			s.internal(w, err)
			return
		}
		in.Provider = cfg.Provider
	}
	if in.Provider != "openai" && in.Provider != "anthropic" {
		s.fail(w, http.StatusBadRequest, "provider", "Choose OpenAI or Anthropic")
		return
	}
	if invalidProviderKey(in.APIKey) {
		s.fail(w, http.StatusBadRequest, "config", "Invalid provider API key")
		return
	}
	key := in.APIKey
	if key == "" {
		s.configMu.Lock()
		stored, err := s.readSecret("ai:" + in.Provider)
		s.configMu.Unlock()
		if err != nil && !errors.Is(err, store.ErrNotFound) {
			s.internal(w, err)
			return
		}
		key = stored
	}
	if key == "" {
		s.fail(w, http.StatusConflict, "ai_key_missing", "Enter an API key for this provider to load its models")
		return
	}
	models, err := s.aiClient.ListModels(r.Context(), in.Provider, key)
	if err != nil {
		// Upstream authentication failures are not Hub session failures. The
		// provider client returns bounded messages without credentials/bodies.
		s.fail(w, http.StatusBadGateway, "ai_provider", err.Error())
		return
	}
	s.json(w, http.StatusOK, aiModelList{Provider: in.Provider, Models: models})
}
