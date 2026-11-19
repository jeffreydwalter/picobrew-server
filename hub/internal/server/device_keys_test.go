package server

import (
	"bytes"
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"fmt"
	"io"
	"log/slog"
	"net/http"
	"strings"
	"sync"
	"testing"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

func keyDeviceInput(name, key string) map[string]any {
	url := "http://" + strings.ReplaceAll(strings.ToLower(name), " ", "-") + ".example"
	return map[string]any{"name": name, "adapter": "pico-standalone", "model": "pico-pro", "base_url": url, "api_key": key, "enabled": false}
}

func saveKeyDevice(t *testing.T, c *http.Client, url, csrf, id string, body map[string]any) domain.DeviceView {
	t.Helper()
	method, want := "POST", http.StatusCreated
	if id != "" {
		method, want = "PUT", http.StatusOK
		url += "/" + id
	}
	status, p := request(t, c, method, url, csrf, body, nil)
	if status != want {
		t.Fatalf("save device: %d %s", status, p)
	}
	var view domain.DeviceView
	if err := json.Unmarshal(p, &view); err != nil {
		t.Fatal(err)
	}
	if _, ok := mapFromJSON(t, p)["key_reused"]; !ok {
		t.Fatal("device response omitted its reuse advisory")
	}
	return view
}

func mapFromJSON(t *testing.T, p []byte) map[string]any {
	t.Helper()
	var value map[string]any
	if err := json.Unmarshal(p, &value); err != nil {
		t.Fatal(err)
	}
	return value
}

func assertReuseViews(t *testing.T, views []domain.DeviceView, want map[string]bool) {
	t.Helper()
	for _, view := range views {
		expected, ok := want[view.ID]
		if !ok || view.KeyReused != expected {
			t.Fatalf("unexpected reuse advisory for %s: %v (expected %v, registered %v)", view.ID, view.KeyReused, expected, ok)
		}
		delete(want, view.ID)
	}
	if len(want) != 0 {
		t.Fatalf("missing device views: %v", want)
	}
}

func TestDeviceKeyReuseAdvisoryTracksRegistrationChanges(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	url := h.URL + "/api/v1/devices"
	key := strings.Repeat("ab", 32)
	a := saveKeyDevice(t, c, url, csrf, "", keyDeviceInput("First brewer", key))
	if a.KeyReused {
		t.Fatal("a single registration matched itself")
	}
	b := saveKeyDevice(t, c, url, csrf, "", keyDeviceInput("Second brewer", key))
	if !b.KeyReused {
		t.Fatal("save response did not warn about the reused key")
	}
	assertReuseViews(t, s.fleet(), map[string]bool{a.ID: true, b.ID: true})

	// Reusing a provisioned key stays allowed. Keys and private comparison
	// fingerprints never enter any authenticated device response.
	decoded, _ := hex.DecodeString(key)
	fingerprint := fmt.Sprintf("%x", sha256.Sum256(decoded))
	for _, path := range []string{"", "/" + a.ID, "/" + b.ID} {
		status, p := request(t, c, "GET", url+path, "", nil, nil)
		if status != 200 || bytes.Contains(p, []byte(key)) || bytes.Contains(p, []byte(fingerprint)) {
			t.Fatalf("credential material escaped through %s: %d", path, status)
		}
	}

	// Blank edits preserve the exact stored token and its advisory.
	a = saveKeyDevice(t, c, url, csrf, a.ID, keyDeviceInput("Renamed brewer", ""))
	if !a.KeyReused {
		t.Fatal("blank edit incorrectly cleared the advisory")
	}
	if stored, err := s.readSecret("device:" + a.ID); err != nil || stored != key {
		t.Fatal("blank edit changed the stored credential", err)
	}
	sim := saveKeyDevice(t, c, url, csrf, "", map[string]any{"name": "Simulator", "adapter": "simulator", "model": "pico-pro", "api_key": key, "enabled": false})
	if sim.KeyReused || sim.KeyConfigured {
		t.Fatal("simulator participated in physical credential reuse")
	}

	// Moving a registration to a distinct existing device credential updates
	// both old members; this operation does not change either appliance.
	b = saveKeyDevice(t, c, url, csrf, b.ID, keyDeviceInput("Second brewer", strings.Repeat("cd", 32)))
	assertReuseViews(t, s.fleet(), map[string]bool{a.ID: false, b.ID: false, sim.ID: false})

	// The advisory compares canonical hex values but authentication preserves
	// case: an uppercase paste is still sent exactly as supplied to the brewer.
	b = saveKeyDevice(t, c, url, csrf, b.ID, keyDeviceInput("Second brewer", strings.ToUpper(key)))
	assertReuseViews(t, s.fleet(), map[string]bool{a.ID: true, b.ID: true, sim.ID: false})
	if stored, err := s.readSecret("device:" + b.ID); err != nil || stored != strings.ToUpper(key) {
		t.Fatal("advisory comparison altered authentication bytes", err)
	}
	if _, err := s.db.Put("brew", "retained", domain.Brew{ID: "retained", DeviceID: b.ID, Notes: "Keep this brew"}, 0); err != nil {
		t.Fatal(err)
	}
	status, p := request(t, c, "DELETE", url+"/"+b.ID, csrf, nil, nil)
	if status != 200 {
		t.Fatalf("remove: %d %s", status, p)
	}
	assertReuseViews(t, s.fleet(), map[string]bool{a.ID: false, sim.ID: false})
	var brew domain.Brew
	if _, err := s.db.Get("brew", "retained", &brew); err != nil || brew.Notes != "Keep this brew" {
		t.Fatal("removing a connection erased archived history", err)
	}
	rows, err := s.db.List("device")
	if err != nil {
		t.Fatal(err)
	}
	for _, row := range rows {
		if bytes.Contains(row, []byte("key_reused")) || bytes.Contains(row, []byte(fingerprint)) || bytes.Contains(row, []byte(key)) {
			t.Fatal("derived credential metadata was persisted in a device record")
		}
	}
	injected := keyDeviceInput("Ignored client warning", key)
	injected["key_reused"] = false
	if status, _ := request(t, c, "POST", url, csrf, injected, nil); status != 400 {
		t.Fatal("client-supplied reuse metadata was accepted", status)
	}
}

func TestDeviceKeyReuseAdvisoryRestartsAndCredentialFailureIsVisible(t *testing.T) {
	cfg := Config{DataDir: t.TempDir(), Password: ownerPassword, PollInterval: time.Hour, Logger: slog.New(slog.NewTextHandler(io.Discard, nil))}
	s, err := New(cfg)
	if err != nil {
		t.Fatal(err)
	}
	key := strings.Repeat("a1", 32)
	devices := []domain.Device{
		{ID: "first", Name: "First", Adapter: "pico-standalone", Model: "pico-pro", BaseURL: "http://192.0.2.11", KeyConfigured: true},
		{ID: "second", Name: "Second", Adapter: "pico-standalone", Model: "pico-s", BaseURL: "http://192.0.2.12", KeyConfigured: true},
	}
	for _, d := range devices {
		secretID := "device:" + d.ID
		if _, err := s.db.PutWithSecret("device", d.ID, d, 0, secretID, s.vault.seal(secretID, key)); err != nil {
			_ = s.Close()
			t.Fatal(err)
		}
	}
	if err := s.Close(); err != nil {
		t.Fatal(err)
	}
	s, err = New(cfg)
	if err != nil {
		t.Fatal(err)
	}
	defer s.Close()
	assertReuseViews(t, s.fleet(), map[string]bool{"first": true, "second": true})

	// Fleet/SSE reads use the startup cache rather than repeatedly decrypting
	// records. A subsequent attachment must report unreadable credentials.
	privateCorruption := []byte("private-corrupt-ciphertext")
	if err := s.db.SaveSecret("device:second", privateCorruption); err != nil {
		t.Fatal(err)
	}
	assertReuseViews(t, s.fleet(), map[string]bool{"first": true, "second": true})
	s.attach(devices[1])
	for _, view := range s.fleet() {
		if view.ID == "second" && (view.Error != "Device credentials or address could not be loaded" || view.Online || view.KeyReused) {
			t.Fatalf("credential failure was hidden or leaked detail: %+v", view)
		}
	}
	if err := s.saveSecret("device:second", key); err != nil {
		t.Fatal(err)
	}
	s.attach(devices[1])
	assertReuseViews(t, s.fleet(), map[string]bool{"first": true, "second": true})
}

func TestDeviceKeyReuseSnapshotsDuringConcurrentRegistrationUpdates(t *testing.T) {
	s, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	url := h.URL + "/api/v1/devices"
	key := strings.Repeat("b2", 32)
	a := saveKeyDevice(t, c, url, csrf, "", keyDeviceInput("First", key))
	b := saveKeyDevice(t, c, url, csrf, "", keyDeviceInput("Second", key))
	var wg sync.WaitGroup
	for range 4 {
		wg.Add(1)
		go func() {
			defer wg.Done()
			for range 50 {
				views := s.fleet()
				for _, view := range views {
					if !view.KeyReused {
						t.Error("fleet snapshot observed an intermediate registration advisory")
					}
				}
			}
		}()
	}
	for range 20 {
		saveKeyDevice(t, c, url, csrf, b.ID, keyDeviceInput("Second", ""))
	}
	wg.Wait()
	assertReuseViews(t, s.fleet(), map[string]bool{a.ID: true, b.ID: true})
}
