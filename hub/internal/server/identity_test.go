package server

import (
	"context"
	"errors"
	"io"
	"log/slog"
	"net/http"
	"net/http/httptest"
	"path/filepath"
	"strings"
	"sync"
	"sync/atomic"
	"testing"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/jeffreydwalter/picobrew-hub/internal/device"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

// These fixtures model authenticated adapter replies without opening any device
// connection. UID changes stand in for an address resolving to another brewer.
type identityAdapter struct {
	device.Adapter
	mu            sync.Mutex
	uid           string
	statusErr     error
	statusEntered chan struct{}
	writes        atomic.Int32
	histories     atomic.Int32
}

func (a *identityAdapter) Discover(context.Context) (domain.Capabilities, error) {
	return domain.Capabilities{APIVersion: 1, FirmwareVersion: "1.0.1", Commands: []string{"stop", "encoder"}}, nil
}
func (a *identityAdapter) Status(context.Context) (domain.Status, error) {
	a.mu.Lock()
	defer a.mu.Unlock()
	if a.statusEntered != nil {
		select {
		case a.statusEntered <- struct{}{}:
		default:
		}
	}
	return domain.Status{DeviceUID: a.uid, Ready: true, Command: "idle"}, a.statusErr
}
func (a *identityAdapter) Command(context.Context, domain.Command) error {
	a.writes.Add(1)
	return nil
}
func (a *identityAdapter) History(context.Context) (domain.History, error) {
	a.histories.Add(1)
	return domain.History{Sessions: []domain.HistorySession{}}, nil
}
func (a *identityAdapter) changeUID(uid string) {
	a.mu.Lock()
	a.uid = uid
	a.mu.Unlock()
}

func identityFixture(t *testing.T, path, uid string) (*Server, *runtimeDevice, *identityAdapter) {
	t.Helper()
	db, err := store.Open(path)
	if err != nil {
		t.Fatal(err)
	}
	ctx, cancel := context.WithCancel(context.Background())
	s := &Server{db: db, ctx: ctx, cancel: cancel, cfg: Config{PollInterval: time.Hour, Logger: slog.New(slog.NewTextHandler(io.Discard, nil))}, devices: map[string]*runtimeDevice{}}
	a := &identityAdapter{uid: uid}
	d := &runtimeDevice{adapter: a, cancel: func() {}, identityMismatch: true, view: domain.DeviceView{Device: domain.Device{ID: "registered", Adapter: "pico-standalone", Model: "pico-pro", Name: "Pico", Enabled: true, BaseURL: "http://192.0.2.10"}}}
	s.devices[d.view.ID] = d
	t.Cleanup(func() { cancel(); s.wg.Wait(); _ = db.Close() })
	return s, d, a
}

func identityRequest(method, path, body string) *http.Request {
	r := httptest.NewRequest(method, path, strings.NewReader(body))
	if body != "" {
		r.Header.Set("Content-Type", "application/json")
	}
	route := chi.NewRouteContext()
	route.URLParams.Add("id", "registered")
	return r.WithContext(context.WithValue(r.Context(), chi.RouteCtxKey, route))
}

func TestPhysicalIdentityPersistsAcrossRestartAndAddressChange(t *testing.T) {
	path := filepath.Join(t.TempDir(), "identity.sqlite")
	uid := strings.Repeat("a", 32)
	s, d, _ := identityFixture(t, path, uid)
	s.refresh(context.Background(), d)
	var pin deviceIdentity
	version, err := s.db.Get("device-identity", "registered", &pin)
	if err != nil || pin.UID != uid || version != 1 || !d.view.Online || d.identityMismatch {
		t.Fatalf("first identity was not persisted: %+v version=%d err=%v", pin, version, err)
	}
	if err := s.db.Close(); err != nil {
		t.Fatal(err)
	}
	restarted, next, adapter := identityFixture(t, path, uid)
	next.view.BaseURL = "http://192.0.2.20"
	restarted.refresh(context.Background(), next)
	version, err = restarted.db.Get("device-identity", "registered", &pin)
	if err != nil || !next.view.Online || next.identityMismatch || pin.UID != uid || version != 1 {
		t.Fatal("same appliance at a new address did not retain its identity")
	}
	adapter.changeUID(strings.Repeat("b", 32))
	restarted.refresh(context.Background(), next)
	if next.view.Online || !next.identityMismatch || next.view.Status.DeviceUID != uid {
		t.Fatal("replacement appliance overwrote the original status")
	}
	version, _ = restarted.db.Get("device-identity", "registered", &pin)
	if pin.UID != uid || version != 1 {
		t.Fatal("persisted identity was silently replaced")
	}
}

func TestIdentityMismatchBlocksControlAndOriginalConnectionRecovers(t *testing.T) {
	uid := strings.Repeat("a", 32)
	s, d, adapter := identityFixture(t, ":memory:", uid)
	s.refresh(context.Background(), d)
	adapter.changeUID(strings.Repeat("b", 32))
	// The mismatch occurs between polls. A write's own fresh status preflight
	// must catch it before dispatching even a supported command.
	r := identityRequest("POST", "/api/v1/devices/registered/commands", `{"command":"stop"}`)
	r.Header.Set("Idempotency-Key", "first-stop-command")
	w := httptest.NewRecorder()
	s.command(w, r)
	if w.Code != 409 || adapter.writes.Load() != 0 || d.view.Online || !d.identityMismatch || s.currentDevice(d) {
		t.Fatalf("wrong appliance received control: status=%d writes=%d body=%s", w.Code, adapter.writes.Load(), w.Body.String())
	}
	for _, method := range []string{"GET", "POST", "PUT", "PATCH", "DELETE"} {
		w := httptest.NewRecorder()
		if _, ok := s.runtime(w, identityRequest(method, "/api/v1/devices/registered", "")); ok || w.Code != 409 {
			t.Fatalf("mismatched runtime accepted %s", method)
		}
	}
	adapter.changeUID(uid)
	w = httptest.NewRecorder()
	s.testDevice(w, identityRequest("POST", "/api/v1/devices/registered/test", ""))
	if w.Code != 200 || !d.view.Online || d.identityMismatch || !s.currentDevice(d) {
		t.Fatalf("original appliance did not recover: %d %s", w.Code, w.Body.String())
	}
	r = identityRequest("POST", "/api/v1/devices/registered/commands", `{"command":"stop"}`)
	r.Header.Set("Idempotency-Key", "second-stop-command")
	w = httptest.NewRecorder()
	s.command(w, r)
	if w.Code != 202 || adapter.writes.Load() != 1 {
		t.Fatalf("restored identity could not operate: %d %s", w.Code, w.Body.String())
	}
}

func TestIdentityMissingOrUnverifiableFailsClosed(t *testing.T) {
	for name, uid := range map[string]string{"empty": "", "long": strings.Repeat("x", 33), "control": "bad\nidentity"} {
		t.Run(name, func(t *testing.T) {
			s, d, _ := identityFixture(t, ":memory:", uid)
			s.refresh(context.Background(), d)
			if d.view.Online || !d.identityMismatch || d.view.Status != nil {
				t.Fatal("invalid identity was accepted")
			}
			var pin deviceIdentity
			if _, err := s.db.Get("device-identity", "registered", &pin); !errors.Is(err, store.ErrNotFound) {
				t.Fatalf("invalid identity was pinned: %v", err)
			}
		})
	}
	t.Run("storage unavailable", func(t *testing.T) {
		s, d, _ := identityFixture(t, ":memory:", strings.Repeat("a", 32))
		_ = s.db.Close()
		s.refresh(context.Background(), d)
		if d.view.Online || !d.identityMismatch || s.currentDevice(d) {
			t.Fatal("unpersisted identity allowed operation")
		}
	})
}

func TestIdentityFirstPinIsAtomicAndNeverOverwritten(t *testing.T) {
	s, d, _ := identityFixture(t, ":memory:", strings.Repeat("a", 32))
	var wg sync.WaitGroup
	for i := range 20 {
		wg.Add(1)
		go func(i int) {
			defer wg.Done()
			uid := strings.Repeat("a", 32)
			if i%2 == 1 {
				uid = strings.Repeat("b", 32)
			}
			if err := s.pinIdentity(d, uid); err != nil {
				var identityErr *device.Error
				if !errors.As(err, &identityErr) || identityErr.Code != "identity_mismatch" {
					t.Error(err)
				}
			}
		}(i)
	}
	wg.Wait()
	var pin deviceIdentity
	version, err := s.db.Get("device-identity", "registered", &pin)
	if err != nil || version != 1 || pin.UID != strings.Repeat("a", 32) && pin.UID != strings.Repeat("b", 32) {
		t.Fatalf("identity creation was not atomic: %+v %d %v", pin, version, err)
	}
}

func TestIdentityMismatchDoesNotScheduleHistory(t *testing.T) {
	s, d, adapter := identityFixture(t, ":memory:", strings.Repeat("a", 32))
	s.refresh(context.Background(), d)
	adapter.mu.Lock()
	adapter.uid = strings.Repeat("b", 32)
	adapter.statusEntered = make(chan struct{}, 1)
	entered := adapter.statusEntered
	adapter.mu.Unlock()
	ctx, cancel := context.WithCancel(context.Background())
	done := make(chan struct{})
	go func() { s.poll(ctx, d); close(done) }()
	select {
	case <-entered:
	case <-time.After(2 * time.Second):
		t.Fatal("poll did not inspect device identity")
	}
	cancel()
	<-done
	s.wg.Wait()
	if adapter.histories.Load() != 0 || d.view.Online || !d.identityMismatch {
		t.Fatal("poll archived a mismatched appliance")
	}
}
