# CC3200 dependencies and complete firmware build

## What builds

The project builds the recovered Pico Pro application with the pinned Energia
CC3200 core, TI driver libraries, a real ARM compiler, and the compiler's
newlib-nano stdio, full-newlib allocation family, libm, libgcc, and libnosys.
The production entry follows the native
`ResetISR` → C++ initialization → Energia `main()` → `setup()` / `loop()` path.
The application composition includes the device, control, RFID, display, input,
recipe, session, network, persistence, PicoStill, and firmware-update services.
See [the production entry](../platform/firmware_main.cpp) and
[application composition](../src/application.c).

The resulting ELF and binary are complete linked firmware candidates. Software
recovery, native SDK integration, and image-layout checks are implemented;
physical execution on a brewer has not been performed. The build does not
establish radio or flash interoperability, electrical timing, full-brew behavior,
or successful installation and recovery on a device.

From the project root, using a Python environment with the packages in
[requirements-test.txt](../requirements-test.txt):

```sh
python3 tools/bootstrap_dependencies.py
make firmware
```

`make firmware` builds the application archives, the native production image,
and its update envelope, then runs native network, constructor, allocation,
serial-formatting and update-package checks. `make verify` additionally runs
the application comparison suites, pin
matching, edit/rebuild verification, completion accounting, and the consolidated
validation report. The [Makefile](../Makefile) is the authoritative target list.

The default dependency locations are `.deps/energia` and `.deps/toolchain`.
`PICOBREW_SDK` and `PICOBREW_ARM_PREFIX` select another SDK directory and compiler
prefix. The prefix includes `arm-none-eabi-`, not just its containing directory.
The build scripts also expose `--sdk-root` and `--arm-prefix`.

### Artifacts and evidence

| File | Purpose |
| --- | --- |
| `build/firmware/pico-pro-recovered.elf` | Complete native executable, including symbols and debug information |
| `build/firmware/pico-pro-recovered.bin` | Raw image beginning with the vector table at `0x20004000` |
| `build/firmware/pico-pro-recovered.map` | Link map, selected providers, and memory allocations |
| `build/firmware/pico-pro-recovered.update.txt` | The raw image encoded in the recovered update protocol |
| `build/firmware/firmware-report.json` | Source and artifact hashes, required entry symbols, image bounds, stack, heap, and remaining RAM |
| `build/firmware/native/validation.json` | Compiler identity, retained application exports, SDK source and warning totals, and link checks |
| `build/firmware/native/compile_report.json` | Actual source hashes, compile commands, and diagnostics |
| `build/firmware/native/link_report.json` | Native link command and diagnostics |
| `build/firmware/native/runtime_boundary.json` | Selected runtime providers, support-library hash, and heap-provider check |
| `build/firmware/native/allocator_providers.json` | Pinned compiler allocation archive and extracted object hashes |
| `analysis/native-allocator-tests.json` | Actual production allocator execution, heap bounds, failure behavior and errno |
| `analysis/validation-summary.json` | Consolidated current test results and their scope |
| `analysis/completion-audit.json` | Per-original-entry source accounting and unmapped-entry check |

Regenerate these reports after changing source. Counts, image sizes, hashes,
and spare RAM belong in the generated reports rather than fixed documentation
totals. Source accounting includes explicitly identified inlined helpers and
dependency substitutions; it is not exhaustive execution-path coverage.

[build_firmware.py](../tools/build_firmware.py) supplies the production entry
and Pico linker to the native SDK builder. The underlying
[build_sdk_validation.py](../tools/build_sdk_validation.py) also supports a
standalone SDK smoke test using `tests/sdk_validation.cpp` by default. A smoke
test built with that default entry is a different artifact from the production
image. The separate `build/application-validation.elf` contains controlled test
fixtures and is never an input to the production link.

## Dependency provenance

The source dependency is Energia `cc3200-core` commit
`c5507401fe46454d38c3ce3af0a140df8fc088a3`. Its `platform.txt` identifies core
version 1.0.3 and GNU ARM 4.8.4-20140725. Driverlib byte matches and selected
runtime matches support this dependency choice; they do not prove that every
routine in the revision matches Pico's original SDK.

The selected rebuild compiler is Arm's GNU Arm Embedded Toolchain
10.3-2021.10 for Linux x86-64. This is a compatible build baseline. The exact
original compiler environment has not been recovered. The historical reasons
for selecting this runnable compiler, the archive URLs, roots, byte counts,
SHA-256 hashes, and target flags are recorded in
[dependencies/lock.json](../dependencies/lock.json). Dependency licenses and
notices remain intact in the archives and extracted trees.

### Project-local bootstrap

[bootstrap_dependencies.py](../tools/bootstrap_dependencies.py) checks archive
size and SHA-256 before extraction. It uses Python's `tarfile.data_filter`, so
Python 3.12 or a release with that safety filter backported is required. It
needs no root privileges or system-wide compiler installation. Use `--sdk-only`
when supplying a separately installed compiler.

The script refuses to overwrite an untracked dependency directory, rejects a
cached archive with the wrong hash, and rejects unexpected patch input. Fresh
source installation, reuse, full compiler extraction, and a subsequent native
compile/link were exercised during recovery. The
[bootstrap validation report](../analysis/bootstrap-full-validation.json)
records that historical installation check; the production reports above
record the current application build.

### Compatibility adaptations

The selected newlib headers declare `utoa` and `srandom` with `unsigned int`,
while the pinned core uses `unsigned long`. Both types are 32 bits on this ARM
ABI, but their declarations still conflict in C/C++. Four hash-guarded edits
in `itoa.h`, `itoa.c`, `WMath.cpp`, and `random.c` align the declarations and
definitions with the selected headers. Algorithm bodies are unchanged. The
lock records each original and patched file hash.

[energia_runtime_compat.c](../platform/energia_runtime_compat.c) supplies the
empty `_fini` final termination hook required by this newlib link. Energia's
normal `main()` never returns. Fini-array processing and registered C++
destructors remain native runtime operations; their distinct contracts are
described in the [SDK boundary review](sdk-boundary-review.md).

Target flags include `-mcpu=cortex-m4`, `-mthumb`, `-mfloat-abi=soft`,
`-mabi=aapcs`, and `--specs=nano.specs`. C uses GNU C11; C++ uses GNU C++11
with exceptions, RTTI, and thread-safe static initialization disabled. SDK
compilation uses `-fcommon` to retain the old core's tentative-definition
model. The selected multilib is `thumb/v7e-m/nofp`.

Upstream warnings are preserved in the compile report. They include legacy
initializers, signedness issues, and suspect return paths; a successful link
does not repair those warnings or validate every unused SDK function.

## Pico memory layout

The production image uses [platform/pico_cc3200.ld](../platform/pico_cc3200.ld).
This is an explicit reconstructed layout for the observed 256 KiB CC3200 RAM
configuration, retaining the native SDK startup and allocator symbols.

| Allocation or constraint | Production setting |
| --- | --- |
| Application RAM origin | `0x20004000`, after the reserved first 16 KiB |
| Application RAM end, exclusive | `0x20040000` |
| Available application span | `0x3c000`, or 240 KiB |
| Vector table | First loadable bytes, at the application origin |
| Native SDK stack | 32 KiB, in the SDK's `pui32Stack` BSS object |
| Heap | 32 KiB, bounded by `_heap` and `_eheap` |
| Stack and heap alignment | 8 bytes |
| BSS and heap | `NOLOAD`; BSS is initialized by native startup |
| Loadable program header | Explicit `PT_LOAD`, without ELF or program headers included |

The stack is already included in the BSS allocation and must not be counted
twice when reading the report. The SDK declares it as a `uint32_t` array whose
natural alignment is four bytes. The linker places that input section first
at an eight-byte boundary so the initial stack pointer satisfies AAPCS at the
reset call boundary. Constructor/destructor arrays and the SDK's `_text`,
`_data`, `_bss`, and heap symbols remain available to native startup.

The explicit `PHDRS` declaration omits `FILEHDR` and `PHDRS` from the loadable
segment. ELF metadata therefore does not extend `PT_LOAD` below `0x20004000`.
The raw binary begins at the vector table, and its extent must match the
file-backed portion of the ELF's loadable data.

[Image inspection](../tools/build_firmware.py) rejects out-of-range load
segments, unexpected load/run relocation, an incorrect vector location or
reset address, a misaligned or misplaced initial stack, an incorrect heap
reservation, undefined symbols, and test-fixture symbols. Linker assertions
also enforce the origin, heap alignment, and physical RAM limit. These checks
establish layout consistency; they do not measure actual stack or heap peaks
during a brew.

### Evidence behind the layout

The original image contains these values:

| Item | Original observed value |
| --- | --- |
| Application load address | `0x20004000` |
| Initial stack pointer | `0x200365b8` |
| BSS end / heap start | `0x20037804` |
| Heap upper limit used by `_sbrk` | `0x2003f808` |

The pinned upstream LaunchPad script starts at the same address but declares
only `0x30000` bytes of SRAM and a 64 KiB heap. Its range ends at `0x20034000`,
below the original stack, BSS, and heap. The original heap span supports a
32 KiB reservation plus alignment. The production linker uses the full
240 KiB application span and a 32 KiB heap, with the real 32 KiB SDK stack.
Original internal symbol addresses are not imposed on the rebuilt code.

## Compact stdio and the bare-metal runtime

The complete application exceeded the physical RAM budget when linked with
the selected compiler's general newlib formatter. The production and function
test builds now both use `--specs=nano.specs`. This retains real compiler
runtime implementations while reducing formatter and library size. Integer
and text `printf` formats are used; `_printf_float` is not requested. Recovered
explicit formatters produce application decimal strings, while `strtod`,
libm, and software floating-point arithmetic remain native runtime functions.
Serial float diagnostics use the actual SDK's `Print` float overload; they do
not enable newlib float `printf`.

The native link selects SDK startup explicitly with `-nostartfiles` and
`-nostdlib`, then supplies `-lm`, `-lc`, `-lgcc`, and `-lnosys` in the library
group. `nano.specs` selects compact stdio and the remaining nano C runtime.
The production builder explicitly selects the coherent full-newlib allocation
family from the same compiler's `thumb/v7e-m/nofp/libc.a` before that group;
the archive and extracted objects are hashed in `allocator_providers.json`.
Every public definition from
the application and adapter archives is rooted with `--undefined`, in
addition to `--whole-archive`, because archive inclusion alone does not
prevent section garbage collection. The final symbol table is checked against
those roots. See [the native builder](../tools/build_sdk_validation.py).

The selected compiler's libnosys machine code and `ENOSYS` definition establish
the following unsupported-operation contracts:

| Hooks | Contract |
| --- | --- |
| `_read`, `_write`, `_close`, `_fstat`, `_lseek`, `_kill`, `_getpid` | Return `-1` and set `errno` to `ENOSYS` (88). |
| `_isatty` | Returns 0 and sets `errno` to `ENOSYS` (88). |
| `_exit` | Enters an infinite loop. |

These hooks do not route standard streams to Serial, the display, WiFi, or
SLFS. Application I/O uses the actual typed SDK interfaces. `_exit` is a
runtime termination endpoint, not the brewer's shutdown or reset procedure.
The build records the selected support-library hash and linked provider for
each retained hook in `runtime_boundary.json`.

### Allocation and bounded SDK heap ownership

The first production runtime linked the compiler's nano allocator directly to
Energia's `_sbrk`. Executing allocation failures in the actual ARM image exposed
two concrete defects: Energia returns zero on exhaustion, while the allocator
expects `(void *)-1`; the pinned nano allocator also dereferences an empty
free-list head on a failing growth path after correcting that return convention.
The unchanged SDK routine additionally accepts increments that move its break
below the heap or wrap the address calculation. These were measured rebuild
runtime defects, not claims about the original PicoBrew allocator.

The production build now selects the full-newlib allocation family supplied by
the same pinned compiler. Related routines that interpret allocation metadata
are selected together; this does not combine nano and full allocation layouts.
The smaller nano stdio remains selected. No replacement allocator algorithm or
guessed chunk layout is introduced.

`__wrap__sbrk` in
[energia_runtime_compat.c](../platform/energia_runtime_compat.c) checks the
signed increment against `_heap` and `_eheap` before calling the genuine SDK
`_sbrk` from `startup_gcc.c`. The SDK remains the sole owner of its heap-break
state. Invalid requests leave the break unchanged, return `(void *)-1`, and set
the syscall `errno` object to `ENOMEM`; native `_sbrk_r` propagates that value
to the caller's reentrancy object. The builder verifies the full-newlib
allocation providers, the adapter and the actual SDK heap provider.

[test_native_allocator.py](../tests/test_native_allocator.py) executes the
production allocator, adapter and SDK heap implementation without substituting
any function. Its 47 scenarios include aligned in-range allocation, zeroing,
multiply overflow, empty/nonempty-heap exhaustion, free/reuse, reallocation
failure with old-data preservation, signed bounds and errno. The final report
records 1,183 native calls and 92,214 checks. These are bounded, single-threaded
ARM tests. Whole-appliance peak memory and interrupt concurrency still require
device validation. The separate function-test ELF uses nano allocation; the
allocation evidence here is bound to the delivered production ELF.

## Board pin matching

Complete constant arrays from the `CC3200-LAUNCHXL` variant match the reference
firmware bytes:

| Array | Bytes | Original address |
| --- | ---: | --- |
| GPIO port bases | 20 | `0x200293f0` |
| Energia pin to hardware pin number | 82 | `0x20029404` |
| Energia pin to GPIO port index | 41 | `0x20029456` |
| Energia pin to GPIO bit mask | 41 | `0x200293c5` |

The alternate `launchpad` directory contains the same arrays. RedBear variants
do not match the three per-pin arrays. The PWM/timer array was not located in
the original image, so it has no equivalent matching claim. The
[pin verification tool](../tools/verify_energia_pin_tables.py) emits fixed-width
constant tables with host GCC and compares their bytes;
[board-pin-validation.json](../analysis/board-pin-validation.json) records the
results.

Energia pin identifiers are logical pins. Logical pin 4 maps to TI GPIO12,
18 to GPIO17, and 19 to GPIO28. Logical pins 25 and 26 map to ADC-capable
PIN58 and PIN59, using channels 1 and 2. The original `analogRead` also accepts
PIN57/channel 0, which the pinned upstream source comments out. That difference
does not change the two thermistor mappings, but the table match does not
establish equivalence of every ADC path or confirm physical board wiring.

Pico-specific socket, storage, hash, and object-lifetime adaptations are covered
in the [SDK boundary review](sdk-boundary-review.md). The
[network recovery ledger](network-recovery.md) records deliberately bounded
failure-path repairs separately from original-behavior comparisons. Hardware
checks are described in [hardware validation](hardware-validation.md).
