# Auxiliary controller, RFID and application entry

## Device protocol

`src/device.c` implements the original shared-SPI selection sequences, stepper
ready polling, capability detection, torque command, compartment echo/dwell
handshake, home move, buzzer toggling, filter wait, service button and thermal
startup check. It uses owned application state and typed hardware/UI callbacks.
Actual Energia calls are installed by `src/application.c` and
`platform/firmware_main.cpp`.

The auxiliary controller uses mode 1, select pin 18 and shared select pin 4.
Command transactions disable interrupts, select the device, wait 1 ms, write the
command, wait 1 ms, clock a 0xff reply byte, wait 1 ms, release both selects, restore
mode 0 and unconditionally enable interrupts. The separate selected-byte path
uses a 100 ms delay. These observed timing/order choices are retained.

Ready command 0x37 accepts 0x0c; 0x58 reports error 18. Waiting permits 151 probes
and 150 intervening 300 ms delays. Capability command 0x29 controls legacy
compartment mapping. The native initialized home byte is 6; a legacy capability
response sets it to 0. The detection routine does not clear an already set
legacy flag when a later response supports the newer mapping.

A successful move sends 0x21, checks the mapped compartment echo, reads a dwell
count below 7, waits dwell × 500 ms and then waits for ready. Buzzer operations
query/toggle command 11 and preserve the returned state rather than assuming
the requested state was reached. All return/error and observable I/O ordering
is represented in the test cases.

`tests/test_device.py` compares 170 scenarios against original ARM. Previously
verified filter/actuator/controller-stop routines are explicit service
boundaries in that suite. Wire observations normalize the SPI byte width;
upper caller-register bits outside that byte are not an electrical output.

## RFID

`src/rfid.c` reconstructs Pico's MFRC522 implementation, rather than importing
a generic reader library with different status values or collision behavior.
It includes register/FIFO operations, hardware CRC, requests, three cascade
levels, select/anticollision, power, halt, reader probing, UID conversion,
encoder cancellation and the PicoPak filter/timer workflow.

The low-level suite executes the complete original and rebuilt reader code
through a deterministic SPI/register/FIFO model. Its 397 cases cover 4-,7- and
10-byte UIDs, CRC and IRQ timeouts, receive capacity, alignment, errors,
collisions, valid-bit handling and known-UID-bit inputs. State, meaningful
returns and ordered I/O events are compared.

An additional 61 application scenarios exercise UID/PicoPak reads through that
actual reader implementation: 41 original/rebuilt cases and 20 capacity repairs.
Original call sites supply initially empty UID text. The reconstruction
rejects insufficient capacity before writing the UID, using canary tests to
confirm it does not reproduce original overruns. A ten-byte UID requires 21
bytes including the terminator.

Instruction inspection resolved a misleading decompilation: the new-card
routine explicitly writes its answer length as 2. It is not uninitialized.
Conversely, the original collision branch really does select a particular bit
instead of interpreting all collision-position bits like common libraries;
that observed quirk is retained and tested.

## Complete entry and composition

`src/application.c` supplies real implementations for every required UI,
recipe, network, device, RFID and phase callback. Its setup follows the original
serial/display/output/input initialization, preferences, timers, WiFi/service
mode, registration, state reporting, stepper/RFID, restart decision, update and
maintenance sequence. Its main loop dispatches the six recovered menu actions.

`tests/test_application.py` compares the setup/menu orchestration at domain
entry boundaries, checks every application service table is populated, executes
the real network-to-framebuffer context binding, and confirms choosing Back in
the language menu preserves the current language. Its 37 checks include 35
original/rebuilt traces and two direct binding regressions. Domain workflows
are tested independently; these are not complete physical brews.

The constructor owns native SDK objects for the full application lifetime,
binds services before enabling timers, and provides the correct page order:
brew name, current step, step progress, temperature, recipe metrics. The
service-button routine retains the original reversed time subtraction.
