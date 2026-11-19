package store_test

import (
	"context"
	"database/sql"
	"encoding/json"
	"errors"
	"net/url"
	"path/filepath"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
	"github.com/mattn/go-sqlite3"
)

func TestSnapshotKeepsRecordsAndSamplesConsistent(t *testing.T) {
	path := filepath.Join(t.TempDir(), "snapshot.db")
	s := open(t, path)
	if _, err := s.Put("brew", "one", map[string]int{"generation": 1}, 0); err != nil {
		t.Fatal(err)
	}
	if err := s.AppendSamples("one", []domain.Sample{{ElapsedMS: 1}}); err != nil {
		t.Fatal(err)
	}
	other, err := sql.Open("sqlite3", (&url.URL{Scheme: "file", Path: path}).String()+"?_busy_timeout=1")
	if err != nil {
		t.Fatal(err)
	}
	defer other.Close()
	err = s.ReadSnapshot(context.Background(), func(snapshot *store.Snapshot) error {
		before, err := snapshot.List("brew")
		if err != nil {
			return err
		}
		// A separate writer either commits under WAL or is blocked by SQLite's
		// immediate transaction. In either case the snapshot must stay unchanged.
		_, err = other.Exec(`UPDATE records SET payload='{"generation":2}' WHERE kind='brew'`)
		var sqliteErr sqlite3.Error
		if err != nil && (!errors.As(err, &sqliteErr) || sqliteErr.Code != sqlite3.ErrBusy) {
			return err
		}
		after, err := snapshot.List("brew")
		if err != nil {
			return err
		}
		if len(before) != 1 || len(after) != 1 || string(before[0]) != string(after[0]) {
			t.Fatal("export mixed record generations")
		}
		samples, err := snapshot.Samples("one")
		if err != nil || len(samples) != 1 || samples[0].ElapsedMS != 1 {
			t.Fatalf("snapshot samples: %+v %v", samples, err)
		}
		return nil
	})
	if err != nil {
		t.Fatal(err)
	}
	if _, err := other.Exec(`UPDATE records SET payload='{"generation":2}' WHERE kind='brew'`); err != nil {
		t.Fatal("snapshot did not release its transaction", err)
	}
	var updated map[string]int
	if _, err := s.Get("brew", "one", &updated); err != nil || updated["generation"] != 2 {
		t.Fatalf("write after snapshot: %v %v", updated, err)
	}
}

func TestSnapshotPropagatesCancellationAndVisitorFailure(t *testing.T) {
	s := open(t, ":memory:")
	ctx, cancel := context.WithCancel(context.Background())
	cancel()
	if err := s.ReadSnapshot(ctx, func(*store.Snapshot) error { t.Error("cancelled visitor ran"); return nil }); !errors.Is(err, context.Canceled) {
		t.Fatalf("cancellation: %v", err)
	}
	want := errors.New("export failed")
	if err := s.ReadSnapshot(context.Background(), func(*store.Snapshot) error { return want }); !errors.Is(err, want) {
		t.Fatalf("visitor error: %v", err)
	}
	if _, err := s.Put("recipe", "one", json.RawMessage(`{"name":"Ale"}`), 0); err != nil {
		t.Fatal("failed export left connection locked", err)
	}
}
