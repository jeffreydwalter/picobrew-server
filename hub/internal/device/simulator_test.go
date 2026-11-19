package device

import (
	"context"
	"encoding/hex"
	"fmt"
	"reflect"
	"sync"
	"testing"
	"time"

	"github.com/jeffreydwalter/picobrew-hub/internal/domain"
)

func TestSimulatorFixturesAreIndependentAndExplicit(t *testing.T) {
	ctx := context.Background()
	pro, idle, z := NewSimulator("demo-pro", "pico-pro"), NewSimulator("demo-s", "pico-s"), NewSimulator("demo-z", "z")
	a, _ := pro.Status(ctx)
	b, _ := idle.Status(ctx)
	c, _ := z.Status(ctx)
	if !a.Busy || b.Busy || !c.Busy || a.SessionID == c.SessionID || a.DeviceUID == c.DeviceUID {
		t.Fatalf("fixtures are not distinct: %+v %+v %+v", a, b, c)
	}
	capabilities, _ := z.Discover(ctx)
	if capabilities.FirmwareVersion != "simulator" || capabilities.HistoryPersistent || capabilities.TelemetryForwarding || capabilities.DedicatedPauseResume {
		t.Fatalf("simulator claims physical or unimplemented capabilities: %+v", capabilities)
	}
	if err := pro.Command(ctx, domain.Command{Command: "stop"}); err != nil {
		t.Fatal(err)
	}
	after, _ := z.Status(ctx)
	if !after.Busy || after.Stopped {
		t.Fatal("stopping one machine changed another")
	}
	if err := idle.UpdateSettings(ctx, map[string]any{"temperature_unit": "C"}); err != nil {
		t.Fatal(err)
	}
	prefs, _ := pro.Settings(ctx)
	if prefs["temperature_unit"] != "F" {
		t.Fatal("settings leaked across simulators")
	}
}

func TestSimulatorBrewRequiresPromptThenRecordsAndCompletes(t *testing.T) {
	ctx := context.Background()
	s := NewSimulator("test", "pico-pro").(*simulator)
	recipe := nativeRecipe()
	recipe.Steps[0].Minutes = 1
	recipe.Steps[0].DrainMinutes = 0
	if err := s.PutRecipe(ctx, recipe); err != nil {
		t.Fatal(err)
	}
	if err := s.Command(ctx, domain.Command{Command: "brew", RecipeID: recipe.ID}); err != nil {
		t.Fatal(err)
	}
	status, _ := s.Status(ctx)
	history, _ := s.History(ctx)
	if !status.Busy || status.StepName != "Press dial to begin" || *status.TargetF != 0 || history.Available {
		t.Fatalf("brew bypassed its prompt: %+v %+v", status, history)
	}
	if err := s.Command(ctx, domain.Command{Command: "encoder", Click: true}); err != nil {
		t.Fatal(err)
	}
	s.mu.Lock()
	s.active.started = time.Now().Add(-2 * time.Second)
	s.mu.Unlock()
	status, _ = s.Status(ctx)
	history, _ = s.History(ctx)
	if status.Busy || status.Command != "idle" || history.State != "completed" || history.DurationMS != 60000 || history.RecordCount != 5 || history.PersistedRecordCount != 0 {
		t.Fatalf("completed recording incorrect: %+v %+v", status, history)
	}
	rows, next, err := s.HistoryPage(ctx, history.ID, 0)
	if err != nil || next != nil || len(rows) != 5 || rows[4].ElapsedMS != 60000 || rows[0].Source != "simulation" {
		t.Fatalf("recording page: %+v %v %v", rows, next, err)
	}
	*rows[0].WortF = 999
	again, _, _ := s.HistoryPage(ctx, history.ID, 0)
	if *again[0].WortF == 999 {
		t.Fatal("history result aliases simulator memory")
	}
}

func TestSimulatorStopLatchesAndPreservesInterruptedHistory(t *testing.T) {
	ctx := context.Background()
	s := NewSimulator("demo-pro", "pico-pro")
	if err := s.Command(ctx, domain.Command{Command: "clear-stop"}); deviceFailure(t, err).StatusCode != 409 {
		t.Fatal(err)
	}
	if err := s.Command(ctx, domain.Command{Command: "stop"}); err != nil {
		t.Fatal(err)
	}
	status, _ := s.Status(ctx)
	history, _ := s.History(ctx)
	if !status.Stopped || status.Busy || *status.TargetF != 0 || history.State != "interrupted" || history.RecordCount == 0 {
		t.Fatalf("stop did not latch: %+v %+v", status, history)
	}
	if err := s.Command(ctx, domain.Command{Command: "rinse"}); deviceFailure(t, err).Code != "stopped" {
		t.Fatal(err)
	}
	if err := s.Command(ctx, domain.Command{Command: "clear-stop"}); err != nil {
		t.Fatal(err)
	}
	status, _ = s.Status(ctx)
	if status.Stopped || status.Busy || status.Command != "idle" {
		t.Fatal("clearing stop resumed the previous brew")
	}
}

func TestSimulatorRecipeCapacityDeletionAndOwnership(t *testing.T) {
	ctx := context.Background()
	s := NewSimulator("test", "pico-pro")
	for i := range 14 {
		recipe := nativeRecipe()
		recipe.ID = fmt.Sprintf("r%d", i)
		if err := s.PutRecipe(ctx, recipe); err != nil {
			t.Fatal(err)
		}
		recipe.Steps[0].TargetF = 999
	}
	if err := s.PutRecipe(ctx, nativeRecipe()); deviceFailure(t, err).StatusCode != 507 {
		t.Fatal(err)
	}
	if err := s.DeleteRecipe(ctx, "r0"); err != nil {
		t.Fatal(err)
	}
	if err := s.PutRecipe(ctx, nativeRecipe()); err != nil {
		t.Fatal(err)
	}
	recipes, _ := s.Recipes(ctx)
	if len(recipes) != 16 {
		t.Fatal("recipe capacity did not recover after deletion")
	}
	for _, recipe := range recipes {
		if recipe.Steps[0].TargetF == 999 {
			t.Fatal("input slice aliases stored recipe")
		}
	}
	recipes[0].Steps[0].Name = "Changed by caller"
	again, _ := s.Recipes(ctx)
	if reflect.DeepEqual(again, recipes) {
		t.Fatal("result slice aliases stored recipe")
	}
	if err := s.Command(ctx, domain.Command{Command: "rinse"}); err != nil {
		t.Fatal(err)
	}
	if err := s.DeleteRecipe(ctx, "r1"); deviceFailure(t, err).StatusCode != 409 {
		t.Fatal("busy simulator allowed recipe removal")
	}
}

func TestSimulatorSharedHistoryCapacityEvictsOldBrews(t *testing.T) {
	s := NewSimulator("test", "pico-pro").(*simulator)
	recipe := nativeRecipe()
	recipe.Steps[0].Minutes = 599
	recipe.Steps[0].DrainMinutes = 0
	firstStart := time.Now().Add(-700 * time.Second)
	s.mu.Lock()
	s.begin(recipe, "brew", firstStart)
	s.advance(time.Now())
	first := s.brews[0].meta.ID
	s.begin(recipe, "brew", firstStart)
	s.advance(time.Now())
	total := 0
	for _, brew := range s.brews {
		total += len(brew.samples)
	}
	retained := len(s.brews)
	s.mu.Unlock()
	if total > 4224 || retained != 1 {
		t.Fatalf("shared capacity not enforced: records=%d brews=%d", total, retained)
	}
	_, _, err := s.HistoryPage(context.Background(), first, 0)
	if deviceFailure(t, err).Code != "history_changed" {
		t.Fatal(err)
	}
}

func TestSimulatorDisplayProgramsAndNoOutboundTelemetry(t *testing.T) {
	ctx := context.Background()
	s := NewSimulator("test", "z").(*simulator)
	display, err := s.Display(ctx)
	data, decodeErr := hex.DecodeString(display.Data)
	if err != nil || decodeErr != nil || len(data) != 1024 || display.Format != "page-lsb-hex" || display.Width != 128 || display.Height != 64 {
		t.Fatalf("invalid framebuffer: %+v %v", display, err)
	}
	if err := s.UpdateTelemetry(ctx, map[string]any{"enabled": true, "host": "brewer.local", "port": 8080}); deviceFailure(t, err).Code != "simulator_only" {
		t.Fatal("simulator enabled outbound network telemetry")
	}
	if err := s.Command(ctx, domain.Command{Command: "rinse"}); err != nil {
		t.Fatal(err)
	}
	if err := s.Command(ctx, domain.Command{Command: "encoder", Click: true}); err != nil {
		t.Fatal(err)
	}
	s.mu.Lock()
	s.programStarted = time.Now().Add(-20 * time.Second)
	s.mu.Unlock()
	status, _ := s.Status(ctx)
	history, _ := s.History(ctx)
	if status.Busy || status.Command != "idle" || history.Available {
		t.Fatal("maintenance did not finish independently of brew history")
	}
}

func TestSimulatorConcurrentReadsAndControls(t *testing.T) {
	s := NewSimulator("demo-pro", "pico-pro")
	ctx := context.Background()
	var wg sync.WaitGroup
	for range 20 {
		wg.Add(1)
		go func() {
			defer wg.Done()
			_, _ = s.Status(ctx)
			_, _ = s.Display(ctx)
			_, _ = s.Settings(ctx)
			_, _ = s.Telemetry(ctx)
			_, _ = s.Recipes(ctx)
			history, _ := s.History(ctx)
			if history.Available {
				_, _, _ = s.HistoryPage(ctx, history.ID, 0)
			}
			_ = s.Command(ctx, domain.Command{Command: "encoder", Delta: 1})
		}()
	}
	wg.Wait()
	cancelled, cancel := context.WithCancel(ctx)
	cancel()
	if err := s.Command(cancelled, domain.Command{Command: "stop"}); deviceFailure(t, err).Code != "cancelled" {
		t.Fatal(err)
	}
	status, _ := s.Status(ctx)
	if status.Stopped {
		t.Fatal("cancelled simulator command mutated state")
	}
}
