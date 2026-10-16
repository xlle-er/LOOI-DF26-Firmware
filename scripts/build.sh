#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")/.."
cli="${ARDUINO_CLI:-arduino-cli}"
if ! command -v "$cli" >/dev/null 2>&1 && [ -x '/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli' ]; then
  cli='/Applications/Arduino IDE.app/Contents/Resources/app/lib/backend/resources/arduino-cli'
fi
"$cli" compile --fqbn 'esp32:esp32:esp32s3:FlashSize=16M,PSRAM=opi,PartitionScheme=app3M_fat9M_16MB,USBMode=hwcdc,CDCOnBoot=default' \
  --build-path "$PWD/build" --output-dir "$PWD/dist" firmware/LooiWifiOta
python3 scripts/package-release.py 2.0.1 dist
