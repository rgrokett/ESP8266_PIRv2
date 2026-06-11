#!/usr/bin/env bash
set -euo pipefail

readonly CORE="esp8266:esp8266"
readonly CORE_VERSION="3.1.2"
readonly BOARD="esp8266:esp8266:huzzah"
readonly INDEX_URL="https://arduino.esp8266.com/stable/package_esp8266com_index.json"

if ! command -v arduino-cli >/dev/null 2>&1; then
  echo "arduino-cli is required: https://arduino.github.io/arduino-cli/" >&2
  exit 1
fi

installed_version="$(
  arduino-cli core list |
    awk -v core="$CORE" '$1 == core { print $2; exit }'
)"

if [[ "$installed_version" != "$CORE_VERSION" ]]; then
  arduino-cli core update-index --additional-urls "$INDEX_URL"
  arduino-cli core install "${CORE}@${CORE_VERSION}" \
    --additional-urls "$INDEX_URL"
fi

arduino-cli compile \
  --fqbn "$BOARD" \
  --build-path build/esp8266-huzzah \
  ESP8266_PIRv2
