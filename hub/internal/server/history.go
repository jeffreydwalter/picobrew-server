package server

import (
	"archive/zip"
	"context"
	"encoding/csv"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"os"
	"sort"
	"strconv"
	"strings"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
	"github.com/jeffreydwalter/picobrew-hub/internal/store"
)

type historyImport struct {
	Session     domain.HistorySession `json:"session"`
	SampleCount int                   `json:"sample_count"`
}

func (s *Server) pullHistory(ctx context.Context, d *runtimeDevice) error {
	d.historyMu.Lock()
	defer d.historyMu.Unlock()
	if err := ctx.Err(); err != nil {
		return err
	}
	d.mu.RLock()
	physical := d.view.Adapter == "pico-standalone"
	identityMismatch := d.identityMismatch
	d.mu.RUnlock()
	if identityMismatch {
		return errors.New("machine identity changed; history import was stopped")
	}
	if physical {
		if _, err := s.verifiedStatus(ctx, d); err != nil {
			return err
		}
	}
	history, e := d.adapter.History(ctx)
	if e != nil {
		return e
	}
	d.mu.RLock()
	v := d.view
	d.mu.RUnlock()
	sessions := history.Sessions
	if len(sessions) == 0 && history.Available {
		sessions = []domain.HistorySession{history.HistorySession}
	}
	var result error
	// Firmware catalogs are newest first. Import oldest first so first-observed
	// times preserve that ordering without inventing wall-clock brew start times.
	for i := len(sessions) - 1; i >= 0; i-- {
		h := sessions[i]
		if !h.Available || h.ID == 0 {
			continue
		}
		if err := ctx.Err(); err != nil {
			return errors.Join(result, err)
		}
		if err := s.pullHistorySession(ctx, d, v.Device, h); err != nil {
			// A failed old session must not prevent saving other available brews.
			result = errors.Join(result, fmt.Errorf("history %d: %w", h.ID, err))
		}
	}
	return result
}

func historyDuration(ms int64) (time.Duration, error) {
	if ms < 0 || ms > int64(^uint64(0)>>1)/int64(time.Millisecond) {
		return 0, errors.New("invalid history elapsed time")
	}
	return time.Duration(ms) * time.Millisecond, nil
}

// mergeHistoryBrew changes only device-owned fields. Re-read and compare-and-swap
// after network work so concurrent edits to notes/rating/recipe links survive.
func (s *Server) mergeHistoryBrew(device domain.Device, h domain.HistorySession, complete bool) (domain.Brew, error) {
	duration, err := historyDuration(h.DurationMS)
	if err != nil {
		return domain.Brew{}, err
	}
	id := device.ID + "-" + strconv.FormatUint(h.ID, 10)
	for attempt := 0; attempt < 8; attempt++ {
		var b domain.Brew
		version, err := s.db.Get("brew", id, &b)
		if errors.Is(err, store.ErrNotFound) {
			version = 0
			b = domain.Brew{ID: id, DeviceID: device.ID, DeviceName: device.Name, Model: device.Model, ObservedAt: time.Now().UTC(), DeviceHistoryID: h.ID, Source: "device"}
		} else if err != nil {
			return b, err
		}
		before := b
		if b.StartedAt == nil && h.State == "recording" {
			at := time.Now().UTC().Add(-duration)
			b.StartedAt = &at
		}
		b.RecipeName, b.State, b.Command = h.RecipeName, h.State, h.Command
		b.Source = "device"
		if device.Adapter == "simulator" {
			b.Source = "simulation"
		}
		b.DurationMS, b.GapCount = h.DurationMS, h.GapCount
		b.Truncated = h.Truncated || h.StorageError || !complete
		b.DeviceName = device.Name
		b.SampleCount, err = s.db.SampleCount(id)
		if err != nil {
			return b, err
		}
		if version > 0 && before == b {
			return b, nil
		}
		if _, err = s.db.Put("brew", id, b, version); errors.Is(err, store.ErrConflict) {
			continue
		}
		return b, err
	}
	return domain.Brew{}, fmt.Errorf("history metadata remained busy: %w", store.ErrConflict)
}

func (s *Server) pullHistorySession(ctx context.Context, d *runtimeDevice, device domain.Device, h domain.HistorySession) error {
	id := device.ID + "-" + strconv.FormatUint(h.ID, 10)
	terminal := h.State == "completed" || h.State == "interrupted"
	var previous historyImport
	_, cacheErr := s.db.Get("history-sync", id, &previous)
	if cacheErr != nil && !errors.Is(cacheErr, store.ErrNotFound) {
		return cacheErr
	}
	if terminal && cacheErr == nil && previous.Session == h {
		count, err := s.db.SampleCount(id)
		if err != nil {
			return err
		}
		var existing domain.Brew
		_, existsErr := s.db.Get("brew", id, &existing)
		if existsErr != nil && !errors.Is(existsErr, store.ErrNotFound) {
			return existsErr
		}
		if existsErr == nil && count == previous.SampleCount {
			_, err = s.mergeHistoryBrew(device, h, true)
			return err
		}
	}
	// Invalidate the completion marker before any partial update. A crash or
	// failed page will leave visible partial data and force the next sync to retry.
	if err := s.db.Delete("history-sync", id); err != nil {
		return err
	}
	b, err := s.mergeHistoryBrew(device, h, false)
	if err != nil {
		return err
	}
	count, readErr := s.readHistoryPages(ctx, d, h, b)
	finalDescription := h
	if readErr == nil {
		// The retained ring can move between pages. A completed download is only
		// complete for the metadata snapshot it started with; otherwise retain the
		// collected samples, mark them partial, and retry from offset zero later.
		latest, err := d.adapter.History(ctx)
		if err != nil {
			readErr = err
		} else {
			sessions := latest.Sessions
			if len(sessions) == 0 && latest.Available {
				sessions = []domain.HistorySession{latest.HistorySession}
			}
			found := false
			for _, candidate := range sessions {
				if candidate.ID == h.ID && candidate.Available {
					found = true
					if !historyAppendOnly(h, candidate) {
						readErr = errors.New("history changed while pages were read; partial samples retained for retry")
					} else {
						finalDescription = candidate
					}
					break
				}
			}
			if !found {
				readErr = errors.New("history session was removed while pages were read")
			}
		}
	}
	// Growth after the final page is normal for a recording brew. It does not
	// invalidate the downloaded prefix. A brew that just ended is retried until
	// its final tail is present, rather than cached as a complete terminal record.
	terminal = finalDescription.State == "completed" || finalDescription.State == "interrupted"
	complete := readErr == nil && (!terminal || count >= finalDescription.RecordCount)
	final, mergeErr := s.mergeHistoryBrew(device, finalDescription, complete)
	if readErr != nil || mergeErr != nil {
		return errors.Join(readErr, mergeErr)
	}
	if terminal && complete {
		_, err = s.db.Put("history-sync", id, historyImport{Session: finalDescription, SampleCount: final.SampleCount}, -1)
	}
	return err
}

func historyAppendOnly(before, after domain.HistorySession) bool {
	if before == after {
		return true
	}
	if before.State != "recording" || (after.State != "recording" && after.State != "completed" && after.State != "interrupted") {
		return false
	}
	firstUnchanged := before.FirstElapsedMS == after.FirstElapsedMS || before.RecordCount == 0
	return after.Available && before.ID == after.ID && before.Command == after.Command &&
		before.RecipeName == after.RecipeName && firstUnchanged &&
		before.RecoveredAfterRestart == after.RecoveredAfterRestart &&
		after.DurationMS >= before.DurationMS && after.RecordCount >= before.RecordCount &&
		after.PersistedRecordCount >= before.PersistedRecordCount && after.GapCount >= before.GapCount &&
		(!before.Truncated || after.Truncated) && (!before.StorageError || after.StorageError)
}

func (s *Server) readHistoryPages(ctx context.Context, d *runtimeDevice, h domain.HistorySession, b domain.Brew) (int, error) {
	// Offsets are relative to the retained firmware ring and can shift while a
	// brew records. Re-read that window; an old offset can silently skip new rows.
	offset := 0
	seen := map[int64]struct{}{}
	for pages := 0; pages < 133; pages++ {
		if err := ctx.Err(); err != nil {
			return len(seen), err
		}
		d.mu.RLock()
		identityMismatch := d.identityMismatch
		d.mu.RUnlock()
		if identityMismatch {
			return len(seen), errors.New("machine identity changed; history import was stopped")
		}
		samples, next, err := d.adapter.HistoryPage(ctx, h.ID, offset)
		if err != nil {
			return len(seen), err
		}
		for i := range samples {
			elapsed, err := historyDuration(samples[i].ElapsedMS)
			if err != nil {
				return len(seen), err
			}
			samples[i].Source = b.Source
			seen[samples[i].ElapsedMS] = struct{}{}
			if b.StartedAt != nil {
				at := b.StartedAt.Add(elapsed)
				samples[i].ObservedAt = &at
			}
		}
		if err = s.db.AppendSamples(b.ID, samples); err != nil {
			return len(seen), err
		}
		if next == nil {
			if len(seen) < h.RecordCount {
				return len(seen), fmt.Errorf("incomplete history: received %d of %d advertised records", len(seen), h.RecordCount)
			}
			return len(seen), nil
		}
		if *next <= offset || len(samples) == 0 {
			return len(seen), errors.New("device history pagination did not advance")
		}
		offset = *next
	}
	return len(seen), errors.New("device history exceeds bounded page count")
}
func (s *Server) syncHistory(w http.ResponseWriter, r *http.Request) {
	if d, ok := s.runtime(w, r); ok {
		if e := s.pullHistory(r.Context(), d); e != nil {
			s.deviceError(w, e)
			return
		}
		s.json(w, 200, map[string]bool{"synced": true})
	}
}
func (s *Server) listBrews(w http.ResponseWriter, r *http.Request) {
	rows, e := s.db.List("brew")
	if e != nil {
		s.internal(w, e)
		return
	}
	out := []domain.Brew{}
	for _, raw := range rows {
		var v domain.Brew
		if json.Unmarshal(raw, &v) == nil {
			out = append(out, v)
		}
	}
	sort.Slice(out, func(i, j int) bool { return out[i].ObservedAt.After(out[j].ObservedAt) })
	s.json(w, 200, out)
}
func (s *Server) brewDetail(id string) (domain.BrewDetail, error) {
	var b domain.Brew
	if _, e := s.db.Get("brew", id, &b); e != nil {
		return domain.BrewDetail{}, e
	}
	samples, e := s.db.Samples(id)
	if e == nil {
		b.SampleCount = len(samples)
	}
	return domain.BrewDetail{Brew: b, Samples: samples}, e
}
func (s *Server) getBrew(w http.ResponseWriter, r *http.Request) {
	v, e := s.brewDetail(chi.URLParam(r, "id"))
	if e != nil {
		s.storeError(w, e)
		return
	}
	s.json(w, 200, v)
}
func (s *Server) updateBrew(w http.ResponseWriter, r *http.Request) {
	var in struct {
		Notes  string `json:"notes"`
		Rating int    `json:"rating"`
	}
	if !s.decode(w, r, &in) {
		return
	}
	if len(in.Notes) > 32000 || in.Rating < 0 || in.Rating > 5 {
		s.fail(w, 400, "notes", "Rating must be 0–5 and notes up to 32000 characters")
		return
	}
	id := chi.URLParam(r, "id")
	var b domain.Brew
	version, e := s.db.Get("brew", id, &b)
	if e != nil {
		s.storeError(w, e)
		return
	}
	b.Notes = in.Notes
	b.Rating = in.Rating
	if _, e = s.db.Put("brew", id, b, version); e != nil {
		s.storeError(w, e)
		return
	}
	s.json(w, 200, b)
}
func number(p *float64) string {
	if p == nil {
		return ""
	}
	return strconv.FormatFloat(*p, 'f', 2, 64)
}
func (s *Server) exportBrew(w http.ResponseWriter, r *http.Request) {
	b, e := s.brewDetail(chi.URLParam(r, "id"))
	if e != nil {
		s.storeError(w, e)
		return
	}
	if r.URL.Query().Get("format") == "csv" {
		w.Header().Set("Content-Type", "text/csv; charset=utf-8")
		w.Header().Set("Content-Disposition", `attachment; filename="brew-`+b.ID+`.csv"`)
		out := csv.NewWriter(w)
		defer out.Flush()
		_ = out.Write([]string{"elapsed_ms", "observed_at", "wort_f", "thermoblock_f", "target_f", "step_seconds", "phase", "controller_status", "compartment", "current_step", "flags", "device_state", "source"})
		for _, v := range b.Samples {
			at := ""
			if v.ObservedAt != nil {
				at = v.ObservedAt.Format(time.RFC3339Nano)
			}
			_ = out.Write([]string{strconv.FormatInt(v.ElapsedMS, 10), at, number(v.WortF), number(v.ThermoblockF), number(v.TargetF), strconv.Itoa(v.StepSeconds), strconv.Itoa(v.Phase), strconv.Itoa(v.ControllerStatus), strconv.Itoa(v.Compartment), strconv.Itoa(v.CurrentStep), strconv.Itoa(v.Flags), strconv.Itoa(v.DeviceState), v.Source})
		}
		return
	}
	w.Header().Set("Content-Disposition", `attachment; filename="brew-`+b.ID+`.json"`)
	s.json(w, 200, b)
}
func (s *Server) backup(w http.ResponseWriter, r *http.Request) {
	// Build a complete, checked archive before committing an HTTP success. A DB
	// or disk error must not produce a valid-looking but incomplete download.
	file, err := os.CreateTemp(s.cfg.DataDir, ".hub-export-*.zip")
	if err != nil {
		s.internal(w, err)
		return
	}
	defer func() {
		_ = file.Close()
		_ = os.Remove(file.Name())
	}()
	if err := s.writeBackup(r.Context(), file); err != nil {
		s.internal(w, err)
		return
	}
	if _, err := file.Seek(0, io.SeekStart); err != nil {
		s.internal(w, err)
		return
	}
	w.Header().Set("Content-Type", "application/zip")
	w.Header().Set("Content-Disposition", `attachment; filename="picobrew-hub-export.zip"`)
	http.ServeContent(w, r, "picobrew-hub-export.zip", time.Time{}, file)
}

func (s *Server) writeBackup(ctx context.Context, target io.Writer) error {
	z := zip.NewWriter(target)
	writeJSON := func(name string, value any) error {
		if err := ctx.Err(); err != nil {
			return err
		}
		f, err := z.Create(name)
		if err != nil {
			return err
		}
		return json.NewEncoder(f).Encode(value)
	}
	err := s.db.ReadSnapshot(ctx, func(snapshot *store.Snapshot) error {
		for _, kind := range []string{"device", "profile", "recipe", "recipe-revision", "brew", "deployment", "batch-deployment", "ai-advice"} {
			rows, err := snapshot.List(kind)
			if err != nil {
				return err
			}
			if kind != "brew" {
				if err := writeJSON(kind+".json", rows); err != nil {
					return err
				}
				continue
			}
			for i, raw := range rows {
				var brew domain.Brew
				if err := json.Unmarshal(raw, &brew); err != nil {
					return fmt.Errorf("decode brew for export: %w", err)
				}
				if brew.ID == "" || strings.ContainsAny(brew.ID, "/\\\x00\r\n") || brew.ID == "." || brew.ID == ".." {
					return errors.New("invalid brew identifier in export")
				}
				samples, err := snapshot.Samples(brew.ID)
				if err != nil {
					return err
				}
				if err := writeJSON("samples/"+brew.ID+".json", samples); err != nil {
					return err
				}
				// A background import commits sample batches before its metadata
				// merge. Report the actual count from this same export snapshot.
				brew.SampleCount = len(samples)
				rows[i], err = json.Marshal(brew)
				if err != nil {
					return err
				}
			}
			if err := writeJSON(kind+".json", rows); err != nil {
				return err
			}
		}
		return nil
	})
	if err != nil {
		_ = z.Close()
		return err
	}
	f, err := z.Create("README.txt")
	if err == nil {
		_, err = fmt.Fprint(f, "Data export only. Records and telemetry share one database snapshot. Credentials, account password and master.key are excluded. For complete disaster recovery, stop the server and back up its entire data directory, including hub.sqlite and master.key. Preserve master.key privately.\n")
	}
	return errors.Join(err, z.Close())
}
