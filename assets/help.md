# AMidiOrgan User Guide

For the complete end-user operations manual (including packaged Windows/macOS installation), open `USER_MANUAL.md` included in the release package (same folder level as the app package contents).

## What This App Does
AMidiOrgan is a live-performance MIDI controller built with JUCE and C++.
It manages voice button groups across three keyboard panels, supports layering and split workflows,
and routes MIDI to connected hardware and software sound modules from a single interface.

The three performance panels are:

- `Upper`
- `Lower`
- `Bass&Drums`

Each panel uses the active configuration and panel data to determine which voices, routing, effects,
presets, and rotary states are available during a performance.

## Supported Modules (Current)

- Deebach BlackBox
- Roland Integra7
- Ketron SD2
- MIDI GM
- MidiView

Contact the developer for additional module support.

## Startup Flow (Recommended)

1. Open the `Start` tab.
2. Select MIDI input and output devices (selections are restored automatically on the next launch when the same ports are available).
3. Select the active sound module using `To Modules`.
4. Load a panel (`.pnl`) and config (`.cfg`) if needed, or create a fresh panel with `New Panel`.
5. Confirm the panel and config labels match what you expect.
6. Move to `Upper`, `Lower`, or `Bass&Drums` and start playing.

## Tab Guide

### Start

- Lists MIDI input and output devices and updates as devices connect or disconnect.
- Lets you choose the active sound module.
- Loads panel and config files.
- `New Panel` creates a new `.pnl` from the currently selected module using that module's first voice for all panel voice buttons.
- The create flow prompts for a file name, blocks duplicates, then saves and auto-loads the new panel.
- Checks config/panel pairing when loading and can warn if the selected files do not belong together.
- Includes a Start-row `Exit` button (same behavior as `Exit` tab).
- Status lines are shown as `Panel: <file>` and `Config: <file>`.

### Upper / Lower / Bass&Drums

- These are the main performance tabs.
- Each tab contains voice button groups, volume controls, mute, and preset recall.
- `Upper` and `Lower` also include rotary controls.
- `Save` and `Save As` write the current panel (`.pnl`) to disk.
- The preset buttons are shared across all three keyboard tabs.

### Sounds

- Assigns an instrument voice to the currently selected voice button.
- Select the target voice button on a keyboard tab before opening `Sounds`.
- Browse in two levels inside the `Sounds for Voice Button` area:
  - Level 1: voice category buttons
  - Level 2: voice buttons in the selected category
- Clicking a voice sends MSB/LSB/PC immediately on the button group's MIDI output channel for audition.
- `Back` returns to categories. `Prev` / `Next` paginate when lists exceed visible space.
- Use `To Upper`, `To Lower`, or `To Bass` to return to the performance tab.

### Effects

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

### Config

- Edits button group routing and behavior:
  - group name
  - MIDI In / Out channel
  - octave shift
  - solo split point for Upper / Lower
- `MIDI Reset` sends a controller reset on all 16 channels.
- `MIDI In Passthru` is a global input-channel filter, not a per-device or per-output setting.
- When it is **ON**, all incoming MIDI channels are allowed through.
- When it is **OFF**, only MIDI input channels assigned to button groups are allowed through; other incoming channels are blocked.
- MIDI channel `16` is still allowed for controller-style traffic even when pass-through is off.
- `Preset MIDI PC` lets external Program Change trigger `Next preset`:
  - Input Channel (`1..16`, default `16`)
  - PC Value (`0..127`, default `0`)
  - Matching Program Change triggers preset-next using the same behavior as the `Next preset` hotkey.
  - Matching Program Change is consumed (not forwarded to outputs) and this trigger check ignores pass-through filtering.
- `SysEx Through` is configured per button group:
  - Enable `SysEx Through` for the group.
  - Pick `SysEx Input` as the source MIDI input device identifier.
  - Matching SysEx fans out to all matching groups and dedupes per output device.
- `UI Profile` changes apply live across Start/Upper/Lower/Bass/Config/Sounds/Effects/Hotkeys/Monitor and resize the app window to the selected fixed profile size.
- Config settings are global to the app and are separate from the currently loaded panel.

### Hotkeys

- Lets you assign keyboard shortcuts for tabs (including **Player**), presets, rotary controls, **and Player Start / Stop**.
- Available values are `A-Z`, `0-9`, and `(None)`.
- `Save` applies the current shortcut map and writes it to disk.
- `Cancel` restores the last applied shortcut map.
- Duplicate non-empty shortcuts are blocked.
- Defaults include **Player tab** on **`l`** (letter L) and **Player Start / Stop** on **`p`**.

### Monitor

- Separates routed **IN** vs **OUT** histories (MIDI **In / Out Monitor** panes).
- Each pane retains up to **50** lines—reaching this limit **automatically disables** capture; use **Clear** and enable again after reviewing.
- `Enabled`/`Disabled`, `Clear`, optional virtual octave keyboard aligned with Solo split (`C4=60 convention` elsewhere).

### Player

- Targets the **currently selected Sound Module** outputs for `.mid` playback (orthogonal to Upper/Lower/Bass group mutes; strip **M**/**S** handle playback gating; **P/Along** can override mute gating).
- Channel strips (`Ch 1..16`) hold per-channel Program/Bank and effect values.
- Selecting a channel strip sends Program/Bank and Effects immediately on that channel.
- **Load MIDI** / **Import MIDI**, **Start**/**Stop**, **Continue**, **Bar** start, **P/Along** (Play Along, default OFF, session-only), **Key +/-** transpose (`-6..+6`), **Tempo** override (`0` follows file map), bar/beat transport readout while playing.
- **Reset GM** restores strip voices from loaded MIDI Program Changes using the **first event per channel** and marks profile save actions dirty.
- During playback, Program Change on configured Player channels is replaced by strip `MSB/LSB/PC`.
- `Enable Program Change remap` applies lookup remapping to remaining Program Change traffic.
- `Scale file CCs with Player strip` is optional (default OFF):
  - Applies only to channels configured from Player tab.
  - Uses `merged = clamp(round(fileValue * stripValue / 127.0))`.
  - Merges `CC1`, `CC7`, `CC11`, `CC71`, `CC72`, `CC73`, `CC74`, `CC91`, `CC93`.
  - Keeps `CC10` (`Pan`) passthrough (not scaled).
- Player profile workflow:
  - `Apply Profile` dropdown lists saved profiles for the loaded MIDI file.
  - `Save Profile` updates active profile values (emphasizes unsaved edits when dirty).
  - If **Sound Module** changed since that profile was loaded, `Save Profile` warns—prefer `Save Profile As` for a new entry, or **Continue** to overwrite; `Save Profile As` skips this warning.
  - `Save Profile As` creates a new profile.
  - `Revert Profile` reloads the active profile.
  - `Load MIDI Profile` loads the selected profile's saved MIDI path, then applies that profile state.
  - **Manage Profiles…** trims/renames list entries offline.
  - `Load MIDI Profile` and **Manage Profiles…** include a live text filter for quickly narrowing the profile list.
  - Dirty profile switches prompt to save/discard/cancel as needed.
  - **Bar** and **P/Along** are session controls and are not persisted in profile files.
- Profiles are saved as sidecar files under `Documents/AMidiOrgan/configs/player_profiles/` and do not rewrite the source `.mid` file.
- If a profile points to a missing MIDI file, Player reports this in status and leaves the current state unchanged.

### Help

- Shows this guide.

### Exit

- Exits the application.
- If panel-related changes are pending, the app may ask whether you want to save before quitting.
- Start-tab `Exit` button routes through the same exit flow.

## Performance Behavior

### Voice Button Groups

- A voice button group is a logical set of related voice buttons.
- Each group uses routing and performance settings from the active config.
- Every group has a volume slider with Up / Down controls.
- Groups can be muted and unmuted during performance.

### Mute and Layering

- Muting is used for fast live layering control.
- When a group is muted, its volume slider and Up / Down controls are disabled.
- While muted, new MIDI Note On events are blocked for that group's output channel (hard mute).
- On mute, the app sends cleanup controllers to that output channel:
  - Sustain Off (`CC64=0`)
  - All Notes Off (`CC123=0`)
- Mute does not send `CC7=0`; channel volume is restored by the normal volume path on unmute.

### Presets

- There are 13 presets in total:
  - `Manual`
  - Presets `1` to `12`
- `Manual` is the default preset on startup.
- The preset row shows six numbered presets at a time:
  - Default bank: `Preset 1` to `Preset 6`
  - Alternate bank: `Preset 7` to `Preset 12`
- `Next` cycles numbered presets as `1 -> ... -> 6 -> 7 -> ... -> 12 -> 1`.
- If `Manual` is active, `Next` recalls the first preset in the currently displayed bank (`1` or `7`).
- Each preset stores the active voice button in each button group across all three keyboard tabs.
- Each preset also stores per-button-group rotary snapshot values used during preset recall.
- Preset programming flow:
  1. Select the target preset.
  2. Adjust the active voice buttons as needed.
  3. Click `Preset Set`.
  4. Click the preset button again to write the snapshot.
- Save the panel if you want preset changes to persist to disk.

### Rotary

- Rotary controls are available on `Upper` and `Lower`.
- Rotary supports `Fast/Slow` and `Brake`.
- Switching between `Upper`, `Lower`, and `Bass&Drums` refreshes the rotary controls from saved manual state.
- Upper and Lower manual rotary states are saved with the panel.
- Presets also store per-group rotary values for recall.

## Keyboard Shortcuts

When the main window has focus, the default shortcuts are:

| Action | Key |
|--------|-----|
| Upper tab | `A` |
| Lower tab | `S` |
| Bass tab | `D` |
| Player tab | `L` |
| Sounds tab | `Z` |
| Effects tab | `X` |
| Monitor tab | `M` |
| Manual preset | `0` |
| Preset 1 | `1` |
| Preset 2 | `2` |
| Preset 3 | `3` |
| Preset 4 | `4` |
| Preset 5 | `5` |
| Preset 6 | `6` |
| Next preset | `7` |
| Upper rotary Fast/Slow | `F` |
| Upper rotary Brake | `B` |
| Lower rotary Fast/Slow | `G` |
| Lower rotary Brake | `N` |
| Player Start / Stop | `P` |

Shortcut notes:

- Upper and Lower rotary shortcuts always target their respective manuals, even when another tab is selected.
- Sounds and Effects shortcuts are intended for use after a target voice button has been selected on a keyboard tab.
- While a `TextEditor`, `ComboBox`, or modal dialog has keyboard focus, global shortcuts are deferred so normal typing continues to work.
- Some plain controls may still allow letter shortcuts when they have focus.
- Preset recall hotkeys are currently defined for `Manual` and `Preset 1..6`; dedicated hotkeys for `Preset 7..12` are not defined.

### Customizing Shortcuts

- Open the `Hotkeys` tab to edit shortcuts.
- `Save` writes the current map to:
  - `Documents/AMidiOrgan/configs/hotkeys.json`
- That file is loaded on startup when it is present.
- If the file is missing, the app uses the built-in defaults shown above.

## Saving, Loading, and File Locations

### User Data Location

AMidiOrgan stores user data under:

- `Documents/AMidiOrgan`

Important subfolders:

- `configs/` for `.cfg` files
- `panels/` for `.pnl` files
- `instruments/` for JSON instrument catalogs
- `configs/hotkeys.json` for keyboard shortcut bindings
- `.cfg` group entries also persist `sysexThrough` and `sysexInputIdentifier`.

### What Each File Stores

- `.cfg`
  - MIDI routing, group naming, split points, pass-through behavior, preset-next Program Change trigger, and related configuration
- `.pnl`
  - voice button assignments
  - button group details
  - presets (`Manual` + `Preset 1..12`)
  - panel-level saved state such as manual rotary values
- Panel load remains backward compatible with older `.pnl` files that contain only `Manual + Preset 1..6`; missing `Preset 7..12` values are initialized to defaults.
- `hotkeys.json`
  - customized keyboard shortcuts

### Pairing and Save Behavior

- Panels and configs are related: a panel stores the config name it expects.
- If you load a panel and config that do not match, the app can warn and let you abort or continue.
- While a config/panel mismatch is acknowledged, normal panel `Save` may remain disabled until the relationship is resolved.
- `Save As` can be used to write a new panel that matches the current config.
- If you change a sound module assignment inside a config that other panels depend on, saving that same config name may be blocked so older panels are not silently broken.

### Practical Save Advice

- Use `Save` regularly after meaningful panel edits.
- Use `Save As` before major changes so you keep a fallback version.
- After saving, reload once to confirm the expected state was written.

## Window Move Tip

- Click and drag in empty space on the top tab bar to move the app window.
- This preserves the slim title-bar layout while still letting you reposition the window.

## MIDI Troubleshooting

### No sound

- Verify that the MIDI output device is connected and selected.
- Check the mute state and output channel on the active voice button group.
- Confirm that the selected module matches the actual sound source.
- Confirm that the intended voice button is active.

### Notes not routing correctly

- Check `Config` page MIDI In / Out channel mapping.
- Verify split note settings for solo groups.
- Confirm the intended config file is loaded.
- For SysEx, verify `SysEx Through` is enabled and `SysEx Input` matches the sending input device.

### Unexpected voice or effect

- Confirm the active preset and active voice button.
- Re-open `Sounds` or `Effects` and verify the assigned values.
- If using custom hotkeys, confirm the current `hotkeys.json` matches what you expect.

## Pre-Performance Checklist (2-3 Minutes)

- Open each main tab once: `Start`, `Upper`, `Lower`, `Bass&Drums`, `Sounds`, `Effects`, `Config`, `Hotkeys`, `Help`.
- Verify MIDI devices are present and stable.
- Trigger notes on `Upper`, `Lower`, and `Bass&Drums`.
- Check mute states and voice button group volume levels.
- Recall at least one saved preset and confirm that the expected voices load.
- If you use custom hotkeys, test one tab shortcut and one preset shortcut.
- Save the current panel snapshot if you changed anything.

## Contact Details

- Anton Minnie, email: `a_minnie@hotmail.com`
