# Changelog

All entries are work on this branch (`arena/57915bfa-eb-fivem`), newest first.
Scope note: everything here is interface, tooling or documentation level. The
payloads this menu's names suggest (script execution, resource stopping, menu
launching, kernel access, ESP, aimbot) are deliberately **not** implemented and
say so in the UI - see README and PR #1.

"row N" refers to the fix table in `BUILD.md` §5.  The PR body carries a summary
and the verification anchor only; per-batch write-ups live here and in the commits.

## Build marker gains bitness (row 100)
- The About marker now distinguishes all four configurations via `_DEBUG` / `_WIN64`:
  Debug x64, Debug Win32, Release x64, Release Win32 (x86).  Bitness matters: a Win32 DLL
  cannot load into a 64-bit host, which section 8 of BUILD.md previously said only in prose.
- The renderer mirrors "Build: Release x64" (the configuration the owner verified) and
  width-asserts all four variants; only preview_config.png re-renders.

## About block names the build configuration
- Config's About block ends with an `#ifdef _DEBUG` line - `Build: Debug (asserts on, slower)`
  or `Build: Release` - because a Debug DLL behaves visibly differently in a host and nothing
  in the UI said which one was loaded.  The renderer mirrors the Release wording (previews
  depict Release) and width-asserts both variants; BUILD row 99 logs it.
- Recon: `.gitignore` already covers every build artefact the real Windows build produces
  (`.vs/`, `Release/`, `Debug/`, `x64/`, `*.obj/.pdb/.ilk/.log`, and the LTCG `.ipdb/.iobj`
  inside `x64/`), and all four configs carry `d3d11.lib;dxgi.lib` - no change needed.

## First real MSVC build: clean
- Owner's Windows build (2026-10-23, Visual Studio, Release x64): "1 erfolgreich, 0 Fehler" -
  the first real compile of this branch, after 43 commits of static-only verification.  The
  output path `x64\Release\EBengin UI.dll` confirms the ProjectName rename end to end.
- The log's 26 translation units are exactly the project set: `font_awesome.cpp` carries
  `ExcludedFromBuild` on purpose because `main.cpp` includes it directly, and the LTCG
  "Previous IPDB not found" lines are first-link information, not warnings.  Both facts are
  now documented in BUILD.md §8 so nobody "fixes" them.
- The F5 dialog from the previous round was resolved the documented way: build instead of
  launch.  The PR's verification anchor records the build; Debug/Win32 feedback still welcome.

## F5 cannot launch a DLL - documented, not "fixed"
- A real Windows build (x64 Debug) followed by F5 produced Visual Studio's "EBengin UI.dll ist
  keine gültige Win32-Anwendung / is not a valid Win32 application" dialog.  That is expected for
  a DLL project: CreateProcess cannot launch a library, and the build had already succeeded.
- Documented in three places so nobody reads it as a broken build again: BUILD.md Option C
  warning, a new §8 Troubleshooting entry naming the two legitimate workflows (build-only via
  Ctrl+Shift+B / build.bat; real debugging via Debugging -> Command or Attach to Process), and a
  README note; BUILD row 98 logs it.
- No project or `.user` file changed: a debugger command pointing at a host process is a
  per-machine choice and stays untracked on purpose.

## Notes and counts after row 97
- menu.cpp now documents why the splash skip must not age toasts: their lifetime runs in
  `Toasts::Render()`, so a warn toast queued at load waits behind the start card at full
  lifetime instead of expiring unseen.
- The PR summary bullet still said "rows 1-96" after row 97 landed; it now says 1-97.

## Splash hint follows the bound toggle key
- The start card's second hint hardcoded "INSERT" although the toggle key is bindable in Config;
  a rebind made the card's own instruction wrong.  It is now formatted with
  `Settings::KeyName(Settings::Current.ToggleKey)` - the same helper the Config button and the
  key-change toast use.  The renderer mirror keeps the default binding (previews depict default
  settings) and says so in a comment.
- README's file table gained the missing `preview_splash.png` row; BUILD.md's Docs & render row
  now lists the `--splash` render; row 97 logs the fix.

## PR body condensed
- On owner request the PR body's 40 numbered per-batch sections were removed; it now carries
  a summary plus the verification anchor.  The write-ups live in this changelog, the commit
  messages and BUILD.md - nothing was deleted from the record.
- The anchor's reviewer line now names `EBengin UI.sln` (it still quoted the pre-rebrand
  solution name), and the legend above no longer points "§N" at the PR body.

## Splash keyboard focus + renderer docstring
- `RenderSplash()` now performs the shell's open-time focus handoff (`SetNextWindowFocus` while
  `g_justOpened` is set), so the Continue button is reachable by keyboard on the first open.
  The flag deliberately stays set: the sidebar's nav-cursor handoff still runs on the shell's
  first frame after Continue.
- The render_preview.py docstring claimed it renders "either the Executor tab ... or the Config
  tab"; it now lists all five outputs including the `--splash` branch.
- Recon: no live old-brand tokens remain - every surviving redENGINE/rE.png mention is a
  historical record (fix-table rows, recolour note, changelog).

## Visible branding in the UI
- Splash / start card: on the first open after inject, `RenderSplash()` in menu.cpp draws a
  centred card instead of the shell - logo 110x127, the muted credit "Remade : by EB", a
  separator, two honest hints and a Continue primary button.  Dismissing it flips one static
  bool; no other state is touched.  New preview `docs/preview_splash.png` (`--splash`).
- Sidebar footer: muted fourth line "Remade : by EB", same wording as the source comment.
- Config right pane: About block - EBengin in accent, credit and ImGui version muted, framed
  by separators.
- The renderer mirrors all three additions; its width asserts caught one over-long hint line
  before shipping, shortened identically in code and renderer.

## Credit line re-credited
- The source credit comment in menu.cpp now reads `//Remade : by EB` (owner decision); it
  previously carried the upstream line `//redENGINE Remade : by xo1337`.
- Upstream attribution stays on record: the README credits still say "Menu UI remade by
  xo1337", and this changelog keeps the historical mentions; BUILD row 94 logs the change.
- No functional or visual change: the comment is source-only, previews unchanged.

## Rebrand: redENGINE -> EBengin
- New logo asset `EBengin.png` replaces `rE.png` (same 1682x1982 transparent canvas, same double
  frame): monogram "EB" - E in the #5D9CD6 -> #3778B5 -> #1D5188 ramp, B in white - plus a two-tone
  wordmark, "EB" in #1D5188 and "engin" in white, in the band below the frame.
- Displayed name renamed to `EBengin`: menu window title (`ImGui::Begin`), sidebar text-logo
  fallback, all 14 MessageBox captions (exec/luamenus/resources), `menu_style.h` comment and the
  preview renderer's title/sample strings.
- Build names follow: `redENGINE UI.sln` -> `EBengin UI.sln` (git mv), `<ProjectName>` and hence the
  DLL/PDB output `EBengin UI`, logo copy in `build.bat`, `ci/build.yml` (and the parked working-tree
  copy of the workflow) updated to the new file names.
- Docs: README title/prose/alt texts/asset table, BUILD.md prose plus row 93, ci/README.md; all four
  preview images re-rendered from the new logo and strings.
- Historical mentions kept on purpose: the recolour note and the upstream credit line in
  README, BUILD rows 105/111/112/138/163/217 and this changelog; the menu.cpp credit comment
  was re-credited afterwards (entry above).

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

## Earlier on this branch (rows 1-67)
- ImGui 1.92.9 migration, the correctness audit, the diagnostics additions, the
  UI/palette upgrades (including the logo gradient recolour), Tables/shell split,
  CI parked as `ci/build.yml`, the preview renderer and `docs/preview.png`,
  TextEditor integration, settings persistence, activity log, toasts, perf graph.
