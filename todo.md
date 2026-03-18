# TODO - AMidiOrgan Review Follow-up

## Environment Note
- JUCE is locally installed at `c:\JUCE` and should be used for later compile/verification passes.

## Current Status Snapshot (Mar 2026)
- [x] Cross-platform CMake scaffold is in place and builds on Windows.
- [x] CI added (`.github/workflows/ci.yml`) for Windows + macOS configure/build/test.
- [x] Debug and Release builds currently compile cleanly (warning cleanup pass completed).
- [x] Automated test portfolio expanded with utility, routing, preset bounds, AppState, and integration-style flows.
- [x] Manual UI smoke checklist added to `README.md`.

## Priority 0 (Critical - memory safety / ownership)
- [x] Remove invalid `delete&` calls on non-heap members:
  - `AMidiButtons.h`: `delete& instrument;`
  - `AMidiInstruments.h`: `delete& jsonInstrumentData;`
  - `AMidiControl.h`: `delete &jsonPanel;`
- [x] Replace singleton ownership misuse (`std::unique_ptr` initialized from `getInstance()`) with non-owning references or raw pointers where appropriate:
  - `AMidiControl.h` (multiple pages/components, e.g. `MidiStartPage`, `ConfigPage`, and others)
  - `AMidiInstruments.h` (`instrumentmodules`)
  - `AMidiRotors.h` (`mididevices`)
- [ ] Verify no singleton is deleted by component teardown/destructor chains.

## Priority 1 (High - behavior correctness)
- [x] Fix note velocity handling in layered note rewrite path:
  - `AMidiDevices.h`: `rewriteSendNoteMessage()` currently uses `getControllerValue()` instead of note velocity.
- [x] Add null-check before forwarding duplicate output to MidiView monitor:
  - `AMidiDevices.h`: `midiOutputs[midiviewidx]->outDevice->sendMessageNow(msg);`

## Priority 2 (Medium - logic / edge cases)
- [x] Fix `PanelPresets::Preset::getPresetLoaded()` to return `presetloaded`.
- [x] Correct preset bounds check in `createPreset()`:
  - currently allows `presetid > 7` logic with `numberpresets = 7`; should enforce valid range `0..numberpresets-1`.
- [~] Audit other array/index accesses for strict bounds checks (MIDI channels and group/button indices).
  - Added targeted guards/tests for MIDI channel transpose/split/layer and preset bounds.
  - Continue broader audit in untouched legacy paths.

## Priority 3 (Stability / maintainability)
- [~] Reduce broad mutable global state in `AMidiUtils.h` over time (prefer encapsulated state where feasible).
  - Slices 1, 2, 3A, 3B, 3C, and 3D completed.
  - Remaining explicit alias/global references are concentrated in `ConfigPage` persistence internals (`saveConfigs` / `loadConfigs` path handling and logging).
- [~] Add lightweight regression checks for:
  - MIDI in/out open/close
  - Voice MSB/LSB/PC send path
  - Effect CC sends
  - Panel/config save+reload
  - Completed in tests: panel/config roundtrip, preset persistence, split/layer routing edges.
  - Still pending: automated MIDI device open/close coverage and richer effect-path assertions.

## State Migration Progress
- [x] **Slice 1**: Introduced `AppState` and alias-backward compatibility.
- [x] **Slice 2**: Migrated `InstrumentModules` / `MidiInstruments` core reads/writes to `AppState`.
- [x] **Slice 3A**: Migrated `ConfigPage` + `MidiStartPage` UI flows to explicit `appState`.
- [x] **Slice 3B**: Migrated `KeyboardPanelPage` state references to explicit `appState`.
- [x] **Slice 3C**: Migrated `InstrumentPanel` panel/config metadata and reload logic.
- [x] **Slice 3D**: Warning-oriented cleanup of shadowed legacy names tied to migration.
- [ ] **Next slice (proposed 3E)**: Finish explicit `appState` usage in `ConfigPage` persistence helpers:
  - `saveConfigs(...)` currently uses alias globals in file path and logs (`configdir`, `configfname`).
  - `loadConfigs(...)` path assembly still references alias globals in some paths.
  - Goal: remove these remaining alias reads/writes from `AMidiControl.h`.

## Later Compile/Verification Pass
- [x] Build on Windows using local JUCE at `c:\JUCE`.
- [x] Run manual smoke test across tabs (Start, Upper, Lower, Bass, Voices, Effects, Config, Help).
- [x] Exercise MIDI routing/layering and preset recall after fixes.
