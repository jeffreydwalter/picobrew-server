package ai

import (
	"context"
	"encoding/json"
	"errors"
	"fmt"
	"io"
	"net/http"
	"net/url"
	"sort"
	"strings"
	"time"
	"unicode"
)

// Model is an account-visible provider model. Listing a model does not establish
// that it supports the assistant's chat endpoint or that inference is enabled.
type Model struct {
	ID   string `json:"id"`
	Name string `json:"name"`
}

const (
	modelListTimeout    = 20 * time.Second
	modelListMaxPage    = 1 << 20
	modelListMaxBytes   = 8 << 20
	modelListMaxPages   = 100
	modelListMaxResults = 10000
)

type modelPage struct {
	Data *[]struct {
		ID          string `json:"id"`
		DisplayName string `json:"display_name"`
	} `json:"data"`
	HasMore *bool  `json:"has_more"`
	LastID  string `json:"last_id"`
}

// ListModels reads the provider's live catalogue using only the supplied key.
// It never saves configuration, invokes a model or follows a provider redirect.
// OpenAI's list is unpaginated; Anthropic uses after_id and has_more. Unknown
// model IDs are retained rather than filtered through a stale version list.
func (c *Client) ListModels(ctx context.Context, provider, key string) ([]Model, error) {
	endpoint := c.OpenAIModelsURL
	switch provider {
	case "openai":
	case "anthropic":
		endpoint = c.AnthropicModelsURL
	default:
		return nil, errors.New("unsupported AI provider")
	}
	if strings.TrimSpace(key) == "" || len(key) > 1024 || strings.ContainsAny(key, "\r\n") {
		return nil, errors.New("set a valid provider API key before loading models")
	}
	u, err := url.Parse(endpoint)
	if err != nil || u.Host == "" || (u.Scheme != "https" && u.Scheme != "http") || u.User != nil {
		return nil, errors.New("the model discovery endpoint is not configured")
	}
	ctx, cancel := context.WithTimeout(ctx, modelListTimeout)
	defer cancel()
	client := c.HTTP
	if client == nil {
		client = http.DefaultClient
	}
	// Keep redirects disabled even when a caller supplies a custom HTTP client.
	boundedClient := *client
	boundedClient.CheckRedirect = func(*http.Request, []*http.Request) error { return http.ErrUseLastResponse }

	models := make([]Model, 0)
	seenModels := make(map[string]bool)
	seenCursors := make(map[string]bool)
	cursor, received, entries := "", 0, 0
	for page := 0; page < modelListMaxPages; page++ {
		if provider == "anthropic" {
			q := u.Query()
			q.Set("limit", "1000")
			q.Del("before_id")
			q.Del("after_id")
			if cursor != "" {
				q.Set("after_id", cursor)
			}
			u.RawQuery = q.Encode()
		}
		r, err := http.NewRequestWithContext(ctx, http.MethodGet, u.String(), nil)
		if err != nil {
			return nil, errors.New("the model discovery request could not be created")
		}
		r.Header.Set("Accept", "application/json")
		if provider == "anthropic" {
			r.Header.Set("x-api-key", key)
			r.Header.Set("anthropic-version", "2023-06-01")
		} else {
			r.Header.Set("Authorization", "Bearer "+key)
		}
		wire, bytesRead, err := readModelPage(&boundedClient, r)
		if err != nil {
			return nil, err
		}
		received += bytesRead
		if received > modelListMaxBytes {
			return nil, errors.New("the provider model list exceeded the total size limit")
		}
		if wire.Data == nil || (provider == "anthropic" && wire.HasMore == nil) {
			return nil, errors.New("the provider returned an invalid model list")
		}
		entries += len(*wire.Data)
		if entries > modelListMaxResults {
			return nil, errors.New("the provider model list exceeded the result limit")
		}
		for _, item := range *wire.Data {
			id := strings.TrimSpace(item.ID)
			if id == "" || len(id) > 512 || strings.IndexFunc(id, func(r rune) bool { return unicode.IsSpace(r) || unicode.IsControl(r) }) >= 0 {
				return nil, errors.New("the provider returned an invalid model identifier")
			}
			if seenModels[id] {
				continue
			}
			name := strings.TrimSpace(item.DisplayName)
			if provider == "openai" || name == "" || len(name) > 512 || strings.IndexFunc(name, unicode.IsControl) >= 0 {
				name = id
			}
			models = append(models, Model{ID: id, Name: name})
			seenModels[id] = true
		}
		if provider == "openai" || !*wire.HasMore {
			sort.Slice(models, func(i, j int) bool {
				a, b := strings.ToLower(models[i].Name), strings.ToLower(models[j].Name)
				if a == b {
					return models[i].ID < models[j].ID
				}
				return a < b
			})
			return models, nil
		}
		// A malformed/repeated cursor must fail instead of publishing a partial
		// list. Build the next request ourselves; never follow a response URL.
		cursor = wire.LastID
		if len(*wire.Data) == 0 || cursor == "" || len(cursor) > 512 || seenCursors[cursor] || cursor != (*wire.Data)[len(*wire.Data)-1].ID {
			return nil, errors.New("the provider returned invalid model pagination")
		}
		seenCursors[cursor] = true
	}
	return nil, errors.New("the provider model list exceeded the page limit")
}

func readModelPage(client *http.Client, r *http.Request) (modelPage, int, error) {
	var page modelPage
	resp, err := client.Do(r)
	if err != nil {
		if errors.Is(r.Context().Err(), context.DeadlineExceeded) {
			return page, 0, errors.New("loading provider models timed out; try again")
		}
		if errors.Is(r.Context().Err(), context.Canceled) {
			return page, 0, errors.New("loading provider models was canceled")
		}
		return page, 0, errors.New("the AI provider could not be reached to load models")
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return page, 0, fmt.Errorf("AI provider returned HTTP %d while loading models; check its API key and model-list permissions", resp.StatusCode)
	}
	p, err := io.ReadAll(io.LimitReader(resp.Body, modelListMaxPage+1))
	if err != nil {
		return page, 0, errors.New("the provider model list response was incomplete")
	}
	if len(p) > modelListMaxPage {
		return page, len(p), errors.New("the provider model list exceeded the page size limit")
	}
	if json.Unmarshal(p, &page) != nil {
		return page, len(p), errors.New("the provider returned an invalid model list")
	}
	return page, len(p), nil
}
