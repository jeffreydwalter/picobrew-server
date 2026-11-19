# Operations

## Configuration

| Setting | Default | Meaning |
|---|---|---|
| `--listen` | `127.0.0.1:8080` | Hub HTTP bind address |
| `--data` | `data` | SQLite database and encryption key directory |
| `--web` | `web/dist` | Production UI assets |
| `--secure-cookies` | false | Mark session cookies Secure; enable for an HTTPS reverse proxy |
| `--demo` | false | Loopback-only, hardware-rejecting simulator workspace |
| `HUB_PASSWORD` | unset | Provision the initial owner account without the setup form; ignored when an account already exists |

The health endpoint is `GET /healthz`. It reports process availability and version, not a successful connection to every brewer. Device connectivity is shown by `/api/v1/devices` after authentication.

Run one process for each data directory. Starting additional instances against the same database does not create a distributed fleet scheduler. The server shuts down its HTTP listener and cancels polling/history/AI workers before closing SQLite.

## Updates

Stop the hub and make a complete data-directory backup before changing versions. Build the new Go binary and frontend from the same revision, then restart using the same `--data` path. The frontend version is part of the application release; do not mix arbitrary old browser assets with a new API build. The SQLite migration gate refuses unknown newer schemas.

Machine identity is tied to its registration and archived history. Use a new registration to replace a machine with a different model or adapter. Updating an address for the same machine is supported. The hub pins the first authenticated hardware UID; a different UID makes that registration unavailable until the original device is restored or the replacement is registered separately. If the firmware is reformatted and native history counters reset, use a new registration rather than merging reused session IDs into its previous archive.

## Recover a native installation

For a shell-managed process, stop it first. With the server stopped:

```sh
PICO_HUB_BACKUP="$HOME/pico-hub-data-$(date +%Y%m%d-%H%M%S).tar.gz"
tar -czf "$PICO_HUB_BACKUP" data
```

Store this archive privately. It includes the encryption key and credential database. To restore, extract it into an empty directory, preserve permissions, and run the compatible hub with `--data` pointing at the restored `data` directory. Do not mix one backup's database with another backup's master key. Keep any existing data directory under a different name until the restored server is verified.

The browser workspace ZIP excludes authentication and provider/device credentials. It is intended for portable inspection and recipe/telemetry exports. Recipes can be imported through the normal recipe import flow; a workspace ZIP is not accepted as a full restore operation.

## Recover a Compose installation

`docker compose stop hub` stops writes. Back up the named `hub-data` volume using your Docker or NAS volume-backup facility, preserving all files and ownership (container UID/GID 10001). Restore into an empty volume while the service is stopped. Start the service and inspect `/healthz` and the fleet page. Keep the volume mounted at `/data`; do not place it in the image layer or source tree.

## Connection failures

- **Host cannot be resolved:** verify the address from the server's network namespace. A browser on your Mac resolving an mDNS name does not mean a remote server or Linux container resolves it. Use the brewer's current IPv4 address when appropriate.
- **Unauthorized from brewer:** replace that machine's stored API key with the provisioned key. Owner login and brewer authentication are separate.
- **Uncertain command or mutation:** inspect the front panel/current stored recipe before issuing a new action. The hub deliberately records this state rather than retrying a command that may have reached the brewer.
- **History is partial:** reconnect while the session is still retained. The next sync retries incomplete data, but samples already overwritten on the device cannot be reconstructed.
- **AI returns an error:** verify the selected provider's API key, model availability, quota and billing. Advice is optional; device polling/control and local history continue without an AI provider.

## Deployment notes

Use HTTPS for browser traffic outside a trusted local environment. The proxy should forward the original Host header and stream SSE without buffering. A cloud host needs a private route to the brewer LAN. Do not forward a brewer's HTTP API directly to the public internet.

The hub does not send emails, SMS or webhooks to third parties. There is no external messaging configuration in this release. The only optional external data transfer built in is AI context sent to the provider you configure; the firmware's telemetry forwarding target is separately configured on the machine.
