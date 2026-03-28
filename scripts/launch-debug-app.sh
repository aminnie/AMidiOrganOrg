#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
APP_PATH="${ROOT_DIR}/build-mac/AMidiOrgan_artefacts/Debug/AMidiOrgan.app"

# Kill existing AMidiOrgan process if present.
pkill -f "AMidiOrgan.app/Contents/MacOS/AMidiOrgan" || true

# Launch a fresh app instance.
open -n "${APP_PATH}"

echo "Launched: ${APP_PATH}"
