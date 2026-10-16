# Device protocol v2

`commands.json` is the command allowlist used by firmware and clients. Obsolete actuator commands are rejected. JSON numbers must be finite. Motion remains bounded by the existing hard speed/duration limits.

## BLE

- Name: `LOOI Body`
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
