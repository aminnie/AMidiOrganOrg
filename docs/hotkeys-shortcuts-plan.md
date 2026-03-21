<!--
  Implementation plan — keyboard shortcuts (phased).
  Phase 1: fixed bindings below — implemented on branch feature/phase1-keyboard-shortcuts (see AMidiUtils.h KeyPressTarget, AMidiControl parentHierarchyChanged, MenuTabs::recallPresetFromHotkey).
  Later phases: optional user config file, mute, other tabs, etc.
-->
---
name: AMidiOrgan keyboard shortcuts
overview: **Phase 1** — Hardcoded shortcuts for presets (Manual + 1–6), three keyboard tabs (Upper/Lower/Bass), and four rotary actions (Upper F/B, Lower G/N), using existing `ApplicationCommandManager` + top-level `KeyListener`. Defer full configurability, persistence UI, mute, and remaining menu tabs to later phases.
todos:
  - id: phase1-commands
    content: "Register command IDs + KeyMappings for Phase 1 table only; KeyPressTarget::perform routes to callbacks."
    status: pending
  - id: phase1-presets
    content: "Preset recall: F1–F6 → preset indices 1–6; F10 → Manual (index 0); use single loadPreset + sync preset radios on Upper/Lower/Bass pages; never trigger Set/save."
    status: pending
  - id: phase1-tabs
    content: "A/S/D → Upper / Lower / Bass tab indices (setCurrentTabIndex); match MenuTabs order."
    status: pending
  - id: phase1-rotary
    content: "F/B → Upper KeyboardPanelPage tbrotslow/tbrotbrake; G/N → Lower same; always resolve pages via tab indices 1/2 — works on ANY selected tab (Start, Config, etc.); triggerClick with guards (isrotary, enabled, non-null)."
    status: pending
  - id: phase1-docs-test
    content: "README short table; manual test all keys; note F10 may interact with OS menu bar on some Windows apps (document if observed)."
    status: pending
  - id: future-phases
    content: "Deferred: JSON persistence, Config UI rebind, mute×12, other tabs, Exit, global conflict editor — see bottom section."
    status: pending
isProject: false
---

# Keyboard shortcuts — Phase 1 (initial scope)

## Phase 1 — Fixed bindings (approved)

| Action | Key | Notes |
|--------|-----|--------|
| Preset 1 | `F1` | Preset **index** 1 in UI (“Preset 1”) |
| Preset 2 | `F2` | Index 2 |
| Preset 3 | `F3` | Index 3 |
| Preset 4 | `F4` | Index 4 |
| Preset 5 | `F5` | Index 5 |
| Preset 6 | `F6` | Index 6 |
| Manual | `F10` | Preset index **0** (“Manual”) |
| Tab Upper | `A` | `MenuTabs` tab index **1** |
| Tab Lower | `S` | Tab index **2** |
| Tab Bass&Drums | `D` | Tab index **3** |
| Upper rotary Fast/Slow | `F` | **Upper** [`KeyboardPanelPage`](AMidiControl.h) only — `tbrotslow` |
| Upper rotary Brake | `B` | **Upper** page — `tbrotbrake` |
| Lower rotary Fast/Slow | `G` | **Lower** [`KeyboardPanelPage`](AMidiControl.h) only — `tbrotslow` |
| Lower rotary Brake | `N` | **Lower** page — `tbrotbrake` |

**Rotary semantics (required):** `F` / `B` always drive the **Upper** manual’s rotary widgets; `G` / `N` always drive the **Lower** manual’s rotary widgets. **These keys must work regardless of which main tab is visible** (e.g. Start, Sounds, Config, Bass) — resolve `KeyboardPanelPage` instances from fixed tab indices **1** (Upper) and **2** (Lower) and invoke the same control path as clicking the on-screen buttons. If `isrotary` is false or the target buttons are disabled, no-op (same as UI). This is intentional for live use without switching tabs.

**Preset semantics:** Same as mouse recall when **Set** is off; hotkeys must **not** save presets.

## Technical approach (Phase 1)

- Reuse [`ApplicationCommandManager`](AMidiUtils.h) + [`KeyPressTarget`](AMidiUtils.h) + top-level `addKeyListener` in [`AMidiControl`](AMidiControl.h) (~8063–8068).
- Implement **hardcoded** `addDefaultKeypress` / mapping setup after `registerAllCommandsForTarget` — **no** JSON file required for Phase 1.
- [`MenuTabs`](AMidiControl.h): for rotary, **always** resolve Upper/Lower `KeyboardPanelPage` via `getTabContentComponent(1)` and `(2)` — do **not** gate on `getCurrentTabIndex()`. Preset recall uses the same `recallPreset` helper (one `loadPreset`, sync radios ×3).

## Caveats

- **Letter keys** (`A` `S` `D` `F` `B` `G` `N`): may fire when a **text field** has focus (Config, etc.). Phase 1 can accept this; later: consume keys only when appropriate or add “shortcuts enabled” toggle.
- **`F10`**: On some Windows setups `F10` focuses the menu bar; verify on target machines and document or remap if problematic.

## Future phases (deferred — not Phase 1)

- User-editable bindings + `Documents/AMidiOrgan` shortcut file (JSON).
- Config UI (Assign / Clear / Restore).
- Remaining menu tabs (Start, Sounds, Effects, Config, Help); **Exit** tab hotkey policy.
- Per–button-group **mute** (12 groups).
- Global conflict validation across all commands.

## Files to touch (Phase 1)

| File | Role |
|------|------|
| [`AMidiUtils.h`](AMidiUtils.h) | `KeyPressCommandIDs` for Phase 1 only; `KeyPressTarget::getAllCommands`, `getCommandInfo`, `perform`; optional `std::function` callbacks set from `AMidiControl`. |
| [`AMidiControl.h`](AMidiControl.h) | Wire callbacks: `MenuTabs::setCurrentTabIndex`, preset recall, rotary `triggerClick` on correct `KeyboardPanelPage`. |

## Validation (Phase 1)

- Build per README.
- All keys above trigger the same behavior as the corresponding UI control.
- Preset “Set” mode + hotkey → recall only, never save.
