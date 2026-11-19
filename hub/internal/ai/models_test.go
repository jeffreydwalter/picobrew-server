package ai

import (
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"net/http/httptest"
	"reflect"
	"strings"
	"testing"
	"time"
)

func TestListModelsUsesProviderCredentialsAndCompleteCatalogue(t *testing.T) {
	for _, provider := range []string{"openai", "anthropic"} {
		t.Run(provider, func(t *testing.T) {
			calls := 0
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				calls++
				if r.Method != http.MethodGet || r.Header.Get("Accept") != "application/json" || r.URL.Path != "/v1/models" {
					t.Errorf("unexpected discovery request: %s %s", r.Method, r.URL)
				}
				if provider == "openai" {
					if r.Header.Get("Authorization") != "Bearer test-model-key" || r.Header.Get("x-api-key") != "" || r.URL.RawQuery != "" {
						t.Error("wrong OpenAI authentication or query")
					}
					_, _ = io.WriteString(w, `{"object":"list","data":[{"id":"future-model","display_name":"ignored"},{"id":" embedding-model "},{"id":"future-model"},{"id":"alpha-model"}]}`)
					return
				}
				if r.Header.Get("x-api-key") != "test-model-key" || r.Header.Get("anthropic-version") != "2023-06-01" || r.Header.Get("Authorization") != "" || r.URL.Query().Get("limit") != "1000" {
					t.Error("wrong Anthropic authentication or page size")
				}
				switch r.URL.Query().Get("after_id") {
				case "":
					_, _ = io.WriteString(w, `{"data":[{"id":"future-model","display_name":"Zeta"},{"id":"second-model","display_name":" Alpha "}],"has_more":true,"last_id":"second-model","next":"https://must-not-be-followed.invalid"}`)
				case "second-model":
					_, _ = io.WriteString(w, `{"data":[{"id":"second-model","display_name":"Alpha"},{"id":"third-model","display_name":""}],"has_more":false,"last_id":"third-model"}`)
				default:
					t.Errorf("incorrect pagination cursor: %q", r.URL.Query().Get("after_id"))
				}
			}))
			defer srv.Close()
			c := New()
			c.OpenAIModelsURL, c.AnthropicModelsURL = srv.URL+"/v1/models", srv.URL+"/v1/models"
			got, err := c.ListModels(context.Background(), provider, "test-model-key")
			want := []Model{{"alpha-model", "alpha-model"}, {"embedding-model", "embedding-model"}, {"future-model", "future-model"}}
			wantCalls := 1
			if provider == "anthropic" {
				want = []Model{{"second-model", "Alpha"}, {"third-model", "third-model"}, {"future-model", "Zeta"}}
				wantCalls = 2
			}
			if err != nil || !reflect.DeepEqual(got, want) || calls != wantCalls {
				t.Fatalf("catalogue = %+v, %v, calls %d; want %+v, calls %d", got, err, calls, want, wantCalls)
			}
		})
	}
}

func TestListModelsEmptyCatalogueIsAnArray(t *testing.T) {
	srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { _, _ = io.WriteString(w, `{"data":[]}`) }))
	defer srv.Close()
	c := New()
	c.OpenAIModelsURL = srv.URL
	got, err := c.ListModels(context.Background(), "openai", "key")
	if err != nil || got == nil || len(got) != 0 {
		t.Fatalf("empty catalogue = %#v, %v", got, err)
	}
}

func TestListModelsRejectsIncompleteAndUnboundedLists(t *testing.T) {
	tooMany := make([]map[string]string, modelListMaxResults+1)
	for i := range tooMany {
		tooMany[i] = map[string]string{"id": fmt.Sprintf("model-%d", i)}
	}
	p, _ := json.Marshal(map[string]any{"data": tooMany})
	for _, tc := range []struct {
		name, provider, body, errorPart string
	}{
		{"missing data", "openai", `{}`, "invalid model list"},
		{"null data", "openai", `{"data":null}`, "invalid model list"},
		{"invalid JSON", "openai", `{"data":`, "invalid model list"},
		{"trailing JSON", "openai", `{"data":[]} {}`, "invalid model list"},
		{"missing identifier", "openai", `{"data":[{"id":""}]}`, "invalid model identifier"},
		{"control in identifier", "openai", `{"data":[{"id":"m\nodel"}]}`, "invalid model identifier"},
		{"page size", "openai", strings.Repeat(" ", modelListMaxPage+1), "page size limit"},
		{"result limit", "openai", string(p), "result limit"},
		{"missing has_more", "anthropic", `{"data":[]}`, "invalid model list"},
		{"empty continuation", "anthropic", `{"data":[],"has_more":true,"last_id":"x"}`, "invalid model pagination"},
		{"missing cursor", "anthropic", `{"data":[{"id":"x"}],"has_more":true}`, "invalid model pagination"},
		{"wrong cursor", "anthropic", `{"data":[{"id":"x"}],"has_more":true,"last_id":"y"}`, "invalid model pagination"},
		{"repeated cursor", "anthropic", `{"data":[{"id":"x"}],"has_more":true,"last_id":"x"}`, "invalid model pagination"},
	} {
		t.Run(tc.name, func(t *testing.T) {
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { _, _ = io.WriteString(w, tc.body) }))
			defer srv.Close()
			c := New()
			c.OpenAIModelsURL, c.AnthropicModelsURL = srv.URL, srv.URL
			models, err := c.ListModels(context.Background(), tc.provider, "key")
			if err == nil || models != nil || !strings.Contains(err.Error(), tc.errorPart) {
				t.Fatalf("published an incomplete list or wrong failure: %+v, %v", models, err)
			}
		})
	}
}

func TestListModelsBoundsTotalPagesAndBytes(t *testing.T) {
	for _, padded := range []bool{false, true} {
		t.Run(fmt.Sprint("padded=", padded), func(t *testing.T) {
			calls := 0
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				calls++
				body := map[string]any{"data": []map[string]string{{"id": fmt.Sprintf("model-%d", calls)}}, "has_more": true, "last_id": fmt.Sprintf("model-%d", calls)}
				if padded {
					body["padding"] = strings.Repeat(" ", modelListMaxPage-200)
				}
				_ = json.NewEncoder(w).Encode(body)
			}))
			defer srv.Close()
			c := New()
			c.AnthropicModelsURL = srv.URL
			models, err := c.ListModels(context.Background(), "anthropic", "key")
			want, wantCalls := "page limit", modelListMaxPages
			if padded {
				want, wantCalls = "total size limit", 9
			}
			if err == nil || models != nil || !strings.Contains(err.Error(), want) || calls != wantCalls {
				t.Fatalf("unbounded pagination: models=%d, err=%v, calls=%d", len(models), err, calls)
			}
		})
	}
}

func TestListModelsRedactsProviderErrorsAndDoesNotRetry(t *testing.T) {
	for _, status := range []int{401, 403, 429, 500} {
		t.Run(fmt.Sprint(status), func(t *testing.T) {
			calls := 0
			srv := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				calls++
				w.WriteHeader(status)
				_, _ = io.WriteString(w, "test-private-key: confidential account detail")
			}))
			defer srv.Close()
			c := New()
			c.OpenAIModelsURL = srv.URL
			_, err := c.ListModels(context.Background(), "openai", "test-private-key")
			if err == nil || strings.Contains(err.Error(), "test-private-key") || strings.Contains(err.Error(), "confidential") || !strings.Contains(err.Error(), fmt.Sprint(status)) || calls != 1 {
				t.Fatalf("unsafe provider failure: %v, calls=%d", err, calls)
			}
		})
	}
}

func TestListModelsNeverFollowsRedirectsEvenWithCustomClient(t *testing.T) {
	for _, provider := range []string{"openai", "anthropic"} {
		t.Run(provider, func(t *testing.T) {
			leaked := false
			target := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) { leaked = true }))
			defer target.Close()
			origin := httptest.NewServer(http.HandlerFunc(func(w http.ResponseWriter, r *http.Request) {
				http.Redirect(w, r, target.URL, http.StatusTemporaryRedirect)
			}))
			defer origin.Close()
			c := New()
			c.HTTP = origin.Client()
			c.OpenAIModelsURL, c.AnthropicModelsURL = origin.URL, origin.URL
			_, err := c.ListModels(context.Background(), provider, "test-private-key")
			if err == nil || leaked {
				t.Fatalf("redirect followed or key disclosed: %v", err)
			}
		})
	}
}

type modelRoundTrip func(*http.Request) (*http.Response, error)

func (f modelRoundTrip) RoundTrip(r *http.Request) (*http.Response, error) { return f(r) }

func TestListModelsBoundsDeadlineAndRespectsCancellation(t *testing.T) {
	c := New()
	c.HTTP = &http.Client{Transport: modelRoundTrip(func(r *http.Request) (*http.Response, error) {
		deadline, ok := r.Context().Deadline()
		if !ok || time.Until(deadline) <= 0 || time.Until(deadline) > modelListTimeout {
			t.Error("model discovery did not add its own bounded deadline")
		}
		return &http.Response{StatusCode: http.StatusOK, Body: io.NopCloser(strings.NewReader(`{"data":[]}`)), Header: make(http.Header)}, nil
	})}
	if _, err := c.ListModels(context.Background(), "openai", "key"); err != nil {
		t.Fatal(err)
	}
	c.HTTP = &http.Client{Transport: modelRoundTrip(func(r *http.Request) (*http.Response, error) {
		deadline, ok := r.Context().Deadline()
		if !ok || time.Until(deadline) > modelListTimeout {
			t.Error("missing bounded model-list deadline")
		}
		<-r.Context().Done()
		return nil, r.Context().Err()
	})}
	ctx, cancel := context.WithTimeout(context.Background(), 20*time.Millisecond)
	defer cancel()
	if _, err := c.ListModels(ctx, "openai", "key"); err == nil || !strings.Contains(err.Error(), "timed out") {
		t.Fatalf("deadline not honored: %v", err)
	}
	ctx, cancel = context.WithCancel(context.Background())
	cancel()
	if _, err := c.ListModels(ctx, "openai", "key"); err == nil || !strings.Contains(err.Error(), "canceled") {
		t.Fatalf("cancellation not honored: %v", err)
	}
}

func TestListModelsValidatesBeforeCallingProvider(t *testing.T) {
	c := New()
	c.HTTP = &http.Client{Transport: modelRoundTrip(func(r *http.Request) (*http.Response, error) {
		t.Error("invalid configuration contacted provider")
		return nil, fmt.Errorf("should not be reached")
	})}
	for _, tc := range []struct{ provider, key string }{{"invalid", "key"}, {"openai", ""}, {"openai", " \t"}, {"openai", "key\r\nheader"}, {"anthropic", strings.Repeat("x", 1025)}} {
		if _, err := c.ListModels(context.Background(), tc.provider, tc.key); err == nil {
			t.Error("invalid model discovery accepted")
		}
	}
	c.OpenAIModelsURL = "https://test-private-key:password@example.invalid"
	if _, err := c.ListModels(context.Background(), "openai", "key"); err == nil || strings.Contains(err.Error(), "test-private-key") {
		t.Fatalf("unsafe endpoint failure: %v", err)
	}
}
