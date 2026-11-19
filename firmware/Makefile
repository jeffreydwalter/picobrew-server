PYTHON ?= python3

.PHONY: all bootstrap build test firmware verify edit-check sdk-check pin-check clean standalone standalone-test standalone-arm-test standalone-storage-test standalone-verify web-test usb-test
.NOTPARALLEL: verify
all: standalone

standalone:
	$(PYTHON) tools/build_firmware.py --standalone

standalone-test:
	$(PYTHON) tools/generate_starter_recipes.py --check
	$(PYTHON) tools/generate_api_explorer.py --check
	$(PYTHON) tools/embed_web.py --check
	$(PYTHON) tests/test_http_server.py
	$(PYTHON) tests/test_energia_http_server.py
	$(PYTHON) tests/test_energia_wifi.py
	$(PYTHON) tests/test_local_api.py
	$(PYTHON) tests/test_local_history.py
	$(PYTHON) tests/test_local_telemetry.py
	$(PYTHON) tests/test_telemetry_receiver.py
	$(PYTHON) tests/test_standalone_control.py

standalone-arm-test:
	$(PYTHON) tests/test_standalone_control_arm.py

standalone-storage-test: standalone
	$(PYTHON) tests/test_native_storage.py --elf build/standalone/firmware/pico-s-pro-standalone.elf

standalone-verify: standalone standalone-test standalone-arm-test standalone-storage-test

web-test:
	$(PYTHON) tests/test_web.py
	$(PYTHON) tests/test_web_features.py

usb-test:
	$(PYTHON) -m unittest discover -s tests -p 'test_pico_*.py'

bootstrap:
	$(PYTHON) tools/bootstrap_dependencies.py

build:
	$(PYTHON) tools/build.py

test: build
	$(PYTHON) tests/test_sensors.py --elf build/application-validation.elf
	$(PYTHON) tests/test_actuators.py --elf build/application-validation.elf
	$(PYTHON) tests/test_state_pid.py --elf build/application-validation.elf --output analysis/state-pid-tests.json
	$(PYTHON) tests/test_control.py --elf build/application-validation.elf --output analysis/control-tests.json
	$(PYTHON) tests/test_phase.py --elf build/application-validation.elf --output analysis/phase-tests.json
	$(PYTHON) tests/test_device.py --elf build/application-validation.elf
	$(PYTHON) tests/test_rfid.py --elf build/application-validation.elf
	$(PYTHON) tests/test_rfid_application.py --elf build/application-validation.elf
	$(PYTHON) tests/test_application.py --elf build/application-validation.elf
	$(PYTHON) tests/test_network.py --elf build/application-validation.elf --output analysis/network-tests.json
	$(PYTHON) tests/test_network_direct.py --elf build/application-validation.elf --output analysis/network-direct-tests.json
	$(PYTHON) tests/test_network_direct_still.py --elf build/application-validation.elf --output analysis/network-direct-still-tests.json
	$(PYTHON) tests/test_recipe.py --elf build/application-validation.elf --output analysis/recipe-tests.json
	$(PYTHON) tests/test_ui.py --elf build/application-validation.elf --output analysis/ui-tests.json
	$(PYTHON) tests/test_ui_interactions.py --elf build/application-validation.elf --output analysis/ui-interactions-tests.json
	$(PYTHON) tests/test_ui_workflows.py --elf build/application-validation.elf --output analysis/ui-workflows-tests.json
	$(PYTHON) tests/test_ui_direct_display.py --elf build/application-validation.elf --output analysis/ui-direct-display-tests.json
	$(PYTHON) tests/test_ui_direct_menus.py --elf build/application-validation.elf --output analysis/ui-direct-menus-tests.json
	$(PYTHON) tests/test_ui_direct_workflows.py --elf build/application-validation.elf --output analysis/ui-direct-workflows-tests.json
	$(PYTHON) tests/test_ui_assets.py --elf build/application-validation.elf --output analysis/ui-assets-tests.json

firmware: build
	$(PYTHON) tools/build_firmware.py --skip-archive-build
	$(PYTHON) tests/test_native_network.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-network-tests.json
	$(PYTHON) tests/test_native_network_direct.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-network-direct-tests.json
	$(PYTHON) tests/test_native_startup.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-startup-tests.json
	$(PYTHON) tests/test_native_allocator.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-allocator-tests.json
	$(PYTHON) tests/test_native_serial.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-serial-tests.json
	$(PYTHON) tests/test_update_package.py

verify: test firmware pin-check edit-check
	$(PYTHON) tools/audit_completion.py
	$(PYTHON) tools/summarize_validation.py

edit-check: build
	$(PYTHON) tests/test_edit_rebuild.py --elf build/application-validation.elf

sdk-check: firmware

pin-check:
	$(PYTHON) tools/verify_energia_pin_tables.py --sdk "$${PICOBREW_SDK:-.deps/energia}" --image reference/original-0.1.34.bin --output analysis/board-pin-validation.json

clean:
	rm -rf build
