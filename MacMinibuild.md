# AMidiOrgan — Mac Mini command-line build

This guide is for a **clean macOS machine** (for example a Mac mini) where you want to build **AMidiOrgan** from **Terminal** with copy-paste commands. It complements the main [README.md](README.md) (Build section).

**Prerequisites:** CMake 3.22+, a JUCE source tree, and Xcode or Xcode Command Line Tools (see below).

---

## 1. One-time setup on a new Mac

### Xcode / compiler

Install **Xcode** from the App Store, or install **Command Line Tools** only:

```bash
xcode-select --install
```

Accept the license if prompted (`sudo xcodebuild -license`).

### CMake

Install **CMake 3.22 or newer**, for example with [Homebrew](https://brew.sh/):

```bash
brew install cmake
```

Or install the macOS package from [cmake.org](https://cmake.org/download/).

Verify:

```bash
cmake --version
```

### Git

Usually provided with Xcode or CLT. Check with `git --version`.

### Optional: Ninja (faster builds)

```bash
brew install ninja
```

Use with `-G Ninja` when configuring (see below).

---

## 2. Get the sources

From a directory where you keep projects:

```bash
git clone https://github.com/aminnie/AMidiOrganOrg.git
cd AMidiOrganOrg
```

### JUCE (required once per clone)

The project expects a local JUCE checkout via `JUCE_ROOT` (see [CMakeLists.txt](CMakeLists.txt)). From the repo root:

```bash
mkdir -p .deps
git clone --depth 1 https://github.com/juce-framework/JUCE.git .deps/JUCE
```

Use:

```text
-DJUCE_ROOT="$PWD/.deps/JUCE"
```

(or an absolute path to any JUCE tree that contains `CMakeLists.txt` at its root).

---

## 3. Recommended: CI-style build (Unix Makefiles / default generator)

This matches the **macOS** steps in [.github/workflows/ci.yml](.github/workflows/ci.yml): single-config tree, `-DCMAKE_BUILD_TYPE`, no `--config` on the build line.

From the **repository root**:

```bash
cmake -S . -B build-mac -DJUCE_ROOT="$PWD/.deps/JUCE" -DCMAKE_BUILD_TYPE=Debug
cmake --build build-mac --target AMidiOrgan
```

**Debug output app** (typical JUCE layout):

```text
build-mac/AMidiOrgan_artefacts/Debug/AMidiOrgan.app
```

**Release** (use a separate build directory or reconfigure):

```bash
cmake -S . -B build-mac-release -DJUCE_ROOT="$PWD/.deps/JUCE" -DCMAKE_BUILD_TYPE=Release
cmake --build build-mac-release --target AMidiOrgan
```

Release app:

```text
build-mac-release/AMidiOrgan_artefacts/Release/AMidiOrgan.app
```

**With Ninja** (if installed):

```bash
cmake -S . -B build-mac -G Ninja -DJUCE_ROOT="$PWD/.deps/JUCE" -DCMAKE_BUILD_TYPE=Debug
cmake --build build-mac --target AMidiOrgan
```

Do **not** mix **Xcode** and **Makefile/Ninja** generators in the same `build-mac` folder. If you switch generators, delete the build directory and reconfigure.

---

## 4. Alternative: Xcode generator

Use this if you prefer **multi-config** builds and Xcode integration (`--config Debug|Release`):

```bash
cmake -S . -B build-mac -G Xcode -DJUCE_ROOT="$PWD/.deps/JUCE"
cmake --build build-mac --config Debug --target AMidiOrgan
```

Typical app bundle:

```text
build-mac/AMidiOrgan_artefacts/Debug/AMidiOrgan.app
```

---

## 5. Run the app

```bash
open build-mac/AMidiOrgan_artefacts/Debug/AMidiOrgan.app
```

Adjust the path if you used a different build folder or `-DCMAKE_BUILD_TYPE=Release`.

---

## 6. Tests and `ctest`

Build the test target:

```bash
cmake --build build-mac --target AMidiOrganTests
```

On **macOS**, [CMakeLists.txt](CMakeLists.txt) registers a **stub** `ctest` entry instead of running the full `AMidiOrganTests` binary (teardown crash on Apple is a known limitation). So `ctest` **confirms the test project is built**, not full runtime parity with Windows.

```bash
ctest --test-dir build-mac --output-on-failure
```

For **Xcode** multi-config builds, use `-C Debug` (or your configuration) with `ctest` if needed.

---

## 7. Optional CMake options (see [CMakeLists.txt](CMakeLists.txt))

- **`AMIDIORGAN_MACOS_BUNDLE_ID`** — default `org.amidiorgan.app`
- **`AMIDIORGAN_MACOS_DEPLOYMENT_TARGET`** — default `12.0`
- **`AMIDIORGAN_MACOS_ARCHS`** — set `CMAKE_OSX_ARCHITECTURES` (for example `arm64` or `arm64;x86_64` for universal)

Example:

```bash
cmake -S . -B build-mac -DJUCE_ROOT="$PWD/.deps/JUCE" -DCMAKE_BUILD_TYPE=Debug \
  -DAMIDIORGAN_MACOS_ARCHS=arm64
```

---

## 8. Troubleshooting

| Issue | What to try |
|--------|-------------|
| CMake cannot find JUCE | Ensure `JUCE_ROOT` points to a directory whose **root** contains JUCE’s `CMakeLists.txt`. |
| Stale or weird errors after changing generator | Remove the build tree (`rm -rf build-mac`) and reconfigure. |
| Link fails or SDK missing | Install/update Xcode or CLT; run `xcode-select -p` and fix path if needed. |
| Gatekeeper blocks unsigned local build | Right-click **Open** once, or adjust Security & Privacy — normal for local dev builds. |

---

## 9. Bash scripts (Option 1 and Option 2)

From the **repository root** on macOS, after `chmod +x scripts/*.sh` if needed:

| Script | Purpose |
|--------|---------|
| [`scripts/mac-bootstrap.sh`](scripts/mac-bootstrap.sh) | **Option 1** — Verifies Xcode/CLT, requires [Homebrew](https://brew.sh/), installs **cmake** (and **git** / **ninja** when needed), clones **JUCE** if missing, then runs `mac-build.sh`. Does **not** install Homebrew or CLT automatically (interactive / security). |
| [`scripts/mac-build.sh`](scripts/mac-build.sh) | **Option 2** — Assumes toolchain + CMake + JUCE are already present; configures and builds (CI-style **Makefile** / **Ninja** by default). |

Examples:

```bash
# First-time or new Mac (Homebrew + CLT already installed)
./scripts/mac-bootstrap.sh

# Later builds only (same flags as bootstrap’s build step)
./scripts/mac-build.sh

# Variants
./scripts/mac-build.sh --release
./scripts/mac-build.sh --ninja
./scripts/mac-build.sh --xcode
```

Default output locations match the manual CMake flow: `build-mac/AMidiOrgan_artefacts/Debug/AMidiOrgan.app` (Debug Makefile), `build-mac-release/.../Release/...` with `--release`, `build-mac-xcode/...` with `--xcode`.

The legacy [`scripts/bootstrap_macos.sh`](scripts/bootstrap_macos.sh) forwards to `mac-bootstrap.sh`.

---

## 10. Related docs

- [README.md](README.md) — full build matrix, Windows steps, and macOS Quick Start
- [.github/workflows/ci.yml](.github/workflows/ci.yml) — automated macOS configure/build commands
