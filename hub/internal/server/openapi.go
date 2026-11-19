package server

import (
	"encoding/json"
	"net/http"
	"reflect"
	"regexp"
	"strconv"
	"strings"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/ai"
	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

type endpoint struct {
	method, path, summary string
	body                  any
}

var endpoints = []endpoint{
	{"GET", "/auth/session", "Read owner session and CSRF token", nil}, {"POST", "/auth/setup", "Create the first owner account", map[string]any{"token": "server-printed-setup-token", "password": "a-long-private-password"}}, {"POST", "/auth/login", "Sign in", map[string]any{"password": "your-password"}}, {"POST", "/auth/logout", "Sign out", map[string]any{}},
	{"GET", "/devices", "List fleet status", nil}, {"POST", "/devices", "Register a machine", map[string]any{"name": "Pico Pro", "model": "pico-pro", "adapter": "pico-standalone", "base_url": "http://192.168.1.50", "api_key": "64-hex-character-device-key", "location": "Brew room"}},
	{"GET", "/devices/{id}", "Read one machine", nil}, {"PUT", "/devices/{id}", "Update a machine connection", map[string]any{"name": "Pico Pro", "model": "pico-pro", "adapter": "pico-standalone", "base_url": "http://192.168.1.50", "location": "Brew room"}}, {"DELETE", "/devices/{id}", "Remove an idle machine", nil}, {"POST", "/devices/{id}/test", "Probe connection and discover capabilities", map[string]any{}}, {"GET", "/devices/{id}/display", "Read raw OLED framebuffer (rotate 180 degrees)", nil}, {"POST", "/devices/{id}/commands", "Queue a capability-checked command once", map[string]any{"command": "encoder", "delta": 1}}, {"GET", "/devices/{id}/recipes", "Read recipes stored on the appliance", nil}, {"GET", "/devices/{id}/settings", "Read device preferences", nil}, {"PATCH", "/devices/{id}/settings", "Update device preferences", map[string]any{"temperature_unit": "F"}}, {"GET", "/devices/{id}/telemetry", "Read forwarding configuration", nil}, {"PUT", "/devices/{id}/telemetry", "Update forwarding configuration", map[string]any{"enabled": false, "host": "", "port": 8080}}, {"POST", "/devices/{id}/sync-history", "Archive retained device history", map[string]any{}}, {"POST", "/devices/{id}/deploy", "Compile and deploy a reviewed recipe without starting it", map[string]any{"recipe_id": "recipe-id", "profile_id": "pico-pro", "batch_liters": 5, "source_revision": 1}},
	{"GET", "/profiles", "List planning and control capabilities by model", nil}, {"GET", "/recipes", "List shared recipes", nil}, {"POST", "/recipes", "Create a recipe", map[string]any{"name": "New recipe", "batch_liters": 5, "efficiency": 55, "ingredients": []any{}, "steps": []any{map[string]any{"name": "Mash", "kind": "mash", "target_c": 67, "minutes": 60, "location": 0, "drain_minutes": 0}}}}, {"GET", "/recipes/{id}", "Read a recipe", nil}, {"PUT", "/recipes/{id}", "Save a recipe using its current revision", map[string]any{"revision": 1}}, {"DELETE", "/recipes/{id}", "Remove a recipe from the library", nil}, {"POST", "/recipes/import", "Import JSON or BeerXML", map[string]any{"format": "beerxml", "data": "<RECIPES>...</RECIPES>"}}, {"POST", "/recipes/{id}/adapt", "Preview a machine-specific recipe adaptation", map[string]any{"profile_id": "pico-pro", "batch_liters": 5}}, {"GET", "/recipes/{id}/export", "Export recipe as JSON or BeerXML (?format=)", nil}, {"GET", "/recipes/{id}/revisions", "Read saved recipe revisions", nil},
	{"GET", "/brews", "List aggregated brew history", nil}, {"GET", "/brews/{id}", "Read all recorded sensors and phases for a brew", nil}, {"PATCH", "/brews/{id}", "Save tasting notes and rating", map[string]any{"notes": "", "rating": 4}}, {"GET", "/brews/{id}/export", "Export full telemetry (?format=csv|json)", nil}, {"GET", "/events", "Stream fleet updates using server-sent events", nil},
	{"POST", "/telemetry", "Receive unsigned observation-only Pico forwarding", map[string]any{"schema_version": 1, "firmware_version": "1.0.0", "status": map[string]any{"device_uid": "known-uid"}}}, {"GET", "/ai/settings", "Read redacted AI provider configuration", nil}, {"PUT", "/ai/settings", "Configure optional provider API credentials", map[string]any{"provider": "openai", "model": "your-model", "api_key": "your-api-key", "enabled": true, "monitor_enabled": false}}, {"POST", "/ai/assist", "Generate a draft, assess live telemetry, or analyze a completed brew", map[string]any{"mode": "recipe", "prompt": "Draft a five-liter porter"}}, {"GET", "/backup", "Export records and telemetry without credentials", nil}, {"GET", "/shopping-list", "Aggregate ingredients (?recipes=id1,id2)", nil}, {"GET", "/openapi.json", "Read this API catalog", nil},
	{"GET", "/ai/advice", "Read retained AI advice, optionally filtered by machine", nil},
	{"GET", "/ai/models", "List provider models using its saved API key", nil},
	{"POST", "/ai/models", "Connect and list models without saving credentials or enabling AI", map[string]any{"provider": "openai", "api_key": "your-api-key"}},
	{"PUT", "/profiles/{id}", "Save measured batch, efficiency and grain limits", map[string]any{"batch_liters": 5, "efficiency": 70, "max_grain_kg": 0}},
	{"DELETE", "/profiles/{id}", "Restore the model's default planning profile", nil},
	{"DELETE", "/devices/{id}/recipes/{recipe_id}", "Delete a recipe stored on an idle appliance", nil},
}

func (s *Server) openapi(w http.ResponseWriter, r *http.Request) {
	s.json(w, 200, openAPIDocument())
}

type schemaMap = map[string]any

func schemaRef(name string) schemaMap       { return schemaMap{"$ref": "#/components/schemas/" + name} }
func schemaArray(items schemaMap) schemaMap { return schemaMap{"type": "array", "items": items} }
func schemaObject(properties schemaMap, required ...string) schemaMap {
	v := schemaMap{"type": "object", "properties": properties, "additionalProperties": false}
	if len(required) > 0 {
		v["required"] = required
	}
	return v
}
func stringSchema() schemaMap { return schemaMap{"type": "string"} }
func numberSchema() schemaMap { return schemaMap{"type": "number", "format": "double"} }
func boolSchema() schemaMap   { return schemaMap{"type": "boolean"} }

// Schemas follow the JSON tags, including flattened embedded structs. Response
// types and request constraints are kept separate: clients do not need to submit
// server-assigned IDs/timestamps just because responses contain those fields.
type schemaRegistry struct{ values schemaMap }

func (r *schemaRegistry) describe(t reflect.Type) schemaMap {
	if t == reflect.TypeOf(time.Time{}) {
		return schemaMap{"type": "string", "format": "date-time"}
	}
	if t == reflect.TypeOf(json.RawMessage{}) {
		return schemaMap{}
	}
	if t.Kind() == reflect.Pointer {
		inner := r.describe(t.Elem())
		if _, ok := inner["$ref"]; ok {
			return schemaMap{"allOf": []any{inner}, "nullable": true}
		}
		copy := schemaMap{}
		for k, v := range inner {
			copy[k] = v
		}
		copy["nullable"] = true
		return copy
	}
	switch t.Kind() {
	case reflect.Struct:
		if t.Name() != "" {
			if _, exists := r.values[t.Name()]; exists {
				return schemaRef(t.Name())
			}
			r.values[t.Name()] = schemaMap{}
		}
		props := schemaMap{}
		required := []string{}
		var fields func(reflect.Type)
		fields = func(structure reflect.Type) {
			for i := 0; i < structure.NumField(); i++ {
				field := structure.Field(i)
				if field.PkgPath != "" {
					continue
				}
				parts := strings.Split(field.Tag.Get("json"), ",")
				name := parts[0]
				if name == "-" {
					continue
				}
				if field.Anonymous && name == "" {
					embedded := field.Type
					if embedded.Kind() == reflect.Pointer {
						embedded = embedded.Elem()
					}
					if embedded.Kind() == reflect.Struct {
						fields(embedded)
						continue
					}
				}
				if name == "" {
					name = field.Name
				}
				props[name] = r.describe(field.Type)
				optional := false
				for _, option := range parts[1:] {
					if option == "omitempty" || option == "omitzero" {
						optional = true
					}
				}
				if !optional {
					required = append(required, name)
				}
			}
		}
		fields(t)
		result := schemaObject(props, required...)
		if t.Name() != "" {
			r.values[t.Name()] = result
			return schemaRef(t.Name())
		}
		return result
	case reflect.Slice, reflect.Array:
		result := schemaArray(r.describe(t.Elem()))
		if t.Kind() == reflect.Slice {
			result["nullable"] = true
		}
		return result
	case reflect.Map:
		return schemaMap{"type": "object", "additionalProperties": r.describe(t.Elem()), "nullable": true}
	case reflect.Bool:
		return boolSchema()
	case reflect.String:
		return stringSchema()
	case reflect.Int, reflect.Int8, reflect.Int16, reflect.Int32, reflect.Int64:
		return schemaMap{"type": "integer", "format": "int64"}
	case reflect.Uint, reflect.Uint8, reflect.Uint16, reflect.Uint32, reflect.Uint64:
		return schemaMap{"type": "integer", "minimum": 0}
	case reflect.Float32, reflect.Float64:
		return numberSchema()
	default:
		return schemaMap{}
	}
}

func (r *schemaRegistry) properties(name string) schemaMap {
	return r.values[name].(schemaMap)["properties"].(schemaMap)
}

func buildSchemas() schemaMap {
	r := &schemaRegistry{values: schemaMap{}}
	for _, value := range []any{domain.Device{}, domain.DeviceView{}, domain.Capabilities{}, domain.Status{}, domain.Display{}, domain.Recipe{}, domain.MachineRecipe{}, domain.Profile{}, domain.Adapted{}, domain.Brew{}, domain.BrewDetail{}, domain.Sample{}, domain.History{}, domain.Command{}, commandReceipt{}, ai.Config{}, ai.Request{}, ai.Result{}, adviceRecord{}, aiModelList{}} {
		r.describe(reflect.TypeOf(value))
	}
	r.properties("DeviceView")["key_reused"] = schemaMap{"type": "boolean", "readOnly": true, "description": "Advisory: another registered physical machine has the same recorded hexadecimal API key, including disabled machines. Derived in memory; no key or fingerprint is returned. Use a separately provisioned key for each brewer. Saving a connection does not change its firmware key."}
	r.values["Error"] = schemaObject(schemaMap{"error": schemaObject(schemaMap{"code": stringSchema(), "message": stringSchema()}, "code", "message")}, "error")
	r.values["AuthSession"] = schemaObject(schemaMap{"authenticated": boolSchema(), "setup_required": boolSchema(), "version": stringSchema(), "demo": boolSchema(), "csrf_token": stringSchema()}, "authenticated", "setup_required", "version", "demo")
	r.values["Authenticated"] = schemaObject(schemaMap{"authenticated": boolSchema()}, "authenticated")
	r.values["Deleted"] = schemaObject(schemaMap{"deleted": boolSchema()}, "deleted")
	r.values["Synced"] = schemaObject(schemaMap{"synced": boolSchema()}, "synced")
	r.values["Accepted"] = schemaObject(schemaMap{"accepted": boolSchema()}, "accepted")
	r.values["DeviceRecipes"] = schemaObject(schemaMap{"recipes": schemaArray(schemaRef("MachineRecipe"))}, "recipes")
	r.values["Deployment"] = schemaObject(schemaMap{"deployed": boolSchema(), "started": boolSchema(), "device_id": stringSchema(), "recipe": schemaRef("MachineRecipe"), "warnings": schemaArray(stringSchema()), "deployment_id": stringSchema()}, "deployed", "started", "device_id", "recipe", "warnings", "deployment_id")
	r.values["Settings"] = schemaMap{"type": "object", "properties": schemaMap{"temperature_unit": schemaMap{"type": "string", "enum": []string{"F", "C"}}, "language": schemaMap{"type": "string", "enum": []string{"en", "es", "fr"}}, "detailed_status": boolSchema()}, "additionalProperties": true}
	r.values["Telemetry"] = schemaMap{"type": "object", "properties": schemaMap{"enabled": boolSchema(), "host": stringSchema(), "port": schemaMap{"type": "integer", "minimum": 1, "maximum": 65535}, "state": stringSchema()}, "additionalProperties": true}
	r.values["Login"] = schemaObject(schemaMap{"password": schemaMap{"type": "string", "maxLength": 256, "writeOnly": true}}, "password")
	r.values["Setup"] = schemaObject(schemaMap{"password": schemaMap{"type": "string", "minLength": 12, "maxLength": 256, "writeOnly": true}, "token": schemaMap{"type": "string", "writeOnly": true}}, "password", "token")
	r.values["DeviceInput"] = schemaObject(schemaMap{"name": schemaMap{"type": "string", "minLength": 1, "maxLength": 100}, "model": schemaMap{"type": "string", "enum": []string{"pico-pro", "pico-s", "pico-c", "z", "zymatic"}}, "adapter": schemaMap{"type": "string", "enum": []string{"pico-standalone", "simulator"}}, "base_url": schemaMap{"type": "string", "description": "Explicit appliance HTTP base URL; required for pico-standalone. Control is currently supported only for S/Pro standalone firmware."}, "api_key": schemaMap{"type": "string", "writeOnly": true, "description": "Standalone firmware key; omit/leave blank on update to preserve the stored key."}, "location": stringSchema(), "enabled": boolSchema()}, "name", "model", "adapter")
	recipeProps := schemaMap{}
	for k, v := range r.properties("Recipe") {
		recipeProps[k] = v
	}
	for _, name := range []string{"id", "created_at", "updated_at"} {
		recipeProps[name] = schemaMap{"type": "string", "readOnly": true}
	}
	r.values["RecipeInput"] = schemaObject(recipeProps, "name", "batch_liters", "efficiency", "steps")
	r.properties("Recipe")["batch_liters"] = schemaMap{"type": "number", "minimum": 0, "exclusiveMinimum": true, "maximum": 10000}
	r.properties("Recipe")["efficiency"] = schemaMap{"type": "number", "minimum": 0, "exclusiveMinimum": true, "maximum": 100, "description": "Measured percentage, not a 0–1 fraction."}
	r.properties("Recipe")["steps"] = schemaMap{"type": "array", "minItems": 1, "maxItems": 256, "items": schemaRef("RecipeStep")}
	for _, name := range []string{"batch_liters", "efficiency", "steps"} {
		recipeProps[name] = r.properties("Recipe")[name]
	}
	r.properties("RecipeStep")["location"] = schemaMap{"type": "integer", "minimum": 0, "maximum": 6, "description": "Zero is unmapped. Deployment needs an explicitly verified 1–6; the hub never infers plumbing."}
	r.properties("RecipeStep")["target_c"] = schemaMap{"type": "number", "minimum": 0, "maximum": 100, "description": "Canonical Celsius; zero is the unheated program sentinel."}
	r.properties("RecipeStep")["minutes"] = schemaMap{"type": "integer", "minimum": 1, "maximum": 43200, "description": "Native S/Pro deployment limits this to 599 minutes."}
	r.properties("RecipeStep")["drain_minutes"] = schemaMap{"type": "integer", "minimum": 0, "maximum": 1440, "description": "Native S/Pro deployment limits this to 120 minutes."}
	r.properties("Ingredient")["kind"] = schemaMap{"type": "string", "enum": []string{"fermentable", "hop", "water", "yeast", "misc"}}
	r.properties("Ingredient")["unit"] = schemaMap{"type": "string", "enum": []string{"kg", "g", "L", "mL", "count"}, "description": "Fermentable: kg; hop: g; water: L; yeast/misc: g, mL, count."}
	r.properties("MachineRecipe")["id"] = schemaMap{"type": "string", "pattern": "^[A-Za-z0-9_-]{1,20}$"}
	r.properties("MachineRecipe")["name"] = schemaMap{"type": "string", "minLength": 1, "maxLength": 31, "pattern": "^[ -~]+$"}
	r.properties("MachineRecipe")["steps"] = schemaMap{"type": "array", "minItems": 1, "maxItems": 15, "items": schemaRef("MachineStep")}
	r.properties("MachineStep")["name"] = schemaMap{"type": "string", "minLength": 1, "maxLength": 21, "pattern": "^[ -~]+$"}
	r.properties("MachineStep")["location"] = schemaMap{"type": "integer", "minimum": 1, "maximum": 6}
	r.properties("MachineStep")["target_f"] = schemaMap{"oneOf": []any{schemaMap{"type": "integer", "enum": []int{0}}, schemaMap{"type": "integer", "minimum": 60, "maximum": 212}}}
	r.properties("MachineStep")["minutes"] = schemaMap{"type": "integer", "minimum": 1, "maximum": 599}
	r.properties("MachineStep")["drain_minutes"] = schemaMap{"type": "integer", "minimum": 0, "maximum": 120}
	command := schemaMap{}
	for k, v := range r.properties("Command") {
		command[k] = v
	}
	command["confirm"] = boolSchema()
	command["command"] = schemaMap{"type": "string", "description": "Must appear in this device's capabilities.commands. The API has no dedicated pause/resume or live recipe editing."}
	r.values["CommandInput"] = schemaObject(command, "command")
	r.values["AdaptInput"] = schemaObject(schemaMap{"profile_id": stringSchema(), "batch_liters": numberSchema()}, "profile_id")
	r.values["DeployInput"] = schemaObject(schemaMap{"recipe_id": stringSchema(), "profile_id": stringSchema(), "batch_liters": numberSchema(), "source_revision": schemaMap{"type": "integer", "minimum": 1}}, "recipe_id", "source_revision")
	r.values["ProfileInput"] = schemaObject(schemaMap{"batch_liters": schemaMap{"type": "number", "minimum": 0, "exclusiveMinimum": true, "maximum": 50}, "efficiency": schemaMap{"type": "number", "minimum": 0, "exclusiveMinimum": true, "maximum": 100}, "max_grain_kg": schemaMap{"type": "number", "minimum": 0, "maximum": 25, "description": "Zero means unverified, not unlimited proven capacity."}}, "batch_liters", "efficiency", "max_grain_kg")
	r.values["ImportInput"] = schemaObject(schemaMap{"format": schemaMap{"type": "string", "enum": []string{"json", "beerxml"}}, "data": schemaMap{"type": "string", "maxLength": 1048576, "description": "Canonical recipe JSON object/array or supported BeerXML 1 text. DTDs and external entities are rejected."}}, "format", "data")
	r.values["BrewNotes"] = schemaObject(schemaMap{"notes": schemaMap{"type": "string", "maxLength": 32000}, "rating": schemaMap{"type": "integer", "minimum": 0, "maximum": 5}}, "notes", "rating")
	r.values["TelemetryInput"] = schemaObject(schemaMap{"enabled": boolSchema(), "host": schemaMap{"type": "string", "maxLength": 127, "description": "Hostname or IPv4 without scheme, port or path; empty only when disabled."}, "port": schemaMap{"type": "integer", "minimum": 1, "maximum": 65535}}, "enabled", "host", "port")
	settingsProps := r.values["Settings"].(schemaMap)["properties"].(schemaMap)
	r.values["SettingsInput"] = schemaObject(settingsProps)
	r.values["SettingsInput"].(schemaMap)["minProperties"] = 1
	r.values["ForwardingEnvelope"] = schemaObject(schemaMap{"schema_version": schemaMap{"type": "integer", "enum": []int{1}}, "firmware_version": stringSchema(), "status": schemaRef("Status")}, "schema_version", "status")
	aiConfig := schemaMap{}
	for k, v := range r.properties("Config") {
		aiConfig[k] = v
	}
	aiConfig["api_key"] = schemaMap{"type": "string", "maxLength": 1024, "writeOnly": true, "description": "Provider API key, not a ChatGPT/Claude consumer account password. Blank preserves the stored key."}
	aiConfig["model"] = schemaMap{"type": "string", "maxLength": 512, "description": "Provider model ID. Model discovery is available before choosing a model or enabling AI."}
	r.values["AISettingsInput"] = schemaObject(aiConfig, "provider")
	r.values["AIModelsInput"] = schemaObject(schemaMap{
		"provider": schemaMap{"type": "string", "enum": []string{"openai", "anthropic"}},
		"api_key":  schemaMap{"type": "string", "maxLength": 1024, "writeOnly": true, "description": "Optional unsaved key, used only for model discovery. Omit to use this provider's saved key. This operation does not change settings or credentials."},
	}, "provider")
	r.properties("aiModelList")["models"] = schemaMap{"type": "array", "items": schemaRef("Model"), "description": "Account-visible model IDs and display names. Availability does not establish compatibility with the provider's chat API."}
	r.properties("Request")["mode"] = schemaMap{"type": "string", "enum": []string{"recipe", "live", "analysis"}}
	r.properties("Request")["prompt"] = schemaMap{"type": "string", "maxLength": 16000}
	r.properties("Result")["recipe"] = schemaRef("Recipe")
	r.properties("adviceRecord")["recipe"] = schemaRef("Recipe")
	return r.values
}

var pathParameters = regexp.MustCompile(`\{([^}]+)\}`)

func operationParameters(e endpoint) []any {
	params := []any{}
	for _, match := range pathParameters.FindAllStringSubmatch(e.path, -1) {
		params = append(params, schemaMap{"name": match[1], "in": "path", "required": true, "schema": stringSchema()})
	}
	if strings.HasSuffix(e.path, "/commands") {
		params = append(params, schemaMap{"name": "Idempotency-Key", "in": "header", "required": true, "description": "A unique 8–128 character key for one intended command. Reusing it returns its receipt without resending; a different command with the same key conflicts.", "schema": schemaMap{"type": "string", "minLength": 8, "maxLength": 128}})
	}
	query := func(name, description string, schema schemaMap) {
		params = append(params, schemaMap{"name": name, "in": "query", "description": description, "required": false, "schema": schema})
	}
	switch e.path {
	case "/recipes/{id}/export":
		query("format", "Output format; JSON preserves every canonical field.", schemaMap{"type": "string", "enum": []string{"json", "beerxml"}, "default": "json"})
	case "/brews/{id}/export":
		query("format", "Full recorded samples without graph downsampling.", schemaMap{"type": "string", "enum": []string{"json", "csv"}, "default": "json"})
	case "/shopping-list":
		query("recipes", "Comma-separated canonical recipe IDs, for example id1,id2. Empty yields an empty list.", stringSchema())
	case "/ai/advice":
		query("device_id", "Limit advice to one registered machine.", stringSchema())
	case "/ai/models":
		if e.method == "GET" {
			query("provider", "Provider whose saved key should be used. Defaults to the selected provider.", schemaMap{"type": "string", "enum": []string{"openai", "anthropic"}})
		}
	}
	return params
}

func requestSchema(e endpoint) (schemaMap, bool) {
	name := ""
	switch e.path {
	case "/auth/setup":
		name = "Setup"
	case "/auth/login":
		name = "Login"
	case "/devices", "/devices/{id}":
		name = "DeviceInput"
	case "/devices/{id}/commands":
		name = "CommandInput"
	case "/devices/{id}/settings":
		name = "SettingsInput"
	case "/devices/{id}/telemetry":
		name = "TelemetryInput"
	case "/devices/{id}/deploy":
		name = "DeployInput"
	case "/profiles/{id}":
		name = "ProfileInput"
	case "/recipes", "/recipes/{id}":
		name = "RecipeInput"
	case "/recipes/{id}/adapt":
		name = "AdaptInput"
	case "/recipes/import":
		name = "ImportInput"
	case "/brews/{id}":
		name = "BrewNotes"
	case "/telemetry":
		name = "ForwardingEnvelope"
	case "/ai/settings":
		name = "AISettingsInput"
	case "/ai/models":
		name = "AIModelsInput"
	case "/ai/assist":
		name = "Request"
	}
	if name != "" {
		return schemaRef(name), true
	}
	return schemaMap{"type": "object", "description": "No JSON fields are required by this action."}, false
}

func successResponses(e endpoint) schemaMap {
	schema := schemaMap{"type": "object"}
	code := "200"
	description := "Successful response"
	switch e.path {
	case "/auth/session":
		schema = schemaRef("AuthSession")
	case "/auth/setup", "/auth/login":
		schema = schemaRef("Authenticated")
	case "/auth/logout":
		code = "204"
	case "/devices":
		if e.method == "GET" {
			schema = schemaArray(schemaRef("DeviceView"))
		} else {
			code = "201"
			schema = schemaRef("DeviceView")
		}
	case "/devices/{id}":
		if e.method == "DELETE" {
			schema = schemaRef("Deleted")
		} else if e.method == "PUT" {
			schema = schemaRef("DeviceView")
		} else {
			schema = schemaRef("DeviceView")
		}
	case "/devices/{id}/test":
		schema = schemaRef("DeviceView")
	case "/devices/{id}/display":
		schema = schemaRef("Display")
	case "/devices/{id}/commands":
		code = "202"
		schema = schemaRef("commandReceipt")
		description = "Accepted into the device queue, not proof of physical execution. Observe the machine/front panel."
	case "/devices/{id}/recipes":
		schema = schemaRef("DeviceRecipes")
	case "/devices/{id}/recipes/{recipe_id}":
		code = "204"
	case "/devices/{id}/settings":
		schema = schemaRef("Settings")
	case "/devices/{id}/telemetry":
		schema = schemaRef("Telemetry")
	case "/devices/{id}/sync-history":
		schema = schemaRef("Synced")
	case "/devices/{id}/deploy":
		schema = schemaRef("Deployment")
	case "/profiles":
		schema = schemaArray(schemaRef("Profile"))
	case "/profiles/{id}":
		schema = schemaRef("Profile")
	case "/recipes":
		if e.method == "GET" {
			schema = schemaArray(schemaRef("Recipe"))
		} else {
			code = "201"
			schema = schemaRef("Recipe")
		}
	case "/recipes/{id}":
		schema = schemaRef("Recipe")
		if e.method == "DELETE" {
			code = "204"
		}
	case "/recipes/import":
		code = "201"
		schema = schemaArray(schemaRef("Recipe"))
	case "/recipes/{id}/adapt":
		schema = schemaRef("Adapted")
	case "/recipes/{id}/revisions":
		schema = schemaArray(schemaRef("Recipe"))
	case "/brews":
		schema = schemaArray(schemaRef("Brew"))
	case "/brews/{id}":
		if e.method == "PATCH" {
			schema = schemaRef("Brew")
		} else {
			schema = schemaRef("BrewDetail")
		}
	case "/telemetry":
		code = "202"
		schema = schemaRef("Accepted")
		description = "Unsigned observation stored; it cannot command a device or override authoritative live state."
	case "/ai/settings":
		schema = schemaRef("Config")
	case "/ai/models":
		schema = schemaRef("aiModelList")
	case "/ai/assist":
		schema = schemaRef("Result")
	case "/ai/advice":
		schema = schemaArray(schemaRef("adviceRecord"))
	case "/shopping-list":
		schema = schemaArray(schemaRef("Ingredient"))
	}
	content := schemaMap{"application/json": schemaMap{"schema": schema}}
	switch e.path {
	case "/events":
		content = schemaMap{"text/event-stream": schemaMap{"schema": stringSchema(), "example": "event: fleet\ndata: []\n\n"}}
		description = "SSE fleet events; each data field is a JSON DeviceView array."
	case "/backup":
		content = schemaMap{"application/zip": schemaMap{"schema": schemaMap{"type": "string", "format": "binary"}}}
		description = "Data export ZIP. Credentials and master.key are excluded; this is not a full disaster-recovery backup."
	case "/recipes/{id}/export":
		content = schemaMap{"application/json": schemaMap{"schema": schemaRef("Recipe")}, "application/xml": schemaMap{"schema": stringSchema()}}
	case "/brews/{id}/export":
		content = schemaMap{"application/json": schemaMap{"schema": schemaRef("BrewDetail")}, "text/csv": schemaMap{"schema": stringSchema()}}
	}
	response := schemaMap{"description": description}
	if code != "204" {
		response["content"] = content
	}
	result := schemaMap{code: response}
	if strings.HasSuffix(e.path, "/commands") {
		result["200"] = schemaMap{"description": "Existing receipt for this Idempotency-Key; the command was not resent.", "content": content}
	}
	return result
}

func operationResponses(e endpoint, public, requiresJSON bool) schemaMap {
	responses := successResponses(e)
	statuses := map[int]string{500: "Storage or internal operation failed."}
	if !public {
		statuses[401] = "Owner session required."
		statuses[403] = "Request origin/CSRF rejected, or demo mode does not permit this host/action."
	}
	if requiresJSON {
		statuses[400] = "Malformed JSON, unknown fields or invalid values."
		statuses[413] = "JSON body exceeds the 2 MiB request limit."
		statuses[415] = "Use application/json."
	}
	if strings.Contains(e.path, "{id}") || e.path == "/shopping-list" || e.path == "/ai/assist" {
		statuses[404] = "Requested record or machine does not exist."
	}
	if strings.HasPrefix(e.path, "/devices/{id}") {
		for code, message := range map[int]string{400: "Invalid device command or configuration.", 409: "Machine state, device file or command key conflict.", 413: "Device rejected an oversized payload.", 422: "Device/target profile rejected this program.", 499: "The device request was cancelled.", 502: "Device unavailable, rejected transport response or mutation outcome unknown; writes are not automatically retried.", 503: "The machine connection/adapter is not configured."} {
			statuses[code] = message
		}
	}
	if e.path == "/auth/login" || e.path == "/auth/setup" {
		statuses[401] = "Incorrect password or setup token."
		statuses[403] = "Invalid request origin."
		statuses[409] = "Owner already configured or action unavailable in demo mode."
		statuses[429] = "Too many sign-in attempts; try again in a minute."
	}
	if e.method == "PUT" || e.method == "PATCH" || e.path == "/devices" && e.method == "POST" {
		statuses[409] = "Record revision or current state conflicts with this request."
	}
	if e.path == "/recipes/{id}/adapt" {
		statuses[422] = "Recipe cannot be mapped to the selected profile without violating its limits."
	}
	if e.path == "/profiles/{id}" && e.method == "PUT" {
		statuses[422] = "Batch volume, efficiency or grain capacity is outside the calibration bounds."
	}
	if e.path == "/recipes/{id}/export" {
		statuses[400] = "Format unsupported, or required BeerXML metadata is missing."
	}
	if e.path == "/ai/assist" {
		statuses[409] = "AI provider/key/model is not enabled."
		statuses[429] = "Another AI request is in progress."
		statuses[502] = "Provider failed or returned an invalid recipe."
	}
	if e.path == "/ai/models" {
		statuses[400] = "Unsupported provider or invalid provider API key."
		statuses[409] = "No API key is saved or supplied for this provider."
		statuses[502] = "Provider model discovery failed; saved credentials and model selection remain unchanged."
	}
	if e.path == "/telemetry" {
		statuses[403] = "Device UID has not been registered and discovered."
	}
	for code, description := range statuses {
		responses[strconv.Itoa(code)] = schemaMap{"description": description, "content": schemaMap{"application/json": schemaMap{"schema": schemaRef("Error")}}}
	}
	return responses
}

func openAPIDocument() schemaMap {
	paths := map[string]any{}
	for _, e := range endpoints {
		path, ok := paths[e.path].(map[string]any)
		if !ok {
			path = map[string]any{}
			paths[e.path] = path
		}
		params := operationParameters(e)
		security := []any{map[string]any{"session": []string{}}}
		if e.method != "GET" {
			security = []any{map[string]any{"session": []string{}, "csrf": []string{}}}
		}
		public := strings.HasPrefix(e.path, "/auth/") && e.path != "/auth/logout" || e.path == "/telemetry"
		if public {
			security = []any{}
		}
		request, requiresJSON := requestSchema(e)
		requiresJSON = requiresJSON && e.body != nil
		op := map[string]any{"operationId": strings.ToLower(e.method) + "_" + strings.NewReplacer("/", "_", "{", "", "}", "", "-", "_").Replace(strings.TrimPrefix(e.path, "/")), "summary": e.summary, "security": security, "parameters": params, "responses": operationResponses(e, public, requiresJSON)}
		if e.body != nil {
			example := e.body
			if e.path == "/recipes/{id}" && e.method == "PUT" {
				example = recipeAPIExample(1)
			}
			if e.path == "/recipes/import" {
				data, _ := json.Marshal(recipeAPIExample(0))
				example = schemaMap{"format": "json", "data": string(data)}
			}
			op["requestBody"] = map[string]any{"required": requiresJSON, "content": map[string]any{"application/json": map[string]any{"schema": request, "example": example}}}
		}
		path[strings.ToLower(e.method)] = op
	}
	return schemaMap{"openapi": "3.0.3", "info": map[string]any{"title": "PicoBrew Hub API", "version": Version, "description": "Same-origin owner API. Obtain the session cookie at login, then read /auth/session for X-CSRF-Token on protected writes. Status codes describe HTTP acceptance, not proof of physical execution. Controls follow discovered capabilities; no dedicated pause/resume or live recipe editing is implemented. AI is advisory and cannot issue commands. Body decoding accepts one JSON value, rejects unknown fields and limits requests to 2 MiB. Recipe files additionally have a 1 MiB import limit. Optional device forwarding is unsigned observation data, not control or authoritative status."}, "servers": []any{map[string]string{"url": "/api/v1"}}, "paths": paths, "components": map[string]any{"schemas": buildSchemas(), "securitySchemes": map[string]any{"session": map[string]string{"type": "apiKey", "in": "cookie", "name": "pico_hub_session"}, "csrf": map[string]string{"type": "apiKey", "in": "header", "name": "X-CSRF-Token"}}}}
}

func recipeAPIExample(revision int64) schemaMap {
	return schemaMap{"name": "New recipe", "batch_liters": 5, "efficiency": 55, "ingredients": []any{}, "steps": []any{schemaMap{"name": "Mash", "kind": "mash", "target_c": 67, "minutes": 60, "location": 0, "drain_minutes": 0}}, "revision": revision}
}
