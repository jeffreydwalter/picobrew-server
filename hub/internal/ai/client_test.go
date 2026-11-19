package ai

import (
	"context"
	"encoding/json"
	"net/http"
	"net/http/httptest"
	"strings"
	"testing"
)

func TestProviderRequestsAndStructuredDraft(t *testing.T) {
	for _, provider := range []string{"openai", "anthropic"} {
		t.Run(provider, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				if r.Method != "POST" || r.Header.Get("Content-Type") != "application/json" {
					t.Errorf("invalid request headers")
				}
				if provider == "openai" && r.Header.Get("Authorization") != "Bearer test-private-key" {
					t.Error("missing OpenAI auth")
				}
				if provider == "anthropic" && (r.Header.Get("x-api-key") != "test-private-key" || r.Header.Get("anthropic-version") == "") {
					t.Error("missing Anthropic auth/version")
				}
				var body map[string]any
				if err := json.NewDecoder(r.Body).Decode(&body); err != nil {
					t.Fatal(err)
				}
				if body["model"] != "selected-model" {
					t.Error("model changed")
				}
				draft := `{"text":"Draft for review","recipe":{"name":"Test ale","batch_liters":5}}`
				w.Header().Set("Content-Type", "application/json")
				if provider == "openai" {
					_ = json.NewEncoder(w).Encode(map[string]any{"choices": []any{map[string]any{"message": map[string]string{"content": "```json\n" + draft + "\n```"}}}})
				} else {
					_ = json.NewEncoder(w).Encode(map[string]any{"content": []any{map[string]string{"type": "text", "text": draft}}})
				}
			}))
			defer srv.Close()
			c := New()
			c.HTTP = srv.Client()
			c.OpenAIURL = srv.URL
			c.AnthropicURL = srv.URL
			out, err := c.Assist(context.Background(), Config{Provider: provider, Model: "selected-model", Enabled: true}, "test-private-key", Request{Mode: "recipe", Prompt: "Build a pale ale"}, map[string]any{"temperature": 65})
			if err != nil {
				t.Fatal(err)
			}
			if out.Text != "Draft for review" || !json.Valid(out.Recipe) || len(out.Limitations) == 0 {
				t.Fatalf("bad structured response: %+v", out)
			}
		})
	}
}

func TestProviderFailuresDoNotExposeBodyOrRetry(t *testing.T) {
	calls := 0
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		calls++
		w.WriteHeader(401)
		_, _ = w.Write([]byte("private key and confidential provider error"))
	}))
	defer srv.Close()
	c := New()
	c.OpenAIURL = srv.URL
	_, err := c.Assist(context.Background(), Config{Provider: "openai", Model: "m", Enabled: true}, "private-key", Request{Mode: "live"}, nil)
	if err == nil || strings.Contains(err.Error(), "confidential") || strings.Contains(err.Error(), "private-key") || calls != 1 {
		t.Fatalf("unsafe failure: %v; calls=%d", err, calls)
	}
}

func TestProviderRedirectDoesNotForwardKey(t *testing.T) {
	leaked := false
	target := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { leaked = true }))
	defer target.Close()
	origin := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
		http.Redirect(w, r, target.URL, http.StatusTemporaryRedirect)
	}))
	defer origin.Close()
	c := New()
	c.OpenAIURL = origin.URL
	_, err := c.Assist(context.Background(), Config{Provider: "openai", Model: "m", Enabled: true}, "key", Request{Mode: "live"}, nil)
	if err == nil || leaked {
		t.Fatalf("redirect accepted or credentials forwarded: %v", err)
	}
}

func TestDisabledAndOversizedRequestsNeverCallProvider(t *testing.T) {
	calls := 0
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { calls++ }))
	defer srv.Close()
	c := New()
	c.OpenAIURL = srv.URL
	cases := []struct {
		cfg   Config
		input Request
	}{{Config{Provider: "openai", Model: "m"}, Request{}}, {Config{Provider: "openai", Model: "m", Enabled: true}, Request{Prompt: strings.Repeat("x", 16001)}}}
	for _, v := range cases {
		if _, err := c.Assist(context.Background(), v.cfg, "key", v.input, nil); err == nil {
			t.Error("invalid request accepted")
		}
	}
	if calls != 0 {
		t.Fatalf("made %d calls", calls)
	}
}
