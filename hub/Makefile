GO ?= go
NPM ?= npm
PYTHON ?= python3
.PHONY: build test run demo web
build: web
	$(GO) build -trimpath -o picobrew-hub ./cmd/picobrew-hub
web:
	$(NPM) --prefix web ci
	$(NPM) --prefix web run build
test:
	$(GO) test -race ./...
	$(PYTHON) -m unittest discover -s tools -p 'test_*.py'
	$(NPM) --prefix web test
run:
	$(GO) run ./cmd/picobrew-hub
demo:
	$(GO) run ./cmd/picobrew-hub --demo --data demo-data
