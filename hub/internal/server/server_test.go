package server

import (
	"bytes"
	"context"
	"encoding/json"
	"io"
	"log/slog"
	"net/http"
	"net/http/cookiejar"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/device"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

const ownerPassword = "an-example-test-password-only"

func newTestServer(t *testing.T) (*Server, *httptest.Server, *http.Client) {
	t.Helper()
	s, err := New(Config{DataDir: t.TempDir(), Password: ownerPassword, PollInterval: time.Hour, Logger: slog.New(slog.NewTextHandler(io.Discard, nil))})
	if err != nil {
		t.Fatal(err)
	}
	h := httptest.NewServer(s)
	jar, _ := cookiejar.New(nil)
	c := &http.Client{Jar: jar, Timeout: 10 * time.Second}
	t.Cleanup(func() {
		h.Close()
		if err := s.Close(); err != nil {
			t.Error(err)
		}
	})
	return s, h, c
}
func request(t *testing.T, c *http.Client, method, url, csrf string, body any, headers map[string]string) (int, []byte) {
	t.Helper()
	var b io.Reader
	if body != nil {
		p, err := json.Marshal(body)
		if err != nil {
			t.Fatal(err)
		}
		b = bytes.NewReader(p)
	}
	r, err := http.NewRequest(method, url, b)
	if err != nil {
		t.Fatal(err)
	}
	if body != nil {
		r.Header.Set("Content-Type", "application/json")
	}
	if csrf != "" {
		r.Header.Set("X-CSRF-Token", csrf)
	}
	for k, v := range headers {
		r.Header.Set(k, v)
	}
	resp, err := c.Do(r)
	if err != nil {
		t.Fatal(err)
	}
	defer resp.Body.Close()
	p, err := io.ReadAll(resp.Body)
	if err != nil {
		t.Fatal(err)
	}
	return resp.StatusCode, p
}
func signIn(t *testing.T, c *http.Client, url string) string {
	t.Helper()
	status, p := request(t, c, "POST", url+"/api/v1/auth/login", "", map[string]string{"password": ownerPassword}, nil)
	if status != 200 {
		t.Fatalf("login %d: %s", status, p)
	}
	_, p = request(t, c, "GET", url+"/api/v1/auth/session", "", nil, nil)
	var v struct {
		Token string `json:"csrf_token"`
	}
	if err := json.Unmarshal(p, &v); err != nil || v.Token == "" {
		t.Fatalf("session: %s (%v)", p, err)
	}
	return v.Token
}
func validRecipe() domain.Recipe {
	return domain.Recipe{Name: "Test Pale", BatchLiters: 5, Efficiency: 55, Type: "All Grain", BoilLiters: 6.5, BoilMinutes: 60, Ingredients: []domain.Ingredient{{Name: "Pale malt", Kind: "fermentable", Type: "Grain", Amount: 1.2, Unit: "kg", Use: "mash"}}, Steps: []domain.RecipeStep{{Name: "Mash", Kind: "mash", TargetC: 67, Minutes: 60, Location: 1}, {Name: "Boil", Kind: "boil", TargetC: 100, Minutes: 60, Location: 2}}}
}

func TestOwnerSessionCSRFRevisionAndSecretStorage(t *testing.T) {
	s, h, c := newTestServer(t)
	status, _ := request(t, c, "GET", h.URL+"/api/v1/devices", "", nil, nil)
	if status != 401 {
		t.Fatalf("unauthenticated devices: %d", status)
	}
	csrf := signIn(t, c, h.URL)
	status, _ = request(t, c, "POST", h.URL+"/api/v1/recipes", "", validRecipe(), nil)
	if status != 403 {
		t.Fatalf("missing csrf: %d", status)
	}
	status, _ = request(t, c, "POST", h.URL+"/api/v1/recipes", csrf, validRecipe(), map[string]string{"Origin": "https://evil.example"})
	if status != 403 {
		t.Fatalf("cross-origin write: %d", status)
	}
	status, p := request(t, c, "POST", h.URL+"/api/v1/recipes", csrf, validRecipe(), nil)
	if status != 201 {
		t.Fatalf("create: %d %s", status, p)
	}
	var recipe domain.Recipe
	_ = json.Unmarshal(p, &recipe)
	if recipe.Revision != 1 || recipe.ID == "" {
		t.Fatalf("bad created record: %s", p)
	}
	original := recipe
	recipe.Notes = "First tasting"
	status, p = request(t, c, "PUT", h.URL+"/api/v1/recipes/"+recipe.ID, csrf, recipe, nil)
	if status != 200 {
		t.Fatalf("save: %d %s", status, p)
	}
	status, p = request(t, c, "PUT", h.URL+"/api/v1/recipes/"+recipe.ID, csrf, original, nil)
	if status != 409 {
		t.Fatalf("stale save accepted: %d %s", status, p)
	}
	key := "test-provider-key-that-must-stay-private"
	status, p = request(t, c, "PUT", h.URL+"/api/v1/ai/settings", csrf, map[string]any{"provider": "openai", "model": "example-model", "api_key": key, "enabled": true, "monitor_enabled": false}, nil)
	if status != 200 || bytes.Contains(p, []byte(key)) {
		t.Fatalf("provider settings: %d %s", status, p)
	}
	encrypted, err := s.db.GetSecret("ai:openai")
	if err != nil || bytes.Contains(encrypted, []byte(key)) {
		t.Fatal("credential was not encrypted", err)
	}
	decoded, err := s.readSecret("ai:openai")
	if err != nil || decoded != key {
		t.Fatal("credential decrypt failed", err)
	}
	if _, err = s.vault.open("device:wrong-purpose", encrypted); err == nil {
		t.Fatal("ciphertext can be substituted across records")
	}
	status, p = request(t, c, "GET", h.URL+"/api/v1/backup", csrf, nil, nil)
	if status != 200 || bytes.Contains(p, []byte(key)) {
		t.Fatal("backup exposed key or failed")
	}
	status, _ = request(t, c, "POST", h.URL+"/api/v1/auth/logout", csrf, map[string]any{}, nil)
	if status != 204 {
		t.Fatal(status)
	}
	status, _ = request(t, c, "GET", h.URL+"/api/v1/recipes", "", nil, nil)
	if status != 401 {
		t.Fatal("logout did not revoke session")
	}
}

func TestSetupAndPasswordSurviveRestart(t *testing.T) {
	dir := t.TempDir()
	cfg := Config{DataDir: dir, Logger: slog.New(slog.NewTextHandler(io.Discard, nil))}
	s, err := New(cfg)
	if err != nil {
		t.Fatal(err)
	}
	token := s.auth.setupToken
	rr := httptest.NewRecorder()
	r := httptest.NewRequest("POST", "http://localhost/api/v1/auth/setup", strings.NewReader(`{"token":"wrong","password":"a-valid-password"}`))
	r.Header.Set("Content-Type", "application/json")
	s.ServeHTTP(rr, r)
	if rr.Code != 401 {
		t.Fatal(rr.Code)
	}
	payload, _ := json.Marshal(map[string]string{"token": token, "password": ownerPassword})
	rr = httptest.NewRecorder()
	r = httptest.NewRequest("POST", "http://localhost/api/v1/auth/setup", bytes.NewReader(payload))
	r.Header.Set("Content-Type", "application/json")
	s.ServeHTTP(rr, r)
	if rr.Code != 200 {
		t.Fatal(rr.Code, rr.Body.String())
	}
	if err = s.Close(); err != nil {
		t.Fatal(err)
	}
	s, err = New(cfg)
	if err != nil {
		t.Fatal(err)
	}
	defer s.Close()
	if s.auth.setupToken != "" || !matchesPassword(ownerPassword, s.auth.password) {
		t.Fatal("owner password did not survive restart")
	}
}

func TestRateLimitUsesHostNotEphemeralSourcePort(t *testing.T) {
	s, _, _ := newTestServer(t)
	for i := 0; i < 11; i++ {
		rr := httptest.NewRecorder()
		r := httptest.NewRequest("POST", "http://localhost/api/v1/auth/login", strings.NewReader(`{"password":"incorrect"}`))
		r.Header.Set("Content-Type", "application/json")
		r.RemoteAddr = "192.0.2.10:" + strings.Repeat("1", i+1)
		s.ServeHTTP(rr, r)
		if i < 10 && rr.Code != 401 {
			t.Fatalf("attempt %d: %d", i, rr.Code)
		}
		if i == 10 && rr.Code != 429 {
			t.Fatalf("source-port rotation bypassed limit: %d", rr.Code)
		}
	}
}

func TestDemoRefusesExistingHardwareRecords(t *testing.T) {
	dir := t.TempDir()
	db, err := store.Open(filepath.Join(dir, "hub.sqlite"))
	if err != nil {
		t.Fatal(err)
	}
	_, err = db.Put("device", "real", domain.Device{ID: "real", Adapter: "pico-standalone", Enabled: true, BaseURL: "http://127.0.0.1:9"}, 0)
	if err != nil {
		t.Fatal(err)
	}
	_ = db.Close()
	s, err := New(Config{DataDir: dir, Demo: true, Logger: slog.New(slog.NewTextHandler(io.Discard, nil))})
	if err == nil {
		s.Close()
		t.Fatal("demo accepted a hardware data directory")
	}
}

type countedAdapter struct {
	device.Adapter
	calls   atomic.Int32
	entered chan<- string
	release <-chan struct{}
	id      string
	failure error
}

func (a *countedAdapter) Command(ctx context.Context, c domain.Command) error {
	a.calls.Add(1)
	if a.entered != nil {
		select {
		case a.entered <- a.id:
		case <-ctx.Done():
			return ctx.Err()
		}
	}
	if a.release != nil {
		select {
		case <-a.release:
		case <-ctx.Done():
			return ctx.Err()
		}
	}
	if a.failure != nil {
		return a.failure
	}
	return a.Adapter.Command(ctx, c)
}
func installTestDevice(t *testing.T, s *Server, id string, a device.Adapter) {
	t.Helper()
	caps, err := a.Discover(context.Background())
	if err != nil {
		t.Fatal(err)
	}
	st, err := a.Status(context.Background())
	if err != nil {
		t.Fatal(err)
	}
	rt := &runtimeDevice{view: domain.DeviceView{Device: domain.Device{ID: id, Name: id, Model: "pico-pro", Adapter: "simulator", Enabled: true}, Online: true, Capabilities: &caps, Status: &st}, adapter: a, cancel: func() {}}
	s.mu.Lock()
	s.devices[id] = rt
	s.mu.Unlock()
}
func TestCommandsAreConcurrentAcrossMachinesAndIdempotent(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	entered := make(chan string, 2)
	release := make(chan struct{})
	defer func() {
		select {
		case <-release:
		default:
			close(release)
		}
	}()
	adapters := []*countedAdapter{{Adapter: device.NewSimulator("one", "pico-pro"), id: "one", entered: entered, release: release}, {Adapter: device.NewSimulator("two", "pico-pro"), id: "two", entered: entered, release: release}}
	for _, a := range adapters {
		installTestDevice(t, s, a.id, a)
	}
	var wg sync.WaitGroup
	results := make(chan int, 2)
	for _, a := range adapters {
		wg.Add(1)
		go func(id string) {
			defer wg.Done()
			status, _ := request(t, c, "POST", h.URL+"/api/v1/devices/"+id+"/commands", csrf, map[string]any{"command": "encoder", "delta": 1}, map[string]string{"Idempotency-Key": "test-command-1"})
			results <- status
		}(a.id)
	}
	for range 2 {
		select {
		case <-entered:
		case <-time.After(3 * time.Second):
			t.Fatal("machines blocked each other's command transport")
		}
	}
	close(release)
	wg.Wait()
	close(results)
	for status := range results {
		if status != 202 {
			t.Fatal("command", status)
		}
	}
	status, _ := request(t, c, "POST", h.URL+"/api/v1/devices/one/commands", csrf, map[string]any{"command": "encoder", "delta": 1}, map[string]string{"Idempotency-Key": "test-command-1"})
	if status != 200 || adapters[0].calls.Load() != 1 {
		t.Fatal("repeated command was delivered twice")
	}
	status, _ = request(t, c, "POST", h.URL+"/api/v1/devices/one/commands", csrf, map[string]any{"command": "encoder", "delta": -1}, map[string]string{"Idempotency-Key": "test-command-1"})
	if status != 409 {
		t.Fatal("idempotency payload mismatch accepted")
	}
}
func TestUnknownCommandOutcomeIsNeverAutomaticallyRetried(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	a := &countedAdapter{Adapter: device.NewSimulator("lost", "pico-pro"), failure: context.DeadlineExceeded}
	installTestDevice(t, s, "lost", a)
	for i := 0; i < 2; i++ {
		status, p := request(t, c, "POST", h.URL+"/api/v1/devices/lost/commands", csrf, map[string]string{"command": "stop"}, map[string]string{"Idempotency-Key": "unconfirmed-command"})
		if (i == 0 && status != 502) || (i == 1 && (status != 200 || !bytes.Contains(p, []byte(`"state":"unknown"`)))) {
			t.Fatalf("outcome: %d %s", status, p)
		}
	}
	if a.calls.Load() != 1 {
		t.Fatal("uncertain command was retransmitted")
	}
}
func TestDeploymentDoesNotStartMachine(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	a := &countedAdapter{Adapter: device.NewSimulator("pro", "pico-pro")}
	installTestDevice(t, s, "pro", a)
	status, p := request(t, c, "POST", h.URL+"/api/v1/recipes", csrf, validRecipe(), nil)
	if status != 201 {
		t.Fatalf("recipe %d %s", status, p)
	}
	var v domain.Recipe
	_ = json.Unmarshal(p, &v)
	status, p = request(t, c, "POST", h.URL+"/api/v1/devices/pro/deploy", csrf, map[string]any{"recipe_id": v.ID, "profile_id": "pico-pro", "batch_liters": 5, "source_revision": v.Revision}, nil)
	if status != 200 {
		t.Fatalf("deploy %d %s", status, p)
	}
	st, err := a.Status(context.Background())
	if err != nil || st.Busy || a.calls.Load() != 0 {
		t.Fatalf("deployment started program: %+v %v", st, err)
	}
	items, err := a.Recipes(context.Background())
	if err != nil || len(items) == 0 {
		t.Fatal("native recipe was not written", err)
	}
	status, _ = request(t, c, "POST", h.URL+"/api/v1/devices/pro/deploy", csrf, map[string]any{"recipe_id": v.ID, "profile_id": "z", "batch_liters": 9.5, "source_revision": v.Revision}, nil)
	if status != 422 {
		t.Fatal("cross-machine deployment accepted")
	}
}
func TestStaticAssetsStayInsideWebRoot(t *testing.T) {
	s, h, c := newTestServer(t)
	dir := t.TempDir()
	s.cfg.WebDir = filepath.Join(dir, "web")
	_ = os.Mkdir(s.cfg.WebDir, 0700)
	_ = os.WriteFile(filepath.Join(s.cfg.WebDir, "index.html"), []byte("PICO SPA"), 0600)
	_ = os.WriteFile(filepath.Join(dir, "secret.txt"), []byte("PRIVATE OUTSIDE"), 0600)
	for _, path := range []string{"/devices/test", "/../secret.txt", "/%2e%2e/secret.txt"} {
		status, p := request(t, c, "GET", h.URL+path, "", nil, nil)
		if bytes.Contains(p, []byte("PRIVATE OUTSIDE")) {
			t.Fatalf("traversal %s %d", path, status)
		}
	}
	status, p := request(t, c, "GET", h.URL+"/devices/test", "", nil, nil)
	if status != 200 || string(p) != "PICO SPA" {
		t.Fatalf("SPA fallback %d %s", status, p)
	}
}

func TestLoginRotationAndExpiredSessionCannotMutate(t *testing.T) {
	s, h, c := newTestServer(t)
	_ = signIn(t, c, h.URL)
	requestURL, _ := http.NewRequest("GET", h.URL, nil)
	old := c.Jar.Cookies(requestURL.URL)[0]
	csrf := signIn(t, c, h.URL)
	req := httptest.NewRequest("POST", "http://localhost/api/v1/auth/logout", nil)
	req.AddCookie(old)
	req.Header.Set("X-CSRF-Token", csrf)
	rr := httptest.NewRecorder()
	s.ServeHTTP(rr, req)
	if rr.Code != 401 {
		t.Fatalf("rotated cookie remained valid: %d", rr.Code)
	}
	s.auth.mu.Lock()
	s.auth.sessions["expired"] = session{CSRF: "expired-token", Expires: time.Now().Add(-time.Hour)}
	s.auth.mu.Unlock()
	req = httptest.NewRequest("POST", "http://localhost/api/v1/recipes", strings.NewReader(`{}`))
	req.AddCookie(&http.Cookie{Name: "pico_hub_session", Value: "expired"})
	req.Header.Set("Content-Type", "application/json")
	rr = httptest.NewRecorder()
	s.ServeHTTP(rr, req)
	if rr.Code != 401 {
		t.Fatalf("expired session plus empty CSRF accepted: %d", rr.Code)
	}
}
