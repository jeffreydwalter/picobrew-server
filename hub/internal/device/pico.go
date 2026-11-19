package device

import (
	"bytes"
	"context"
	"crypto/tls"
	"encoding/hex"
	"encoding/json"
	"errors"
	"io"
	"math"
	"net"
	"net/http"
	"net/url"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"time"
	"unicode/utf8"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

const maxRequestBytes = 4095
const maxResponseBytes = 64 << 10

type pico struct {
	base         *url.URL
	key          string
	client       *http.Client
	gate         chan struct{}
	capabilities *domain.Capabilities
}

var apiKey = regexp.MustCompile(`^[0-9a-fA-F]{64}$`)
var errorCode = regexp.MustCompile(`^[a-zA-Z0-9_-]{1,64}$`)

func NewPico(baseURL, key string) (Adapter, error) { return newPico(baseURL, key, false) }

// allowLoopback is private and exists only for local protocol tests. Real device
// registration must use NewPico; the simulator needs no HTTP endpoint.
func newPico(baseURL, key string, allowLoopback bool) (*pico, error) {
	base, err := url.Parse(baseURL)
	if err != nil || base == nil || (base.Scheme != "http" && base.Scheme != "https") || base.Host == "" || base.Hostname() == "" || base.User != nil || base.RawQuery != "" || base.ForceQuery || base.Fragment != "" || base.Opaque != "" || (base.Path != "" && base.Path != "/") || base.RawPath != "" {
		return nil, invalid("Device URL must contain only an HTTP(S) scheme, hostname and optional port")
	}
	if !apiKey.MatchString(key) {
		return nil, invalid("Device API key must contain exactly 64 hexadecimal characters")
	}
	host := strings.TrimSuffix(strings.ToLower(base.Hostname()), ".")
	if strings.ContainsAny(host, "% \\") || (!allowLoopback && (host == "localhost" || strings.HasSuffix(host, ".localhost"))) {
		return nil, invalid("Device URL uses a prohibited address")
	}
	if ip := net.ParseIP(host); ip != nil && prohibitedIP(ip, allowLoopback) {
		return nil, invalid("Device URL uses a prohibited address")
	}
	if port := base.Port(); port != "" {
		value, err := strconv.Atoi(port)
		if err != nil || value < 1 || value > 65535 {
			return nil, invalid("Device port must be from 1 to 65535")
		}
	}
	base.Path = ""
	dialer := &net.Dialer{Timeout: 3 * time.Second}
	transport := &http.Transport{Proxy: nil, DisableKeepAlives: true, MaxConnsPerHost: 1, ForceAttemptHTTP2: false, TLSClientConfig: &tls.Config{MinVersion: tls.VersionTLS12}}
	transport.DialContext = func(ctx context.Context, network, address string) (net.Conn, error) {
		host, port, err := net.SplitHostPort(address)
		if err != nil {
			return nil, errors.New("invalid device address")
		}
		addresses, err := net.DefaultResolver.LookupIPAddr(ctx, host)
		if err != nil {
			return nil, errors.New("device hostname could not be resolved")
		}
		for _, address := range addresses {
			if prohibitedIP(address.IP, allowLoopback) {
				return nil, errors.New("device hostname resolved to a prohibited address")
			}
		}
		for _, address := range addresses {
			connection, err := dialer.DialContext(ctx, network, net.JoinHostPort(address.IP.String(), port))
			if err == nil {
				return connection, nil
			}
			if ctx.Err() != nil {
				return nil, ctx.Err()
			}
		}
		return nil, errors.New("device connection failed")
	}
	return &pico{base: base, key: key, gate: make(chan struct{}, 1), client: &http.Client{Transport: transport, Timeout: 3 * time.Second, CheckRedirect: func(*http.Request, []*http.Request) error { return http.ErrUseLastResponse }}}, nil
}

func prohibitedIP(ip net.IP, allowLoopback bool) bool {
	return ip == nil || ip.IsUnspecified() || ip.IsMulticast() || ip.IsLinkLocalUnicast() || ip.IsLinkLocalMulticast() || (!allowLoopback && ip.IsLoopback()) || ip.Equal(net.ParseIP("100.100.100.200"))
}

func (p *pico) lock(ctx context.Context) error {
	if ctx.Err() != nil {
		return &Error{Code: "cancelled", Message: "Device request was cancelled before dispatch"}
	}
	select {
	case p.gate <- struct{}{}:
		if ctx.Err() != nil {
			p.unlock()
			return &Error{Code: "cancelled", Message: "Device request was cancelled while queued"}
		}
		return nil
	case <-ctx.Done():
		return &Error{Code: "cancelled", Message: "Device request was cancelled while queued"}
	}
}
func (p *pico) unlock() { <-p.gate }

func (p *pico) request(ctx context.Context, method, path string, value, destination any, statuses ...int) error {
	var body []byte
	var err error
	write := method != http.MethodGet
	if value != nil {
		body, err = json.Marshal(value)
		if err != nil {
			return invalid("Device request could not be encoded")
		}
		if len(body) > maxRequestBytes {
			return &Error{StatusCode: 413, Code: "payload_too_large", Message: "Device request exceeds 4095 bytes"}
		}
	}
	endpoint := *p.base
	parts := strings.SplitN(path, "?", 2)
	endpoint.Path = parts[0]
	if len(parts) == 2 {
		endpoint.RawQuery = parts[1]
	}
	request, err := http.NewRequestWithContext(ctx, method, endpoint.String(), bytes.NewReader(body))
	if err != nil {
		return invalid("Device request could not be prepared")
	}
	request.Header.Set("Authorization", "Bearer "+p.key)
	request.Header.Set("Accept", "application/json")
	request.Close = true
	if write {
		request.Header.Set("Content-Type", "application/json")
		request.ContentLength = int64(len(body))
	}
	response, err := p.client.Do(request)
	if err != nil {
		return &Error{Code: "transport_error", Message: "Device did not return a complete response", UnknownOutcome: write}
	}
	defer response.Body.Close()
	data, readErr := io.ReadAll(io.LimitReader(response.Body, maxResponseBytes+1))
	if readErr != nil || len(data) > maxResponseBytes {
		return &Error{StatusCode: response.StatusCode, Code: "invalid_response", Message: "Device response is incomplete or exceeds its size limit", UnknownOutcome: write}
	}
	accepted := false
	for _, status := range statuses {
		if response.StatusCode == status {
			accepted = true
			break
		}
	}
	if !accepted {
		failure := struct {
			Error   string `json:"error"`
			Message string `json:"message"`
		}{}
		_ = json.Unmarshal(data, &failure)
		if !errorCode.MatchString(failure.Error) || p.cleanMessage(failure.Error) != failure.Error {
			failure.Error = "device_rejected"
		}
		message := p.cleanMessage(failure.Message)
		if message == "" {
			message = http.StatusText(response.StatusCode)
		}
		if message == "" {
			message = "Device returned an unexpected status"
		}
		return &Error{StatusCode: response.StatusCode, Code: failure.Error, Message: message, UnknownOutcome: write && (response.StatusCode >= 500 || response.StatusCode == 408 || response.StatusCode >= 200 && response.StatusCode < 400)}
	}
	if destination != nil {
		if err := json.Unmarshal(data, destination); err != nil {
			return &Error{StatusCode: response.StatusCode, Code: "invalid_response", Message: "Device returned invalid JSON", UnknownOutcome: write}
		}
	}
	return nil
}

func (p *pico) cleanMessage(message string) string {
	for _, secret := range []string{p.key, strings.ToLower(p.key), strings.ToUpper(p.key), p.base.String(), p.base.Host, p.base.Hostname()} {
		if secret != "" {
			message = regexp.MustCompile("(?i)"+regexp.QuoteMeta(secret)).ReplaceAllString(message, "[redacted]")
		}
	}
	message = strings.Map(func(r rune) rune {
		if r < 32 || r == 127 {
			return ' '
		}
		return r
	}, message)
	if len(message) > 256 {
		message = message[:256]
		for !utf8.ValidString(message) {
			message = message[:len(message)-1]
		}
	}
	return strings.TrimSpace(message)
}

func (p *pico) discover(ctx context.Context) (domain.Capabilities, error) {
	var reply struct {
		domain.Capabilities
		Mode string `json:"mode"`
	}
	if err := p.request(ctx, http.MethodGet, "/api/v1", nil, &reply, 200); err != nil {
		return domain.Capabilities{}, err
	}
	if reply.APIVersion != 1 || reply.Mode != "standalone" {
		return domain.Capabilities{}, &Error{Code: "unsupported_firmware", Message: "Device does not expose the supported standalone API v1"}
	}
	capabilities := reply.Capabilities
	p.capabilities = &capabilities
	result := capabilities
	result.Commands = append([]string(nil), capabilities.Commands...)
	return result, nil
}
func (p *pico) Discover(ctx context.Context) (domain.Capabilities, error) {
	if err := p.lock(ctx); err != nil {
		return domain.Capabilities{}, err
	}
	defer p.unlock()
	return p.discover(ctx)
}
func (p *pico) Status(ctx context.Context) (domain.Status, error) {
	var result domain.Status
	if err := p.lock(ctx); err != nil {
		return result, err
	}
	defer p.unlock()
	err := p.request(ctx, http.MethodGet, "/api/v1/status", nil, &result, 200)
	return result, err
}
func (p *pico) Display(ctx context.Context) (domain.Display, error) {
	var result domain.Display
	if err := p.lock(ctx); err != nil {
		return result, err
	}
	defer p.unlock()
	err := p.request(ctx, http.MethodGet, "/api/v1/display", nil, &result, 200)
	_, hexErr := hex.DecodeString(result.Data)
	if err == nil && (result.Width != 128 || result.Height != 64 || result.Format != "page-lsb-hex" || len(result.Data) != 2048 || hexErr != nil) {
		err = &Error{Code: "invalid_response", Message: "Device display format is unsupported"}
	}
	return result, err
}
func (p *pico) Recipes(ctx context.Context) ([]domain.MachineRecipe, error) {
	if err := p.lock(ctx); err != nil {
		return nil, err
	}
	defer p.unlock()
	var list struct {
		Recipes []struct {
			ID string `json:"id"`
		} `json:"recipes"`
	}
	if err := p.request(ctx, http.MethodGet, "/api/v1/recipes", nil, &list, 200); err != nil {
		return nil, err
	}
	if len(list.Recipes) > 16 {
		return nil, &Error{Code: "invalid_response", Message: "Device recipe inventory exceeds capacity"}
	}
	result := make([]domain.MachineRecipe, 0, len(list.Recipes))
	for _, summary := range list.Recipes {
		if !recipeID.MatchString(summary.ID) {
			return nil, &Error{Code: "invalid_response", Message: "Device returned an invalid recipe ID"}
		}
		var recipe domain.MachineRecipe
		if err := p.request(ctx, http.MethodGet, "/api/v1/recipes/"+summary.ID, nil, &recipe, 200); err != nil {
			return nil, err
		}
		if recipe.ID != summary.ID || validateRecipe(recipe) != nil {
			return nil, &Error{Code: "invalid_response", Message: "Device returned an invalid recipe"}
		}
		result = append(result, recipe)
	}
	return result, nil
}
func (p *pico) PutRecipe(ctx context.Context, recipe domain.MachineRecipe) error {
	if err := validateRecipe(recipe); err != nil {
		return err
	}
	if err := p.lock(ctx); err != nil {
		return err
	}
	defer p.unlock()
	path := "/api/v1/recipes/" + recipe.ID
	if err := p.request(ctx, http.MethodPut, path, recipe, nil, 200, 201); err != nil {
		return err
	}
	var observed domain.MachineRecipe
	if err := p.request(ctx, http.MethodGet, path, nil, &observed, 200); err != nil {
		return &Error{Code: "verification_failed", Message: "Recipe write was accepted but readback failed", UnknownOutcome: true}
	}
	if !reflect.DeepEqual(recipe, observed) {
		return &Error{Code: "verification_failed", Message: "Recipe readback does not match the intended recipe", UnknownOutcome: true}
	}
	return nil
}
func (p *pico) DeleteRecipe(ctx context.Context, id string) error {
	if !recipeID.MatchString(id) {
		return invalid("Recipe ID is invalid")
	}
	if err := p.lock(ctx); err != nil {
		return err
	}
	defer p.unlock()
	return p.request(ctx, http.MethodDelete, "/api/v1/recipes/"+id, nil, nil, 204)
}
func (p *pico) Command(ctx context.Context, command domain.Command) error {
	if err := validateCommand(command); err != nil {
		return err
	}
	if err := p.lock(ctx); err != nil {
		return err
	}
	defer p.unlock()
	if p.capabilities == nil {
		if _, err := p.discover(ctx); err != nil {
			return err
		}
	}
	allowed := false
	for _, value := range p.capabilities.Commands {
		if value == command.Command {
			allowed = true
			break
		}
	}
	if !allowed {
		return &Error{StatusCode: 400, Code: "unsupported_command", Message: "Device did not advertise this command"}
	}
	var reply struct {
		Accepted bool   `json:"accepted"`
		Command  string `json:"command"`
	}
	if err := p.request(ctx, http.MethodPost, "/api/v1/commands", command, &reply, 202); err != nil {
		return err
	}
	if !reply.Accepted || reply.Command != command.Command {
		return &Error{Code: "invalid_response", Message: "Command acceptance could not be verified", UnknownOutcome: true}
	}
	return nil
}
func (p *pico) readMap(ctx context.Context, path string) (map[string]any, error) {
	if err := p.lock(ctx); err != nil {
		return nil, err
	}
	defer p.unlock()
	var result map[string]any
	err := p.request(ctx, http.MethodGet, path, nil, &result, 200)
	return result, err
}
func (p *pico) Settings(ctx context.Context) (map[string]any, error) {
	return p.readMap(ctx, "/api/v1/settings")
}
func (p *pico) Telemetry(ctx context.Context) (map[string]any, error) {
	return p.readMap(ctx, "/api/v1/telemetry")
}
func (p *pico) UpdateSettings(ctx context.Context, settings map[string]any) error {
	if err := validateSettings(settings); err != nil {
		return err
	}
	if err := p.lock(ctx); err != nil {
		return err
	}
	defer p.unlock()
	return p.request(ctx, http.MethodPatch, "/api/v1/settings", settings, nil, 200)
}
func (p *pico) UpdateTelemetry(ctx context.Context, settings map[string]any) error {
	if err := validateTelemetry(settings); err != nil {
		return err
	}
	if err := p.lock(ctx); err != nil {
		return err
	}
	defer p.unlock()
	return p.request(ctx, http.MethodPut, "/api/v1/telemetry", settings, nil, 200)
}
func (p *pico) History(ctx context.Context) (domain.History, error) {
	var result domain.History
	if err := p.lock(ctx); err != nil {
		return result, err
	}
	defer p.unlock()
	err := p.request(ctx, http.MethodGet, "/api/v1/history", nil, &result, 200)
	if err == nil && (!reflect.DeepEqual(result.Fields, historyFields) || len(result.Sessions) > 8) {
		err = &Error{Code: "invalid_response", Message: "Device history schema is unsupported"}
	}
	return result, err
}
func (p *pico) HistoryPage(ctx context.Context, id uint64, offset int) ([]domain.Sample, *int, error) {
	if id == 0 || id > math.MaxUint32 || offset < 0 || offset > 4224 {
		return nil, nil, invalid("History ID or offset is invalid")
	}
	if err := p.lock(ctx); err != nil {
		return nil, nil, err
	}
	defer p.unlock()
	var reply struct {
		ID      uint64       `json:"id"`
		Offset  int          `json:"offset"`
		Samples [][]*float64 `json:"samples"`
		Next    *int         `json:"next_offset"`
	}
	path := "/api/v1/history/samples/" + strconv.Itoa(offset) + "?id=" + strconv.FormatUint(id, 10)
	if err := p.request(ctx, http.MethodGet, path, nil, &reply, 200); err != nil {
		return nil, nil, err
	}
	if reply.ID != id || reply.Offset != offset || len(reply.Samples) > 32 || offset+len(reply.Samples) > 4224 || reply.Next != nil && (*reply.Next != offset+len(reply.Samples) || len(reply.Samples) == 0 || *reply.Next > 4224) {
		return nil, nil, &Error{Code: "invalid_response", Message: "Device history page identity or continuation is invalid"}
	}
	result := make([]domain.Sample, 0, len(reply.Samples))
	for _, row := range reply.Samples {
		if len(row) != 11 {
			return nil, nil, &Error{Code: "invalid_response", Message: "Device history row has an invalid field count"}
		}
		for _, index := range []int{0, 4, 5, 6, 7, 8, 9, 10} {
			low, high := float64(0), float64(255)
			if index == 0 {
				high = math.MaxUint32
			} else if index == 4 {
				low, high = math.MinInt32, math.MaxInt32
			}
			if row[index] == nil || math.IsNaN(*row[index]) || math.IsInf(*row[index], 0) || *row[index] != math.Trunc(*row[index]) || *row[index] < low || *row[index] > high {
				return nil, nil, &Error{Code: "invalid_response", Message: "Device history row has invalid integer fields"}
			}
		}
		for _, index := range []int{1, 2, 3} {
			if row[index] != nil && (math.IsNaN(*row[index]) || math.IsInf(*row[index], 0) || *row[index] < -3276.7 || *row[index] > 3276.7) {
				return nil, nil, &Error{Code: "invalid_response", Message: "Device history row has an invalid temperature"}
			}
		}
		if len(result) > 0 && int64(*row[0]) < result[len(result)-1].ElapsedMS {
			return nil, nil, &Error{Code: "invalid_response", Message: "Device history page elapsed times are not ordered"}
		}
		result = append(result, domain.Sample{ElapsedMS: int64(*row[0]), WortF: row[1], ThermoblockF: row[2], TargetF: row[3], StepSeconds: int(*row[4]), Phase: int(*row[5]), ControllerStatus: int(*row[6]), Compartment: int(*row[7]), CurrentStep: int(*row[8]), Flags: int(*row[9]), DeviceState: int(*row[10]), Source: "device"})
	}
	return result, reply.Next, nil
}
