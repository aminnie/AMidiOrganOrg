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
6. Use `New Panel` on `Start`, enter a new `.pnl` name, and confirm it saves + auto-loads.
7. Retry with the same name and confirm duplicate warning/no overwrite.
8. Open `Upper` and confirm voice selection + playback.
9. In `Upper`, toggle the new Group 1/Group 2 `Rotary` selector checkboxes (inside each group border, lower-left area), and confirm:
   - only one checkbox stays selected
   - rotary Fast/Slow/Brake follows the selected group's module/routing
10. Repeat rotary selector validation on `Lower`.
11. Save panel, reload panel, and confirm Upper/Lower rotary target selections persist.
12. Open `Monitor`, enable capture, and verify outgoing MIDI lines appear.
13. Confirm `USER_MANUAL.md` opens from package folder.

## 3) Runtime Checks (macOS)

1. Extract the macOS package.
2. Launch `AMidiOrgan.app`.
3. Confirm app opens to tabbed UI.
4. Confirm `Start` tab can select MIDI In/Out devices.
5. Load one config (`.cfg`) and one panel (`.pnl`).
6. Use `New Panel` on `Start`, enter a new `.pnl` name, and confirm it saves + auto-loads.
7. Retry with the same name and confirm duplicate warning/no overwrite.
8. Open `Upper` and confirm voice selection + playback.
9. In `Upper`, toggle the new Group 1/Group 2 `Rotary` selector checkboxes (inside each group border, lower-left area), and confirm:
   - only one checkbox stays selected
   - rotary Fast/Slow/Brake follows the selected group's module/routing
10. Repeat rotary selector validation on `Lower`.
11. Save panel, reload panel, and confirm Upper/Lower rotary target selections persist.
12. Open `Monitor`, enable capture, and verify outgoing MIDI lines appear.
13. Confirm `USER_MANUAL.md` opens from package folder.

## 4) Behavior Verification

- Startup restore works when last session files exist.
- Hard mute behavior blocks Note On/Off while muted.
- Upper/Lower rotary selector (Group 1/2) remains mutually exclusive and restores from saved panel state.
- `Help` tab wording points to packaged `USER_MANUAL.md`.
- No obvious mismatch between `README.md`, `assets/help.md`, and `USER_MANUAL.md`.

## 5) Release Pipeline Validation (CI)

The release workflow should fail if package manifest checks fail on either platform.
Ensure these CI checks pass before marking a release as published.
