package server

import (
	"archive/zip"
	"bytes"
	"context"
	"encoding/json"
	"errors"
	"io"
	"log/slog"
	"net/http/httptest"
	"os"
	"path/filepath"
	"strings"
	"testing"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/device"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

// Embedding supplies unrelated adapter operations; history tests deliberately
// implement only the two read operations and never invoke machine commands.
type historyAdapter struct {
	device.Adapter
	catalog func(context.Context) (domain.History, error)
	page    func(context.Context, uint64, int) ([]domain.Sample, *int, error)
}

func (a *historyAdapter) History(ctx context.Context) (domain.History, error) { return a.catalog(ctx) }
func (a *historyAdapter) HistoryPage(ctx context.Context, id uint64, offset int) ([]domain.Sample, *int, error) {
	return a.page(ctx, id, offset)
}
func historyFixture(t *testing.T, path string) (*Server, *runtimeDevice, *historyAdapter) {
	t.Helper()
	db, err := store.Open(path)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = db.Close() })
	a := &historyAdapter{}
	d := &runtimeDevice{view: domain.DeviceView{Device: domain.Device{ID: "machine", Name: "Brewer", Model: "pico-pro"}}, adapter: a}
	return &Server{db: db}, d, a
}
func historyDescription(id uint64, count int) domain.HistorySession {
	return domain.HistorySession{Available: true, ID: id, State: "completed", Command: "brew", RecipeName: "Ale", RecordCount: count, PersistedRecordCount: count, DurationMS: int64(count) * 1000}
}
func historyInt(value int) *int { return &value }

func TestHistorySyncPreservesEditsDuringNetworkRead(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	h := historyDescription(1, 1)
	a.catalog = func(context.Context) (domain.History, error) {
		return domain.History{Sessions: []domain.HistorySession{h}}, nil
	}
	started := time.Now().UTC().Add(-time.Hour)
	b := domain.Brew{ID: "machine-1", DeviceID: "machine", DeviceHistoryID: 1, ObservedAt: started, StartedAt: &started, Notes: "before", Rating: 1}
	if _, err := s.db.Put("brew", b.ID, b, 0); err != nil {
		t.Fatal(err)
	}
	entered, release := make(chan struct{}), make(chan struct{})
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		close(entered)
		<-release
		return []domain.Sample{{ElapsedMS: 1000, Phase: 3}}, nil, nil
	}
	done := make(chan error, 1)
	go func() { done <- s.pullHistory(context.Background(), d) }()
	<-entered
	var edited domain.Brew
	version, err := s.db.Get("brew", b.ID, &edited)
	if err != nil {
		t.Fatal(err)
	}
	edited.Notes = "edited during sync"
	edited.Rating = 5
	edited.RecipeID = "shared-recipe"
	edited.RecipeRevision = 8
	if _, err := s.db.Put("brew", b.ID, edited, version); err != nil {
		t.Fatal(err)
	}
	close(release)
	if err := <-done; err != nil {
		t.Fatal(err)
	}
	var got domain.Brew
	if _, err := s.db.Get("brew", b.ID, &got); err != nil {
		t.Fatal(err)
	}
	if got.Notes != edited.Notes || got.Rating != 5 || got.RecipeID != "shared-recipe" || got.RecipeRevision != 8 {
		t.Fatalf("user metadata overwritten: %+v", got)
	}
	if got.SampleCount != 1 || got.State != "completed" || got.Truncated || !got.ObservedAt.Equal(started) || !got.StartedAt.Equal(started) {
		t.Fatalf("device metadata: %+v", got)
	}
}

func TestCompletedHistoryFingerprintSurvivesRestartAndInvalidatesChanges(t *testing.T) {
	path := filepath.Join(t.TempDir(), "hub.db")
	s, d, a := historyFixture(t, path)
	h := historyDescription(1, 1)
	calls := 0
	a.catalog = func(context.Context) (domain.History, error) {
		return domain.History{Sessions: []domain.HistorySession{h}}, nil
	}
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		calls++
		samples := []domain.Sample{{ElapsedMS: 1000}}
		if h.RecordCount == 2 {
			samples = append(samples, domain.Sample{ElapsedMS: 2000})
		}
		return samples, nil, nil
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	if calls != 1 {
		t.Fatalf("unchanged completed history downloaded %d times", calls)
	}
	if err := s.db.Close(); err != nil {
		t.Fatal(err)
	}
	reopened, err := store.Open(path)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = reopened.Close() })
	s.db = reopened
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	if calls != 1 {
		t.Fatal("restart lost completion fingerprint")
	}
	h = historyDescription(1, 2)
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	if calls != 2 {
		t.Fatal("changed record count did not trigger fresh download")
	}
	h.State = "recording"
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	if calls != 4 {
		t.Fatal("active ring was incorrectly treated as immutable")
	}
}

func TestPartialHistoryRemainsVisibleRetriesAndDoesNotBlockOtherBrews(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	fail := true
	a.catalog = func(context.Context) (domain.History, error) {
		return domain.History{Sessions: []domain.HistorySession{historyDescription(1, 2), historyDescription(2, 1)}}, nil
	}
	a.page = func(_ context.Context, id uint64, offset int) ([]domain.Sample, *int, error) {
		if id == 2 {
			return []domain.Sample{{ElapsedMS: 1000}}, nil, nil
		}
		if offset == 0 {
			return []domain.Sample{{ElapsedMS: 1000}}, historyInt(1), nil
		}
		if fail {
			return nil, nil, errors.New("connection interrupted")
		}
		return []domain.Sample{{ElapsedMS: 2000}}, nil, nil
	}
	if err := s.pullHistory(context.Background(), d); err == nil {
		t.Fatal("partial import reported success")
	}
	var partial, other domain.Brew
	if _, err := s.db.Get("brew", "machine-1", &partial); err != nil {
		t.Fatal(err)
	}
	if partial.SampleCount != 1 || !partial.Truncated {
		t.Fatalf("partial data hidden: %+v", partial)
	}
	if _, err := s.db.Get("brew", "machine-2", &other); err != nil || other.SampleCount != 1 || other.Truncated {
		t.Fatalf("other brew blocked: %+v %v", other, err)
	}
	var checkpoint historyImport
	if _, err := s.db.Get("history-sync", "machine-1", &checkpoint); !errors.Is(err, store.ErrNotFound) {
		t.Fatalf("partial import cached as complete: %v", err)
	}
	fail = false
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	if _, err := s.db.Get("brew", "machine-1", &partial); err != nil {
		t.Fatal(err)
	}
	if partial.SampleCount != 2 || partial.Truncated {
		t.Fatalf("retry failed: %+v", partial)
	}
	samples, err := s.db.Samples("machine-1")
	if err != nil || len(samples) != 2 {
		t.Fatalf("retry duplicated samples: %+v %v", samples, err)
	}
	for _, sample := range samples {
		if sample.Source != "device" || sample.ObservedAt != nil {
			t.Fatal("completed history invented an absolute timestamp")
		}
	}
}

func TestHistoryRejectsShortAndNonadvancingPages(t *testing.T) {
	for _, mode := range []string{"short", "stuck", "endless"} {
		t.Run(mode, func(t *testing.T) {
			s, d, a := historyFixture(t, ":memory:")
			calls := 0
			a.catalog = func(context.Context) (domain.History, error) {
				return domain.History{Sessions: []domain.HistorySession{historyDescription(1, 2)}}, nil
			}
			a.page = func(_ context.Context, _ uint64, offset int) ([]domain.Sample, *int, error) {
				calls++
				switch mode {
				case "short":
					return []domain.Sample{{ElapsedMS: 1000}}, nil, nil
				case "stuck":
					return []domain.Sample{{ElapsedMS: 1000}}, historyInt(offset), nil
				default:
					return []domain.Sample{{ElapsedMS: int64(offset)}}, historyInt(offset + 1), nil
				}
			}
			if err := s.pullHistory(context.Background(), d); err == nil {
				t.Fatal("invalid pagination accepted")
			}
			if mode == "endless" && calls != 133 {
				t.Fatalf("pagination was not bounded: %d", calls)
			}
			var b domain.Brew
			if _, err := s.db.Get("brew", "machine-1", &b); err != nil || !b.Truncated {
				t.Fatalf("incomplete marker: %+v %v", b, err)
			}
		})
	}
}

func TestHistoryDoesNotOverwriteCorruptMetadata(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	a.catalog = func(context.Context) (domain.History, error) {
		return domain.History{Sessions: []domain.HistorySession{historyDescription(1, 1)}}, nil
	}
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		t.Error("network pages requested after corrupt metadata")
		return nil, nil, nil
	}
	if _, err := s.db.Put("brew", "machine-1", map[string]any{"duration_ms": "corrupt", "notes": "preserve me"}, 0); err != nil {
		t.Fatal(err)
	}
	if err := s.pullHistory(context.Background(), d); err == nil || !strings.Contains(err.Error(), "decode") {
		t.Fatalf("metadata error swallowed: %v", err)
	}
	var retained map[string]any
	if _, err := s.db.Get("brew", "machine-1", &retained); err != nil || retained["notes"] != "preserve me" {
		t.Fatalf("corrupt record replaced: %#v %v", retained, err)
	}
}

func TestHistoryChangingRingIsNotMarkedComplete(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	h := historyDescription(1, 1)
	h.State = "recording"
	reads := 0
	a.catalog = func(context.Context) (domain.History, error) {
		reads++
		if reads > 1 {
			h.FirstElapsedMS = 1000
			h.DurationMS = 2000
		}
		return domain.History{Sessions: []domain.HistorySession{h}}, nil
	}
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		return []domain.Sample{{ElapsedMS: 1000}}, nil, nil
	}
	if err := s.pullHistory(context.Background(), d); err == nil || !strings.Contains(err.Error(), "changed") {
		t.Fatalf("moving ring accepted: %v", err)
	}
	var b domain.Brew
	if _, err := s.db.Get("brew", "machine-1", &b); err != nil || !b.Truncated || b.SampleCount != 1 {
		t.Fatalf("moving ring samples lost: %+v %v", b, err)
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatalf("stable retry: %v", err)
	}
}

func TestSimulatorHistoryRemainsLabeledSimulation(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	d.view.Adapter = "simulator"
	a.catalog = func(context.Context) (domain.History, error) {
		return domain.History{Sessions: []domain.HistorySession{historyDescription(1, 1)}}, nil
	}
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		return []domain.Sample{{ElapsedMS: 1000, Source: "simulation"}}, nil, nil
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	b, err := s.brewDetail("machine-1")
	if err != nil || b.Source != "simulation" || len(b.Samples) != 1 || b.Samples[0].Source != "simulation" {
		t.Fatalf("simulation misrepresented: %+v %v", b, err)
	}
}

func TestHistoryActiveGrowthKeepsDownloadedPrefix(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	before := historyDescription(1, 2)
	before.State = "recording"
	after := historyDescription(1, 3)
	after.State = "recording"
	calls := 0
	a.catalog = func(context.Context) (domain.History, error) {
		calls++
		h := before
		if calls > 1 {
			h = after
		}
		return domain.History{Sessions: []domain.HistorySession{h}}, nil
	}
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		return []domain.Sample{{ElapsedMS: 0}, {ElapsedMS: 1000}}, nil, nil
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatalf("ordinary active growth failed: %v", err)
	}
	b, err := s.brewDetail("machine-1")
	if err != nil || b.DurationMS != 3000 || b.SampleCount != 2 || b.Truncated {
		t.Fatalf("active prefix: %+v %v", b, err)
	}
	var checkpoint historyImport
	if _, err := s.db.Get("history-sync", b.ID, &checkpoint); !errors.Is(err, store.ErrNotFound) {
		t.Fatal("active history was cached as terminal")
	}
}

func TestHistoryEndingDuringReadRetriesFinalTail(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	before := historyDescription(1, 2)
	before.State = "recording"
	after := historyDescription(1, 3)
	calls := 0
	a.catalog = func(context.Context) (domain.History, error) {
		calls++
		h := after
		if calls == 1 {
			h = before
		}
		return domain.History{Sessions: []domain.HistorySession{h}}, nil
	}
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		values := []domain.Sample{{ElapsedMS: 0}, {ElapsedMS: 1000}}
		if calls > 2 {
			values = append(values, domain.Sample{ElapsedMS: 2000})
		}
		return values, nil, nil
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	b, err := s.brewDetail("machine-1")
	if err != nil || !b.Truncated || b.State != "completed" || b.SampleCount != 2 {
		t.Fatalf("terminal tail was claimed complete: %+v %v", b, err)
	}
	var checkpoint historyImport
	if _, err := s.db.Get("history-sync", b.ID, &checkpoint); !errors.Is(err, store.ErrNotFound) {
		t.Fatal("incomplete terminal tail cached")
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	b, err = s.brewDetail("machine-1")
	if err != nil || b.Truncated || b.SampleCount != 3 {
		t.Fatalf("terminal retry failed: %+v %v", b, err)
	}
	if _, err := s.db.Get("history-sync", b.ID, &checkpoint); err != nil {
		t.Fatal("complete terminal tail was not cached")
	}
}

func TestHistoryFirstImportKeepsCatalogOrder(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	a.catalog = func(context.Context) (domain.History, error) {
		return domain.History{Sessions: []domain.HistorySession{historyDescription(3, 1), historyDescription(2, 1), historyDescription(1, 1)}}, nil
	}
	a.page = func(context.Context, uint64, int) ([]domain.Sample, *int, error) {
		return []domain.Sample{{ElapsedMS: 0}}, nil, nil
	}
	if err := s.pullHistory(context.Background(), d); err != nil {
		t.Fatal(err)
	}
	w := httptest.NewRecorder()
	s.listBrews(w, httptest.NewRequest("GET", "/api/v1/brews", nil))
	var got []domain.Brew
	if err := json.Unmarshal(w.Body.Bytes(), &got); err != nil || len(got) != 3 {
		t.Fatalf("list: %s %v", w.Body.String(), err)
	}
	for i, want := range []uint64{3, 2, 1} {
		if got[i].DeviceHistoryID != want || got[i].StartedAt != nil {
			t.Fatalf("catalog order or unknown clock lost: %+v", got)
		}
	}
}

func TestHistoryStopsOnIdentityMismatch(t *testing.T) {
	s, d, a := historyFixture(t, ":memory:")
	d.identityMismatch = true
	a.catalog = func(context.Context) (domain.History, error) {
		t.Fatal("history accessed after identity mismatch")
		return domain.History{}, nil
	}
	if err := s.pullHistory(context.Background(), d); err == nil {
		t.Fatal("mismatched device imported")
	}
}

type failedExportWriter struct{ err error }

func (w failedExportWriter) Write([]byte) (int, error) { return 0, w.err }

func TestBackupChecksArchiveAndExcludesCredentials(t *testing.T) {
	s, _, _ := historyFixture(t, ":memory:")
	s.cfg.DataDir = t.TempDir()
	s.cfg.Logger = slog.New(slog.NewTextHandler(io.Discard, nil))
	b := domain.Brew{ID: "machine-1", SampleCount: 0}
	for kind, value := range map[string]any{"brew": b, "profile": map[string]any{"efficiency": 63}, "ai-advice": map[string]string{"text": "Review the temperature"}, "config": map[string]string{"password": "excluded-password"}} {
		if _, err := s.db.Put(kind, b.ID, value, 0); err != nil {
			t.Fatal(err)
		}
	}
	if err := s.db.SaveSecret("private", []byte("excluded-secret")); err != nil {
		t.Fatal(err)
	}
	if err := s.db.AppendSamples(b.ID, []domain.Sample{{ElapsedMS: 1000, Source: "device"}}); err != nil {
		t.Fatal(err)
	}
	w := httptest.NewRecorder()
	s.backup(w, httptest.NewRequest("GET", "/api/v1/backup", nil))
	if w.Code != 200 || w.Header().Get("Content-Type") != "application/zip" {
		t.Fatalf("export: %d %s", w.Code, w.Body.String())
	}
	z, err := zip.NewReader(bytes.NewReader(w.Body.Bytes()), int64(w.Body.Len()))
	if err != nil {
		t.Fatal(err)
	}
	files := map[string][]byte{}
	for _, f := range z.File {
		r, err := f.Open()
		if err != nil {
			t.Fatal(err)
		}
		data, err := io.ReadAll(r)
		_ = r.Close()
		if err != nil || bytes.Contains(data, []byte("excluded-")) {
			t.Fatalf("invalid or secret-bearing entry %s: %v", f.Name, err)
		}
		files[f.Name] = data
	}
	for _, name := range []string{"device.json", "profile.json", "recipe.json", "recipe-revision.json", "brew.json", "deployment.json", "ai-advice.json", "samples/machine-1.json", "README.txt"} {
		if files[name] == nil {
			t.Fatalf("missing export entry: %s", name)
		}
	}
	var samples []domain.Sample
	if err := json.Unmarshal(files["samples/machine-1.json"], &samples); err != nil || len(samples) != 1 {
		t.Fatalf("samples: %s %v", files["samples/machine-1.json"], err)
	}
	var brews []domain.Brew
	if err := json.Unmarshal(files["brew.json"], &brews); err != nil || len(brews) != 1 || brews[0].SampleCount != len(samples) {
		t.Fatalf("export count did not match snapshot samples: %s %v", files["brew.json"], err)
	}
	entries, err := os.ReadDir(s.cfg.DataDir)
	if err != nil || len(entries) != 0 {
		t.Fatalf("temporary archive was retained: %v %v", entries, err)
	}
	want := errors.New("disk write failed")
	if err := s.writeBackup(context.Background(), failedExportWriter{want}); !errors.Is(err, want) {
		t.Fatalf("archive write/close failure swallowed: %v", err)
	}
}

func TestBackupFailureNeverPublishesPartialZIP(t *testing.T) {
	for _, failure := range []string{"database", "corrupt-brew"} {
		t.Run(failure, func(t *testing.T) {
			s, _, _ := historyFixture(t, ":memory:")
			s.cfg.DataDir = t.TempDir()
			s.cfg.Logger = slog.New(slog.NewTextHandler(io.Discard, nil))
			if failure == "database" {
				_ = s.db.Close()
			} else if _, err := s.db.Put("brew", "broken", map[string]any{"id": "broken", "duration_ms": "invalid"}, 0); err != nil {
				t.Fatal(err)
			}
			w := httptest.NewRecorder()
			s.backup(w, httptest.NewRequest("GET", "/api/v1/backup", nil))
			if w.Code != 500 || !strings.HasPrefix(w.Header().Get("Content-Type"), "application/json") || bytes.HasPrefix(w.Body.Bytes(), []byte("PK")) {
				t.Fatalf("partial archive was published: %d %s %s", w.Code, w.Header().Get("Content-Type"), w.Body.String())
			}
			entries, err := os.ReadDir(s.cfg.DataDir)
			if err != nil || len(entries) != 0 {
				t.Fatalf("failed archive retained: %v %v", entries, err)
			}
		})
	}
}
