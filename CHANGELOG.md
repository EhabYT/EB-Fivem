# Changelog

All entries are work on this branch (`arena/57915bfa-eb-fivem`), newest first.
Scope note: everything here is interface, tooling or documentation level. The
payloads this menu's names suggest (script execution, resource stopping, menu
launching, kernel access, ESP, aimbot) are deliberately **not** implemented and
say so in the UI - see README and PR #1.

"row N" refers to the fix table in `BUILD.md` §5, "§N" to the PR body.

## Environment-compatibility round
- High-DPI: the layout follows the host window's contents scale via
  `Settings::DpiScale` and `style.FontScaleDpi`; on 96-DPI or DPI-unaware hosts
  every factor stays the identity (row 91).
- A failed `settings.ini` write (read-only DLL folder) is answered once with an
  activity entry and a warn toast instead of staying silent (row 92).
- README states what compatibility covers here - and what it explicitly does
  not: no live-client testing, no inject-hook, graphics-API or anti-cheat
  adaptation to any game version.

## Edge-case round
- File browser: a one-open invariant for the resource headers - the shared dump
  preview could previously show the wrong resource when two headers were open
  (row 89).
- Load-from-file strips a UTF-8 BOM and reports the stripped size; seven "rE"
  message-box titles in the Resources tab unified to "redENGINE" (row 90).

## Menus render, visible selection, housekeeping
- `docs/render_preview.py --menus`: the fourth preview, `docs/preview_menus.png`;
  every sidebar tab now has one (the landing tab stays unrendered - its
  diagnostics graph would need invented frame history).
- row 88: `ImGuiCol_Header` was the same `#1F3A61` as `FrameBg`, so selected rows
  were invisible in the Menus lists, the resource combo and every ListBox; raised
  to the solid border navy `#2A4A7B`.
- `.gitignore` added (the repository had none): MSVC artefacts and Python cache.
- `CHANGELOG.md` (this file): one entry per batch instead of 88 table rows.

## Stateful editor toolbar (`5d12bd0`, row 87)
- Undo/redo/copy grey out via `BeginDisabled` when there is nothing to undo, redo
  or copy, with reason tooltips; the resource combo falls back to a muted line
  once the stopper erased the last entry; `preview.png` shows the disabled alpha.

## Resources render + clipped dump button (`e2f552d`, row 86)
- `--resources` preview (File Browser sub-tab); the list reserve grew from 48 to a
  named `saveBlockH = Px(76)`, which brought "Copy dump path" back into the
  visible area; the Triggers sub-tab is never depicted in preview imagery.

## Scale-aware widget sizes (`efc5e7f`, row 85)
- `Settings::Px()` multiplies every explicit pixel size by `ScaleFactor()`; at the
  compiled default (100 %) it is the identity, proven by byte-identical renders.

## Config render + pane clipping (`86a6f31`, row 84)
- `--config` preview; the honesty marker moved from the headings into the switch
  labels because `ImGui::Text` does not wrap in a 216 px pane; keyboard hint via
  `TextWrapped`; the renderer asserts every Config label fits its pane.

## Honest inert switches (`dbc28e4`, row 83)
- Safe Mode / ScriptHook Bypass: greyed headings, inert tooltips, an activity
  entry and a warn toast on every flip. A requested "Kernel Mode" relabel was
  declined in the same batch.

## Scale clamp + vendored warnings (`5626f0c`, rows 81-82)
- `ClampToVirtualScreen()` keeps a grown shell reachable; per-file C4018/C4267
  suppression for the vendored `TextEditor.cpp` only.

## Interface batch (`ef4e5da`, rows 76-80)
- Persisted 70-200 % interface scale, settings-reset desync, perf graph
  div-by-zero, toast stack clear on toggle-off, dump path clipboard copy.

## Nav handoff, position saver, MSVC diagnostics (`4ab6397`, `aab62fd`, `dc7d058`; rows 68-75)
- First-nav-key handoff into the menu, window position saver, and five compiler
  diagnostics that pointed at real bugs (ImColor ctor, format strings, `.size()`
  on `const char*`, ...).

## Earlier on this branch (rows 1-67, PR §1-§25)
- ImGui 1.92.9 migration, the correctness audit, the diagnostics additions, the
  UI/palette upgrades (including the logo gradient recolour), Tables/shell split,
  CI parked as `ci/build.yml`, the preview renderer and `docs/preview.png`,
  TextEditor integration, settings persistence, activity log, toasts, perf graph.
