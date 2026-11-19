// Package device adapts explicitly configured appliances and isolated simulators.
package device

import (
	"context"
	"fmt"
	"regexp"
	"strconv"
	"strings"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

type Adapter interface {
	Discover(context.Context) (domain.Capabilities, error)
	Status(context.Context) (domain.Status, error)
	Display(context.Context) (domain.Display, error)
	Recipes(context.Context) ([]domain.MachineRecipe, error)
	PutRecipe(context.Context, domain.MachineRecipe) error
	DeleteRecipe(context.Context, string) error
	Command(context.Context, domain.Command) error
	Settings(context.Context) (map[string]any, error)
	UpdateSettings(context.Context, map[string]any) error
	Telemetry(context.Context) (map[string]any, error)
	UpdateTelemetry(context.Context, map[string]any) error
	History(context.Context) (domain.History, error)
	HistoryPage(context.Context, uint64, int) ([]domain.Sample, *int, error)
}

// Error never includes a device address, credential or raw transport diagnostic.
// UnknownOutcome means a mutation may have reached the device; observe/read back
// before deciding what to do. It must never trigger an automatic write retry.
type Error struct {
	StatusCode     int    `json:"status"`
	Code           string `json:"code"`
	Message        string `json:"message"`
	UnknownOutcome bool   `json:"unknown_outcome"`
}

func (e *Error) Error() string { return e.Code + ": " + e.Message }

func invalid(message string) error {
	return &Error{StatusCode: 400, Code: "invalid_request", Message: message}
}

var recipeID = regexp.MustCompile(`^[A-Za-z0-9_-]{1,20}$`)
var commands = []string{"brew", "sous-vide", "cold-brew", "manual", "rinse", "first-rinse", "descale", "circulate", "drain-reservoir", "drain-filter", "rack-beer", "utilities", "settings", "stop", "clear-stop", "encoder"}

func validLabel(value string, max int) bool {
	if len(value) == 0 || len(value) > max {
		return false
	}
	for _, ch := range value {
		if ch < 32 || ch > 126 {
			return false
		}
	}
	return true
}

func validateRecipe(recipe domain.MachineRecipe) error {
	if !recipeID.MatchString(recipe.ID) || !validLabel(recipe.Name, 31) || len(recipe.Steps) < 1 || len(recipe.Steps) > 15 {
		return invalid("Recipe ID, name or step count is invalid")
	}
	for i, step := range recipe.Steps {
		if !validLabel(step.Name, 21) || step.Location < 1 || step.Location > 6 || (step.TargetF != 0 && (step.TargetF < 60 || step.TargetF > 212)) || step.Minutes < 1 || step.Minutes > 599 || step.DrainMinutes < 0 || step.DrainMinutes > 120 {
			return invalid(fmt.Sprintf("Recipe step %d is outside the firmware limits", i+1))
		}
	}
	return nil
}

func validateCommand(command domain.Command) error {
	known := false
	for _, value := range commands {
		if value == command.Command {
			known = true
			break
		}
	}
	if !known {
		return invalid("Command is unsupported; dedicated pause/resume is unavailable")
	}
	switch command.Command {
	case "brew":
		if !recipeID.MatchString(command.RecipeID) || command.Delta != 0 || command.Click {
			return invalid("Brew requires only a valid recipe_id")
		}
	case "encoder":
		if command.RecipeID != "" || command.Delta < -100 || command.Delta > 100 || command.Delta == 0 && !command.Click {
			return invalid("Encoder requires movement from -100 to 100 or one click")
		}
	default:
		if command.RecipeID != "" || command.Delta != 0 || command.Click {
			return invalid("This command does not accept additional parameters")
		}
	}
	return nil
}

func validateSettings(settings map[string]any) error {
	if len(settings) == 0 {
		return invalid("At least one preference is required")
	}
	for key, value := range settings {
		switch key {
		case "temperature_unit":
			if value != "F" && value != "C" {
				return invalid("Temperature unit must be F or C")
			}
		case "language":
			if value != "en" && value != "es" && value != "fr" {
				return invalid("Language must be en, es or fr")
			}
		case "detailed_status":
			if _, ok := value.(bool); !ok {
				return invalid("Detailed status must be a boolean")
			}
		default:
			return invalid("Unknown preference")
		}
	}
	return nil
}

func validateTelemetry(settings map[string]any) error {
	if len(settings) != 3 {
		return invalid("Telemetry requires enabled, host and port")
	}
	enabled, ok := settings["enabled"].(bool)
	if !ok {
		return invalid("Telemetry enabled must be boolean")
	}
	host, ok := settings["host"].(string)
	if !ok || len(host) > 127 || enabled && host == "" || host != "" && !validTelemetryHost(host) {
		return invalid("Telemetry host must be a hostname or IPv4 address without a scheme or port")
	}
	var port float64
	switch value := settings["port"].(type) {
	case int:
		port = float64(value)
	case float64:
		port = value
	default:
		return invalid("Telemetry port must be an integer")
	}
	if port < 1 || port > 65535 || port != float64(int(port)) {
		return invalid("Telemetry port must be from 1 to 65535")
	}
	return nil
}

func validTelemetryHost(host string) bool {
	labels := strings.Split(host, ".")
	if len(labels) == 4 {
		var address uint32
		valid := true
		for _, label := range labels {
			number, err := strconv.Atoi(label)
			if err != nil || len(label) < 1 || len(label) > 3 || number < 0 || number > 255 || strings.IndexFunc(label, func(r rune) bool { return r < '0' || r > '9' }) >= 0 {
				valid = false
				break
			}
			address = address<<8 | uint32(number)
		}
		if valid {
			return address != 0 && address != 0xffffffff
		}
	}
	numeric := true
	for _, label := range labels {
		if len(label) == 0 || len(label) > 63 || label[0] == '-' || label[len(label)-1] == '-' {
			return false
		}
		for _, ch := range label {
			if !(ch >= 'a' && ch <= 'z' || ch >= 'A' && ch <= 'Z' || ch >= '0' && ch <= '9' || ch == '-') {
				return false
			}
			if ch < '0' || ch > '9' {
				numeric = false
			}
		}
	}
	return !numeric
}

var historyFields = []string{"elapsed_ms", "wort_f", "thermoblock_f", "target_f", "step_seconds", "phase", "controller_status", "compartment", "current_step", "flags", "device_state"}
