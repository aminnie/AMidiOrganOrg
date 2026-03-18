# TODO - AMidiOrgan Review Follow-up

## Environment Note
- JUCE is locally installed at `c:\JUCE` and should be used for later compile/verification passes.

## Priority 0 (Critical - memory safety / ownership)
- [ ] Remove invalid `delete&` calls on non-heap members:
  - `AMidiButtons.h`: `delete& instrument;`
  - `AMidiInstruments.h`: `delete& jsonInstrumentData;`
  - `AMidiControl.h`: `delete &jsonPanel;`
- [ ] Replace singleton ownership misuse (`std::unique_ptr` initialized from `getInstance()`) with non-owning references or raw pointers where appropriate:
  - `AMidiControl.h` (multiple pages/components, e.g. `MidiStartPage`, `ConfigPage`, and others)
  - `AMidiInstruments.h` (`instrumentmodules`)
  - `AMidiRotors.h` (`mididevices`)
- [ ] Verify no singleton is deleted by component teardown/destructor chains.

## Priority 1 (High - behavior correctness)
- [ ] Fix note velocity handling in layered note rewrite path:
  - `AMidiDevices.h`: `rewriteSendNoteMessage()` currently uses `getControllerValue()` instead of note velocity.
- [ ] Add null-check before forwarding duplicate output to MidiView monitor:
  - `AMidiDevices.h`: `midiOutputs[midiviewidx]->outDevice->sendMessageNow(msg);`

## Priority 2 (Medium - logic / edge cases)
- [ ] Fix `PanelPresets::Preset::getPresetLoaded()` to return `presetloaded`.
- [ ] Correct preset bounds check in `createPreset()`:
  - currently allows `presetid > 7` logic with `numberpresets = 7`; should enforce valid range `0..numberpresets-1`.
- [ ] Audit other array/index accesses for strict bounds checks (MIDI channels and group/button indices).

## Priority 3 (Stability / maintainability)
- [ ] Reduce broad mutable global state in `AMidiUtils.h` over time (prefer encapsulated state where feasible).
- [ ] Add lightweight regression checks for:
  - MIDI in/out open/close
  - Voice MSB/LSB/PC send path
  - Effect CC sends
  - Panel/config save+reload

## Later Compile/Verification Pass
- [ ] Build on Windows using local JUCE at `c:\JUCE`.
- [ ] Run manual smoke test across tabs (Start, Upper, Lower, Bass, Voices, Effects, Config, Help).
- [ ] Exercise MIDI routing/layering and preset recall after fixes.
