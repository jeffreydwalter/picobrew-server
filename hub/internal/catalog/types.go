// Package catalog exposes the pinned community archive bundled with the Hub.
// Source records remain immutable; library copies are independent user records.
package catalog

import (
	"encoding/json"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

type Source struct {
	ID              string         `json:"id"`
	Name            string         `json:"name"`
	Repository      string         `json:"repository"`
	Upstream        string         `json:"upstream"`
	Commit          string         `json:"commit"`
	License         string         `json:"license"`
	Attribution     string         `json:"attribution"`
	Notes           string         `json:"notes"`
	RecordCount     int            `json:"record_count"`
	ValidCount      int            `json:"valid_count"`
	EntryCount      int            `json:"entry_count"`
	ExcludedCount   int            `json:"excluded_count"`
	ExcludedReasons map[string]int `json:"excluded_reasons"`
	AssetCount      int            `json:"asset_count"`
	ReferenceCount  int            `json:"reference_count"`
}

type Summary struct {
	ID                string   `json:"id"`
	Name              string   `json:"name"`
	Kind              string   `json:"kind"`
	SourceID          string   `json:"source_id"`
	SourceModel       string   `json:"source_model"`
	Models            []string `json:"models"`
	DeployModels      []string `json:"deploy_models"`
	Style             string   `json:"style"`
	Author            string   `json:"author"`
	Description       string   `json:"description"`
	ABV               *float64 `json:"abv,omitempty"`
	IBU               *float64 `json:"ibu,omitempty"`
	SRM               *float64 `json:"srm,omitempty"`
	ImageURL          string   `json:"image_url,omitempty"`
	ImageKind         string   `json:"image_kind,omitempty"`
	SourceRecordCount int      `json:"source_record_count"`
	CanCopy           bool     `json:"can_copy"`
	CanDeploy         bool     `json:"can_deploy"`
	Featured          bool     `json:"featured"`
	Official          bool     `json:"official"`
}

type Provenance = domain.RecipeProvenance

type SourceRecord struct {
	ID     string `json:"id"`
	Path   string `json:"path"`
	SHA256 string `json:"sha256"`
	Bytes  int64  `json:"bytes"`
}

type Detail struct {
	Summary
	Recipe           *domain.Recipe        `json:"recipe,omitempty"`
	NativeProgram    *domain.MachineRecipe `json:"native_program,omitempty"`
	Warnings         []string              `json:"warnings"`
	Provenance       Provenance            `json:"provenance"`
	SourceRecords    []SourceRecord        `json:"source_records"`
	SourceURL        string                `json:"source_url"`
	Notes            string                `json:"notes"`
	SourceParameters json.RawMessage       `json:"source_parameters,omitempty"`
}

type Asset struct {
	Path        string   `json:"path"`
	URL         string   `json:"url"`
	Title       string   `json:"title"`
	Kind        string   `json:"kind"`
	Models      []string `json:"models"`
	SourceID    string   `json:"source_id"`
	SourcePath  string   `json:"source_path"`
	SHA256      string   `json:"sha256"`
	Bytes       int64    `json:"bytes"`
	ContentType string   `json:"content_type"`
	Attribution string   `json:"attribution"`
	License     string   `json:"license"`
	Bundled     bool     `json:"bundled"`
	DownloadURL string   `json:"download_url,omitempty"`
}

type Reference struct {
	ID          string   `json:"id"`
	Title       string   `json:"title"`
	Description string   `json:"description"`
	Models      []string `json:"models"`
	SourceID    string   `json:"source_id"`
	AssetPath   string   `json:"asset_path"`
	URL         string   `json:"url"`
	ContentType string   `json:"content_type"`
	SourceURL   string   `json:"source_url"`
	Bundled     bool     `json:"bundled"`
	Available   bool     `json:"available"`
	DownloadURL string   `json:"download_url,omitempty"`
	SHA256      string   `json:"sha256"`
}

type References struct {
	Items  []Reference `json:"items"`
	Assets []Asset     `json:"assets"`
}

type Query struct {
	Query, Source, Kind, Model, Style string
	Offset, Limit                    int
}

type Page struct {
	Items   []Summary `json:"items"`
	Total   int       `json:"total"`
	Offset  int       `json:"offset"`
	Limit   int       `json:"limit"`
	Sources []Source  `json:"sources"`
	Styles  []string  `json:"styles"`
}
