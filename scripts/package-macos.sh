#!/usr/bin/env bash
set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
BUILD_DIR="${ROOT_DIR}/build"
VERSION=""

usage() {
  cat <<EOF
Usage: $(basename "$0") --version <version> [--build-dir <path>]

Creates a deterministic macOS release zip containing:
- AMidiOrgan.app
- docs/ seed content
- USER_MANUAL.md
- README-USER.txt
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --version)
      VERSION="${2:-}"
      shift 2
      ;;
    --build-dir)
      BUILD_DIR="${2:-}"
      shift 2
      ;;
    -h|--help)
      usage
      exit 0
      ;;
    *)
      echo "Unknown option: $1" >&2
      usage >&2
      exit 1
      ;;
  esac
done

if [[ -z "${VERSION}" ]]; then
  echo "--version is required" >&2
  usage >&2
  exit 1
fi

if [[ "${BUILD_DIR}" != /* ]]; then
  BUILD_DIR="${ROOT_DIR}/${BUILD_DIR}"
fi

APP_PATH="${BUILD_DIR}/AMidiOrgan_artefacts/Release/AMidiOrgan.app"
if [[ ! -d "${APP_PATH}" ]]; then
  echo "App bundle not found: ${APP_PATH}" >&2
  exit 1
fi

DIST_DIR="${ROOT_DIR}/dist"
PACKAGE_ROOT_NAME="AMidiOrgan-${VERSION}-macos"
PACKAGE_DIR="${DIST_DIR}/${PACKAGE_ROOT_NAME}"
ZIP_PATH="${DIST_DIR}/${PACKAGE_ROOT_NAME}.zip"

rm -rf "${PACKAGE_DIR}" "${ZIP_PATH}"
mkdir -p "${PACKAGE_DIR}"

cp -R "${APP_PATH}" "${PACKAGE_DIR}/"
cp -R "${ROOT_DIR}/docs" "${PACKAGE_DIR}/docs"
cp "${ROOT_DIR}/USER_MANUAL.md" "${PACKAGE_DIR}/USER_MANUAL.md"

cat > "${PACKAGE_DIR}/README-USER.txt" <<'EOF'
AMidiOrgan packaged release

How to start:
1) Open AMidiOrgan.app
2) If blocked on first launch, right-click app -> Open
3) Read USER_MANUAL.md for full setup and operating guidance

The docs/ folder is included for runtime seed data and reference assets.
EOF

mkdir -p "${DIST_DIR}"
(
  cd "${DIST_DIR}"
  zip -r -q "${PACKAGE_ROOT_NAME}.zip" "${PACKAGE_ROOT_NAME}"
)

echo "Created package: ${ZIP_PATH}"
