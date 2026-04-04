#!/usr/bin/env bash
# Option 2: Build AMidiOrgan on macOS assuming Xcode/CLT, CMake, Git, and JUCE are already available.
# Run from anywhere; paths are resolved from this script's location.
#
# Usage:
#   ./scripts/mac-build.sh
#   ./scripts/mac-build.sh --release
#   ./scripts/mac-build.sh --ninja
#   ./scripts/mac-build.sh --xcode
#   BUILD_DIR=build-custom ./scripts/mac-build.sh
#
# Environment (optional overrides):
#   JUCE_ROOT   Path to JUCE (default: <repo>/.deps/JUCE)
#   BUILD_DIR   CMake build tree (defaults depend on flags; see below)

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JUCE_ROOT="${JUCE_ROOT:-${ROOT_DIR}/.deps/JUCE}"

USE_XCODE=0
USE_NINJA=0
BUILD_TYPE="Debug"
# Default build dirs avoid mixing generators in one tree (see MacMinibuild.md).
BUILD_DIR="${BUILD_DIR:-}"

usage() {
  cat <<EOF
Usage: $(basename "$0") [--release] [--ninja] [--xcode]

  Default: Unix Makefiles (or Ninja with --ninja), Debug, build-mac

  --release   Release build (build-mac-release for Makefile/Ninja; --config Release for Xcode)
  --ninja     Use Ninja generator (requires ninja on PATH)
  --xcode     Use Xcode generator (build-mac-xcode)

Environment: JUCE_ROOT, BUILD_DIR (optional overrides)
EOF
}

while [[ $# -gt 0 ]]; do
  case "$1" in
    --release)
      BUILD_TYPE="Release"
      shift
      ;;
    --ninja)
      USE_NINJA=1
      shift
      ;;
    --xcode)
      USE_XCODE=1
      shift
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

if [[ -z "${BUILD_DIR}" ]]; then
  if [[ "${USE_XCODE}" -eq 1 ]]; then
    BUILD_DIR="${ROOT_DIR}/build-mac-xcode"
  elif [[ "${BUILD_TYPE}" == "Release" ]]; then
    BUILD_DIR="${ROOT_DIR}/build-mac-release"
  else
    BUILD_DIR="${ROOT_DIR}/build-mac"
  fi
fi

require_cmd() {
  if ! command -v "$1" >/dev/null 2>&1; then
    echo "Required command not found: $1" >&2
    exit 1
  fi
}

require_cmd cmake

if [[ ! -f "${JUCE_ROOT}/CMakeLists.txt" ]]; then
  echo "JUCE not found at ${JUCE_ROOT} (expected CMakeLists.txt)." >&2
  echo "Clone JUCE, e.g.:" >&2
  echo "  mkdir -p \"$(dirname "${JUCE_ROOT}")\" && git clone --depth 1 https://github.com/juce-framework/JUCE.git \"${JUCE_ROOT}\"" >&2
  echo "Or run: ./scripts/mac-bootstrap.sh" >&2
  exit 1
fi

CMAKE_VER_LINE="$(cmake --version | head -1)"
# Expect: cmake version 3.28.1
CMajor="$(echo "${CMAKE_VER_LINE}" | /usr/bin/awk '{ for(i=1;i<=NF;i++) if($i ~ /^[0-9]+\.[0-9]+/) { split($i,a,"."); print a[1]; exit } }')"
CMinor="$(echo "${CMAKE_VER_LINE}" | /usr/bin/awk '{ for(i=1;i<=NF;i++) if($i ~ /^[0-9]+\.[0-9]+/) { split($i,a,"."); print a[2]; exit } }')"
CMajor="${CMajor:-0}"
CMinor="${CMinor:-0}"
if [[ "${CMajor}" -lt 3 ]] || { [[ "${CMajor}" -eq 3 ]] && [[ "${CMinor}" -lt 22 ]]; }; then
  echo "CMake 3.22+ required. Found: ${CMAKE_VER_LINE}" >&2
  exit 1
fi

echo "==> AMidiOrgan macOS build (Option 2)"
echo "    Repo:       ${ROOT_DIR}"
echo "    JUCE_ROOT:  ${JUCE_ROOT}"
echo "    Build dir:  ${BUILD_DIR}"
if [[ "${USE_XCODE}" -eq 1 ]]; then
  echo "    Generator:  Xcode (multi-config)"
  echo "    Config:     ${BUILD_TYPE}"
else
  echo "    Generator:  $([[ "${USE_NINJA}" -eq 1 ]] && echo Ninja || echo Unix Makefiles)"
  echo "    Build type: ${BUILD_TYPE}"
fi

cd "${ROOT_DIR}"

if [[ "${USE_XCODE}" -eq 1 ]]; then
  XCODE_CONFIG="Debug"
  [[ "${BUILD_TYPE}" == "Release" ]] && XCODE_CONFIG="Release"
  cmake -S . -B "${BUILD_DIR}" -G Xcode -DJUCE_ROOT="${JUCE_ROOT}"
  cmake --build "${BUILD_DIR}" --config "${XCODE_CONFIG}" --target AMidiOrgan
  echo ""
  echo "Built: ${BUILD_DIR}/AMidiOrgan_artefacts/${XCODE_CONFIG}/AMidiOrgan.app"
  echo "Run:   open \"${BUILD_DIR}/AMidiOrgan_artefacts/${XCODE_CONFIG}/AMidiOrgan.app\""
else
  GEN_ARGS=()
  if [[ "${USE_NINJA}" -eq 1 ]]; then
    require_cmd ninja
    GEN_ARGS+=(-G Ninja)
  fi
  # ${GEN_ARGS[@]+...} avoids "unbound variable" under `set -u` when the array is empty (macOS bash 3.2).
  cmake -S . -B "${BUILD_DIR}" ${GEN_ARGS[@]+"${GEN_ARGS[@]}"} -DJUCE_ROOT="${JUCE_ROOT}" -DCMAKE_BUILD_TYPE="${BUILD_TYPE}"
  cmake --build "${BUILD_DIR}" --target AMidiOrgan
  ART_SUB="${BUILD_TYPE}"
  echo ""
  echo "Built: ${BUILD_DIR}/AMidiOrgan_artefacts/${ART_SUB}/AMidiOrgan.app"
  echo "Run:   open \"${BUILD_DIR}/AMidiOrgan_artefacts/${ART_SUB}/AMidiOrgan.app\""
fi
