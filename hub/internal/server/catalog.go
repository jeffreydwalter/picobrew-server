package server

import (
	"bytes"
	"errors"
	"io"
	"mime"
	"net/http"
	"os"
	"path"
	"path/filepath"
	"strconv"
	"time"

	"github.com/go-chi/chi/v5"
	"github.com/jeffreydwalter/picobrew-hub/internal/catalog"
	"github.com/jeffreydwalter/picobrew-hub/internal/recipe"
)

func (s *Server) openCatalog(w http.ResponseWriter) (*catalog.Archive, bool) {
	a, err := catalog.Open()
	if err != nil {
		s.internal(w, err)
		return nil, false
	}
	return a, true
}

func (s *Server) catalogError(w http.ResponseWriter, err error) {
	if errors.Is(err, catalog.ErrNotFound) {
		s.fail(w, 404, "not_found", "Catalogue entry or source is unavailable")
		return
	}
	s.internal(w, err)
}

func (s *Server) listCatalog(w http.ResponseWriter, r *http.Request) {
	q := r.URL.Query()
	query := catalog.Query{Query: q.Get("q"), Source: q.Get("source"), Kind: q.Get("kind"), Model: q.Get("model"), Style: q.Get("style"), Limit: 24}
	if len(query.Query) > 512 || len(query.Source) > 128 || len(query.Kind) > 64 || len(query.Model) > 64 || len(query.Style) > 512 {
		s.fail(w, 400, "catalog_query", "Catalogue filters are too long")
		return
	}
	for key, target := range map[string]*int{"offset": &query.Offset, "limit": &query.Limit} {
		if q.Has(key) {
			value, err := strconv.Atoi(q.Get(key))
			if err != nil || value < 0 || key == "limit" && (value < 1 || value > 100) {
				s.fail(w, 400, "catalog_pagination", "Use a nonnegative offset and a limit from 1 to 100")
				return
			}
			*target = value
		}
	}
	a, ok := s.openCatalog(w)
	if !ok {
		return
	}
	s.json(w, 200, a.List(query))
}

func (s *Server) getCatalogEntry(w http.ResponseWriter, r *http.Request) {
	a, ok := s.openCatalog(w)
	if !ok {
		return
	}
	detail, err := a.Detail(chi.URLParam(r, "id"))
	if err != nil {
		s.catalogError(w, err)
		return
	}
	s.json(w, 200, detail)
}

func (s *Server) copyCatalogEntry(w http.ResponseWriter, r *http.Request) {
	var in struct{}
	if !s.decode(w, r, &in) {
		return
	}
	a, ok := s.openCatalog(w)
	if !ok {
		return
	}
	detail, err := a.Detail(chi.URLParam(r, "id"))
	if err != nil {
		s.catalogError(w, err)
		return
	}
	if !detail.CanCopy || detail.Recipe == nil {
		s.fail(w, 422, "reference_only", "This source has no verified editable conversion. Use the original record and its notes for review.")
		return
	}
	v := *detail.Recipe
	if err := recipe.Validate(v); err != nil {
		s.internal(w, err)
		return
	}
	s.recipeMu.Lock()
	defer s.recipeMu.Unlock()
	v.ID = randomID()
	v.Revision = 1
	v.CreatedAt = time.Now().UTC()
	v.UpdatedAt = v.CreatedAt
	if _, err := s.db.Put("recipe", v.ID, v, 0); err != nil {
		s.storeError(w, err)
		return
	}
	if _, err := s.db.Put("recipe-revision", v.ID+":1", v, 0); err != nil {
		s.cfg.Logger.Error("Catalogue copy revision archive failed", "id", v.ID, "error", err)
	}
	s.json(w, 201, v)
}

func (s *Server) catalogOriginal(w http.ResponseWriter, r *http.Request) {
	a, ok := s.openCatalog(w)
	if !ok {
		return
	}
	data, record, err := a.Original(chi.URLParam(r, "id"), r.URL.Query().Get("record"))
	if err != nil {
		s.catalogError(w, err)
		return
	}
	w.Header().Set("Content-Type", "application/json")
	w.Header().Set("Content-Disposition", mime.FormatMediaType("attachment", map[string]string{"filename": path.Base(record.Path)}))
	w.Header().Set("ETag", `"`+record.SHA256+`"`)
	_, _ = w.Write(data)
}

// Optional PDFs are read only from the configured data directory. OpenRoot
// contains path resolution and the manifest hash rejects substituted content.
// This endpoint never falls back to a remote download.
func (s *Server) optionalCatalogAsset(asset catalog.Asset) ([]byte, error) {
	root, err := os.OpenRoot(filepath.Join(s.cfg.DataDir, "catalog-references"))
	if err != nil {
		return nil, err
	}
	defer root.Close()
	f, err := root.Open(asset.Path)
	if err != nil {
		return nil, err
	}
	defer f.Close()
	info, err := f.Stat()
	if err != nil {
		return nil, err
	}
	if !info.Mode().IsRegular() || info.Size() != asset.Bytes || info.Size() > catalog.MaxFileBytes {
		return nil, errors.New("catalog: optional reference size mismatch")
	}
	data, err := io.ReadAll(io.LimitReader(f, catalog.MaxFileBytes+1))
	if err != nil {
		return nil, err
	}
	if err := catalog.Verify(data, asset.Bytes, asset.SHA256); err != nil {
		return nil, err
	}
	return data, nil
}

func (s *Server) catalogReferences(w http.ResponseWriter, r *http.Request) {
	a, ok := s.openCatalog(w)
	if !ok {
		return
	}
	refs := a.References(r.URL.Query().Get("model"))
	for i := range refs.Items {
		if refs.Items[i].Bundled {
			continue
		}
		asset, found := a.Asset(refs.Items[i].AssetPath)
		if found {
			_, err := s.optionalCatalogAsset(asset)
			refs.Items[i].Available = err == nil
		}
	}
	s.json(w, 200, refs)
}

func (s *Server) catalogAsset(w http.ResponseWriter, r *http.Request) {
	a, ok := s.openCatalog(w)
	if !ok {
		return
	}
	data, asset, err := a.AssetData(chi.URLParam(r, "*"))
	if errors.Is(err, catalog.ErrNotBundled) {
		data, err = s.optionalCatalogAsset(asset)
		if err != nil {
			s.fail(w, 404, "reference_unavailable", "This optional reference is not installed or does not match the pinned source")
			return
		}
	}
	if err != nil {
		s.catalogError(w, err)
		return
	}
	w.Header().Set("Content-Type", asset.ContentType)
	w.Header().Set("ETag", `"`+asset.SHA256+`"`)
	if asset.ContentType == "application/pdf" {
		w.Header().Set("Content-Disposition", mime.FormatMediaType("attachment", map[string]string{"filename": path.Base(asset.Path)}))
	}
	http.ServeContent(w, r, path.Base(asset.Path), time.Time{}, bytes.NewReader(data))
}
