package store

import (
	"context"
	"database/sql"
	"encoding/json"
	"errors"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

// Snapshot exposes only records and telemetry from one database transaction.
// Secrets are deliberately unavailable. It is valid only during ReadSnapshot.
type Snapshot struct {
	tx  *sql.Tx
	ctx context.Context
}

// ReadSnapshot keeps an export consistent while records and samples change.
// The visitor must not call other Store methods: the snapshot owns its single
// connection until the callback returns. Callback errors abort the transaction.
func (s *Store) ReadSnapshot(ctx context.Context, visit func(*Snapshot) error) error {
	if visit == nil {
		return errors.New("store: snapshot visitor is required")
	}
	tx, err := s.db.BeginTx(ctx, &sql.TxOptions{ReadOnly: true})
	if err != nil {
		return err
	}
	defer tx.Rollback()
	if err := visit(&Snapshot{tx: tx, ctx: ctx}); err != nil {
		return err
	}
	return tx.Commit()
}

func (s *Snapshot) List(kind string) ([]json.RawMessage, error) {
	rows, err := s.tx.QueryContext(s.ctx, "SELECT payload FROM records WHERE kind=? ORDER BY id", kind)
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
		if !json.Valid(payload) {
			return nil, errors.New("store: corrupt record in snapshot")
		}
		result = append(result, json.RawMessage(payload))
	}
	return result, rows.Err()
}

func (s *Snapshot) Samples(brewID string) ([]domain.Sample, error) {
	rows, err := s.tx.QueryContext(s.ctx, "SELECT payload FROM samples WHERE brew_id=? ORDER BY elapsed_ms,source", brewID)
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
