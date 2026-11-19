# Optional telemetry forwarding

The Pico can send live readings to an HTTP receiver on your network. Forwarding
is **off by default** and saves its destination on the Pico when enabled.
Brewing, the browser console, and on-device brew history work independently of
the receiver. Exporting a brew from **Brew history** does not delete it.

This feature uses plain HTTP on a trusted LAN. The Pico's API key, Wi-Fi
credentials, and provider secrets are never included in forwarded snapshots.
No AI service is included.

## Start the optional receiver

The repository includes a Python standard-library receiver for macOS or Linux.
It saves complete snapshots in SQLite; no additional Python packages are needed.
Run this from the repository root:

```sh
python3 tools/pico_telemetry_receiver.py \
    --bind 0.0.0.0 \
    --port 8080 \
    --database private/pico-telemetry.sqlite3
```

Keep that process running while you want to collect readings. `Ctrl+C` stops
the receiver. The Pico continues brewing and recording local history.

The receiver defaults to `127.0.0.1` if `--bind` is omitted. The explicit
`0.0.0.0` above makes it reachable on the computer's IPv4 interfaces. In the Pico
settings, use the computer's **LAN IPv4 address or resolvable host name**, not
`0.0.0.0` or `127.0.0.1`. Permit inbound traffic on the chosen port through the
computer's firewall if needed.

The default database location is `private/pico-telemetry.sqlite3`. The repository
ignores `private/`. On POSIX systems, the receiver creates a new private directory
with mode `0700` and restricts the database file to `0600`. An explicitly supplied
database path can be outside the repository.

## Enable forwarding in the browser

While the brewer is idle, open **Settings → Send live readings**:

1. Enter the receiver's host name or IPv4 address and its port, normally `8080`.
2. Check **Send readings to a receiver**.
3. Click **Save forwarding**.

The host field accepts up to 127 ASCII characters. Enter a host only, without
`http://`, a path, credentials, or a port suffix. The separate port field accepts
`1` through `65535`. The receiver path is fixed at `/api/v1/telemetry`.

The status below the form reports delivery progress and counters. To turn
forwarding off, uncheck the option and save while idle. The destination is
retained for later use.

## Configure it through the Pico API

The same path has different roles on the two devices:

| Destination | Method and path | Purpose |
| --- | --- | --- |
| Pico | `GET /api/v1/telemetry` | Read saved configuration and current delivery status |
| Pico | `PUT /api/v1/telemetry` | Save the complete configuration while idle |
| Receiver computer | `POST /api/v1/telemetry` | Accept a snapshot sent by the Pico |

The Pico management requests use the existing device key. The receiver requires
no Pico key. From the repository root, replace the example Pico and receiver
addresses below:

```sh
PICO_URL='http://192.168.1.100'

pico_api() {
    printf 'Authorization: Bearer %s\n' "$(tr -d '\r\n' < private/api.key)" |
        curl --silent --show-error --fail-with-body --header @- "$@"
}

pico_api "$PICO_URL/api/v1/telemetry"

pico_api --request PUT --header 'Content-Type: application/json' \
    --data '{"enabled":true,"host":"192.168.1.20","port":8080}' \
    "$PICO_URL/api/v1/telemetry"
```

`PUT` requires all three fields: `enabled`, `host`, and `port`. An empty host is
allowed only when disabled. Use `enabled:false` with the same host and port to
disable forwarding. Configuration changes return `409` while a workflow is busy.
The **API explorer** in the console exposes these same GET and PUT operations.

## Delivery and status

When enabled, the Pico schedules a current snapshot approximately every
**15 seconds**. Delivery is best effort. The sender uses bounded socket work,
backs off after failures, and does not accumulate an offline queue. Readings
missed while the receiver or network is unavailable are not backfilled when it
returns. The separate on-device history and its browser exports remain available.

Host names are resolved only while the brewer is idle. A cached result can be
used during brewing; changing networks or an observed disconnection clears the
cache. A hostname destination can therefore report `waiting-idle` until the
brewer next becomes idle. A numeric IPv4 destination needs no DNS lookup.

The GET response contains the saved `enabled`, `host`, and `port`, the fixed
`interval_ms:15000` and `path`, and a `status` object:

| Status field | Meaning |
| --- | --- |
| `state` | `disabled`, `offline`, `waiting-idle`, `ready`, `connecting`, `sending`, `receiving`, or `backoff` |
| `resolved` | A receiver IPv4 address is currently available |
| `sent` | Uploads acknowledged with an HTTP 2xx status |
| `failed` | Failed delivery attempts |
| `dropped` | Skipped or lost snapshots, including failed deliveries; this overlaps `failed` |
| `last_success_ms` | Brewer uptime at the last successful upload; zero before success |
| `last_http_status` | Last receiver HTTP status, or zero before a response |
| `last_error` | `none`, `dns`, `socket`, `connect`, `send`, `receive`, `http`, `timeout`, `config`, or `capacity` |
| `storage_error` | The Pico encountered a destination-configuration storage error |

Delivery counters and last-success timing are volatile and reset on reboot.
Uptime is elapsed time since the Pico booted, not a date or UTC timestamp.

## Snapshot format and another receiver

The Pico sends JSON with `Content-Type: application/json` and an explicit
`Content-Length`. The outer envelope contains `schema_version`,
`firmware_version`, and the complete current `GET /api/v1/status` object:

```json
{
  "schema_version": 1,
  "firmware_version": "1.0.0",
  "status": {
    "uptime_ms": 123456,
    "ready": true,
    "device_uid": "example-device-id",
    "busy": true,
    "stopped": false,
    "stop_reason": "none",
    "command": "brew",
    "device_state": 3,
    "phase": 1,
    "controller_status": 0,
    "compartment": 2,
    "wort_f": 152.2,
    "thermoblock_f": 180.3,
    "target_f": 152.0,
    "step_seconds": 1200,
    "current_step": 1,
    "step_count": 5,
    "recipe_name": "House pale ale",
    "step_name": "Mash",
    "session_id": "example-session",
    "filter_inserted": true,
    "encoder": {"position": 0, "min": -100, "max": 100}
  }
}
```

Temperature fields stay in Fahrenheit regardless of the browser's display
preference. Unavailable readings are `null`. The authoritative schemas are
`TelemetryUpload` and `Status` in [openapi.json](openapi.json).

Any equivalent HTTP sink can accept these uploads. It should return a final
HTTP 2xx response after accepting the snapshot. The included receiver returns
`204` only after its SQLite transaction commits, accepts bodies up to 4096 bytes,
rejects invalid framing or payloads, and closes each connection. It exposes no
appliance controls or database download routes.

## Inspect or export the SQLite data

The `snapshots` table stores `id`, `received_at`, `device_uid`, `session_id`,
`uptime_ms`, `firmware_version`, and `payload_json`. `received_at` is the
receiver computer's UTC receipt time; `payload_json` preserves the JSON it
received. The raw JSON includes all status values above.

This standard-library command prints the received count and exports a snapshot
of the stored rows to a new JSON Lines file under `private/`. It opens the
database read-only and leaves its records intact:

```sh
python3 - <<'PY'
from contextlib import closing
from datetime import datetime, timezone
import json, os, sqlite3
from pathlib import Path

os.umask(0o077)
stamp = datetime.now(timezone.utc).strftime('%Y%m%d-%H%M%S')
output = Path('private') / f'pico-telemetry-{stamp}.jsonl'
with closing(sqlite3.connect('file:private/pico-telemetry.sqlite3?mode=ro', uri=True)) as db:
    print('Received snapshots:', db.execute('SELECT COUNT(*) FROM snapshots').fetchone()[0])
    with output.open('x', encoding='utf-8') as stream:
        for received_at, raw in db.execute('SELECT received_at,payload_json FROM snapshots ORDER BY id'):
            stream.write(json.dumps({'received_at': received_at, **json.loads(raw)}) + '\n')
print(output)
PY
```

The export is an archive of received live snapshots. The Pico's retained brew
recordings can separately be exported as JSON or CSV from **Brew history**.
