package store_test

import (
	"errors"
	"math"
	"path/filepath"
	"reflect"
	"sync"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

func open(t *testing.T, path string) *store.Store {
	t.Helper()
	s, err := store.Open(path)
	if err != nil {
		t.Fatal(err)
	}
	t.Cleanup(func() { _ = s.Close() })
	return s
}

func TestRestartKeepsRecordsSecretsAndSamples(t *testing.T) {
	path := filepath.Join(t.TempDir(), "private", "hub ?#.db")
	s := open(t, path)
	want := domain.Recipe{ID: "ale", Name: "Ale", BatchLiters: 5, Revision: 7}
	if version, err := s.Put("recipes", "ale", want, 0); err != nil || version != 1 {
		t.Fatalf("create: %d %v", version, err)
	}
	if err := s.SaveSecret("device", []byte{1, 2, 3}); err != nil {
		t.Fatal(err)
	}
	if err := s.AppendSamples("brew", []domain.Sample{{ElapsedMS: 1000, Source: "device", Phase: 2}}); err != nil {
		t.Fatal(err)
	}
	if err := s.Close(); err != nil {
		t.Fatal(err)
	}
	s = open(t, path)
	var got domain.Recipe
	version, err := s.Get("recipes", "ale", &got)
	if err != nil || version != 1 || !reflect.DeepEqual(got, want) {
		t.Fatalf("restart: %#v v%d %v", got, version, err)
	}
	secret, err := s.GetSecret("device")
	if err != nil || !reflect.DeepEqual(secret, []byte{1, 2, 3}) {
		t.Fatalf("secret: %v %v", secret, err)
	}
	samples, err := s.Samples("brew")
	if err != nil || len(samples) != 1 || samples[0].Phase != 2 {
		t.Fatalf("samples: %+v %v", samples, err)
	}
	if err := s.DeleteSecret("device"); err != nil {
		t.Fatal(err)
	}
	if _, err := s.GetSecret("device"); !errors.Is(err, store.ErrNotFound) {
		t.Fatalf("deleted secret: %v", err)
	}
}

func TestAtomicVersionsAndSeparateKinds(t *testing.T) {
	s := open(t, ":memory:")
	if v, err := s.Put("recipes", "a", map[string]int{"n": 1}, 0); err != nil || v != 1 {
		t.Fatalf("initial: %d %v", v, err)
	}
	for _, expected := range []int64{0, 2, 100} {
		if _, err := s.Put("recipes", "a", map[string]int{"n": 99}, expected); !errors.Is(err, store.ErrConflict) {
			t.Fatalf("expected conflict for %d: %v", expected, err)
		}
	}
	if v, err := s.Put("recipes", "a", map[string]int{"n": 2}, 1); err != nil || v != 2 {
		t.Fatalf("CAS: %d %v", v, err)
	}
	if v, err := s.Put("recipes", "a", map[string]int{"n": 3}, -1); err != nil || v != 3 {
		t.Fatalf("overwrite: %d %v", v, err)
	}
	if _, err := s.Put("devices", "a", map[string]int{"n": 4}, 0); err != nil {
		t.Fatal(err)
	}
	if _, err := s.Put("recipes", "missing", 1, 5); !errors.Is(err, store.ErrConflict) {
		t.Fatalf("missing CAS: %v", err)
	}
	if _, err := s.Put("recipes", "a", math.NaN(), -1); err == nil {
		t.Fatal("invalid JSON accepted")
	}
	var got map[string]int
	if v, err := s.Get("recipes", "a", &got); err != nil || v != 3 || got["n"] != 3 {
		t.Fatalf("failed mutation changed record: %#v %d %v", got, v, err)
	}
	if err := s.Delete("recipes", "a"); err != nil {
		t.Fatal(err)
	}
	if err := s.Delete("recipes", "a"); err != nil {
		t.Fatal(err)
	}
	if _, err := s.Get("recipes", "a", &got); !errors.Is(err, store.ErrNotFound) {
		t.Fatal(err)
	}
}

func TestPutWithSecretCommitsTogetherAndRollsBackConflict(t *testing.T) {
	s := open(t, ":memory:")
	if version, err := s.PutWithSecret("device", "one", "original", 0, "device:one", []byte("encrypted-one")); err != nil || version != 1 {
		t.Fatalf("create: %d %v", version, err)
	}
	if _, err := s.PutWithSecret("device", "one", "conflicting", 0, "device:one", []byte("encrypted-conflict")); !errors.Is(err, store.ErrConflict) {
		t.Fatalf("conflict: %v", err)
	}
	var record string
	if version, err := s.Get("device", "one", &record); err != nil || version != 1 || record != "original" {
		t.Fatalf("conflict changed record: %q %d %v", record, version, err)
	}
	if secret, err := s.GetSecret("device:one"); err != nil || string(secret) != "encrypted-one" {
		t.Fatalf("conflict changed secret: %q %v", secret, err)
	}
	if version, err := s.PutWithSecret("device", "one", "renamed", 1, "", nil); err != nil || version != 2 {
		t.Fatalf("preserve: %d %v", version, err)
	}
	if secret, err := s.GetSecret("device:one"); err != nil || string(secret) != "encrypted-one" {
		t.Fatalf("nil ciphertext changed secret: %q %v", secret, err)
	}
	if _, err := s.PutWithSecret("device", "one", "invalid secret", 2, "", []byte("encrypted-new")); err == nil {
		t.Fatal("invalid secret accepted")
	}
	if version, err := s.Get("device", "one", &record); err != nil || version != 2 || record != "renamed" {
		t.Fatalf("invalid secret changed record: %q %d %v", record, version, err)
	}
	if version, err := s.PutWithSecret("device", "one", "rotated", 2, "device:one", []byte("encrypted-two")); err != nil || version != 3 {
		t.Fatalf("rotate: %d %v", version, err)
	}
	if secret, err := s.GetSecret("device:one"); err != nil || string(secret) != "encrypted-two" {
		t.Fatalf("rotation: %q %v", secret, err)
	}
}

func TestCompetingHandlesHaveOneCASWinner(t *testing.T) {
	path := filepath.Join(t.TempDir(), "hub.db")
	first, second := open(t, path), open(t, path)
	if _, err := first.Put("recipes", "a", 0, 0); err != nil {
		t.Fatal(err)
	}
	results := make(chan error, 2)
	start := make(chan struct{})
	var wg sync.WaitGroup
	for i, s := range []*store.Store{first, second} {
		wg.Add(1)
		go func(s *store.Store, n int) {
			defer wg.Done()
			<-start
			_, err := s.Put("recipes", "a", n, 1)
			results <- err
		}(s, i)
	}
	close(start)
	wg.Wait()
	close(results)
	wins, conflicts := 0, 0
	for err := range results {
		if err == nil {
			wins++
		} else if errors.Is(err, store.ErrConflict) {
			conflicts++
		} else {
			t.Fatal(err)
		}
	}
	if wins != 1 || conflicts != 1 {
		t.Fatalf("winners %d conflicts %d", wins, conflicts)
	}
}

func TestSamplesDeduplicateByBrewSourceTimeAndSort(t *testing.T) {
	s := open(t, ":memory:")
	values := []domain.Sample{{ElapsedMS: 2000, Source: "z", Phase: 1}, {ElapsedMS: 1000, Source: "b", Phase: 2}, {ElapsedMS: 1000, Source: "a", Phase: 3}}
	if err := s.AppendSamples("one", values); err != nil {
		t.Fatal(err)
	}
	if err := s.AppendSamples("two", values); err != nil {
		t.Fatal(err)
	}
	values[0].Phase = 4
	if err := s.AppendSamples("one", values); err != nil {
		t.Fatal(err)
	}
	got, err := s.Samples("one")
	if err != nil {
		t.Fatal(err)
	}
	if len(got) != 3 || got[0].Source != "a" || got[1].Source != "b" || got[2].Phase != 4 {
		t.Fatalf("ordered/deduped: %+v", got)
	}
	count, err := s.SampleCount("two")
	if err != nil || count != 3 {
		t.Fatalf("other brew: %d %v", count, err)
	}
	bad := math.NaN()
	if err := s.AppendSamples("one", []domain.Sample{{ElapsedMS: 3000}, {ElapsedMS: 4000, WortF: &bad}}); err == nil {
		t.Fatal("invalid batch accepted")
	}
	if count, err := s.SampleCount("one"); err != nil || count != 3 {
		t.Fatalf("partial batch saved: %d %v", count, err)
	}
	if err := s.AppendSamples("one", []domain.Sample{{ElapsedMS: -1}}); err == nil {
		t.Fatal("negative time accepted")
	}
}

func TestListIsOrderedAndEmptyArraysAreNonNil(t *testing.T) {
	s := open(t, ":memory:")
	for _, id := range []string{"b", "a"} {
		if _, err := s.Put("recipes", id, id, 0); err != nil {
			t.Fatal(err)
		}
	}
	list, err := s.List("recipes")
	if err != nil || len(list) != 2 || string(list[0]) != `"a"` {
		t.Fatalf("list: %s %v", list, err)
	}
	if list, err := s.List("none"); err != nil || list == nil || len(list) != 0 {
		t.Fatalf("empty list: %v %v", list, err)
	}
	if samples, err := s.Samples("none"); err != nil || samples == nil || len(samples) != 0 {
		t.Fatalf("empty samples: %v %v", samples, err)
	}
}
