#!/usr/bin/env bash
# Deprecated: use scripts/mac-bootstrap.sh (Option 1) or scripts/mac-build.sh (Option 2).

set -euo pipefail
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
echo "bootstrap_macos.sh: forwarding to mac-bootstrap.sh (CI-style Makefile build by default)." >&2
exec "${SCRIPT_DIR}/mac-bootstrap.sh" "$@"
