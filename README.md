# AMidiOrgan Features

AMidiOrgan is a live-performance MIDI controller built with [JUCE](https://juce.com). It is designed for rigs with multiple MIDI keyboards and sound modules, bringing routing, layering, preset recall, voice editing, and effect control into one interface.

The application is organized around three performance panels:

- `Upper`
- `Lower`
- `Bass&Drums`

Each panel uses the active configuration and panel data to determine which voices, routing, effects, presets, and rotary states are available during a performance. A panel can represent an organ style, a layered setup, or a song-specific registration.

## UI Screenshots

### Start Tab

![Start Tab](docs/images/tab-start.png)

### Upper Tab

![Upper Tab](docs/images/tab-upper.png)

### Lower Tab

![Lower Tab](docs/images/tab-lower.png)

### Bass Tab

![Bass Tab](docs/images/tab-bass.png)

### Sounds Tab

![Sounds Tab](docs/images/tab-sounds.png)

### Effects Tab

![Effects Tab](docs/images/tab-effects.png)

### Config Tab

![Config Tab](docs/images/tab-config.png)

## Functional Overview

Supported MIDI hardware and software sound modules:

- Deebach BlackBox
- Roland Integra7
- Ketron SD2
- MIDI GM
- Contact developer for additional module support.

### 1. Startup Flow

1. Open the `Start` tab.
2. Select MIDI input and output devices.
3. Select the active sound module using `To Modules`.
4. Load a panel (`.pnl`) and config (`.cfg`) if needed.
5. Confirm the panel and config labels match what you expect.
6. Move to `Upper`, `Lower`, or `Bass&Drums` and begin playing.

### 2. Tab Guide

#### Start

- Lists MIDI input and output devices and updates as devices connect or disconnect.
- Lets you choose the active sound module.
- Loads panel and config files.
- Checks config and panel pairing when loading, and can warn if the selected files do not belong together.

#### Upper / Lower / Bass&Drums

- These are the main performance tabs.
- Each tab contains voice button groups, volume controls, mute, and preset recall.
- `Upper` and `Lower` also include rotary controls.
- `Save` and `Save As` write the current panel (`.pnl`) to disk.
- The preset buttons are shared across all three keyboard tabs.

#### Sounds

- Assigns an instrument voice to the currently selected voice button.
- Select the target voice button on a keyboard tab before opening `Sounds`.
- The selected sound is sent on the button group's MIDI output channel so you can audition it.
- Use `To Upper`, `To Lower`, or `To Bass` to return to the performance tab.

#### Effects

- Edits per-voice MIDI effect values in real time.
- Select the target voice button on a keyboard tab before opening `Effects`.
- Effect changes are applied on the button group's MIDI output channel as you edit them.
- Use `To Upper`, `To Lower`, or `To Bass` to return to the performance tab.
- Current effect set:
  - VOL
  - EXP
  - REV
  - CHO
  - MOD
  - TIM
  - ATK
  - REL
  - BRI
  - PAN

#### Config

- Edits button group routing and behavior:
  - group name
  - MIDI In / Out channel
  - octave shift
  - solo split point for Upper / Lower
- `MIDI Reset` sends a controller reset on all 16 channels.
- `MIDI pass-through` controls whether channels not assigned to a button group are blocked or allowed through.
- **Default Effects Vol** and **Default Effects Bri** set starting values for new voice assignments (Sounds tab route and fresh panel initialization); they are saved in the config file (defaults: Vol 100, Bri 30).
- Config settings are global to the app and are separate from the currently loaded panel.

#### Hotkeys

- Lets you assign keyboard shortcuts for tabs, presets, and rotary controls.
- Available values are `A-Z`, `0-9`, and `(None)`.
- `Save` applies the current shortcut map and writes it to disk.
- `Cancel` restores the last applied shortcut map.
- Duplicate non-empty shortcuts are blocked.

#### Help

- Shows the embedded user guide.

#### Exit

- Exits the application.
- If panel-related changes are pending, the app may ask whether you want to save before quitting.

### 3. Voice Buttons

- Each voice button stores a MIDI instrument sound from the active device or module.
- Each button also stores its own MIDI effect values, so two buttons in the same group can sound and behave differently.

### 4. Voice Button Groups

- A voice button group is a logical set of related voice buttons.
- Group layout and routing are configured in the `Config` tab.
- Each group stores:
  - MIDI In and Out settings
  - octave shift
  - solo split settings for Upper or Lower
- Every group has a volume slider with Up / Down controls.
- Volume slider, Up/Down, and Mute send MIDI on the group's **current** MIDI Out channel (after loading or saving Config), not a channel frozen at app startup.
- Group volume changes are for live balancing and are not written back into the active voice button's stored instrument volume value.

#### Mute and Layering

- Muting is used for fast live layering control.
- When a group is muted, its volume slider and Up / Down controls are disabled.
- MIDI notes from the group's input are still transmitted to the group's output.
- This allows layered note routing while keeping the group's audible level at zero until needed.
- The first button group In channel forwards all MIDI messages directly to its Out channel by default.
- Layering to groups 2, 3, and 4 forwards Note On and Note Off using the active voice button's sound and effects.

### 5. Instrument Panels

- The system uses one instrument panel containing:
  - Upper keyboard panel
  - Lower keyboard panel
  - Bass&Drums keyboard panel
- Panels can be created by organ style, by setup, or by song.
- A panel can contain up to 96 voice buttons across 12 button groups.
- Panel save (`.pnl`) stores:
  - all voice button values
  - button group details
  - 7 preset configurations
- Button groups are color-coded by keyboard panel.

### 6. Presets

- There are 7 presets in total:
  - `Manual`
  - Presets `1` to `6`
- `Manual` is the default preset on startup.
- Each preset stores the active voice button in each button group across all three keyboard tabs.
- Each preset also stores per-button-group rotary snapshot values used during preset recall.
- Preset programming flow:
  - Select the target preset.
  - Adjust the active voice buttons as needed across panels.
  - Click `Preset Set`, then click the preset button again to write the snapshot.
- Changing the Upper or Lower rotary controls also updates the active preset's stored rotary state.
- Save the panel if you want preset changes to persist to disk.

### 7. Rotary

- Rotary controls are available on `Upper` and `Lower`.
- Rotary supports `Fast/Slow` and `Brake`.
- Switching between `Upper`, `Lower`, and `Bass&Drums` refreshes the rotary controls from saved manual state without sending extra MIDI on tab change.
- Upper and Lower manual rotary states are saved with the panel.
- Presets also store per-group rotary values for recall.

### 8. Config and Panel Save Behavior

- Config settings apply globally to the app and are separate from the currently loaded panel.
- Panels and configs are related: a panel stores the config name it expects.
- If you load a panel and config that do not match, the app can warn and let you abort or continue.
- While a config and panel mismatch is acknowledged, normal panel `Save` may stay disabled until the relationship is resolved.
- `Save As` can be used to write a new panel that matches the current config.
- If you change a sound module assignment inside a config that other panels depend on, saving that same config name may be blocked so older panels are not silently broken.

### 9. File Locations

AMidiOrgan stores user data under:

- `Documents/AMidiOrgan`

Important subfolders:

- `configs/` for `.cfg` files
- `panels/` for `.pnl` files
- `instruments/` for JSON instrument catalogs
- `configs/hotkeys.json` for keyboard shortcut bindings

### 10. Keyboard Shortcuts (Phase 1)

When the main window has focus:

| Action | Key |
|--------|-----|
| Preset 1–6 | `1`–`6` |
| Manual preset | `0` |
| Upper / Lower / Bass tab | A / S / D |
| Sounds tab | Z |
| Effects tab | X |
| Upper rotary Fast/Slow | F |
| Upper rotary Brake | B |
| Lower rotary Fast/Slow | G |
| Lower rotary Brake | N |

Upper and Lower rotary keys always target their respective manuals, even when another tab is selected.

### Editing Shortcuts

Use the `Hotkeys` tab (between `Config` and `Help`) to assign each command to a key from `A-Z` or `0-9`, or `(None)` for no mapping. `Save` writes `Documents/AMidiOrgan/configs/hotkeys.json` and applies the mapping; the app also loads that file on startup. If two or more commands share the same non-empty key, `Save` is blocked and a warning is shown. `Cancel` discards unsaved edits in the tab and restores the last applied bindings.

While a `TextEditor` or `ComboBox` has keyboard focus, including inside modal dialogs, global shortcuts are deferred so normal typing and selection still work.

Letter shortcuts may still fire when focus is on other controls, such as a plain button.



## 12. Build

### Prerequisites

- CMake 3.22 or newer
- JUCE source checkout (for example: `C:/JUCE` on Windows or `./.deps/JUCE` in this repo on macOS)
- macOS builds: Xcode + Command Line Tools

### Windows (Visual Studio generator)

From the repository root:

```powershell
cmake -S . -B build -DJUCE_ROOT="C:/JUCE"
cmake --build build --config Debug --target AMidiOrgan
```

Output executable:

- `build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe`

### Run (Windows)

From the repository root:

```powershell
# Run Debug build in a new process
Start-Process "build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe"

# Optional: run in the current terminal (foreground)
& "build/AMidiOrgan_artefacts/Debug/AMidiOrgan.exe"
```

To run Release:

```powershell
cmake --build build --config Release --target AMidiOrgan
Start-Process "build/AMidiOrgan_artefacts/Release/AMidiOrgan.exe"
```

### Test (Windows)

From the repository root:

```powershell
# Build the test executable
cmake --build build --config Debug --target AMidiOrganTests

# Run tests
ctest --test-dir build -C Debug --output-on-failure
```

### Continuous Integration (GitHub Actions)

- Workflow file: `.github/workflows/ci.yml`
- Triggers: push to `main`, pull requests targeting `main`, and manual dispatch.
- Platforms: `windows-latest` and `macos-latest`.
- Pipeline steps:
  - Configure CMake using a checked-out JUCE source tree.
  - Build `AMidiOrganTests`.
  - Run `ctest`.
  - Build `AMidiOrgan` (Debug build on both platforms).
- Current regression tests cover utility bounds, MIDI split/layer routing, preset/config persistence roundtrips, MIDI controller reset emission, shutdown-ownership crash paths, shortcut focus deferral (text fields vs global hotkeys), and hotkey duplicate detection rules.

### Recommended Manual UI Smoke Test

After a successful build, run this quick checklist (5-10 minutes):

1. Launch the app and open each tab once:
  - `Start`, `Upper`, `Lower`, `Bass&Drums`, `Sounds`, `Effects`, `Config`, `Help`.
2. On `Start`:
  - Load a config file.
  - Load a panel file.
  - Confirm panel/config labels update and mismatch coloring behaves as expected.
3. On `Upper`, `Lower`, and `Bass&Drums`:
  - Click several voice buttons and confirm active-state behavior.
  - Use panel `Save` and `Save As`, then reload the saved panel.
4. On `Sounds` and `Effects`:
  - Change a voice and a few effect values.
  - Return to a keyboard tab and confirm state is preserved.
5. On `Config`:
  - Change one mapping value, save, reload, and confirm it persists.
  - Click into a text field and type digits/letters; confirm **global shortcuts do not** change tabs or presets while typing. Click the tab bar or an empty area, then confirm shortcuts work again.
6. If MIDI hardware is connected:
  - Open/close MIDI input and output devices and confirm no crash/hang.

### macOS (Xcode generator)

From the repository root:

```bash
# Optional one-time JUCE checkout (matches CI source-based flow)
mkdir -p .deps
git clone --depth 1 https://github.com/juce-framework/JUCE.git .deps/JUCE

# Configure, build tests, run ctest, and build app
cmake -S . -B build-mac -G Xcode -DJUCE_ROOT="$PWD/.deps/JUCE"
cmake --build build-mac --config Debug --target AMidiOrganTests
ctest --test-dir build-mac -C Debug --output-on-failure
cmake --build build-mac --config Debug --target AMidiOrgan
```

Typical output app bundle:

- `build-mac/AMidiOrgan_artefacts/Debug/AMidiOrgan.app`

### macOS Quick Start (Mac mini)

Step-by-step command-line guide for a clean Mac (Makefile vs Xcode, `ctest` notes): [MacMinibuild.md](MacMinibuild.md). **Bash automation:** `scripts/mac-bootstrap.sh` (first-time + deps) and `scripts/mac-build.sh` (repeat builds); see MacMinibuild §9.

Use this as the repeatable baseline on a dedicated macOS build machine:

```bash
# 1) Clone and enter repo
git clone https://github.com/aminnie/AMidiOrganOrg.git
cd AMidiOrganOrg

# 2) (One time) Get JUCE source locally
mkdir -p .deps
git clone --depth 1 https://github.com/juce-framework/JUCE.git .deps/JUCE

# 3) Configure + build
cmake -S . -B build-mac -G Xcode -DJUCE_ROOT="$PWD/.deps/JUCE"
cmake --build build-mac --config Debug --target AMidiOrgan

# 4) Optional tests
cmake --build build-mac --config Debug --target AMidiOrganTests
ctest --test-dir build-mac -C Debug --output-on-failure

# 5) Run app bundle
open build-mac/AMidiOrgan_artefacts/Debug/AMidiOrgan.app
```

Recommended upkeep on a Mac mini:

- Run `git fetch --prune` periodically.
- Keep builds in `build-mac` only (avoid mixing generators in one build dir).
- If something looks stale, remove `build-mac` and reconfigure from scratch.

### Notes

- UI images are packaged from `assets/*.png` via `juce_add_binary_data(...)` in `CMakeLists.txt`.
- On macOS, `docs/` is copied into `AMidiOrgan.app/Contents/Resources/docs` during build so first-run data seeding works when launching the bundle outside the repo tree.
- On startup, the app seeds `Documents/AMidiOrgan` from `docs/` on first run, then ensures missing files under `configs/`, `instruments/` (JSON instrument catalogs), and `panels/` (`.pnl` panel files) are restored on subsequent runs (without overwriting existing user-edited files).
- On macOS, the current test executable is compiled but runtime execution is temporarily disabled in CTest due a shutdown-time crash; app build validation remains fully enabled.

### Asset Naming Contract

The following asset filenames are referenced by code through `BinaryData` symbols and should remain stable unless code and CMake are updated together:

- `assets/keyboard.png`
- `assets/icons8arrowdown32.png`
- `assets/icons8arrowup32.png`
- `assets/icons8arrowdown32click.png`
- `assets/icons8arrowup32click.png`


### 13. General

- Built using JUCE and C++.
- Application can be compiled for multiple operating systems and devices.
- Current CI validates Windows and MacOS builds.
- Touch panel is supported; mouse/keyboard navigation is optional.

### 14. Device Support

- MIDI keyboards:
  - Every button group supports MIDI keyboard input and/or shared MIDI In.
  - For dedicated solo keyboard input, set split value to `0` for MIDI Out.
- Sound modules:
  - Uses custom JSON sound-module device files.
  - Supports hardware/software modules that present as MIDI devices after module file is added.
- Displays:
  - Optimized for Waveshare 11.9" capacitive touch screen (1480x320):
    - [Waveshare 11.9" HDMI LCD](https://www.waveshare.com/11.9inch-hdmi-lcd.htm)
  - Application auto-centers on typical HD 15.6" displays.
  - Contact the developer for additional display requests.

### 15. Up Next

- Use JUCE to compile/test controller with Raspberry Pi
  - Goal: cost-effective standalone controller + display without requiring a PC.
- Add optional hardware support for selected buttons and sliders.

## Contact Details

- Anton Minnie: `a_minnie@hotmail.com`

