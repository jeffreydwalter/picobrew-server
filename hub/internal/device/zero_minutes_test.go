package device

import (
	"context"
	"net/http"
	"sync/atomic"
	"testing"
)

func TestZeroMinuteSupportIsFreshlyDiscoveredBeforeWriting(t *testing.T) {
	program := nativeRecipe()
	program.Steps[0].Minutes = 0
	var supported atomic.Bool
	var writes, discoveries atomic.Int32
	p := testPico(t, func(w http.ResponseWriter, r *http.Request) {
		switch {
		case r.URL.Path == "/api/v1":
			discoveries.Add(1)
			replyJSON(w, 200, map[string]any{"api_version": 1, "mode": "standalone", "zero_minute_steps": supported.Load(), "recipe_capacity": 16, "step_capacity": 15})
		case r.Method == "PUT":
			writes.Add(1)
			replyJSON(w, 200, program)
		default:
			replyJSON(w, 200, program)
		}
	})
	err := deviceFailure(t, p.PutRecipe(context.Background(), program))
	if writes.Load() != 0 || err.UnknownOutcome || err.Code != "unsupported_zero_minute_steps" {
		t.Fatalf("old firmware received zero-minute program: %+v", err)
	}
	supported.Store(true)
	if err := p.PutRecipe(context.Background(), program); err != nil {
		t.Fatal(err)
	}
	supported.Store(false) // Firmware can be downgraded while this Hub stays up.
	err = deviceFailure(t, p.PutRecipe(context.Background(), program))
	if writes.Load() != 1 || discoveries.Load() != 3 || err.UnknownOutcome {
		t.Fatal("cached capability permitted a write after downgrade")
	}
}
