# AMidiOrgan End-User Manual

## 1) What AMidiOrgan Does

AMidiOrgan is a live MIDI performance controller for keyboard players who use multiple sound modules or software MIDI targets.

You use it to:

- Select and switch voices quickly during performance
- Layer/split keyboard sounds by MIDI channel and button group
- Edit and store per-voice effect settings
- Save and recall live presets
- Monitor outgoing MIDI traffic for troubleshooting

The three performance manuals are:

- `Upper`
- `Lower`
- `Bass&Drums`

---

## 2) Before You Start

### Supported Runtime Platforms

- Windows (packaged executable)
- macOS (packaged `.app`)

### What You Need

- At least one MIDI input device (keyboard/controller)
- At least one MIDI output target (sound module, USB MIDI device, software MIDI destination)
- AMidiOrgan packaged release from your distributor/team

---

## 3) Installation and First Launch

## Windows (Packaged Executable)

1. Download the AMidiOrgan release package.
2. Extract/unzip it to a folder you can access easily (for example `C:\AMidiOrgan`).
3. Open the extracted folder and launch `AMidiOrgan.exe`.
4. If Windows SmartScreen appears, select **More info** then **Run anyway** (only if your source is trusted).
5. On first launch, allow any requested local permissions.

Expected result:

- AMidiOrgan opens to the main window and shows tabs (`Start`, `Upper`, `Lower`, `Bass&Drums`, and others).

## macOS (Packaged App)

1. Download the AMidiOrgan release package.
2. Extract/unzip it.
3. Move `AMidiOrgan.app` to `Applications` (recommended).
4. Launch the app:
   - If macOS blocks first launch, right-click the app, choose **Open**, then confirm.
5. If prompted for security/permissions, allow as needed.

Expected result:

- AMidiOrgan opens and the tab bar is visible.

## First-Run Data Folder

AMidiOrgan stores user files under:

- `Documents/AMidiOrgan`

Key subfolders/files:

- `configs/` (`.cfg` files, `hotkeys.json`, sticky device/session files)
- `panels/` (`.pnl` files)
- `instruments/` (module definition JSON files)

---

## 4) Hardware and Device Setup

1. Connect all MIDI keyboards/controllers and sound modules.
2. Power on devices before starting AMidiOrgan when possible.
3. Open AMidiOrgan and go to `Start`.
4. Confirm your MIDI devices appear in the Input/Output lists.
5. Select the required MIDI In and MIDI Out devices.
6. Select the active module with `To Modules` if needed by your rig.

Expected result:

- Your selected devices remain shown on `Start`.
- Pressing keys on your controller produces monitorable MIDI activity once monitoring is enabled.

Notes:

- MIDI device selections are remembered across launches.
- If you reconnect hardware to a different USB port, device names/IDs may differ and may need re-selection.

---

## 5) Quick Start (First Sound in 5 Minutes)

1. On `Start`, select MIDI In and MIDI Out devices.
2. Load a config (`.cfg`) and panel (`.pnl`) if your setup uses prepared files.
3. Open `Upper`.
4. Click a voice button in Group 1.
5. Play your keyboard and verify sound output.
6. Use group volume controls to balance level.
7. Save panel changes if you changed assignments.

Expected result:

- You hear sound from the selected module.
- Voice button state changes are visible and playable.

---

## 6) Screen-by-Screen Operating Guide

## 6.1 Start Tab

Primary use: startup and system setup.

You can:

- Select MIDI input and output devices
- Load panel (`.pnl`) and config (`.cfg`) files
- Select active sound module
- Verify panel/config status labels
- Use the Start-row `Exit` button (same behavior as the `Exit` tab)

Behavior:

- Last used panel/config can auto-restore at startup when files still exist.
- Start tab remembers selected MIDI ports via sticky device storage.
- Panel/config status lines are displayed as `Panel: <file>` and `Config: <file>`.

Best practice:

- Always confirm the loaded panel/config pair before performance.

Expected result:

- `Start` shows the correct device, panel, and config status before you continue.

## 6.2 Upper / Lower / Bass&Drums Tabs

Primary use: live play and switching.

Each tab contains:

- Voice button groups
- Group volume controls (`Up/Down` + slider)
- Group mute buttons
- Preset controls (shared across manuals)
- Save/Save As panel controls

Voice button workflow:

1. Click a voice button to make it active.
2. Play keyboard to hear that voice routing.
3. Use `Sounds` / `Effects` shortcut buttons to edit that specific voice.

Important warning indicator:

- After selecting a voice, if that voice has `VOL=0`, `EXP=0`, or `BRI=0`, the `Effects` shortcut button turns orange.
- Orange means that voice may produce no audible sound until corrected.

Rotary controls:

- Available on `Upper` and `Lower`.
- Use Fast/Slow and Brake according to your module settings.

Expected result:

- You can switch voices quickly while playing, and each group responds on its configured route.

## 6.3 Sounds Tab

Primary use: assign instrument voice to selected voice button.

How to use:

1. Select target voice button from a performance tab first.
2. Open `Sounds`.
3. Use level 1 category buttons.
4. Select a voice from level 2.
5. Use `Back`, `Prev`, and `Next` for navigation and pagination.
6. Return with `To Upper`, `To Lower`, or `To Bass`.

Behavior:

- Selection sends audition Program/Bank data immediately on the group output channel.
- The border title includes module name when available (for context).

Expected result:

- Newly selected voice is heard when you return to a performance tab.

## 6.4 Effects Tab

Primary use: edit per-voice effects in real time.

How to use:

1. Select target voice button from performance tab.
2. Open `Effects`.
3. Adjust effect values as needed.
4. Return to performance tab and test audibility.

Effect parameters:

- `VOL`, `EXP`, `REV`, `CHO`, `MOD`, `TIM`, `ATK`, `REL`, `BRI`, `PAN`

Important:

- `VOL`, `EXP`, and `BRI` set to `0` can result in no audible output for some voices/modules.

Expected result:

- Effect changes are audible immediately (or visible in Monitor as CC traffic).

## 6.5 Config Tab

Primary use: routing and behavior setup.

You can configure:

- Group naming
- MIDI input/output channels
- Split and octave behavior
- Default effect values for new assignments
- Module and channel mapping per group

Validation behavior:

- Duplicate `(same module + same MIDI Out channel)` across groups is blocked.
- This prevents ambiguous/unsafe routing.

Operational guidance:

- Apply and verify config changes before live use.
- Reopen performance tabs and test each affected group after config updates.
- `UI Profile` applies live across Start/Upper/Lower/Bass/Config/Sounds/Effects/Hotkeys/Monitor and resizes the app window to the selected fixed profile size.

Expected result:

- Each group routes only to its intended channel/module mapping.

## 6.6 Hotkeys Tab

Primary use: keyboard shortcut customization.

You can:

- Assign or clear hotkeys
- Save hotkey map to disk
- Cancel unsaved edits

Rules:

- Duplicate non-empty key assignments are not allowed.
- Saved hotkeys persist to `Documents/AMidiOrgan/configs/hotkeys.json`.

Default includes `Monitor` tab hotkey (`M`).

Expected result:

- Saved shortcuts continue working after app restart.

## 6.7 Monitor Tab

Primary use: troubleshoot outgoing MIDI.

Controls:

- `Enable` / `Disabled` toggle for capture
- `Clear` to clear visible history
- Virtual MIDI keyboard + `Octave` control

Behavior:

- Capture runs globally while enabled (not limited to when tab is visible).
- Disabling capture keeps existing history until manually cleared.
- Monitor lines include message/channel details and routed module context.

Use cases:

- Confirm routing path/channel
- Check whether mute gates are blocking note traffic
- Verify CC/program changes are being emitted

Expected result:

- You can see outgoing MIDI lines while enabled and retain history until cleared.

## 6.8 Help Tab

- Displays in-app user guidance.
- Use it for quick reference during setup/performance.

## 6.9 Exit

- Closes application.
- If panel changes are pending, app may prompt to save first.
- The Start-tab `Exit` button uses the same exit flow.

---

## 7) Presets and Live Performance Workflow

Preset basics:

- `Manual` plus presets `1` to `6`
- Shared across `Upper`, `Lower`, `Bass&Drums`

Typical preset programming:

1. Select target preset.
2. Set active voice buttons per group.
3. Adjust rotary/mute state as desired.
4. Use preset set flow.
5. Save panel to persist to disk.

Live usage tip:

- Keep one known-safe preset as fallback in case of rapid recovery needs.

---

## 8) Mute Behavior (Important for Stage Use)

When a group is muted:

- Group volume controls are disabled
- New Note On and Note Off messages on that output channel are blocked
- App sends:
  - `CC64=0` (Sustain Off)
  - `CC123=0` (All Notes Off)

What it does not do:

- It does not send `CC7=0` as part of mute action

---

## 9) Saving and Loading Files

### Config (`.cfg`)

Stores routing/global behavior.

### Panel (`.pnl`)

Stores voice assignments, group state, and preset data.

### Recommended Save Discipline

- Save after meaningful edits.
- Use `Save As` before large experimental changes.
- Reload once to verify expected state.

---

## 10) Troubleshooting

## No Sound

1. Verify correct MIDI Out device selected on `Start`.
2. Check target group is not muted.
3. Check selected voice is correct and active.
4. Open `Effects`; ensure `VOL`, `EXP`, `BRI` are not zero.
5. Use `Monitor` tab to confirm outgoing MIDI events.
6. If events are visible but sound is still missing, verify the sound module is on the expected receive channel and patch.

## Wrong Module or Channel

1. Open `Config` and verify group module/channel mapping.
2. Confirm no invalid assumptions from old config files.
3. Use `Monitor` tab to verify actual routed output channel/module context.

## MIDI Devices Not Showing

1. Reconnect USB/MIDI cables.
2. Power-cycle external module if needed.
3. Restart AMidiOrgan.
4. Re-select device in `Start` tab.

## Loaded Panel/Config Looks Wrong

1. Verify panel and config labels on `Start`.
2. Reload expected files manually.
3. If needed, restore from your last known-good saved versions.

---

## 11) Pre-Performance Checklist

Before rehearsal/gig:

1. Launch app and confirm expected panel/config loaded.
2. Verify MIDI In/Out devices are selected.
3. Test each manual (`Upper`, `Lower`, `Bass&Drums`) with at least one voice.
4. Confirm preset buttons and one preset recall action.
5. Confirm mute/unmute on one group behaves as expected.
6. Enable `Monitor` briefly and verify outbound traffic appears.
7. Save any final edits before performance start.

---

## 12) Quick Reference: Everyday Flow

1. Start app
2. Confirm devices and loaded files
3. Select voices per manual
4. Adjust sounds/effects
5. Validate with monitor if needed
6. Save panel/config updates

---

## 13) One-Page Quick Start Card

Use this as a fast setup sheet before rehearsal or stage:

1. Launch AMidiOrgan.
2. Open `Start`.
3. Select MIDI In device.
4. Select MIDI Out device.
5. Load your config (`.cfg`).
6. Load your panel (`.pnl`).
7. Open `Upper` and select a voice.
8. Play and confirm sound.
9. If no sound, open `Effects` and verify `VOL/EXP/BRI` are not `0`.
10. Open `Monitor`, enable capture, confirm outgoing MIDI appears.
11. Test one preset recall.
12. Save panel if you changed assignments.

If anything is wrong, return to `Start` first and re-check device/panel/config selections.

---

## 14) Glossary

- **Panel**: Performance state file (`.pnl`) including voices and presets
- **Config**: Routing and behavior file (`.cfg`)
- **Voice Button**: Button storing one instrument/effect state
- **Button Group**: Cluster of voice buttons with shared route/volume/mute context
- **MIDI In / Out**: Input source and output destination channels/devices
- **Hard Mute**: Mute logic that blocks note traffic and sends cleanup controllers

