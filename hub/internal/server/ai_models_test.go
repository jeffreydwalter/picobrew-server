package server

import (
	"bytes"
	"encoding/json"
	"errors"
	"io"
	"net/http"
	"net/http/httptest"
	"reflect"
	"strings"
	"sync/atomic"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/ai"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

func TestAIModelsConnectsBeforeConfigurationWithoutSaving(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	provider := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.Method != http.MethodGet || r.Header.Get("Authorization") != "Bearer unsaved-private-key" {
			t.Error("discovery did not use the supplied key")
		}
		_, _ = io.WriteString(w, `{"data":[{"id":"available-model"}]}`)
	}))
	defer provider.Close()
	s.aiClient.OpenAIModelsURL = provider.URL
	status, p := request(t, c, "POST", h.URL+"/api/v1/ai/models", csrf, map[string]string{"provider": "openai", "api_key": "unsaved-private-key"}, nil)
	var got aiModelList
	if err := json.Unmarshal(p, &got); err != nil || status != 200 || got.Provider != "openai" || len(got.Models) != 1 || got.Models[0].ID != "available-model" {
		t.Fatalf("model discovery required enabled/model settings: %d %s (%v)", status, p, err)
	}
	if bytes.Contains(p, []byte("unsaved-private-key")) {
		t.Fatal("discovery returned the key")
	}
	var cfg ai.Config
	if _, err := s.db.Get("config", "ai", &cfg); !errors.Is(err, store.ErrNotFound) {
		t.Fatalf("discovery saved configuration: %+v (%v)", cfg, err)
	}
	if _, err := s.db.GetSecret("ai:openai"); !errors.Is(err, store.ErrNotFound) {
		t.Fatalf("discovery persisted its temporary key: %v", err)
	}
	status, p = request(t, c, "GET", h.URL+"/api/v1/ai/models", csrf, nil, nil)
	if status != 409 || !bytes.Contains(p, []byte("ai_key_missing")) {
		t.Fatalf("unsaved key remained available: %d %s", status, p)
	}
}

func TestAIModelsSavedKeysAreIsolatedAndNeverReplaced(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	for _, provider := range []string{"openai", "anthropic"} {
		status, p := request(t, c, "PUT", h.URL+"/api/v1/ai/settings", csrf, map[string]any{"provider": provider, "enabled": false, "api_key": provider + "-saved-key"}, nil)
		if status != 200 {
			t.Fatalf("save provider key without enabling: %d %s", status, p)
		}
	}
	var before ai.Config
	version, err := s.db.Get("config", "ai", &before)
	if err != nil {
		t.Fatal(err)
	}
	keys := map[string][]byte{}
	for _, provider := range []string{"openai", "anthropic"} {
		keys[provider], err = s.db.GetSecret("ai:" + provider)
		if err != nil {
			t.Fatal(err)
		}
	}
	requests := make(chan string, 4)
	provider := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		if r.URL.Path == "/openai" {
			if r.Header.Get("x-api-key") != "" {
				t.Error("Anthropic credential sent to OpenAI")
			}
			requests <- "openai:" + strings.TrimPrefix(r.Header.Get("Authorization"), "Bearer ")
		} else {
			if r.Header.Get("Authorization") != "" {
				t.Error("OpenAI credential sent to Anthropic")
			}
			requests <- "anthropic:" + r.Header.Get("x-api-key")
		}
		_, _ = io.WriteString(w, `{"data":[{"id":"available-model"}],"has_more":false}`)
	}))
	defer provider.Close()
	s.aiClient.OpenAIModelsURL, s.aiClient.AnthropicModelsURL = provider.URL+"/openai", provider.URL+"/anthropic"
	for _, tc := range []struct {
		method, query string
		body          any
		want          string
	}{
		{"GET", "", nil, "anthropic:anthropic-saved-key"},
		{"GET", "?provider=openai", nil, "openai:openai-saved-key"},
		{"POST", "", map[string]string{"provider": "openai"}, "openai:openai-saved-key"},
		{"POST", "", map[string]string{"provider": "anthropic", "api_key": "candidate-key"}, "anthropic:candidate-key"},
	} {
		status, p := request(t, c, tc.method, h.URL+"/api/v1/ai/models"+tc.query, csrf, tc.body, nil)
		if status != 200 {
			t.Fatalf("discovery failed: %d %s", status, p)
		}
		if got := <-requests; got != tc.want {
			t.Fatalf("wrong provider credential: %q, want %q", got, tc.want)
		}
	}
	var after ai.Config
	newVersion, err := s.db.Get("config", "ai", &after)
	if err != nil || version != newVersion || !reflect.DeepEqual(before, after) {
		t.Fatalf("discovery changed active settings: %+v, %+v (%v)", before, after, err)
	}
	for provider, beforeKey := range keys {
		afterKey, err := s.db.GetSecret("ai:" + provider)
		if err != nil || !bytes.Equal(beforeKey, afterKey) {
			t.Errorf("discovery replaced %s saved key (%v)", provider, err)
		}
	}
}

func TestAIModelsAuthenticationAndValidationPrecedeProviderAccess(t *testing.T) {
	s, h, c := newTestServer(t)
	var calls atomic.Int64
	provider := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { calls.Add(1) }))
	defer provider.Close()
	s.aiClient.OpenAIModelsURL, s.aiClient.AnthropicModelsURL = provider.URL, provider.URL
	valid := map[string]string{"provider": "openai", "api_key": "candidate-key"}
	for _, method := range []string{"GET", "POST"} {
		status, _ := request(t, c, method, h.URL+"/api/v1/ai/models", "", valid, nil)
		if status != 401 {
			t.Errorf("unauthenticated %s discovery returned %d", method, status)
		}
	}
	csrf := signIn(t, c, h.URL)
	status, _ := request(t, c, "POST", h.URL+"/api/v1/ai/models", "", valid, nil)
	if status != 403 {
		t.Errorf("missing CSRF accepted: %d", status)
	}
	status, _ = request(t, c, "POST", h.URL+"/api/v1/ai/models", csrf, valid, map[string]string{"Origin": "https://unrelated.example"})
	if status != 403 {
		t.Errorf("cross-origin discovery accepted: %d", status)
	}
	for _, tc := range []struct {
		name, method, query string
		body                any
		status              int
	}{
		{"GET missing key", "GET", "", nil, 409},
		{"GET unsupported", "GET", "?provider=invalid", nil, 400},
		{"GET cannot accept a key in query", "GET", "?provider=openai&api_key=candidate", nil, 409},
		{"POST missing key", "POST", "", map[string]string{"provider": "openai"}, 409},
		{"POST no JSON", "POST", "", nil, 415},
		{"POST missing provider", "POST", "", map[string]string{"api_key": "key"}, 400},
		{"POST unsupported", "POST", "", map[string]string{"provider": "invalid", "api_key": "key"}, 400},
		{"POST unknown field", "POST", "", map[string]string{"provider": "openai", "api_key": "key", "endpoint": provider.URL}, 400},
		{"POST overlong key", "POST", "", map[string]string{"provider": "openai", "api_key": strings.Repeat("x", 1025)}, 400},
		{"POST blank key", "POST", "", map[string]string{"provider": "openai", "api_key": "  "}, 400},
		{"POST newline key", "POST", "", map[string]string{"provider": "openai", "api_key": "key\nheader"}, 400},
		{"POST CR key", "POST", "", map[string]string{"provider": "openai", "api_key": "key\rheader"}, 400},
		{"POST NUL key", "POST", "", map[string]string{"provider": "openai", "api_key": "key\x00header"}, 400},
		{"POST tab key", "POST", "", map[string]string{"provider": "openai", "api_key": "key\theader"}, 400},
	} {
		t.Run(tc.name, func(t *testing.T) {
			status, p := request(t, c, tc.method, h.URL+"/api/v1/ai/models"+tc.query, csrf, tc.body, nil)
			if status != tc.status {
				t.Fatalf("returned %d %s, want %d", status, p, tc.status)
			}
		})
	}
	if calls.Load() != 0 {
		t.Fatalf("invalid requests made %d provider calls", calls.Load())
	}
}

func TestAIModelsUpstreamFailuresAreRedactedWithoutLoggingOut(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	status, p := request(t, c, "PUT", h.URL+"/api/v1/ai/settings", csrf, map[string]any{"provider": "openai", "enabled": false, "model": "previous-model", "api_key": "saved-private-key"}, nil)
	if status != 200 {
		t.Fatal(status, string(p))
	}
	var before ai.Config
	version, err := s.db.Get("config", "ai", &before)
	if err != nil {
		t.Fatal(err)
	}
	var upstreamStatus, calls atomic.Int64
	provider := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		calls.Add(1)
		w.WriteHeader(int(upstreamStatus.Load()))
		_, _ = io.WriteString(w, "candidate-private-key: confidential provider account details")
	}))
	defer provider.Close()
	s.aiClient.OpenAIModelsURL = provider.URL
	for _, code := range []int{401, 403, 429, 500} {
		upstreamStatus.Store(int64(code))
		calls.Store(0)
		status, p = request(t, c, "POST", h.URL+"/api/v1/ai/models", csrf, map[string]string{"provider": "openai", "api_key": "candidate-private-key"}, nil)
		if status != 502 || !bytes.Contains(p, []byte(`"code":"ai_provider"`)) || bytes.Contains(p, []byte("private-key")) || bytes.Contains(p, []byte("confidential")) || calls.Load() != 1 {
			t.Fatalf("unsafe upstream %d response: %d %s, calls %d", code, status, p, calls.Load())
		}
		status, p = request(t, c, "GET", h.URL+"/api/v1/ai/settings", csrf, nil, nil)
		if status != 200 {
			t.Fatalf("provider failure invalidated Hub session: %d %s", status, p)
		}
	}
	var after ai.Config
	newVersion, err := s.db.Get("config", "ai", &after)
	if err != nil || version != newVersion || !reflect.DeepEqual(before, after) {
		t.Fatalf("provider failure changed settings: %+v, %+v (%v)", before, after, err)
	}
	key, err := s.readSecret("ai:openai")
	if err != nil || key != "saved-private-key" {
		t.Fatalf("provider failure replaced saved key (%v)", err)
	}
}

func TestAISettingsAcceptsLongDiscoveredModelIDs(t *testing.T) {
	_, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	model := "ft:" + strings.Repeat("custom-model-", 25)
	status, p := request(t, c, "PUT", h.URL+"/api/v1/ai/settings", csrf, map[string]any{"provider": "openai", "model": model, "api_key": "test-key", "enabled": true}, nil)
	var cfg ai.Config
	if err := json.Unmarshal(p, &cfg); err != nil || status != 200 || cfg.Model != model {
		t.Fatalf("long discovered model could not be saved: %d %s (%v)", status, p, err)
	}
}
