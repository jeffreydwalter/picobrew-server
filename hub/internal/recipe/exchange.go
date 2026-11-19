package recipe

import (
	"bytes"
	"encoding/json"
	"encoding/xml"
	"errors"
	"fmt"
	"io"
	"math"
	"strings"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

// Import accepts canonical JSON (one recipe or an array) or the supported
// BeerXML 1 recipe subset. Imports are bounded and validated as a whole.
func Import(data []byte, format string) ([]domain.Recipe, error) {
	if len(data) == 0 || len(data) > MaxImportBytes {
		return nil, errors.New("recipe import must contain 1–1048576 bytes")
	}
	var recipes []domain.Recipe
	switch strings.ToLower(strings.TrimSpace(format)) {
	case "json", "application/json":
		if err := uniqueJSON(data); err != nil {
			return nil, err
		}
		decoder := json.NewDecoder(bytes.NewReader(data))
		decoder.DisallowUnknownFields()
		trimmed := bytes.TrimSpace(data)
		if len(trimmed) == 0 {
			return nil, errors.New("empty JSON")
		}
		if trimmed[0] == '[' {
			if err := decoder.Decode(&recipes); err != nil {
				return nil, err
			}
		} else {
			var r domain.Recipe
			if err := decoder.Decode(&r); err != nil {
				return nil, err
			}
			recipes = []domain.Recipe{r}
		}
	case "xml", "beerxml", "application/xml", "text/xml":
		var err error
		recipes, err = importXML(data)
		if err != nil {
			return nil, err
		}
	default:
		return nil, fmt.Errorf("unsupported recipe import format %q", format)
	}
	if len(recipes) == 0 || len(recipes) > 256 {
		return nil, errors.New("import requires 1–256 recipes")
	}
	for i, r := range recipes {
		if err := Validate(r); err != nil {
			return nil, fmt.Errorf("recipe %d: %w", i+1, err)
		}
	}
	return recipes, nil
}

// Export emits canonical JSON or a BeerXML recipe. JSON retains every hub field.
// BeerXML carries its supported standardized fields plus a HUB_STEPS extension
// to retain explicit routing/durations when the file returns to this hub.
func Export(r domain.Recipe, format string) ([]byte, error) {
	if err := Validate(r); err != nil {
		return nil, err
	}
	var data []byte
	var err error
	switch strings.ToLower(strings.TrimSpace(format)) {
	case "json", "application/json":
		data, err = json.MarshalIndent(r, "", "  ")
	case "xml", "beerxml", "application/xml", "text/xml":
		data, err = exportXML(r)
	default:
		return nil, fmt.Errorf("unsupported recipe export format %q", format)
	}
	if err == nil && len(data) > MaxImportBytes {
		return nil, errors.New("export exceeds the 1 MiB exchange limit")
	}
	return data, err
}

// Walk JSON tokens first to reject ambiguous duplicate keys and trailing values.
func uniqueJSON(data []byte) error {
	d := json.NewDecoder(bytes.NewReader(data))
	d.UseNumber()
	var value func(int) error
	value = func(depth int) error {
		if depth > 64 {
			return errors.New("JSON nesting exceeds 64 levels")
		}
		token, err := d.Token()
		if err != nil {
			return err
		}
		delim, ok := token.(json.Delim)
		if !ok {
			return nil
		}
		switch delim {
		case '{':
			seen := map[string]bool{}
			for d.More() {
				key, err := d.Token()
				if err != nil {
					return err
				}
				name, ok := key.(string)
				if !ok {
					return errors.New("invalid JSON object key")
				}
				if seen[name] {
					return fmt.Errorf("duplicate JSON key %q", name)
				}
				seen[name] = true
				if err := value(depth + 1); err != nil {
					return err
				}
			}
		case '[':
			for d.More() {
				if err := value(depth + 1); err != nil {
					return err
				}
			}
		default:
			return errors.New("invalid JSON delimiter")
		}
		_, err = d.Token()
		return err
	}
	if err := value(0); err != nil {
		return err
	}
	if _, err := d.Token(); err != io.EOF {
		if err == nil {
			return errors.New("multiple JSON documents")
		}
		return err
	}
	return nil
}

type xmlRecipes struct {
	XMLName xml.Name    `xml:"RECIPES"`
	Recipes []xmlRecipe `xml:"RECIPE"`
}
type xmlRecipe struct {
	XMLName            xml.Name        `xml:"RECIPE"`
	Name               string          `xml:"NAME"`
	Version            int             `xml:"VERSION"`
	Type               string          `xml:"TYPE"`
	Brewer             string          `xml:"BREWER"`
	Batch              *float64        `xml:"BATCH_SIZE"`
	BoilSize           *float64        `xml:"BOIL_SIZE"`
	BoilTime           *float64        `xml:"BOIL_TIME"`
	Efficiency         *float64        `xml:"EFFICIENCY"`
	Notes              string          `xml:"NOTES,omitempty"`
	Style              xmlStyle        `xml:"STYLE"`
	Fermentables       []xmlIngredient `xml:"FERMENTABLES>FERMENTABLE"`
	Hops               []xmlIngredient `xml:"HOPS>HOP"`
	Yeasts             []xmlIngredient `xml:"YEASTS>YEAST"`
	Miscs              []xmlIngredient `xml:"MISCS>MISC"`
	Waters             []xmlWater      `xml:"WATERS>WATER"`
	Mash               xmlMash         `xml:"MASH"`
	PrimaryAge         *float64        `xml:"PRIMARY_AGE,omitempty"`
	PrimaryTemp        *float64        `xml:"PRIMARY_TEMP,omitempty"`
	SecondaryAge       *float64        `xml:"SECONDARY_AGE,omitempty"`
	SecondaryTemp      *float64        `xml:"SECONDARY_TEMP,omitempty"`
	TertiaryAge        *float64        `xml:"TERTIARY_AGE,omitempty"`
	TertiaryTemp       *float64        `xml:"TERTIARY_TEMP,omitempty"`
	FermentationStages int             `xml:"FERMENTATION_STAGES,omitempty"`
	Carbonation        *float64        `xml:"CARBONATION,omitempty"`
	CarbonationTemp    *float64        `xml:"CARBONATION_TEMP,omitempty"`
	ForcedCarbonation  string          `xml:"FORCED_CARBONATION,omitempty"`
	HubSteps           string          `xml:"HUB_STEPS,omitempty"`
}
type xmlStyle struct {
	Name    string `xml:"NAME"`
	Version int    `xml:"VERSION"`
}
type xmlIngredient struct {
	Name           string   `xml:"NAME"`
	Version        int      `xml:"VERSION"`
	Type           string   `xml:"TYPE,omitempty"`
	Form           string   `xml:"FORM,omitempty"`
	Amount         *float64 `xml:"AMOUNT"`
	Use            string   `xml:"USE,omitempty"`
	Time           *float64 `xml:"TIME,omitempty"`
	Alpha          *float64 `xml:"ALPHA,omitempty"`
	Yield          *float64 `xml:"YIELD,omitempty"`
	Color          *float64 `xml:"COLOR,omitempty"`
	AmountIsWeight string   `xml:"AMOUNT_IS_WEIGHT,omitempty"`
}
type xmlMash struct {
	Name    string        `xml:"NAME"`
	Version int           `xml:"VERSION"`
	Steps   []xmlMashStep `xml:"MASH_STEPS>MASH_STEP"`
}
type xmlMashStep struct {
	Name    string   `xml:"NAME"`
	Version int      `xml:"VERSION"`
	Type    string   `xml:"TYPE"`
	Temp    *float64 `xml:"STEP_TEMP"`
	Time    *float64 `xml:"STEP_TIME"`
	Infuse  *float64 `xml:"INFUSE_AMOUNT,omitempty"`
	Ramp    *float64 `xml:"RAMP_TIME,omitempty"`
}
type xmlWater struct {
	Name        string   `xml:"NAME"`
	Version     int      `xml:"VERSION"`
	Amount      *float64 `xml:"AMOUNT"`
	Calcium     *float64 `xml:"CALCIUM,omitempty"`
	Bicarbonate *float64 `xml:"BICARBONATE,omitempty"`
	Sulfate     *float64 `xml:"SULFATE,omitempty"`
	Chloride    *float64 `xml:"CHLORIDE,omitempty"`
	Sodium      *float64 `xml:"SODIUM,omitempty"`
	Magnesium   *float64 `xml:"MAGNESIUM,omitempty"`
	PH          *float64 `xml:"PH,omitempty"`
}

func pointer(value float64) *float64 { return &value }
func number(value *float64, name string) (float64, error) {
	if value == nil || !finite(*value) {
		return 0, fmt.Errorf("BeerXML %s is required and must be finite", name)
	}
	return *value, nil
}
func enum(value string, allowed ...string) (string, error) {
	for _, candidate := range allowed {
		if strings.EqualFold(strings.TrimSpace(value), candidate) {
			return candidate, nil
		}
	}
	return "", fmt.Errorf("unsupported or missing value %q (expected %s)", value, strings.Join(allowed, ", "))
}

func xmlDecoder(data []byte) *xml.Decoder {
	d := xml.NewDecoder(bytes.NewReader(data))
	d.CharsetReader = func(charset string, input io.Reader) (io.Reader, error) {
		if !strings.EqualFold(charset, "ISO-8859-1") {
			return nil, fmt.Errorf("unsupported XML charset %q; use UTF-8 or ISO-8859-1", charset)
		}
		bytes, err := io.ReadAll(io.LimitReader(input, MaxImportBytes+1))
		if err != nil {
			return nil, err
		}
		if len(bytes) > MaxImportBytes {
			return nil, errors.New("XML input too large")
		}
		runes := make([]rune, len(bytes))
		for i, b := range bytes {
			runes[i] = rune(b)
		}
		return strings.NewReader(string(runes)), nil
	}
	return d
}

func importXML(data []byte) ([]domain.Recipe, error) {
	d := xmlDecoder(data)
	depth := 0
	roots := 0
	root := ""
	for {
		token, err := d.Token()
		if err == io.EOF {
			break
		}
		if err != nil {
			return nil, err
		}
		switch t := token.(type) {
		case xml.Directive:
			return nil, errors.New("XML directives/DTDs and external entities are not permitted")
		case xml.StartElement:
			if depth == 0 {
				roots++
				root = t.Name.Local
			}
			depth++
			if depth > 64 {
				return nil, errors.New("XML nesting exceeds 64 levels")
			}
		case xml.EndElement:
			depth--
		case xml.CharData:
			if depth == 0 && strings.TrimSpace(string(t)) != "" {
				return nil, errors.New("text outside XML root")
			}
		}
	}
	if roots != 1 {
		return nil, errors.New("BeerXML requires exactly one root element")
	}
	var records []xmlRecipe
	if root == "RECIPES" {
		var doc xmlRecipes
		if err := xmlDecoder(data).Decode(&doc); err != nil {
			return nil, err
		}
		records = doc.Recipes
	} else if root == "RECIPE" {
		var r xmlRecipe
		if err := xmlDecoder(data).Decode(&r); err != nil {
			return nil, err
		}
		records = []xmlRecipe{r}
	} else {
		return nil, errors.New("BeerXML root must be RECIPES or RECIPE")
	}
	result := make([]domain.Recipe, 0, len(records))
	for i, record := range records {
		r, err := fromXML(record)
		if err != nil {
			return nil, fmt.Errorf("BeerXML recipe %d: %w", i+1, err)
		}
		result = append(result, r)
	}
	return result, nil
}

func fromXML(x xmlRecipe) (domain.Recipe, error) {
	r := domain.Recipe{Name: x.Name, Style: x.Style.Name, Notes: x.Notes, Ingredients: []domain.Ingredient{}, Steps: []domain.RecipeStep{}}
	if x.Version != 1 {
		return r, errors.New("only BeerXML VERSION 1 is supported")
	}
	var err error
	if r.Type, err = enum(x.Type, "All Grain", "Partial Mash", "Extract"); err != nil {
		return r, err
	}
	if r.BatchLiters, err = number(x.Batch, "BATCH_SIZE"); err != nil {
		return r, err
	}
	if r.BoilLiters, err = number(x.BoilSize, "BOIL_SIZE"); err != nil {
		return r, err
	}
	if r.BoilMinutes, err = number(x.BoilTime, "BOIL_TIME"); err != nil {
		return r, err
	}
	if r.Efficiency, err = number(x.Efficiency, "EFFICIENCY"); err != nil {
		return r, err
	}
	for _, group := range []struct {
		kind  string
		items []xmlIngredient
	}{{"fermentable", x.Fermentables}, {"hop", x.Hops}, {"yeast", x.Yeasts}, {"misc", x.Miscs}} {
		for _, item := range group.items {
			in := domain.Ingredient{Name: item.Name, Kind: group.kind, Type: item.Type, Form: item.Form}
			if item.Version != 1 {
				return r, fmt.Errorf("%s %q requires VERSION 1", group.kind, item.Name)
			}
			amount, err := number(item.Amount, "AMOUNT")
			if err != nil {
				return r, err
			}
			switch group.kind {
			case "fermentable":
				if in.Type, err = enum(item.Type, "Grain", "Sugar", "Extract", "Dry Extract", "Adjunct"); err != nil {
					return r, err
				}
				in.Unit = "kg"
				in.Amount = amount
				in.Use = "mash"
				if in.Type == "Sugar" || in.Type == "Extract" || in.Type == "Dry Extract" {
					in.Use = "boil"
				}
				if in.Yield, err = number(item.Yield, "YIELD"); err != nil {
					return r, err
				}
				if in.Color, err = number(item.Color, "COLOR"); err != nil {
					return r, err
				}
			case "hop":
				in.Unit = "g"
				in.Amount = amount * 1000
				if in.Alpha, err = number(item.Alpha, "ALPHA"); err != nil {
					return r, err
				}
				use, err := enum(item.Use, "Boil", "Dry Hop", "Mash", "First Wort", "Aroma")
				if err != nil {
					return r, err
				}
				in.Use = strings.ReplaceAll(strings.ToLower(use), " ", "-")
				if in.Minutes, err = number(item.Time, "TIME"); err != nil {
					return r, err
				}
			case "yeast", "misc":
				if item.AmountIsWeight != "" {
					if _, err = enum(item.AmountIsWeight, "TRUE", "FALSE"); err != nil {
						return r, err
					}
				}
				in.Unit = "mL"
				if strings.EqualFold(item.AmountIsWeight, "TRUE") {
					in.Unit = "g"
				}
				in.Amount = amount * 1000
				if group.kind == "yeast" {
					if in.Type, err = enum(item.Type, "Ale", "Lager", "Wheat", "Wine", "Champagne"); err != nil {
						return r, err
					}
					if in.Form, err = enum(item.Form, "Liquid", "Dry", "Slant", "Culture"); err != nil {
						return r, err
					}
					in.Use = "fermentation"
				} else {
					if in.Type, err = enum(item.Type, "Spice", "Fining", "Water Agent", "Herb", "Flavor", "Other"); err != nil {
						return r, err
					}
					use, err := enum(item.Use, "Boil", "Mash", "Primary", "Secondary", "Bottling")
					if err != nil {
						return r, err
					}
					in.Use = strings.ToLower(use)
					if in.Minutes, err = number(item.Time, "TIME"); err != nil {
						return r, err
					}
				}
			}
			r.Ingredients = append(r.Ingredients, in)
		}
	}
	for _, water := range x.Waters {
		if water.Version != 1 {
			return r, errors.New("water requires VERSION 1")
		}
		amount, err := number(water.Amount, "water AMOUNT")
		if err != nil {
			return r, err
		}
		r.Ingredients = append(r.Ingredients, domain.Ingredient{Name: water.Name, Kind: "water", Amount: amount, Unit: "L", Use: "mash"})
		chemistry := map[string]float64{}
		for key, value := range map[string]*float64{"calcium": water.Calcium, "bicarbonate": water.Bicarbonate, "sulfate": water.Sulfate, "chloride": water.Chloride, "sodium": water.Sodium, "magnesium": water.Magnesium, "ph": water.PH} {
			if value != nil {
				chemistry[key] = *value
			}
		}
		if r.Water != nil {
			a, _ := json.Marshal(r.Water)
			b, _ := json.Marshal(chemistry)
			if !bytes.Equal(a, b) {
				return r, errors.New("multiple distinct water chemistry profiles cannot be represented by one recipe water profile")
			}
		}
		r.Water = chemistry
	}
	for _, step := range x.Mash.Steps {
		if step.Version != 1 {
			return r, errors.New("mash step requires VERSION 1")
		}
		typeName, err := enum(step.Type, "Infusion", "Temperature", "Decoction")
		if err != nil {
			return r, err
		}
		target, err := number(step.Temp, "STEP_TEMP")
		if err != nil {
			return r, err
		}
		minutes, err := number(step.Time, "STEP_TIME")
		if err != nil {
			return r, err
		}
		if math.Trunc(minutes) != minutes || minutes < 1 || minutes > 43200 {
			return r, errors.New("mash step times must be whole positive minutes; fractional durations are not rounded")
		}
		in := domain.RecipeStep{Name: step.Name, Kind: "mash", TargetC: target, Minutes: int(minutes), Type: typeName}
		if step.Infuse != nil {
			in.InfuseLiters = *step.Infuse
		}
		if step.Ramp != nil {
			in.RampMinutes = *step.Ramp
		}
		r.Steps = append(r.Steps, in)
	}
	if r.BoilMinutes > 0 {
		if math.Trunc(r.BoilMinutes) != r.BoilMinutes || r.BoilMinutes > 43200 {
			return r, errors.New("boil time must be whole minutes for the canonical step program")
		}
		r.Steps = append(r.Steps, domain.RecipeStep{Name: "Boil", Kind: "boil", TargetC: 100, Minutes: int(r.BoilMinutes)})
	}
	if x.HubSteps != "" {
		if err := uniqueJSON([]byte(x.HubSteps)); err != nil {
			return r, err
		}
		d := json.NewDecoder(strings.NewReader(x.HubSteps))
		d.DisallowUnknownFields()
		if err := d.Decode(&r.Steps); err != nil {
			return r, fmt.Errorf("HUB_STEPS: %w", err)
		}
	}
	for i, phase := range []struct{ age, temp *float64 }{{x.PrimaryAge, x.PrimaryTemp}, {x.SecondaryAge, x.SecondaryTemp}, {x.TertiaryAge, x.TertiaryTemp}} {
		if phase.age == nil && phase.temp == nil {
			continue
		}
		if phase.age == nil || phase.temp == nil {
			return r, errors.New("fermentation phase requires both duration and temperature")
		}
		r.Fermentation = append(r.Fermentation, domain.FermentationStep{Name: []string{"Primary", "Secondary", "Tertiary"}[i], Days: *phase.age, TargetC: *phase.temp})
	}
	if x.FermentationStages > 0 && x.FermentationStages != len(r.Fermentation) {
		return r, errors.New("FERMENTATION_STAGES does not match recorded phases")
	}
	if x.Carbonation != nil {
		r.Carbonation.Volumes = *x.Carbonation
	}
	if x.CarbonationTemp != nil {
		r.Carbonation.TemperatureC = *x.CarbonationTemp
	}
	if x.ForcedCarbonation != "" {
		forced, err := enum(x.ForcedCarbonation, "TRUE", "FALSE")
		if err != nil {
			return r, err
		}
		r.Carbonation.Method = "natural"
		if forced == "TRUE" {
			r.Carbonation.Method = "forced"
		}
	}
	return r, nil
}

func exportXML(r domain.Recipe) ([]byte, error) {
	typeName, err := enum(r.Type, "All Grain", "Partial Mash", "Extract")
	if err != nil {
		return nil, fmt.Errorf("recipe type is needed for BeerXML: %w", err)
	}
	if r.BoilLiters <= 0 || r.BoilMinutes <= 0 {
		return nil, errors.New("BeerXML export requires explicit positive boil_liters and boil_minutes; they are not guessed from finished batch size")
	}
	x := xmlRecipe{Name: r.Name, Version: 1, Type: typeName, Brewer: "PicoBrew Hub", Batch: pointer(r.BatchLiters), BoilSize: pointer(r.BoilLiters), BoilTime: pointer(r.BoilMinutes), Efficiency: pointer(r.Efficiency), Notes: r.Notes, Style: xmlStyle{Name: r.Style, Version: 1}, Mash: xmlMash{Name: "Mash", Version: 1}}
	for _, in := range r.Ingredients {
		item := xmlIngredient{Name: in.Name, Version: 1, Type: in.Type, Form: in.Form}
		switch in.Kind {
		case "fermentable":
			if item.Type, err = enum(in.Type, "Grain", "Sugar", "Extract", "Dry Extract", "Adjunct"); err != nil {
				return nil, fmt.Errorf("fermentable %q type: %w", in.Name, err)
			}
			item.Amount = pointer(in.Amount)
			item.Yield = pointer(in.Yield)
			item.Color = pointer(in.Color)
			x.Fermentables = append(x.Fermentables, item)
		case "hop":
			item.Amount = pointer(in.Amount / 1000)
			item.Alpha = pointer(in.Alpha)
			item.Time = pointer(in.Minutes)
			if item.Use, err = enum(strings.ReplaceAll(in.Use, "-", " "), "Boil", "Dry Hop", "Mash", "First Wort", "Aroma"); err != nil {
				return nil, err
			}
			x.Hops = append(x.Hops, item)
		case "yeast", "misc":
			if in.Unit == "count" {
				return nil, fmt.Errorf("ingredient %q uses count; BeerXML needs an explicit mass or volume", in.Name)
			}
			item.Amount = pointer(in.Amount / 1000)
			item.AmountIsWeight = "FALSE"
			if in.Unit == "g" {
				item.AmountIsWeight = "TRUE"
			}
			if in.Kind == "yeast" {
				if item.Type, err = enum(in.Type, "Ale", "Lager", "Wheat", "Wine", "Champagne"); err != nil {
					return nil, err
				}
				if item.Form, err = enum(in.Form, "Liquid", "Dry", "Slant", "Culture"); err != nil {
					return nil, err
				}
				x.Yeasts = append(x.Yeasts, item)
			} else {
				if item.Type, err = enum(in.Type, "Spice", "Fining", "Water Agent", "Herb", "Flavor", "Other"); err != nil {
					return nil, err
				}
				if item.Use, err = enum(in.Use, "Boil", "Mash", "Primary", "Secondary", "Bottling"); err != nil {
					return nil, err
				}
				item.Time = pointer(in.Minutes)
				x.Miscs = append(x.Miscs, item)
			}
		case "water":
			w := xmlWater{Name: in.Name, Version: 1, Amount: pointer(in.Amount)}
			for key, value := range r.Water {
				switch key {
				case "calcium":
					w.Calcium = pointer(value)
				case "bicarbonate":
					w.Bicarbonate = pointer(value)
				case "sulfate":
					w.Sulfate = pointer(value)
				case "chloride":
					w.Chloride = pointer(value)
				case "sodium":
					w.Sodium = pointer(value)
				case "magnesium":
					w.Magnesium = pointer(value)
				case "ph":
					w.PH = pointer(value)
				default:
					return nil, fmt.Errorf("unsupported water chemistry key %q for BeerXML", key)
				}
			}
			x.Waters = append(x.Waters, w)
		}
	}
	if len(r.Water) > 0 && len(x.Waters) == 0 {
		return nil, errors.New("BeerXML water chemistry requires an explicit water ingredient/volume")
	}
	for _, step := range r.Steps {
		if step.Kind != "mash" && step.Kind != "mash-out" {
			continue
		}
		typeName := step.Type
		if typeName == "" {
			typeName = "Temperature"
		}
		if typeName, err = enum(typeName, "Infusion", "Temperature", "Decoction"); err != nil {
			return nil, err
		}
		item := xmlMashStep{Name: step.Name, Version: 1, Type: typeName, Temp: pointer(step.TargetC), Time: pointer(float64(step.Minutes))}
		if step.InfuseLiters > 0 {
			item.Infuse = pointer(step.InfuseLiters)
		}
		if step.RampMinutes > 0 {
			item.Ramp = pointer(step.RampMinutes)
		}
		x.Mash.Steps = append(x.Mash.Steps, item)
	}
	steps, err := json.Marshal(r.Steps)
	if err != nil {
		return nil, err
	}
	x.HubSteps = string(steps)
	if len(r.Fermentation) > 3 {
		return nil, errors.New("BeerXML supports at most primary, secondary and tertiary phases")
	}
	for i, phase := range r.Fermentation {
		switch i {
		case 0:
			x.PrimaryAge = pointer(phase.Days)
			x.PrimaryTemp = pointer(phase.TargetC)
		case 1:
			x.SecondaryAge = pointer(phase.Days)
			x.SecondaryTemp = pointer(phase.TargetC)
		case 2:
			x.TertiaryAge = pointer(phase.Days)
			x.TertiaryTemp = pointer(phase.TargetC)
		}
	}
	x.FermentationStages = len(r.Fermentation)
	if r.Carbonation.Volumes > 0 {
		x.Carbonation = pointer(r.Carbonation.Volumes)
		x.CarbonationTemp = pointer(r.Carbonation.TemperatureC)
	}
	if r.Carbonation.Method != "" {
		method, err := enum(r.Carbonation.Method, "forced", "natural")
		if err != nil {
			return nil, err
		}
		x.ForcedCarbonation = "FALSE"
		if method == "forced" {
			x.ForcedCarbonation = "TRUE"
		}
	}
	data, err := xml.MarshalIndent(xmlRecipes{Recipes: []xmlRecipe{x}}, "", "  ")
	if err != nil {
		return nil, err
	}
	return append([]byte(xml.Header), data...), nil
}
