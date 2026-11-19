package server

import (
	"crypto/sha256"
	"encoding/hex"
	"encoding/json"
	"net/http"
	"os"
	"path/filepath"
	"reflect"
	"testing"

	"github.com/jeffreydwalter/picobrew-hub/internal/catalog"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

func TestCatalogBrowseCopyAndOriginal(t *testing.T) {
	s, h, c := newTestServer(t)
	status, _ := request(t, c, "GET", h.URL+"/api/v1/catalog", "", nil, nil)
	if status != 401 {
		t.Fatalf("unauthenticated catalogue: %d", status)
	}
	csrf := signIn(t, c, h.URL)
	status, body := request(t, c, "GET", h.URL+"/api/v1/catalog?source=picopak-library&model=pico-pro&limit=2", "", nil, nil)
	var page catalog.Page
	if status != 200 || json.Unmarshal(body, &page) != nil || len(page.Items) != 2 || page.Total <= 2 {
		t.Fatalf("browse %d: %s", status, body)
	}
	status, body = request(t, c, "GET", h.URL+"/api/v1/catalog/picopak-64", "", nil, nil)
	var detail catalog.Detail
	if status != 200 || json.Unmarshal(body, &detail) != nil || detail.Recipe == nil {
		t.Fatalf("detail %d: %s", status, body)
	}
	status, _ = request(t, c, "POST", h.URL+"/api/v1/catalog/picopak-64/copy", "", map[string]any{}, nil)
	if status != 403 {
		t.Fatalf("copy without csrf: %d", status)
	}
	status, body = request(t, c, "POST", h.URL+"/api/v1/catalog/picopak-64/copy", csrf, map[string]any{}, nil)
	var copied domain.Recipe
	if status != 201 || json.Unmarshal(body, &copied) != nil {
		t.Fatalf("copy %d: %s", status, body)
	}
	if copied.ID == "" || copied.Revision != 1 || !reflect.DeepEqual(copied.NativeProgram, detail.NativeProgram) || copied.Provenance.CatalogID != detail.ID || copied.BatchLiters != 0 || len(copied.Ingredients) != 0 {
		t.Fatalf("copy changed original: %s", body)
	}
	var saved domain.Recipe
	if _, err := s.db.Get("recipe", copied.ID, &saved); err != nil || !reflect.DeepEqual(saved, copied) {
		t.Fatal("copy not saved", err)
	}
	var revision domain.Recipe
	if _, err := s.db.Get("recipe-revision", copied.ID+":1", &revision); err != nil {
		t.Fatal("revision not saved", err)
	}
	status, body = request(t, c, "POST", h.URL+"/api/v1/catalog/picopak-64/copy", csrf, map[string]any{}, nil)
	var second domain.Recipe
	if status != 201 || json.Unmarshal(body, &second) != nil || second.ID == copied.ID {
		t.Fatalf("repeat copy overwrote library %d: %s", status, body)
	}
	if _, err := s.db.Get("recipe", copied.ID, &saved); err != nil || !reflect.DeepEqual(saved, copied) {
		t.Fatal("source copy changed existing library", err)
	}
	rows, err := s.db.List("deployment")
	if err != nil || len(rows) != 0 {
		t.Fatal("catalogue copy deployed a program")
	}
	status, body = request(t, c, "GET", h.URL+"/api/v1/catalog/picopak-64/source", "", nil, nil)
	if status != 200 || catalog.Verify(body, detail.SourceRecords[0].Bytes, detail.SourceRecords[0].SHA256) != nil {
		t.Fatalf("source changed %d: %s", status, body)
	}
	status, _ = request(t, c, "GET", h.URL+"/api/v1/catalog/picopak-64/source?record=17", "", nil, nil)
	if status != 404 {
		t.Fatalf("unlisted original accepted %d", status)
	}
	for _, url := range []string{"/api/v1/catalog?limit=0", "/api/v1/catalog?limit=101", "/api/v1/catalog?offset=-1", "/api/v1/catalog?offset=bad"} {
		status, _ = request(t, c, "GET", h.URL+url, "", nil, nil)
		if status != 400 {
			t.Fatalf("bad filter %s: %d", url, status)
		}
	}
}

func TestCatalogReferencesAndUnsupportedCopy(t *testing.T) {
	_, h, c := newTestServer(t)
	csrf := signIn(t, c, h.URL)
	status, body := request(t, c, "GET", h.URL+"/api/v1/catalog?source=chiefwigms&model=pico-pro", "", nil, nil)
	var page catalog.Page
	if status != 200 || json.Unmarshal(body, &page) != nil || len(page.Items) == 0 {
		t.Fatalf("chief sources %d: %s", status, body)
	}
	for _, item := range page.Items {
		if item.CanCopy {
			t.Fatalf("unverified Pico named location mapping copied: %s", item.ID)
		}
		status, _ = request(t, c, "POST", h.URL+"/api/v1/catalog/"+item.ID+"/copy", csrf, map[string]any{}, nil)
		if status != 422 {
			t.Fatalf("reference copied %s: %d", item.ID, status)
		}
	}
	status, body = request(t, c, "GET", h.URL+"/api/v1/catalog/references?model=pico-pro", "", nil, nil)
	var refs catalog.References
	if status != 200 || json.Unmarshal(body, &refs) != nil || len(refs.Items) == 0 {
		t.Fatalf("references %d: %s", status, body)
	}
	foundBundled, foundOptional := false, false
	for _, ref := range refs.Items {
		status, data := request(t, c, "GET", h.URL+ref.URL, "", nil, nil)
		if ref.Bundled {
			foundBundled = true
			sum := sha256.Sum256(data)
			if status != 200 || !ref.Available || hex.EncodeToString(sum[:]) != ref.SHA256 {
				t.Fatalf("bundled reference %s: %d", ref.ID, status)
			}
		} else {
			foundOptional = true
			if status != 404 || ref.Available {
				t.Fatalf("absent PDF advertised %s: %d", ref.ID, status)
			}
		}
	}
	if !foundBundled || !foundOptional {
		t.Fatal("reference fixtures incomplete")
	}
	for _, asset := range refs.Assets {
		if !asset.Bundled || asset.ContentType != "image/svg+xml" {
			continue
		}
		resp, err := c.Get(h.URL + asset.URL)
		if err != nil {
			t.Fatal(err)
		}
		resp.Body.Close()
		if resp.StatusCode != 200 || resp.Header.Get("Content-Type") != "image/svg+xml" || resp.Header.Get("X-Content-Type-Options") != "nosniff" {
			t.Fatal("asset headers", resp.StatusCode, resp.Header)
		}
		break
	}
	status, _ = request(t, c, http.MethodGet, h.URL+"/api/v1/catalog/assets/not-in-manifest", "", nil, nil)
	if status != 404 {
		t.Fatalf("unlisted asset: %d", status)
	}
}

func TestOptionalReferenceRequiresManifestBytesAndContainedPath(t *testing.T) {
	s, _, _ := newTestServer(t)
	root := filepath.Join(s.cfg.DataDir, "catalog-references")
	if err := os.MkdirAll(filepath.Join(root, "support"), 0700); err != nil {
		t.Fatal(err)
	}
	data := []byte("%PDF-verified-test-source")
	sum := sha256.Sum256(data)
	asset := catalog.Asset{Path: "support/manual.pdf", Bytes: int64(len(data)), SHA256: hex.EncodeToString(sum[:])}
	file := filepath.Join(root, asset.Path)
	if err := os.WriteFile(file, data, 0600); err != nil {
		t.Fatal(err)
	}
	if got, err := s.optionalCatalogAsset(asset); err != nil || string(got) != string(data) {
		t.Fatal("valid local reference rejected", err)
	}
	if err := os.WriteFile(file, []byte("%PDF-modified-test-source"), 0600); err != nil {
		t.Fatal(err)
	}
	if _, err := s.optionalCatalogAsset(asset); err == nil {
		t.Fatal("substituted reference accepted")
	}
	if err := os.Remove(file); err != nil {
		t.Fatal(err)
	}
	out := filepath.Join(t.TempDir(), "outside.pdf")
	if err := os.WriteFile(out, data, 0600); err != nil {
		t.Fatal(err)
	}
	if err := os.Symlink(out, file); err != nil {
		t.Fatal(err)
	}
	if _, err := s.optionalCatalogAsset(asset); err == nil {
		t.Fatal("symlink escaped reference root")
	}
}
