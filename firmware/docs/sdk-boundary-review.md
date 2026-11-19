# Native Energia and TI boundary review

The complete application uses ordinary recovered C for its logic and typed
C++ adapters for the pinned Energia/TI dependencies. The production boundary
includes [energia_cc3200.cpp](../platform/energia_cc3200.cpp),
[energia_network.cpp](../platform/energia_network.cpp),
[pico_http_client.cpp](../platform/pico_http_client.cpp), and
[firmware_main.cpp](../platform/firmware_main.cpp). The build compiles these
against the genuine SDK declarations and links the native startup and runtime.
It does not call code or reuse application globals at the original firmware's
absolute RAM addresses.

Current compiler commands, source hashes, exports, diagnostics, and runtime
providers are recorded under `build/firmware/native/`; image checks and source
hashes are in `build/firmware/firmware-report.json`. The consolidated
`analysis/validation-summary.json` separates original-versus-rebuilt ARM
comparisons, bounded-input regressions, and integration checks. The earlier
[SDK boundary audit](../analysis/sdk-boundary-audit.json) and
[bootstrap link report](../analysis/sdk-bootstrap-link-validation.json)
preserve intermediate ABI and installation evidence; their source and export
totals are historical, not totals for the completed application.

## Native objects and startup

[PicoEnergiaObjects](../include/picobrew/energia_cc3200.h) contains aligned
storage for two Still `IPAddress` objects, the `PicoHttpClient`, and the server
`IPAddress`. Each storage uses `sizeof(T)` and `alignas(T)` from the actual
target type and is placement-constructed with its real C++ constructor.
`IPAddress` is a native object with a virtual table, not merely four octets.
`PicoHttpClient` derives from the real `WiFiClient`; its socket behavior is
described below.

The bridge uses native member access and dispatch. Original object addresses,
guessed virtual-table offsets, and hand-invented SDK layouts are not ABI
requirements. The earlier compiler audit records concrete `IPAddress` and
base `WiFiClient` dimensions; the production build obtains its storage sizes
directly from the native types, including the derived client.

[firmware_main.cpp](../platform/firmware_main.cpp) owns static application,
control, network, initialization, and object-storage instances. During normal
C++ initialization, its binder calls the recovered application initializer,
constructs the SDK objects in the recovered application order, constructs the
recovered RFID reader, and installs the real service bindings. `setup()` and
`loop()` then call the complete recovered application entry points. The RFID
binding now calls `pico_rfid_construct`; it is not an unresolved callback.

Control binding installs state and mandatory UI callbacks without enabling
timers or writing GPIO. The recovered setup sequence performs peripheral
initialization. Default server selection comes from `PICO_SERVER_HOST` and
`PICO_SERVER_PORT`, while connection and local-server behavior stay in the
recovered network logic.

Object storage is constructed once and remains alive for the full program.
`object()` is used only after construction. Live objects must not be copied
as byte storage or constructed again in place. Control state and callback
contexts likewise have static lifetime because interrupt handlers retain
access to them.

### Destructor registration

The bridge calls the ARM EABI interface as
`__aeabi_atexit(object, destructor, &__dso_handle)`. Disassembly of a
compiler-generated global client constructor in the selected target build
confirmed that argument order and use of the actual DSO-handle address. The
selected runtime wrapper swaps the first two arguments and calls
`__cxa_atexit`.

`destroy_http` invokes the real `PicoHttpClient` destructor and its native
`WiFiClient` base destructor. In this SDK the destructor returns immediately
for an unused client; for an assigned socket it copies client state into
`WiFiClass::clients`. The adapter does not replace that operation with a
guessed socket close. The registration return value is retained in
`destructor_registration_result`; the production startup currently does not
consume it. Normal Energia execution does not exit.

[energia_runtime_compat.c](../platform/energia_runtime_compat.c) supplies an
empty `_fini` termination hook for the selected bare-metal newlib link.
`__libc_fini_array` runs fini-array finalizers before `_fini`. Destructors
registered through `__aeabi_atexit` use the separate atexit/exit-procedure
mechanism; `_fini` is not a substitute for their registration or execution.

## GPIO, timers, SPI, and interrupts

GPIO, ADC, delay, PRCM, interrupt, timer, and SPI operations call the actual
Energia/TI declarations. This target has 32-bit `unsigned int`, `unsigned long`,
and `uint32_t`; the wrapper conversions preserve their value range. GPIO
arguments are narrowed to Energia's `uint8_t` logical-pin type. Pin-table
matching and its limits are documented in [SDK build](sdk-build.md).

Timer and GPIO interrupt callbacks use `void (*)(void)`. Normal linking
supplies their ARM Thumb addresses. `IntMasterEnable()` reports whether
interrupts were disabled before the call; it is not a general operation-success
flag. The adapter's boolean conversion preserves that meaning.

All control UI callbacks are required by `pico_energia_bind_control`. The
production binder supplies pause, filter indication, display, and fatal-heater
handlers from the recovered application. A returning fatal-heater callback
is followed by an infinite loop so control cannot resume through that path.
Device and MFRC522 operations use their recovered ordinary-source drivers
with the native GPIO/SPI bridge. Link and controlled trace tests do not establish
interrupt timing or electrical behavior on the physical board.

## Pico-specific network behavior

### Native `PicoHttpClient`

[PicoHttpClient](../include/picobrew/pico_http_client.h) is a native subclass
of `WiFiClient` with no added data fields. It uses the real protected socket
index, `WiFiClass` socket arrays, and typed SimpleLink structures. Its custom
`connectPico` method preserves the original Pico connection paths rather than
assuming upstream `WiFiClient::connect` has identical behavior.

| Path | Recovered operation |
| --- | --- |
| Foreground connection | Perform the blocking connect, then pass the two 32-bit words `{2, 0}` with length 8 for both nonblocking and keepalive socket options. |
| Background connection | Set nonblocking with integer value 1 and length 4 before connecting. On `SL_EALREADY`, select for writability using the original `{10 seconds, 500 microseconds}` timeout. |
| Background completion | Preserve the original positive-select rule: a clear socket bit is accepted; a set bit requires a successful second connect. Set the final nonblocking and keepalive options with value 1 and length 4. |
| Connection failure | Close the newly allocated socket and return failure without assigning the client slot. |

The foreground option payload and background select rule were verified from
the original instructions. They are observable Pico differences from the
pinned upstream implementation, including the unusual clear-bit acceptance
rule. Native tests compare the production class's SimpleLink call traces with
the corresponding original instructions. They do not prove that every radio
firmware revision accepts these options identically.

The WiFi association bridge also preserves Pico's fast-reconnect policy value
2, where upstream `begin()` enables policy value 3. It uses the real
`SlSecParams_t`, connection/profile APIs, and SDK connection state. Scans,
DNS, MAC address, connection status, stream reads/writes, and disconnect use
the genuine SDK implementations. Diagnostics use typed ping and IP-configuration
structures rather than guessed memory layouts.

### SLFS and boot selection

The original Pico filesystem wrapper and the pinned SDK expose different
success conventions. `SLFS::open` and `SLFS::close` return `SL_FS_OK` (zero)
on success. The bridge explicitly compares against that constant before
returning the application-level boolean. Treating those native return values
as booleans directly would invert success and failure.

Writes return the actual byte count, and application persistence/update paths
check the required count and close result. Open, short-write, negative-write,
and close failures do not become successful saves or update activation. The
network recovery ledger distinguishes these explicit failure-path repairs
from valid-I/O behavior retained from the original.

Boot selection accesses `/sys/mcubootinfo.bin` through real `sl_FsOpen`,
`sl_FsRead` / `sl_FsWrite`, and `sl_FsClose` calls. It supplies initialized
token and handle storage, transfers exactly eight bytes, and requires both
the exact transfer count and a successful close. An initialized token is not
a claim to recover a secure-file token or bypass an inaccessible file.
Successful file calls and boot activation have only been exercised through
controlled boundaries, not on a device's flash.

### Hardware MD5

The adapter enables the DTHE clock, registers a real SHAMD5 interrupt handler,
enables the original interrupt mask, waits for the ready indication with the
recovered deadline, selects MD5, and calls `SHAMD5DataProcess`. It checks the
TI routine's boolean result. The ISR tests the real status bits, disables
serviced sources, and sets the ready flag for the original ready event.

The caller supplies the start timestamp so the deadline retains the original
scope across input preparation. This fixes the earlier bridge's incorrect
hash setup without replacing hashing with fabricated output. Native tests
exercise clock, interrupt, algorithm, data-processing, timeout, and failure
calls at the TI boundary.

The application uses this primitive for the MAC-derived UID and for the
recovered update checksum. The latter hashes the decimal representation of
a signed hexadecimal-nibble sum; it is not a digest of the firmware bytes
and is not a new authentication mechanism. Protocol details and deliberate
repairs are in [network recovery](network-recovery.md).

## Runtime and production layout

Application, adapter, SDK, and function-test compilation consistently use the
selected compiler's `--specs=nano.specs`. The native link retains newlib-nano
integer/text formatting, native `strtod`, libm, libgcc software arithmetic,
and libnosys unsupported-operation handlers. No float-printf extension is
requested; serial float diagnostics call the real SDK float formatter.

The production allocator comes from the same pinned compiler's full-newlib
allocation family. Native failure execution exposed an address-zero access in
the initially selected nano allocation path and invalid SDK heap increments.
The bounded adapter now validates signed growth/shrink requests, returns the
newlib failure sentinel and sets `ENOMEM`, while the genuine Energia `_sbrk`
retains sole ownership of the heap break. Provider checks prevent a mixture
of allocator metadata layouts or a generic libnosys heap implementation.
Forty-seven native allocation scenarios pass without substituted functions;
[SDK build](sdk-build.md) documents the defects, selection and exact limits.

The production linker reserves the 240 KiB application span
`0x20004000..0x20040000` (end exclusive), a 32 KiB heap, and the native SDK's
32 KiB BSS stack. It aligns the initial stack to eight bytes and keeps ELF
headers outside `PT_LOAD`. Detailed layout evidence and runtime-provider
contracts are in [SDK build](sdk-build.md). The original vendor linker script
and original internal symbol addresses have not been reproduced verbatim.

The native builder includes the application archives with `--whole-archive`
and also roots every public definition with `--undefined`. This prevents
section garbage collection from silently dropping unreferenced exports.
It rejects undefined or missing rooted symbols. The production-image inspector
additionally verifies the vector, reset entry, RAM bounds, heap, stack,
loadable extent, and absence of test fixtures. Current totals and memory
headroom are taken from the generated firmware report.

## Verification scope

[test_network.py](../tests/test_network.py) runs the recovered application
against controlled platform operations and compares applicable paths with
the original ARM instructions. [test_native_network.py](../tests/test_native_network.py)
executes the actual production ELF's client, SLFS, boot-file, and hash bridge
with controlled SimpleLink/TI boundary calls. Update-package tests also feed
the built image's encoded payload through the recovered receiver. These
checks exercise the real application and native adapter implementations;
test fixtures are not linked into the delivered production image.

`analysis/network-tests.json`, `analysis/native-network-tests.json`,
`analysis/validation-summary.json`, and `build/firmware/firmware-report.json`
record the current results and hashes after verification. The completion audit
accounts for indexed original application entries; it does not establish
every input, error path, or concurrent schedule. The source audit checks that
production code contains no original application address calls or unresolved
decompiler constructs.

The remaining acceptance work is on hardware: radio and serial-flash behavior,
actual pin routing and peripheral timing, interrupt concurrency, memory peaks,
thermal operation, a full brew, and installation/recovery. A complete native
link and controlled ARM tests support a modifiable firmware candidate; they
do not constitute physical brewer validation. The selected SDK is a documented
compatible dependency, with Pico differences recovered explicitly, rather than
a claim that the vendor's complete original SDK source has been recovered.
