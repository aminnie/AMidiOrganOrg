#!/usr/bin/env bash

set -euo pipefail

ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
JUCE_ROOT="${JUCE_ROOT:-${ROOT_DIR}/.deps/JUCE}"
BUILD_DIR="${BUILD_DIR:-${ROOT_DIR}/build-mac}"
GENERATOR="${GENERATOR:-Xcode}"

echo "==> Checking required macOS build tools"
if ! command -v cmake >/dev/null 2>&1; then
  echo "cmake is not installed. Install CMake 3.22+ and retry."
  exit 1
fi

if ! command -v xcodebuild >/dev/null 2>&1; then
  echo "xcodebuild is not available. Install Xcode + Command Line Tools and retry."
  exit 1
fi

echo "==> CMake: $(cmake --version | awk 'NR==1 {print $3}')"
echo "==> Xcode: $(xcodebuild -version | awk 'NR==1 {print $2}')"

echo "==> Ensuring JUCE source exists at ${JUCE_ROOT}"
if [[ ! -f "${JUCE_ROOT}/CMakeLists.txt" ]]; then
  mkdir -p "$(dirname "${JUCE_ROOT}")"
  git clone --depth 1 https://github.com/juce-framework/JUCE.git "${JUCE_ROOT}"
else
  echo "JUCE already present, skipping clone."
fi

echo "==> Configuring initial macOS build directory"
cmake -S "${ROOT_DIR}" -B "${BUILD_DIR}" -G "${GENERATOR}" -DJUCE_ROOT="${JUCE_ROOT}"

cat <<EOF
Bootstrap complete.

Next steps:
  cmake --build "${BUILD_DIR}" --config Debug --target AMidiOrgan
  open "${BUILD_DIR}/AMidiOrgan_artefacts/Debug/AMidiOrgan.app"
EOF
