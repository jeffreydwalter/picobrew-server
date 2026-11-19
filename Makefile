GO ?= go
NPM ?= npm
PYTHON ?= python3

.PHONY: help hub hub-test hub-run hub-demo firmware firmware-test firmware-usb-test firmware-bootstrap
.DEFAULT_GOAL := help

help:
	@printf '%s\n' 'make hub / hub-test / hub-run / hub-demo' 'make firmware / firmware-test / firmware-usb-test / firmware-bootstrap' 'Docker: docker compose -f hub/compose.yaml up --build -d'

hub:
	$(MAKE) -C hub build GO="$(GO)" NPM="$(NPM)"

hub-test:
	$(MAKE) -C hub test GO="$(GO)" NPM="$(NPM)"

hub-run:
	$(MAKE) -C hub run GO="$(GO)"

hub-demo:
	$(MAKE) -C hub demo GO="$(GO)"

firmware:
	$(MAKE) -C firmware standalone PYTHON="$(PYTHON)"

firmware-test:
	$(MAKE) -C firmware standalone-test PYTHON="$(PYTHON)"

firmware-usb-test:
	$(MAKE) -C firmware usb-test PYTHON="$(PYTHON)"

firmware-bootstrap:
	$(MAKE) -C firmware bootstrap PYTHON="$(PYTHON)"
