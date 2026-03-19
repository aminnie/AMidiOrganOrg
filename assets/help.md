# AMidiOrgan Help

## What This App Does
AMidiOrgan is a live-performance MIDI controller built with JUCE and C++.  
It manages Voice Button Groups across three keyboard panels (`Upper`, `Lower`, `Bass&Drums`),
supports layering/splits, and routes MIDI to connected hardware and software sound modules.

## Supported Modules (Current)
- Deebach BlackBox
- Roland Integra7
- Ketron SD2
- MIDI GM
- Custom MIDI (JSON-based)

## Startup Flow (Recommended)
1. Open **MIDI Start Page** (`Start` tab).
2. Select MIDI input/output devices.
3. Select the instrument module (`To Modules`).
4. Load panel (`.pnl`) and config (`.cfg`) if needed.
5. Confirm panel/config labels and begin from **Upper** / **Lower** / **Bass&Drums**.

## Tab Guide
- **Start**
  - MIDI input/output devices are dynamically listed on connect/disconnect.
  - Instrument module selection and panel/config load.
- **Upper / Lower / Bass&Drums**
  - Voice Button Groups, volume sliders, mute, presets.
  - Rotary controls on Upper/Lower.
  - Panel Save and Save As.
- **Sounds**
  - Assign instrument voices to selected buttons.
- **Effects**
  - Edit per-voice MIDI effects in real time.
- **Config**
  - Edit MIDI In/Out routing, split notes, octave shift, and MIDI pass-through.
- **Help**
  - This guide.

## Important Behavior Notes
- Groups 2, 3, and 4 may start muted depending on preset state.
- If muted:
  - Group volume slider and Up/Down buttons are disabled by design.
- Mute is used for fast live layering control.

## Window Move Tip
- Click and drag in empty space on the top tabs bar to move the app window.
- This keeps the 6px title bar while still allowing screen repositioning.

## Save / Load Tips
- Use **Save** frequently after meaningful edits.
- Use **Save As** before major changes to preserve a fallback panel.
- After save, reload once to validate persisted state.

## MIDI Troubleshooting
- **No sound**
  - Verify MIDI output device is connected/selected.
  - Check Voice Button Group mute states and output channels.
  - Confirm selected module matches active sound source.
- **Notes not routing correctly**
  - Check Config page MIDI In/Out channel mapping.
  - Verify split note settings for solo groups.
- **Unexpected voice/effect**
  - Confirm active voice button and preset selection.
  - Re-open Sounds/Effects pages and verify assigned values.

## Pre-Performance Checklist (2-3 Minutes)
- Open each main tab once.
- Verify MIDI device list is stable (no missing outputs).
- Trigger notes on Upper, Lower, and Bass&Drums.
- Check mute states and Voice Button Group volume levels.
- Confirm preset recall for one saved preset.
- Save current panel snapshot.

## Contact Details
- Anton Minnie, email: `a_minnie@hotmail.com`
