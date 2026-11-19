// Package store keeps hub records, opaque encrypted secrets and brew telemetry
// in one SQLite database. Record versions are independent of JSON contents.
package store

import (
	"database/sql"
	"encoding/json"
	"errors"
	"fmt"
	"net/url"
	"os"
	"path/filepath"
	"strings"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	_ "github.com/mattn/go-sqlite3"
)

var (
	ErrNotFound = errors.New("store: not found")
	ErrConflict = errors.New("store: version conflict")
)

type Store struct{ db *sql.DB }

// Open creates or opens a private database, enables WAL with a five-second busy
// timeout, and applies schema changes atomically. Close releases its connections.
func Open(path string) (*Store, error) {
	if strings.TrimSpace(path) == "" {
		return nil, errors.New("store: database path is required")
	}
	var dsn string
	if path == ":memory:" {
		dsn = ":memory:"
	} else {
		absolute, err := filepath.Abs(path)
		if err != nil {
			return nil, err
		}
		if err := os.MkdirAll(filepath.Dir(absolute), 0700); err != nil {
			return nil, err
		}
		file, err := os.OpenFile(absolute, os.O_CREATE|os.O_RDWR, 0600)
		if err != nil {
			return nil, err
		}
		if err := file.Close(); err != nil {
			return nil, err
		}
		dsn = (&url.URL{Scheme: "file", Path: absolute}).String()
	}
	db, err := sql.Open("sqlite3", dsn+"?_busy_timeout=5000&_foreign_keys=on&_journal_mode=WAL&_synchronous=FULL&_txlock=immediate")
	if err != nil {
		return nil, err
	}
	// A single connection also preserves the identity of :memory: databases.
	// WAL and the busy timeout coordinate independent hub processes/handles.
	db.SetMaxOpenConns(1)
	db.SetMaxIdleConns(1)
	s := &Store{db: db}
	if err := s.migrate(); err != nil {
		_ = db.Close()
		return nil, fmt.Errorf("store: open database: %w", err)
	}
	return s, nil
}

func (s *Store) migrate() error {
	tx, err := s.db.Begin()
	if err != nil {
		return err
	}
	defer tx.Rollback()
	var version int
	if err := tx.QueryRow("PRAGMA user_version").Scan(&version); err != nil {
		return err
	}
	if version > 1 {
		return fmt.Errorf("unsupported database schema version %d", version)
	}
	if version == 0 {
		for _, statement := range []string{
			`CREATE TABLE records (kind TEXT NOT NULL, id TEXT NOT NULL, version INTEGER NOT NULL CHECK(version > 0), payload BLOB NOT NULL, PRIMARY KEY(kind,id))`,
			`CREATE TABLE secrets (id TEXT PRIMARY KEY NOT NULL, ciphertext BLOB NOT NULL)`,
			`CREATE TABLE samples (brew_id TEXT NOT NULL, source TEXT NOT NULL, elapsed_ms INTEGER NOT NULL CHECK(elapsed_ms >= 0), payload BLOB NOT NULL, PRIMARY KEY(brew_id,source,elapsed_ms))`,
			`CREATE INDEX samples_chronological ON samples(brew_id,elapsed_ms,source)`,
			`PRAGMA user_version = 1`,
		} {
			if _, err := tx.Exec(statement); err != nil {
				return err
			}
		}
	}
	return tx.Commit()
}

func (s *Store) Close() error { return s.db.Close() }

func key(kind, id string) error {
	if strings.TrimSpace(kind) == "" || strings.TrimSpace(id) == "" {
		return errors.New("store: kind and id are required")
	}
	return nil
}

func (s *Store) Get(kind, id string, dst any) (int64, error) {
	if err := key(kind, id); err != nil {
		return 0, err
	}
	var payload []byte
	var version int64
	err := s.db.QueryRow("SELECT version,payload FROM records WHERE kind=? AND id=?", kind, id).Scan(&version, &payload)
	if errors.Is(err, sql.ErrNoRows) {
		return 0, ErrNotFound
	}
	if err != nil {
		return 0, err
	}
	if err := json.Unmarshal(payload, dst); err != nil {
		return 0, fmt.Errorf("store: decode %s: %w", kind, err)
	}
	return version, nil
}

// Put atomically creates/replaces JSON. expectedVersion=-1 is unconditional,
// zero is create-only, and a positive version is compare-and-swap. Conflicting
// writes never alter the previous record. A successful version starts at one.
func (s *Store) Put(kind, id string, value any, expectedVersion int64) (int64, error) {
	payload, err := recordPayload(kind, id, value, expectedVersion)
	if err != nil {
		return 0, err
	}
	return putRecord(s.db, kind, id, payload, expectedVersion)
}

func recordPayload(kind, id string, value any, expectedVersion int64) ([]byte, error) {
	if err := key(kind, id); err != nil {
		return nil, err
	}
	if expectedVersion < -1 {
		return nil, errors.New("store: invalid expected version")
	}
	return json.Marshal(value)
}

type recordWriter interface{ QueryRow(string, ...any) *sql.Row }

func putRecord(writer recordWriter, kind, id string, payload []byte, expectedVersion int64) (int64, error) {
	var version int64
	var err error
	switch {
	case expectedVersion == -1:
		err = writer.QueryRow(`INSERT INTO records(kind,id,version,payload) VALUES(?,?,1,?) ON CONFLICT(kind,id) DO UPDATE SET version=records.version+1,payload=excluded.payload RETURNING version`, kind, id, payload).Scan(&version)
	case expectedVersion == 0:
		err = writer.QueryRow(`INSERT INTO records(kind,id,version,payload) VALUES(?,?,1,?) ON CONFLICT(kind,id) DO NOTHING RETURNING version`, kind, id, payload).Scan(&version)
	default:
		err = writer.QueryRow(`UPDATE records SET version=version+1,payload=? WHERE kind=? AND id=? AND version=? RETURNING version`, payload, kind, id, expectedVersion).Scan(&version)
	}
	if errors.Is(err, sql.ErrNoRows) {
		return 0, ErrConflict
	}
	return version, err
}

// PutWithSecret commits a versioned record and its encrypted credential together.
// Nil ciphertext leaves the existing secret untouched. An optimistic conflict,
// secret write failure or failed commit rolls back both changes.
func (s *Store) PutWithSecret(kind, id string, value any, expectedVersion int64, secretID string, ciphertext []byte) (int64, error) {
	payload, err := recordPayload(kind, id, value, expectedVersion)
	if err != nil {
		return 0, err
	}
	if ciphertext != nil && (strings.TrimSpace(secretID) == "" || len(ciphertext) == 0) {
		return 0, errors.New("store: secret id and encrypted bytes are required")
	}
	tx, err := s.db.Begin()
	if err != nil {
		return 0, err
	}
	defer tx.Rollback()
	version, err := putRecord(tx, kind, id, payload, expectedVersion)
	if err != nil {
		return 0, err
	}
	if ciphertext != nil {
		if _, err := tx.Exec("INSERT INTO secrets(id,ciphertext) VALUES(?,?) ON CONFLICT(id) DO UPDATE SET ciphertext=excluded.ciphertext", secretID, ciphertext); err != nil {
			return 0, err
		}
	}
	if err := tx.Commit(); err != nil {
		return 0, err
	}
	return version, nil
}

// List returns records in stable identifier order. Versions are retrieved with
// Get; application fields such as Recipe.Revision remain application-owned.
func (s *Store) List(kind string) ([]json.RawMessage, error) {
	rows, err := s.db.Query("SELECT payload FROM records WHERE kind=? ORDER BY id", kind)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	result := make([]json.RawMessage, 0)
	for rows.Next() {
		var payload []byte
		if err := rows.Scan(&payload); err != nil {
			return nil, err
		}
		result = append(result, json.RawMessage(payload))
	}
	return result, rows.Err()
}

func (s *Store) Delete(kind, id string) error {
	if err := key(kind, id); err != nil {
		return err
	}
	_, err := s.db.Exec("DELETE FROM records WHERE kind=? AND id=?", kind, id)
	return err
}

// Secret methods store bytes already encrypted by the application. This package
// does not possess an encryption key and never treats these bytes as plaintext.
func (s *Store) SaveSecret(id string, encrypted []byte) error {
	if strings.TrimSpace(id) == "" || len(encrypted) == 0 {
		return errors.New("store: secret id and encrypted bytes are required")
	}
	_, err := s.db.Exec("INSERT INTO secrets(id,ciphertext) VALUES(?,?) ON CONFLICT(id) DO UPDATE SET ciphertext=excluded.ciphertext", id, encrypted)
	return err
}
func (s *Store) GetSecret(id string) ([]byte, error) {
	var encrypted []byte
	err := s.db.QueryRow("SELECT ciphertext FROM secrets WHERE id=?", id).Scan(&encrypted)
	if errors.Is(err, sql.ErrNoRows) {
		return nil, ErrNotFound
	}
	return encrypted, err
}
func (s *Store) DeleteSecret(id string) error {
	_, err := s.db.Exec("DELETE FROM secrets WHERE id=?", id)
	return err
}

// AppendSamples commits one complete batch. A repeated source/time pair replaces
// that sample, permitting corrected readings without growing the history twice.
func (s *Store) AppendSamples(brewID string, samples []domain.Sample) error {
	if strings.TrimSpace(brewID) == "" {
		return errors.New("store: brew id is required")
	}
	if len(samples) == 0 {
		return nil
	}
	payloads := make([][]byte, len(samples))
	for i, sample := range samples {
		if sample.ElapsedMS < 0 {
			return errors.New("store: elapsed time cannot be negative")
		}
		payload, err := json.Marshal(sample)
		if err != nil {
			return err
		}
		payloads[i] = payload
	}
	tx, err := s.db.Begin()
	if err != nil {
		return err
	}
	defer tx.Rollback()
	statement, err := tx.Prepare("INSERT INTO samples(brew_id,source,elapsed_ms,payload) VALUES(?,?,?,?) ON CONFLICT(brew_id,source,elapsed_ms) DO UPDATE SET payload=excluded.payload")
	if err != nil {
		return err
	}
	defer statement.Close()
	for i, sample := range samples {
		if _, err := statement.Exec(brewID, sample.Source, sample.ElapsedMS, payloads[i]); err != nil {
			return err
		}
	}
	return tx.Commit()
}

func (s *Store) Samples(brewID string) ([]domain.Sample, error) {
	rows, err := s.db.Query("SELECT payload FROM samples WHERE brew_id=? ORDER BY elapsed_ms,source", brewID)
	if err != nil {
		return nil, err
	}
	defer rows.Close()
	result := make([]domain.Sample, 0)
	for rows.Next() {
		var payload []byte
		var sample domain.Sample
		if err := rows.Scan(&payload); err != nil {
			return nil, err
		}
		if err := json.Unmarshal(payload, &sample); err != nil {
			return nil, err
		}
		result = append(result, sample)
	}
	return result, rows.Err()
}

func (s *Store) SampleCount(brewID string) (int, error) {
	var count int
	err := s.db.QueryRow("SELECT COUNT(*) FROM samples WHERE brew_id=?", brewID).Scan(&count)
	return count, err
}
