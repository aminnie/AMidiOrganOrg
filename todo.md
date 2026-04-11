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
  - Slice 3E completed the `ConfigPage` persistence helper conversion to explicit `appState` usage.
  - Remaining alias/global references are now in non-persistence legacy paths and can be migrated incrementally.
- [~] Add lightweight regression checks for:
  - MIDI in/out open/close
  - Voice MSB/LSB/PC send path
  - Effect CC sends
  - Panel/config save+reload
  - Completed in tests: panel/config roundtrip, preset persistence, split/layer routing edges.
  - Still pending: richer effect-path assertions.

## State Migration Progress
- [x] **Slice 1**: Introduced `AppState` and alias-backward compatibility.
- [x] **Slice 2**: Migrated `InstrumentModules` / `MidiInstruments` core reads/writes to `AppState`.
- [x] **Slice 3A**: Migrated `ConfigPage` + `MidiStartPage` UI flows to explicit `appState`.
- [x] **Slice 3B**: Migrated `KeyboardPanelPage` state references to explicit `appState`.
- [x] **Slice 3C**: Migrated `InstrumentPanel` panel/config metadata and reload logic.
- [x] **Slice 3D**: Warning-oriented cleanup of shadowed legacy names tied to migration.
- [x] **Slice 3E**: Finished explicit `appState` usage in `ConfigPage` persistence helpers:
  - `saveConfigs(...)` and `loadConfigs(...)` now use `appState` path/log fields instead of alias globals.
  - Preserved existing on-disk file formats and locations.

## Later Compile/Verification Pass
- [x] Build on Windows using local JUCE at `c:\JUCE`.
- [x] Run manual smoke test across tabs (Start, Upper, Lower, Bass, Voices, Effects, Config, Help).
- [x] Exercise MIDI routing/layering and preset recall after fixes.

## Start tab: sticky MIDI ports (future)

- [ ] **Forget saved ports**: UI control on Start (or settings) to clear remembered MIDI In/Out without editing `configs/midi_sticky_devices.json` manually.
- [ ] **Separate profiles**: Named profiles for different rigs (e.g. home vs stage), each with its own saved port set; pick a profile instead of a single global sticky file.

## SysEx routing UI management (phase-2 candidate)

Current status:
- [x] Removed `configs/midi_sysex_routes.json` backend and associated route-management logic.

### Go/No-Go checklist (compact)

Go now with Config-section UI if all are true:
- [ ] JSON-only editing has caused repeated friction in real usage.
- [ ] At least two rigs have been tested and need quicker route changes.
- [ ] Existing SysEx backend is stable across startup, reconnect, and route refresh cases.

No-Go for standalone tab (stay lightweight) if any are true:
- [ ] SysEx route changes are infrequent.
- [ ] Config page can absorb a small "SysEx Routes" section without crowding.
- [ ] Team prefers minimal UI scope until more field feedback is collected.

### Phase 1 (preferred): Config subsection
- [ ] Add compact CRUD rows in Config: input identifier + output identifier/module target.
- [ ] Validate on save: non-empty input, one destination required, no duplicate input keys.
- [ ] Reuse existing dirty/save/cancel behavior from Config workflows.
- [ ] Show unresolved-route status when target devices are unavailable.

### Phase 2: Promote to dedicated tab only if needed
- [ ] Promote only when Config becomes crowded or SysEx editing becomes frequent.
- [ ] Keep same JSON schema + resolver path to avoid migration risk.
- [ ] Add optional test route helper/log action after phase-1 adoption feedback.

## Keyboard shortcuts (future)

Phase 1 bindings and focus behavior are documented in `README.md`. Optional later work:

- [ ] User-editable bindings and a shortcut map file under `Documents/AMidiOrgan` (e.g. JSON).
- [ ] Config UI: assign, clear, restore defaults; validate conflicts across commands.
- [ ] Hotkeys for tabs not yet covered (e.g. Start, Config, Help) and policy for Exit / app quit.
- [ ] Per–button-group mute shortcuts (12 groups).
- [ ] Broader “shortcuts enabled” toggle or conflict editor if global vs. control focus remains awkward.

## UI polish roadmap (new)

### Polish Pass 1 (implemented)
- [x] Remove overlapping keyboard-tab `Exit` control near `Save` / `Save As`.
- [x] Standardize Presets row controls so `Set` and `Next` use the same width and visual weight.
- [x] Add contextual tooltips for key actions (`Set`, `Next`, `Save`, `Save As`, and tab quick-navigation buttons).
- [x] Improve file-status readability by adding status-label scaling and explicit status text/tooltips for panel/config filenames.

### Polish Pass 2 (proposal)
- [ ] Normalize horizontal spacing/gutters across all action rows (Start, Keyboard, Sounds, Effects, Config).
- [ ] Standardize section heading contrast and label font hierarchy for faster visual scan.
- [ ] Group Hotkeys rows visually by function clusters (tabs, presets, rotary) without changing bindings.
- [ ] Add consistent disabled-state tooltip language across guarded tabs/actions.

## Two-screen-size support (proposal)

Target profiles:
- Current: `1480 x 320`
- Large: `2560 x 720`

Guiding decision:
- [ ] Support exactly two named UI layouts first; do not expose freeform resize or independent X/Y stretching in the initial implementation.

### Phase 1: Window/profile infrastructure
- [ ] Add shared UI profile definitions for `1480 x 320` and `2560 x 720`.
- [ ] Centralize window/profile selection logic in `Main.cpp` and `AMidiControl.h` so the app can open in either supported size.
- [ ] Keep the current size as the default profile until the large profile is visually validated.
- [ ] Optionally plan later persistence of the last-used profile, but keep it out of the first implementation pass.

### Phase 2: Shared layout metrics
- [ ] Introduce a shared `UiMetrics`/layout-profile struct for margins, gutters, button sizes, label heights, slider sizes, tab depth, and font scaling.
- [ ] Replace new magic numbers with metrics helpers rather than adding more page-local literals.
- [ ] Use the current layout as the baseline reference, then define a tuned large-profile metric set instead of mathematically stretching everything.

### Phase 3: Move layout into `resized()`
- [ ] Refactor targeted pages so constructors create controls and `resized()` computes bounds from active metrics.
- [ ] Avoid relying on transform-only scaling for the `2560 x 720` profile because width/height growth is not proportional to the current layout.
- [ ] Keep behavior, routing, and persistence unchanged while moving geometry logic.

### Phase 4: Convert pages in recommended order
- [ ] Convert `MidiStartPage` first as the proving ground for profile-driven layout.
- [ ] Convert `VoicesPage` next and validate browser/group/action-row spacing at both sizes.
- [ ] Convert `EffectsPage` next and validate rotary control sizing, text boxes, and route/cancel controls at both sizes.
- [ ] Convert `KeyboardPanelPage` after the pattern is stable; expect this to be the largest part of the work due to dense hard-coded group/button geometry.
- [ ] Convert `ConfigPage` after the keyboard layout approach is proven.
- [ ] Leave `MonitorPage`, `HelpPage`, and `HotkeysPage` mostly intact unless profile-specific tuning is actually needed.

### Phase 5: Large-profile tuning
- [ ] Tune the `2560 x 720` profile intentionally for larger controls, fonts, gutters, and group spacing so it feels designed rather than stretched.
- [ ] Decide how extra vertical space should be used on the large profile instead of merely centering the old 320px-tall layout.
- [ ] Review status/file labels, tab-bar depth, quick-navigation buttons, and footer alignment for large-screen readability.

### Phase 6: Validation
- [ ] Verify both supported profiles launch correctly and preserve the same tab order, workflows, and MIDI behavior.
- [ ] Manually smoke-test Start, Upper, Lower, Bass&Drums, Sounds, Effects, Config, Hotkeys, Monitor, and Help at both sizes.
- [ ] Validate combo boxes, popup menus, bubble messages, viewport pages, and window-drag behavior in both profiles.
- [ ] Confirm panel/config save-reload, voice selection, effect sends, and preset flows remain unchanged after layout refactors.

### Phase 7: Go/no-go checkpoints
- [ ] After `MidiStartPage`, decide whether the metrics/profile approach is clean enough to continue.
- [ ] After `VoicesPage` and `EffectsPage`, decide whether the large profile still looks intentional without page-specific hacks.
- [ ] Before starting `KeyboardPanelPage`, confirm the project is worth the remaining effort; this page will likely determine the real total cost.

### Effort and risk note
- [ ] Expect low effort for window/profile shell changes alone, but those changes are not enough to deliver a good `2560 x 720` UI by themselves.
- [ ] Expect medium effort for the shared metrics infrastructure plus conversion of `MidiStartPage`, `VoicesPage`, and `EffectsPage`; this is the best prototype milestone.
- [ ] Expect high effort for full delivery because `KeyboardPanelPage` contains the densest hard-coded geometry and will likely dominate the schedule.
- [ ] Primary risk: the large profile can devolve into page-specific exceptions unless the metrics/resized pattern stays disciplined.
- [ ] Recommended stop point: if `VoicesPage` and `EffectsPage` still need too many one-off adjustments, reconsider whether a full second layout is worth completing.

### Recommended first milestone
- [ ] Implement only the profile shell plus shared `UiMetrics` infrastructure first.
- [ ] Convert `MidiStartPage`, `VoicesPage`, and `EffectsPage` to metrics-driven `resized()` layout.
- [ ] Defer `KeyboardPanelPage` and `ConfigPage` until the first three pages prove the approach is maintainable.
- [ ] Treat the milestone as successful only if both `1480 x 320` and `2560 x 720` feel intentional without blurry scaling or page-specific hacks.
- [ ] Re-evaluate scope before continuing into `KeyboardPanelPage`.
