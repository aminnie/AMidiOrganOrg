# AGENTS.md

## Purpose
This repository contains `AMidiOrgan`, a JUCE-based C++ MIDI organ controller UI.
Use this file as the default guidance for AI/code agents working in this project.
For canonical build/run commands, use the `Build` section in `README.md`.

## Project Layout
- `Main.cpp`: JUCE application bootstrap and top-level window setup.
- `AMidiControl.h`: primary UI and orchestration layer (tabs/pages, routing, panel/config load-save).
- `AMidiDevices.h`: MIDI device discovery, routing, note rewrite/layer logic.
- `AMidiInstruments.h`: instrument model + JSON instrument catalog loader.
- `AMidiButtons.h`: custom button types holding panel/instrument metadata.
- `AMidiRotors.h`: rotary speed ramp threads and MIDI CC emission.
- `AMidiUtils.h`: shared constants, globals, enums, and utility helpers.
- `AMidiHotkeys.h`: hotkey binding persistence (`configs/hotkeys.json`) and Hotkeys tab UI.
- `README.md`: product behavior/features and user-facing notes.
- `assets/`: PNG UI images bundled at build time via `juce_add_binary_data(...)`; keep required filenames stable unless code/CMake are updated together.

## Tech Context
- Language: C++ (JUCE application code, mostly header-heavy).
- Framework: JUCE (desktop GUI + MIDI + ValueTree persistence).
- Runtime model: event-driven UI thread + MIDI callbacks + a few worker threads.

## Working Rules For Agents
- Keep changes minimal and local; avoid broad refactors in `AMidiControl.h` unless asked.
- Preserve existing MIDI behavior unless the task is explicitly about routing/effects changes.
- Prefer explicit bounds checks for all MIDI channel/index lookups.
- When touching persistence, keep ValueTree property names backward compatible.
- Do not silently change on-disk locations under user Documents (`AMidiOrgan/...`).

## User Documents layout (`Documents/AMidiOrgan`)
- **`configs/`** — `.cfg` configuration files (see `AppState::configdir`); also **`hotkeys.json`** (keyboard shortcut map) written by the Hotkeys tab, and **`midi_sticky_devices.json`** (last MIDI In/Out selections on the Start tab).
- **`panels/`** — `.pnl` instrument panel files (see `AppState::paneldir`); canonical read/write target; older installs may still load from legacy `instruments/` or module-named folders via fallbacks.
- **`instruments/`** — JSON instrument catalogs only (not `.pnl`).

## Ownership And Lifetime Rules (Critical)
- JUCE singleton instances from `juce_DeclareSingleton` are non-owning when accessed via `getInstance()`.
- Do **not** store singleton pointers in owning smart pointers (`std::unique_ptr`) unless ownership transfer is intentional and safe.
- Never `delete` references or stack-owned members.
- Prefer references or raw non-owning pointers for singleton access in components/pages.

## Threading And MIDI Safety
- `handleIncomingMidiMessage` can run on a MIDI thread; avoid expensive UI work there.
- Use async/message-thread handoff when updating visible components from callback contexts.
- Keep MIDI send paths (`sendToOutputs`, note rewrite) low-latency and allocation-light.

## Data And Domain Conventions
- MIDI channels are treated as `1..16`; many arrays are intentionally sized to `17` for direct indexing.
- Voice buttons represent instruments plus per-button effect values.
- Button groups map input channels to one or more output layers.
- Presets capture active voice button selection + mute/rotary state by button group.

## State Migration Progress
- Completed slices: `Slice 1`, `Slice 2`, `Slice 3A`, `Slice 3B`, `Slice 3C`, `Slice 3D`.
- Current migration posture: most UI and instrument/panel flows use explicit `appState`; legacy alias globals remain only in a small number of persistence helpers.
- Next recommended slice (`3E`):
  - finish `ConfigPage` persistence helper conversion in `AMidiControl.h` (`saveConfigs` / `loadConfigs` internals) where alias globals are still referenced for path/log values.
  - preserve existing on-disk file formats and paths while replacing alias reads/writes with explicit `appState` fields.

## Validation Checklist After Changes
- Build succeeds for the active JUCE target.
- App launches and tab navigation works.
- MIDI input/output device list can open/close devices without crash.
- Voice selection sends MSB/LSB/PC correctly.
- Effects sliders emit expected CC values.
- Save/reload of config and panel files remains functional.

## UI controls
- Prefer **JUCE** components (`juce::` / modules such as `juce_gui_basics`) for UI where they offer a suitable control—cross-platform behavior and theming stay consistent. Do not reach for OS-native widgets when a JUCE equivalent exists unless there is a clear, documented reason.

## Style Notes
- Match existing naming and formatting style in touched files.
- Add short comments only when logic is non-obvious (routing, split behavior, threading edges).
- Keep logs actionable; do not spam logs inside tight loops unless debugging.
