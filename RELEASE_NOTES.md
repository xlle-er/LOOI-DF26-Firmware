# 2.0.0

- Motor-only ESP32-S3 firmware; removes pitch servo and I²C dependencies.
- Wi-Fi hotspot provisioning, persistent network and motor calibration settings.
- Unified authenticated HTTP/BLE management and bounded motor tests.
- Exclusive control leases, stale-token rejection and stop-on-timeout.
- Local-file and verified HTTPS release OTA with size/SHA-256 checks.

首次 USB 安装需要 16MB Flash、OPI PSRAM 和双 3MB OTA 分区。
First USB installation requires 16MB Flash, OPI PSRAM and dual 3MB OTA partitions.

Hardware validation remains required. No automatic rollback or firmware signatures are provided.
