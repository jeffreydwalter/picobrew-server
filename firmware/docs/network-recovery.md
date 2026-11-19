# Network, storage, identity, update and PicoStill recovery

This domain is ordinary C application code with an explicit C++ Energia/CC3200 bridge. Production functions do not call original image addresses, use original RAM addresses, allocate guessed C++ objects, or invoke reconstructed vtable slots. Original addresses in tests and this document identify evidence from the 0.1.34 reference image.

## Source and integration

| File | Responsibility |
| --- | --- |
| `include/picobrew/network.h` | Owned network/settings/Still state, typed platform and UI operations, public application services |
| `src/network/network.c` | WiFi policy, reconnect/DNS status, HTTP framing and framed/hex response readers, URL escaping and original decimal formatting |
| `src/network/api.c` | Registration, sessions, recipe/PicoPak/actions APIs, logs, lists and Still permissions |
| `src/network/storage.c` | Preferences, credentials, UID, shuttle scale, recovery records and file modes |
| `src/network/still_network.c` | Still action HTTP, eight-field telemetry, discovery, AP/LAN configuration and session commands |
| `src/network/update.c` | Firmware envelope, chunk writes, checksum, boot selection and update workflow |
| `include/picobrew/text.h`, `src/network/text.c` | Shared original CString predicate and bounded-copy semantics, including warnings |
| `platform/pico_http_client.cpp` | The two Pico-specific socket connection paths on a genuine native `WiFiClient` object |
| `platform/energia_network.cpp` | Actual SDK WiFi, socket I/O, SLFS, hardware MD5, boot file, reset, ping and address configuration calls |

`PicoNetwork` shares the application's `PicoAppState` and `PicoSettings`. All required UI and recipe-decoder services are installed by application composition. The platform object receives a pointer to an already constructed `PicoHttpClient`. Native class construction, destruction and method dispatch use their C++ declarations.

`server_host` and `server_port` default to the original `picobrew.com:80`. The application can configure them at construction; response handling and endpoint grammar remain the recovered protocol. The observed offline setting disables step/progress logging. It does not turn every API operation into a successful no-op.

The upstream SLFS API returns zero for successful open/close, while Pico's original fork returns 1 on success. The bridge normalizes the real upstream result explicitly. The original close wrapper also treated a negative close error as true; the recovered bridge returns false on that path. The original delete wrapper always returned 1, whereas the native callback preserves the SDK's zero-success/negative-error result. Application deletion callers ignore that result. These return contracts are recorded separately from operation and filename equivalence.

## HTTP and application payloads

Requests use `GET <path> HTTP/1.0\r\nHost: <host>\nConnection: close\n\n`. The original mixed line endings and five 5 ms delays are retained. Response readers skip bytes through the first `#`, then consume either a `#` or `|` terminated field. Recipe images are 1024 decoded bytes, supplied as hexadecimal pairs separated by `|`.

Common endpoints begin `/API/pico/`. UID spelling and parameter case are significant:

| Endpoint | Additional parameters / result |
| --- | --- |
| `register?uid=…` | `T`/other non-`F` response, exact `F` fatal-registration response, or longer `F:` registration code |
| `getSession?uid=…` | `sesType`; nonempty session ID of at most 21 characters |
| `createPicoPak?uid=…` | `recipeid`; returned recipe identifier of at most 20 characters |
| `getActionsNeeded?uid=…` | Comma-separated numeric action IDs |
| `getRecipe?uid=…` | `rfid`, `ibu`, `abv`; recipe text followed by optional bitmap data |
| `picoChangeState?picoUID=…` | `state`; background connection policy |
| `error?uid=…` | `code`, `rfid` |
| `log?uid=…` | `sesId`, integer `wort`/`therm`, escaped `step`, optional `event`, `error`, `sesType`, `timeLeft`, `shutScale`; Still sessions also send `t1`, `t2`, `p` |
| `getAssociatedPaks?uid=…`, `recipelist?uid=…` | `identifier,title|` records; 19 loaded records per page; adjacent duplicate titles collapsed for associated packs |
| `canUsePicoStill`, `hasCleanedAck`, `setCleanedAck` | `picoUid`, `picoStillUid`; single-character `T` grants the corresponding result |
| `checkFirmware?uid=…` | `version=0.1.34`; single `T` after opening hash requests an update |
| `getFirmware?uid=…` | Streaming envelope described below |

The formatter truncates binary floating-point values, as the original does; replacing it with rounded `%f` changes payload bytes. URL escaping preserves the original backslash-to-`%7C` mapping. Tests compare both behaviors with original ARM instructions.

## Persistent files

| File | Original format / capacity |
| --- | --- |
| `ssid.txt` | Raw SSID, file capacity 33 bytes |
| `wifiSecurity.txt` | Security digit: WEP 5, open 7, other values below 7 take WPA path; capacity 2 |
| `wifiPass.txt` | Raw password, capacity 64; omitted for open networks |
| `userPref.txt` | Temperature-unit digit immediately followed by decimal language index; capacity 5 |
| `SS.txt` | Shuttle scaler with three truncated fractional digits; capacity 6 |
| `sys/uid.txt` | Lowercase MD5 hex of six MAC bytes; capacity 33 |
| `restart.txt` | `session_type:recipe_id:abv:ibu:step:active_minutes:drain_minutes:resume_state:calibrated:`; capacity 100 |
| `/sys/mcubootinfo.bin` | Eight bytes: selection in byte 0, reserved bytes 1–3, little-endian magic `0xabcddcba` in bytes 4–7 |

Recovery is supported only for session types 0 and 1. Non-forced saves require elapsed time of at least 60001 ms. A fresh save updates the time before the file write, including when that write subsequently fails; that ordering is preserved.

## PicoStill wire protocol

The nine action paths are `ping`, `configure`, `stillip`, `connecttolan`, `start`, `end`, `data`, `fanon`, `fanoff`. Ping, IP query and data query use GET; the others use POST. Every request includes a decimal `Content-Length` and a blank line. Configure loads the stored home credentials and sends `ssid=<SSID>&pass=<password>` without introducing new escaping.

The Still AP is `PicoStill_<identifier>` with password `12345678`; its address is `192.168.4.1`. Configuration discovers its home IP, requests LAN connection, then reconnects the Pico to its stored home network. The recovered three-attempt configuration loop and delays are explicit source.

Telemetry contains eight comma-separated fields. Fields 0–4 are parsed through single precision, then stored as doubles; fields 5–7 are converted to integer booleans. Empty fields become `-1.0f`. The five numeric fields map to temperatures 1–3, an unestablished fourth reading, and pressure. Fields without established meaning retain neutral names rather than invented physical interpretations.

Each parsed field also produces the original `data[index]=` diagnostic followed by the single-precision value at two decimal places and a newline. The typed `serial_write_float` platform callback delegates formatting to the pinned SDK. The parser rejects a field as soon as its fifteenth character is read, matching the original limit. Communication failure counting preserves the original eight-bit wrap from 255 to 0 before copying the result into the wider telemetry field.

## Firmware update format and activation

The envelope is `<decimal byte count>,<checksum>|<hex payload>#`. The checksum is MD5 of the decimal signed nibble sum, where every fifteenth hexadecimal character starting at index 0 is subtracted and the others are added. It is not MD5 of the firmware bytes. Streaming writes use 248-byte chunks, followed by the remaining bytes at the terminator.

The normal update path selects factory boot, removes image 3, and writes `/sys/mcuimg2.bin` with capacity `0x2f800`. The recovery-image path writes `/sys/mcuimg1.bin` with capacity `0xf000`. Successful activation reads the boot record, sets its magic, and changes slot 1 to 2 or any other slot to 1. This source retains those observed transitions; it does not infer a new bootloader protocol from filenames.

Boot-file operations use an explicit zero token for the observed nonsecure boot record. The original destination-open caller supplies zero at 0x20012f8c; subsequent original wrappers also exposed caller-scratch token dependence. The native bridge has a defined input contract and rejects open/read/write/close failure. It never claims a secure-token recovery that was not established.

## Corrections established by direct comparisons

The additional direct suites found and corrected four observable discrepancies in the reconstruction:

| Source | Restored behavior | Evidence |
| --- | --- | --- |
| `src/network/update.c` | `pico_update_available` emits the exact `TIMEOUT` line only when its deadline expires. A disconnected client exits without that line. | Nine update-availability cases in `analysis/network-direct-tests.json`, including response, read-error, disconnect and deadline paths |
| `src/network/still_network.c` | The status failure counter wraps as an eight-bit value before updating telemetry. | Status-reader cases cover prior counts 0, 1, 254 and 255 in `analysis/network-direct-still-tests.json` |
| `src/network/still_network.c` | `pico_still_wait_response` emits `waitForStill() [ap=%s] [delay=%u] [to=%u]` before its initial delay. | Fourteen wait cases compare serial output and service order across success, retry, connection failure and timeout paths |
| `src/network/still_network.c`, `platform/energia_network.cpp` | Telemetry diagnostics pass the actual single-precision value and precision 2 to the native formatter; the original 14-character field limit is retained. | Twelve parser cases compare values, state, serial labels and formatter arguments. A separate 106-case production callback suite compares exact emitted bytes through the actual SDK formatter. |

The remaining newly exercised storage, WiFi connection, logging, DNS and update workflow paths matched the existing implementation at their declared service boundaries. The close/delete/ping return distinctions below were existing explicit reconstruction contracts, rather than newly introduced equivalence fixes.

## Deliberate repairs and limits of equivalence

The tests distinguish original/source comparisons from source-only failure-path checks. These repairs are intentional and are not described as exact equivalence on the affected inputs:

| Change | Evidence and boundary | Validation |
| --- | --- | --- |
| Actual buffer capacities include NUL; overlong text/framed fields are rejected before overflow | Original text reader0x2000a538 stores its terminator after the maximum input count; original response reader has a similar boundary. Restart allocation is 101 bytes for 100 payload bytes. Rejected file prefixes are explicitly terminated. | Framing, storage and repairs groups include canaries and oversized records |
| Explicit security string `{digit, 0}` |0x2000581c pushes incoming r1;0x20005842 replaces only its first byte. Both callers follow UI/display functions without establishing a string argument. Controlled nonzero r1 cases prove the dependency, but do not prove every such seed is naturally reached. | Three defined zero-seed original comparisons; direct source credential round trips |
| Exact byte-count and close checks for file/request writes | Original writer0x20005678 accepts a returned count merely below capacity; receiver compares only the low write-result byte and ignores destination-open result. | Short, negative/open, close, failed-boot and malformed-envelope cases reject success |
| Bounded action parsing does not consume stale prior-field digits | Original0x20010914 passes output-array capacity as input-position limit to0x20008c90 and ignores parse failure. With response `1,,bad,23,0` and capacity8, original controlled execution yields `[1,23,3]`; new code yields `[1,23,0]`. | Dedicated repaired vector, plus valid-domain original comparisons |
| Malformed Still telemetry is rejected without modifying the destination, and the status reader propagates parser failure | Instructions0x200064d8..0x200065d0 clear the 16-byte text field, not the float array; early termination still consumes all eight float slots. The original status reader also ignores the parser's false return. | Twelve direct parser cases supplement the earlier canary checks. Six separate status-reader repair cases compare malformed original acceptance with rebuilt rejection at failure counts 0, 254 and 255. |
| Failed Still configure request cannot inspect uninitialized response text |0x2000bf08 ignores transport result before comparing the first response byte. | Failed connect returns false |
| Overlong Still IDs/scans, invalid selections and out-of-range create-size arguments are rejected | Original identifier copy writes into16 bytes with unchecked strcpy; file-mode table is only defined through `0x00ff0000`. | Existing identifier/create-size checks plus five rebuilt-only scan checks: 23 matches, overlong SSID, overlong identifier, negative selection and selection beyond the match count. Original out-of-array writes are not executed. |
| UID recomputation writes one32-character result rather than appending into existing contents | Original0x20005c00 repeatedly strcat's into the global UID. Normal startup starts empty. | Known MAC digest and repeat-computation checks |
| Boot transition failure propagates and prevents reset | Original0x2001839c/0x200183d0 ignore failed read/write; original update resets unconditionally after attempted activation. | Boot read/write/close failure scenarios |
| `pico_still_stop` returns whether a stop request succeeded | The original no-session path returns the serial print byte count, which its appliance callers ignore. The composed application also ignores the new return on this path. | Four active-session cases compare both bodies directly. Two inactive cases execute both bodies and compare state, serial output and absence of service requests; the incidental original return is not compared with the defined false result. |
| Native close failures return false | The original wrapper tests its legacy SLFS result for nonzero and therefore accepts negative close errors. Native SLFS uses zero success. | Two normalized success/no-open contract cases and two explicit error-repair cases compare actual SLFS execution, call traces and file state. |
| Native ping errors return zero with an initialized report | The original ignores the driver status and can read an unwritten stack byte when no report is supplied. | Six successful ping cases compare command fields and the low byte of received-packet counts. Three separate error cases check the defined zero result. |

Original valid-input quirks are retained: wait-for-local-IP returns true after a timeout when its error-display flag is false; response deadline arithmetic retains its original wrap behavior; the image-carousel fetch increments page count twice for each extra image. The UI owner separately preserves the original shared first-image callback. Those are observable recovered behaviors, not speculative feature improvements.

The shared CString helper preserves original warnings and counts. Internal `net_copy` calls are distinct bounded copies corresponding to other original strcpy/strncpy use; they do not universally reproduce CString warnings. Actual network CString use in the session-ID API now calls `pico_text_copy` with the network logger.

## Verification

The portable suites execute the recovered C and the original Thumb functions in separate ARM emulator instances. The native suites execute the production C++ bridge and actual SDK methods. Reports bind the exact ELF and original image where original comparisons are made. Platform and subordinate application services are controlled explicitly; no live radio or flash operations occur.

| Test source and report | Passing scenarios | Evidence |
| --- | ---: | --- |
| `tests/test_network.py` → `analysis/network-tests.json` | 3,453 | Existing numerical, text, status, DNS, HTTP, storage, Still, update and API groups; a mixture of original comparisons, bounded I/O contracts and repairs |
| `tests/test_network_direct.py` → `analysis/network-direct-tests.json` | 185 | Per-entry direct original/rebuilt comparisons for 21 storage, connection, logging, resolution and update entries |
| `tests/test_network_direct_still.py` → `analysis/network-direct-still-tests.json` | 152 comparisons/contract checks, plus 11 repairs | 142 direct function comparisons, four folded IPv4-logger caller comparisons, four folded-predicate checks and two inactive-stop effect comparisons across 16 entries; the 11 malformed-input repairs remain separate |
| `tests/test_native_network.py` → `analysis/native-network-tests.json` | 32 | 15 original/native socket comparisons and 17 controlled filesystem/MD5 boundary cases |
| `tests/test_native_network_direct.py` → `analysis/native-network-direct-tests.json` | 56 | Five native mappings: 40 direct comparisons, nine delete-return contracts, two normalized close contracts and five close/ping failure repairs |
| `tests/test_native_serial.py` → `analysis/native-serial-tests.json` | 106 | Supplemental exact-byte comparisons of the registered float callback and original formatter; no additional original network entry is claimed |

The native ISR cases cover all 16 interrupt masks with both initial ready states. The WiFi initialization thunk forwards both SDK outcomes. Delete cases execute both SLFS implementations and compare filenames and ordered requests, while retaining the different return contracts. Ping cases compare the first 20 command bytes, IPv4 target `0x08080404`, address family and callback arguments; unused original command padding is outside the comparison.

The ledger accounts for **114 original network-domain entries**. All 47 entries previously supported only by source review now have recorded behavioral evidence. There are **48 entries with per-entry execution or contract evidence**: 43 with direct function comparisons, one folded logger tested through its actual caller, one folded predicate contract, two SDK return-contract mappings, and one telemetry-clear mapping exercised through the initializer. The other **66 entries retain group-scoped evidence** from the existing suites. Those group totals are not per-entry counts and do not establish that every mapped helper was invoked directly.

Five of the 43 direct mappings delegate to the UI ledger: the Still cleaned confirmation, communication failure, completion, temperature warning and fan menu each have three direct cases in `analysis/ui-direct-menus-tests.json`. Telemetry clearing at `0x2000489a` is cross-referenced to `analysis/recovery/core.json` and the 12 initializer scenarios in `analysis/state-pid-tests.json`; it is called during initialization rather than invoked as a standalone test. The folded home-IP predicate is checked by executing `pico_network_ipv4_is_zero` and negating its result in the harness; those four cases do not execute the application's `still_home` wrapper. Cross-domain cases remain counted in their owning suites.

Run the network reports from the project root after building the validation and production ELFs:

```sh
python tests/test_network.py --elf build/application-validation.elf --output analysis/network-tests.json
python tests/test_network_direct.py --elf build/application-validation.elf --output analysis/network-direct-tests.json
python tests/test_network_direct_still.py --elf build/application-validation.elf --output analysis/network-direct-still-tests.json
python tests/test_native_network.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-network-tests.json
python tests/test_native_network_direct.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-network-direct-tests.json
python tests/test_native_serial.py --elf build/firmware/pico-pro-recovered.elf --output analysis/native-serial-tests.json
```

`analysis/recovery/network.json` records the evidence method, case selector and count for each newly tested mapping, preserves group evidence, and hashes its source files. Compile/link success and controlled comparisons do not establish electrical timing, flash-device behavior, radio interoperability or successful operation on a brewer. Those remain device validation work.
