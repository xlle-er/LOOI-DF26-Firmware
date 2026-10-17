# Device protocol v2

`commands.json` is the command allowlist used by firmware and clients. Obsolete actuator commands are rejected. JSON numbers must be finite. Motion remains bounded by the existing hard speed/duration limits.

## BLE

- Name: `LOOI-<ID suffix or custom name>` (2.2.0+); older firmware uses `LOOI Body`
- Service: `7f2c2b6a-2d8f-4f6b-9a59-8a7f9d7c0001`
- Write: `7f2c2b6a-2d8f-4f6b-9a59-8a7f9d7c0002`
- Notify: `7f2c2b6a-2d8f-4f6b-9a59-8a7f9d7c0003`

Commands are UTF-8 JSON with a trailing newline. Notifications may span chunks; decode incrementally and split by newline. Include `id`; replies echo it. Motion, stop, ping and existing config commands preserve their previous wire shapes. Management writes include `admin_password`. BLE does not add encrypted pairing in this version.

## HTTP

- `GET /`: built-in management panel, HTTP Basic authentication.
- `GET /api/status`: device identity, capabilities, command list, network/calibration/OTA state, `control_owner`, `control_token`, and `csrf_token`. Never returns router passwords.
- `POST /api/command`: the same JSON command dispatcher as BLE.
- `POST /update`: multipart form with one `firmware` application image.

All APIs require HTTP Basic `admin:<device password>`. Mutating requests require `X-OTA-Token` from status and, if an Origin is present, the same origin. Command requests also require a stable per-client `X-Looi-Client` value. A control source is `ble` or `http:<client ID>`.

## Operations

| Command | Fields / behavior |
| --- | --- |
| `device_status`, `ota_status`, `calibration_get` | Reads current status; does not move motors |
| `wifi_scan` | Start asynchronous scan; poll until `scanning=false`, then consume `networks` |
| `wifi_configure` | `ssid`, `password`; empty password supports open networks; otherwise 8–63 bytes |
| `wifi_forget` | Persist empty network configuration and reopen hotspot |
| `calibration_set` | Boolean `left_invert`, `right_invert`, `swap`; or `reset=true`; or shortcut `reverse_forward` / `reverse_steering` |
| `control_acquire` | Claims an idle/same-client controller and returns a new `control_token`; `takeover=true` explicitly replaces another controller |
| `control_renew` | Current `control_token`; renew lease within one second |
| `control_release` | Release this client's control |
| `calibration_test` | Current `control_token`; `direction` is forward/backward/left/right/left_forward/left_backward/right_forward/right_backward; always <=300 ms |
| `motion` | `linear`, `angular`, `duration_ms`, optional `ramp_ms`, `label`; HTTP also supplies current `control_token` |
| `stop` | Always permitted for authenticated HTTP clients; invalidates ownership and pending old tokens |
| `ota_start`, `ota_exit` | Enter locked upgrade mode / leave it when no upload is active |
| `firmware_check` | Fetch latest trusted release manifest; ESP must be connected and time synchronized |
| `firmware_install` | `version` from the checked manifest; verified HTTPS download and OTA |

Successful management replies use `type=ack`, `cmd`, `accepted=true`; failures use `type=error`, `accepted=false`, `message`. Existing `config_get` and `ping` retain `config` and `pong` responses.

Uploads include `X-Firmware-Size` and lowercase hexadecimal `X-Firmware-SHA256`. Firmware must fit the inactive 3 MB application partition. A successful update schedules reboot; reconnect and verify `firmware_version`. A failed upload can be retried or exited, and cannot resume old motor movement.

### Post-update hotspot option (2.0.1+)

`ota_start` and `firmware_install` accept optional boolean `close_ap_after_update` (default `true`). Status reports `ota_ap_shutdown_supported`, `close_ap_after_update` and `ap_suppressed_after_ota`. The selection is bound to the OTA target partition and consumed on the following boot. Failed/incomplete OTA must not suppress the original partition's hotspot; BOOT explicitly reopens it. Prior versions require manually leaving the hotspot. This option never chooses the phone's internet connection.

## Hardware configuration protocol v1 (firmware 2.1.0+)

`hardware_get` returns normal device identity/status plus `hardware_profile`, `hardware_configured`, `hardware_ready`, `hardware_error`, `hardware_protocol: 1`, `flash_mb`, `physical_flash_bytes`, `psram_bytes` and `partition`. An unconfigured profile has an empty `pins` object and `hardware_ready: false`. Board names are `n16r8` and `n8r8`.

`hardware_set` takes a complete `hardware_profile`:

```json
{"type":"hardware_set","id":"unique-request-id","hardware_profile":{"version":1,"board":"n16r8","pins":{"ena":17,"in1":16,"in2":15,"in3":7,"in4":6,"enb":5}}}
```

HTTP uses existing Basic/CSRF authentication; BLE and USB mutations require `admin_password`. Changes revoke control and stop before saving. An accepted write replies with `restarting: true`, then reboots after 1.5 seconds. Never infer the new active profile from the acknowledgement: reconnect, read it back and require `hardware_ready: true`. Failed writes preserve the previous NVS blob; active mapping never changes before reboot.

USB UART0 (115200) and native Serial/JTAG use newline-delimited JSON with string IDs up to 80 bytes and requests up to 2048 bytes. Only `hardware_get`, `hardware_set`, `device_status` are accepted. Ignore boot log lines and correlate reply IDs; errors are `{type:"error",accepted:false,message:...}`. No USB motion or Wi-Fi credentials commands exist. Changing passwords still follows the existing firmware management policy.

Pin order is ENA, IN1, IN2, IN3, IN4, ENB; A is left, B is right. All must be distinct integers from the board profile's allowed subset. GPIO38/48 (board LEDs), straps, UART0, native USB and memory pins are reserved. This profile covers DevKitC-1-compatible layouts, not every board carrying an N16R8/N8R8 module.

OTA applications embed `LOOI_HW_V1|BOARD|1|PARTITION|END`, checked across streaming upload chunks before activating the new application. Missing/mismatched descriptors, including 2.0.x packages, are rejected. This is compatibility metadata, not a cryptographic signature. N16R8 keeps the legacy manifest URL; N8R8 uses `looi-s3-n8r8-manifest.json`. Local-file uploads follow the same device-side policy. Clients remember the last connected board for offline downloads; connect once to an N8R8 device before preparing its first offline package.


### OTA update channels

`firmware_check` and `firmware_install` accept `channel: "stable" | "preview"`, default `stable`; installation must supply the same channel as the preceding check. Unknown channels are rejected. Stable uses the existing latest/download manifest; preview uses releases/download/preview/ with the same per-board manifest names. The rolling `preview` release is marked prerelease and contains only channel pointers to immutable version-tagged application URLs. The device always enforces profile compatibility, HTTPS, size and SHA-256 regardless of channel. Local-file OTA is an explicit selection outside this online-channel preference.

## Device identity and asynchronous OTA (2.2.0+)

BLE advertises the service UUID unchanged. The full local name is in scan response: `LOOI-<last six ID characters>` by default, or `LOOI-<custom name>`. `device_id` and mDNS hostname never change. Identity is a separate NVS blob; the v2 network/calibration record and hardware profile are preserved.

| Command | Request / result |
| --- | --- |
| `admin_auth` | Verify current administration credentials; returns status without credentials |
| `device_name_set` | `name`: visible UTF-8 text, 1–24 bytes; persists, ACKs `restarting: true`, then reboots |
| `admin_password_set` | `new_password`: 8–63 printable ASCII characters; becomes HTTP, BLE/USB management and hotspot password after reboot |
| `ota_start` | Optional `hotspot: true` explicitly opens AP; AP also opens when STA is disconnected |
| `firmware_check` | Returns `job_id`, `ota_phase: checking`; poll `ota_status` until `ready` or `failed` |
| `firmware_install` | Checked `version` and matching `channel`; returns job acknowledgement before downloading |

Management mutations and authentication over BLE/USB require `admin_password`. HTTP retains Basic authentication, CSRF and client identifiers. Status advertises `device_identity`, `admin_password`, `async_ota` capabilities, plus `device_name_custom` / `ap_name_custom`. Passwords are never returned. Name/password updates stop motors, deliver the ACK using the current connection, then reboot after 1.5 seconds; clients reconnect and verify identity.

OTA status reports `job_id`, `ota_phase` (`idle`, `checking`, `ready`, `downloading`, `failed`, `rebooting`), `ota_progress` (0–100), `error`, and the checked `release`. Network reads run on a worker; the Arduino loop alone owns flash writes and services status between bounded chunks. An active download cannot be replaced or exited. Failed jobs leave motors locked until explicit exit; clients may offer hotspot recovery after the job has ended. A reboot acknowledgement is not proof of completion: reconnect and verify `device_id` and `firmware_version`.
