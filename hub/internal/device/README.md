# Device adapters

This package separates the hub from appliance protocols. It supports the
standalone REST API v1 implemented by `picobrew-pico-pro-firmware`, and an
independent in-memory simulator. A Z, Zymatic or Pico C **simulator** does not
establish support for controlling that model's stock firmware. There is no stock
PicoBrew cloud-protocol adapter in this package.

## Physical Pico S / Pro connection

`NewPico(baseURL, key)` accepts an explicitly configured HTTP or HTTPS endpoint
and the device's 64-character hexadecimal API key. Discovery requires API version
1 and `mode: "standalone"`; commands must also be advertised by that device.
Firmware semantic versions are reported independently of the API version.

The endpoint must be a host with an optional port, without a username, password,
path, query or fragment. Private LAN addresses are supported. Loopback,
unspecified, multicast and link-local addresses, localhost names and the
`100.100.100.200` metadata address are rejected. DNS results are checked at each
connection, then the validated numeric address is used for that connection.
HTTP proxy environment variables are ignored. The private loopback override is
used only by the package's local protocol tests.

One adapter serializes all requests to its device. Waiting for the serialization
lock respects context cancellation; each HTTP request has a three-second timeout.
The caller's context can also bound a complete inventory or recipe-write operation.
Connections close after each response, redirects are rejected, JSON writes use
an explicit content length, and no mutation is automatically retried. Requests
are limited to 4,095 body bytes; responses are limited to 64 KiB. Error messages
are bounded and strip the configured address and credential. HTTPS verifies the
server certificate through the normal system trust store.

| Adapter operation | Appliance endpoint and behavior |
| --- | --- |
| Discover | `GET /api/v1`; API version, capacities, command support |
| Status | `GET /api/v1/status`; temperatures remain Fahrenheit regardless of display preference |
| Display | `GET /api/v1/display`; 128 × 64, 1,024 framebuffer bytes encoded as 2,048 hexadecimal characters |
| Recipes | `GET /api/v1/recipes`, then each full recipe; this happens only on an explicit inventory request |
| PutRecipe | `PUT /api/v1/recipes/{id}`, followed by exact GET readback under the same serialization lock |
| DeleteRecipe | `DELETE /api/v1/recipes/{id}`; succeeds on 204, with the firmware enforcing idle-state rules |
| Command | `POST /api/v1/commands`; requires a valid payload, advertised support and a matching 202 acceptance response |
| Settings / UpdateSettings | `GET` / `PATCH /api/v1/settings` |
| Telemetry / UpdateTelemetry | `GET` / `PUT /api/v1/telemetry`; the latter requires all three configuration fields |
| History | `GET /api/v1/history` |
| HistoryPage | `GET /api/v1/history/samples/{offset}?id={id}` |

`PutRecipe` returning nil means readback matched the complete intended native
recipe, including its ID. It does not start a brew. A successful command means the
firmware accepted the command; preparation prompts and physical interlocks still
apply. Dedicated pause/resume commands and arbitrary actuator or temperature
overrides are not implemented by this firmware. The stop command latches heating
and pumping off; clearing that latch does not resume the interrupted operation.

For display rendering, raw pixel `(x, y)` is bit `y & 7` of byte
`x + (y / 8) * 128`. Rotate the decoded framebuffer 180 degrees for the upright
front-panel view. This transformation belongs to the presentation layer.

### Mutation outcomes

`device.Error` exposes `StatusCode`, `Code`, `Message` and `UnknownOutcome`.
Invalid input and ordinary device rejections are definite failures. A lost write
response, server-side write failure, unexpected write status or failed recipe
verification may mean the appliance changed despite the error. These cases set
`UnknownOutcome`. Read the device's current state or recipe back before deciding
on another action. A hub idempotency key cannot make an unacknowledged appliance
write intrinsically idempotent.

## Retained brew history

The physical recorder retains up to eight brew entries with a shared capacity of
4,224 samples. `RecordCount` counts currently retained rows, including a RAM tail;
`PersistedRecordCount` describes the committed subset. History IDs are numeric
32-bit IDs and must not be confused with the string `Status.SessionID`.

Pages contain at most 32 rows and 11 fixed fields: elapsed milliseconds; wort,
thermoblock and target temperatures; step seconds; phase; controller status;
compartment; current step; flags; and device state. Unavailable temperatures are
null. The adapter validates page identity, continuation, field counts, numeric
bounds and ordering within the page. Raw state codes are preserved. It does not
invent wall-clock timestamps: the appliance's stored records provide elapsed
time, not a trustworthy brew start time.

The recorder can change while pages are read. A removed history ID produces 409;
the caller must compare metadata before and after a download, retain partial
data honestly and retry a fresh snapshot later. The hub's history synchronizer
implements that policy. Old brews can disappear before reconnection, and a
restart can lose an uncommitted tail. Already missing samples cannot be recovered
from this API. Exporting or archiving history does not delete it on the device.

## Simulator

`NewSimulator(id, model)` creates isolated state without opening network
connections. Its firmware version is `simulator`, device UID is prefixed with
`simulator:`, samples use source `simulation`, and it advertises no persistent
history or outbound telemetry forwarding. The model label is descriptive.

Each simulator supports stored recipes, settings, a rendered OLED, encoder input,
preparation prompts, accelerated brews, maintenance completion, stop latching and
retained history. Time runs at 60 simulated seconds per real second. History is
volatile and has the same eight-entry / 4,224-row shared limits. The `demo-pro`
and `demo-z` fixtures start in different simulated brew phases; `demo-s` starts
idle. All fixtures are independent. No simulator command controls a real brewer.

## Verification

`go test -race ./internal/device` exercises local HTTP fixtures, request framing
and authentication, recipe readback, mutation ambiguity, serialization,
cancellation, redirect rejection, bounded/redacted errors, history schema checks
and simulator lifecycle/isolation. These tests do not contact an appliance and
do not substitute for physical protocol or brewing validation.
