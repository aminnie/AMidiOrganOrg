# AMidiOrgan Release Smoke Checklist

Use this checklist before publishing a Windows/macOS release package.

## 1) Package Manifest Checks

For each platform archive, verify all required files are present:

- Application binary
  - Windows: `AMidiOrgan.exe`
  - macOS: `AMidiOrgan.app`
- `docs/` seed content folder
- `USER_MANUAL.md`
- `README-USER.txt`

Expected archive naming:

- `AMidiOrgan-vX.Y.Z-windows-x64.zip`
- `AMidiOrgan-vX.Y.Z-macos.zip`

Optional:

- `checksums.txt`

## 2) Runtime Checks (Windows)

1. Extract the Windows package to a clean folder.
2. Launch `AMidiOrgan.exe`.
3. Confirm app opens to tabbed UI.
4. Confirm `Start` tab can select MIDI In/Out devices.
5. Load one config (`.cfg`) and one panel (`.pnl`).
6. Open `Upper` and confirm voice selection + playback.
7. Open `Monitor`, enable capture, and verify outgoing MIDI lines appear.
8. Confirm `USER_MANUAL.md` opens from package folder.

## 3) Runtime Checks (macOS)

1. Extract the macOS package.
2. Launch `AMidiOrgan.app`.
3. Confirm app opens to tabbed UI.
4. Confirm `Start` tab can select MIDI In/Out devices.
5. Load one config (`.cfg`) and one panel (`.pnl`).
6. Open `Upper` and confirm voice selection + playback.
7. Open `Monitor`, enable capture, and verify outgoing MIDI lines appear.
8. Confirm `USER_MANUAL.md` opens from package folder.

## 4) Behavior Verification

- Startup restore works when last session files exist.
- Hard mute behavior blocks Note On/Off while muted.
- `Help` tab wording points to packaged `USER_MANUAL.md`.
- No obvious mismatch between `README.md`, `assets/help.md`, and `USER_MANUAL.md`.

## 5) Release Pipeline Validation (CI)

The release workflow should fail if package manifest checks fail on either platform.
Ensure these CI checks pass before marking a release as published.
