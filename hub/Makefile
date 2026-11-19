GO ?= go
NPM ?= npm
.PHONY: build test run demo web
build: web
	$(GO) build -trimpath -o picobrew-hub ./cmd/picobrew-hub
web:
	$(NPM) --prefix web ci
	$(NPM) --prefix web run build
test:
	$(GO) test -race ./...
	$(NPM) --prefix web test
run:
	$(GO) run ./cmd/picobrew-hub
demo:
	$(GO) run ./cmd/picobrew-hub --demo --data demo-data
