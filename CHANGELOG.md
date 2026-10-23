# Changelog

All entries are work on this branch (`arena/57915bfa-eb-fivem`), newest first.
Scope note: everything here is interface, tooling or documentation level. The
payloads this menu's names suggest (script execution, resource stopping, menu
launching, kernel access, ESP, aimbot) are deliberately **not** implemented and
say so in the UI - see README and PR #1.

"row N" refers to the fix table in `BUILD.md` §5.  The PR body carries a summary
and the verification anchor only; per-batch write-ups live here and in the commits.

## The editor shows unsaved work (row 119)
- With Save and Load in the toolbar, the one remaining silent state was "the buffer holds
  edits that exist nowhere else".  `exec.cpp` now polls `TextEditor::IsTextChanged()` directly
  after the widget render - the flag is frame-scoped, cleared at the end of the editor's own
  `Render()` - and accumulates it into a session dirty flag that a successful load, save or
  clear resets.  While set, the status line appends `| modified`.
- `SetText()` from our own load/clear raises the same widget flag, so those calls suppress one
  frame; otherwise loading a file would immediately report itself as modified.
- BUILD section 9's config cell also catches up with the About block's current contents
  (not-writable flag, both buttons).

## First-start toast arrives readable; diagnostics name the shell size (rows 117-118)
- Toasts live four seconds, and `Settings::Load()` runs long before the overlay first opens -
  so the row-113 "no settings.ini yet" toast expired unseen on every first start.  `Load()` now
  leaves a flag, and `Menu::Render()` consumes it via `Settings::ConsumeFirstStartNotice()` on
  the first visible frame; the activity entry stays where it was (the log does not expire).
- The Copy diagnostics line appends `| shell: WxH` from `900/540 * Settings::ScaleFactor()` -
  the same factor `ApplyScale()` writes into `style.WindowMinSize` - so a report carries the
  produced pixel size, not just the factors behind it.

## Clipboard writer consolidated, stopper trace, folder fallback (rows 114-116)
- `resources.cpp` carried its own file-static clipboard helper (HWND parameter, duplicated
  OpenClipboard/GlobalAlloc/CF_TEXT sequence) plus an `extern HWND window` that existed only to
  feed it; the dump-path and trigger copy buttons now use the shared `clipboard.h` writer, and
  the legacy helper and the extern are deleted - one sequence, one failure behaviour.
- The stopper's removal - the one honest stub that visibly changes state - answered only with a
  modal box; it now also leaves an activity entry and an info toast, like the Execute and Run
  requests do.
- `Open settings folder` falls back to exploring the folder itself when `settings.ini` does not
  exist yet (first start), instead of handing Explorer a `/select,` target that is not there.

## Overwrite protection and a first-start notice (rows 112-113)
- `TextEditor::SetText` clears the undo buffer, so `Load from File` and the toolbar Clear used
  to discard an unsaved buffer unrecoverably, without asking.  Both now confirm with
  `MB_OKCANCEL` naming the byte count whenever the buffer is non-empty; Cancel keeps the
  buffer and logs itself, a confirmed Clear also drops the status line's file reference.
- `Settings::Load()` notices a missing `settings.ini` (`GetFileAttributesA`) and answers the
  first start with an activity entry naming the path - "compiled defaults in use; the file
  appears with the first saved change" - plus an info toast, so the state is visible instead
  of inferred.

## Paths you can see and reach (rows 110-111)
- The editor status line appends `| file: <base name>` once a load or save succeeded
  (`exec.cpp` file-static, base name only), so the origin of the buffer on screen survives the
  next edit and the renderer documents the no-file default.
- Config's About block gains `Open settings folder` under `Copy diagnostics`:
  `ShellExecuteA(NULL, "explore", NULL, "/select,<ini path>", ...)` opens Explorer with
  settings.ini highlighted - the Dumps folder sits next to it, so attaching files to a report
  is two clicks.  A return code of 32 or less answers with an activity entry (code included)
  and a warn toast instead of silence; `shell32.lib` via pragma comment.

## Failure visibility and a copyable session log (rows 108-109)
- A failed settings.ini write used to answer once (warn toast + activity entry) and then go
  silent, although the consequence - every later change lost on restart - persists for the
  whole session.  The Config About block now shows a permanent red
  `Settings not writable - changes are lost on restart.` line while `Settings::SaveFailed()`
  is true.
- The landing tab's session log gains a `Copy` button next to `Clear`: all entries, as
  displayed (newest first), CRLF-joined, onto the Windows clipboard - the report path for the
  only timestamped trace the session leaves.  Success and clipboard-unavailable paths answer
  with a toast and an activity entry of their own.
- Both use a new header-only `clipboard.h` (CF_TEXT, moveable global, free-on-refuse), so the
  Win32 clipboard writer exists once instead of once per tab; no project-file change needed.

## Save to File: the editor buffer can leave the menu as a file (row 107)
- The Executor had `Load from File` but no save path - a buffer written or pasted in the menu
  could only leave through the clipboard.  A save icon now ends the editor toolbar:
  `GetSaveFileNameA` with the same filter, `lua` default extension and the dialog's own
  overwrite prompt; byte-accurate `fwrite` with the written count checked against the buffer
  size (partial write = "disk full?" warn toast), and open/write failures reported by message
  box, activity entry and toast instead of silence.
- Renderer mirrors the sixth toolbar icon (toggle shifted one slot right) and the docstring
  names it; README toolbar bullet and BUILD section 9 follow; preview_executor.png re-rendered.

## Version strings from the compiled library, settings path in diagnostics (row 106)
- The About line and the Copy diagnostics string hardcoded `1.92.9`; both now format
  `ImGui::GetVersion()`, i.e. the version the linked imgui.cpp was actually compiled with, so
  a future vendored update cannot leave them lying.
- The diagnostics line appends `| settings: <path>` via a now-public `Settings::IniPath()`, so
  a report names the exact file to attach; the buffer grew from 256 to 640 bytes for MAX_PATH.
- No pixel changes: the rendered strings are identical at the vendored tag, previews untouched.

## Copy diagnostics: one click, environment attached (row 105)
- Config's About block ends with a `Copy diagnostics` button: one line with name, build
  configuration, host DPI and interface scale, ImGui version and toggle key goes to the
  Windows clipboard (CF_TEXT, moveable global, proper free-on-failure), answered by a success
  or warn toast plus an activity entry.  Reports from other machines now carry their
  environment instead of being asked for it piece by piece.
- BUILD.md section 9's tab-bodies row and the renderer (button mirror + label assert) follow;
  preview_config.png re-rendered.

## About block reports the two scale factors (row 104)
- The shell size on any machine is host contents scale (`Settings::DpiScale`, row 91) times the
  persisted interface scale, but neither number was visible anywhere - so a "the menu is tiny/
  huge" report from another machine could not be placed.  Config's About block now ends with
  `Host: DPI 1.00, interface 100%` (formatted live from both settings); the renderer mirrors the
  96-DPI default and width-asserts the string.
- The PR summary's fix-table bullet is reworded count-free ("defect rows plus documentation,
  style and diagnostics rows") so it can no longer drift out of date the way "102 rows" did
  when row 103 landed.

## README credit line: Menu UI made by EB
- The credits line now reads "Menu UI made by EB" - the owner's decision, after the offer to
  re-credit it stood open since the rebrand.  The ImGui credit and the rest of the line are
  untouched.
- The in-UI credit wording stays "Remade : by EB" (splash card, sidebar footer, Config About
  block, menu.cpp source comment) exactly as explicitly set earlier; only the README line
  changed.  BUILD row 103 logs it and row 94's forward claim now points at it.

## Architecture views synced with splash and HUD
- README lifecycle diagram: the `IsMenuOpen?` no-branch now runs through the HUD decision
  (`[ui] hud`) into `Menu::RenderHud`, and the render node names the splash card on first
  open; BUILD.md section 9's Shell row lists the nav rail, selection wash, splash card and
  HUD plate.  The README keeps its own rule: change the frame loop, change the diagram.

## Doc consistency after the HUD batch
- The renderer docstring lists all six outputs now (--hud included); the README Config
  caption names the FPS HUD switch among the Interface preferences.
- PR summary refreshed in one authoritative pass: the bullet list now covers the style
  refinement, the build-configuration marker and the FPS HUD, and counts the BUILD.md table
  at 102 rows / six preview renders.  The earlier "rows 1-97" correction had been clobbered
  by a later full-body PATCH built from a stale local copy - full-body PATCHes replace
  everything, so the summary is now rewritten from the live body in a single step.

## FPS line becomes a HUD
- `Menu::RenderHud()` draws the sidebar's FPS string as a top-left plate over the game scene
  whenever the overlay is closed: background draw list (over the scene, under any window),
  dark rounded backing so it stays legible on bright frames, accent text, scale-aware via
  `Settings::Px()` and `ImGui::GetFontSize()`.  The numbers remain ImGui's own smoothed
  present-loop counters - interface diagnostics, not game state.
- Config -> `FPS HUD` switch (default on), persisted as `[ui] hud` with the other interface
  prefs; the present loop draws the HUD only while the overlay is closed, so the line is
  never shown twice.
- The switch first overflowed the Config left pane by 18px - the renderer's pane-overflow
  assert caught it before shipping; the keyboard hint was compacted from three wrapped lines
  to two (same information) to make room.
- New `--hud` render (`docs/preview_hud.png`) on a mid-tone scene stand-in; README paragraph
  and table row, BUILD Docs & render row and row 102.

## Style refinement: depth and palette cohesion (row 101)
- Nav rail: the sidebar table cell is painted one notch darker than the canvas via
  `TableSetBgColor(ImGuiTableBgTarget_CellBg)` - WindowBg darkened 18%, z-safe because cell
  backgrounds render under their content.  The panel buttons now read as a rail.
- Selected nav row: soft accent wash fading left to right (alpha 22 -> 0) over the pressed
  colour, under the existing 3px marker - the row reads lit from the side without tinting
  its label.
- Palette cohesion: `TextDisabled` is the muted accent (64,109,154) instead of stock grey,
  `PopupBg` sits on canvas navy, scrollbar grabs run raised -> hover -> pressed like every
  other interactive surface, and the scale slider's 12px grab is a pill (`GrabRounding` 6;
  the resources scrollbar mirror also moved to the style's `ScrollbarRounding` 8).
- No metric changed, so every width/height assert still holds; four previews re-rendered,
  the splash card (no sidebar) keeps its hash.

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
- Upstream attribution stayed on record in the README credits line until the owner
  re-credited it (see the top entry); this changelog keeps the provenance mentions.
  BUILD row 94 logs the comment change, row 103 the README line.
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
