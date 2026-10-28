# Changelog

All entries are work on this repository's session branches, newest first:
PR #1's `arena/57915bfa-eb-fivem` (merged into `main` as `8d8ea8e`), then the
current `arena/c5d3e349-eb-fivem`.
Scope note: everything here is interface, tooling or documentation level. The
payloads this menu's names suggest (script execution, resource stopping, menu
launching, kernel access, ESP, aimbot) are deliberately **not** implemented and
say so in the UI - see README and PR #1.

"row N" refers to the fix table in `BUILD.md` §5.  The PR body carries a summary
and the verification anchor only; per-batch write-ups live here and in the commits.

## Fallback sidebar wordmark hit-target and reset-toast wording fix (row 245)
- `menu.cpp` (`RenderSidebar`): when `EBengin.png` is missing next to the DLL (`!LogoLoaded`),
  `ImGui::Dummy(ImVec2(Settings::Px(76.0f), Settings::Px(68.0f)))` ran immediately after
  `ImGui::TextColored(..., "EBengin")` and before `ImGui::IsItemHovered()` / `ImGui::IsItemClicked()`.
  Because `ImGui::Dummy` calls `ItemAdd`, it overwrote `GImGui->LastItemData` with the blank spacer
  rect below `"EBengin"` (and at `x = 0..76` after `TextColored`'s newline), leaving the visible
  `"EBengin"` wordmark unresponsive while the empty space below it acted as the home button.  The
  `Dummy` spacer now runs after the hover/click checks so `IsItem*` inspects the visible `"EBengin"`
  wordmark while preserving the exact sidebar height.
- `config.cpp`: adjacent string literals in the Reset Interface Settings toast (`"...and last"`
  `"tab apply on next start."`, row 231) lacked a separating space and rendered `"lasttab"`.
  Restored the space; a regex scan of all multi-line string literals across the repository confirmed
  no other missing space.

## Missing `else` in Execute after row 229 could read `resources[0]` on an empty list (row 244)
- While checking the `Execute` handler (`exec.cpp`) referenced in the `_cfx_internal` verification
  step, found that removing the `else if (strcmp(resources[selectedResource], "_cfx_internal") == 0)`
  branch in row 229 had also removed the trailing `else`, leaving a bare `{ ... }` block after
  `if (resources.empty())`.
- If all entries had been removed in the Stopper tab (`resources.empty() == true`), clicking
  `Execute` opened `"No resource selected."` and then fell straight through into the report-only
  block, indexing `resources[selectedResource]` on an empty `std::vector<const char*>` (out-of-bounds
  read / crash).
- Fix: restored `else` before the report-only block in `exec.cpp`.  Scanned every `.cpp`/`.h` file
  for any other `}` followed by an unattached `{` block - `exec.cpp` was the only hit.

## Fault lines carry code and address; probe DLL and advanced-debugging docs (row 243)
- Applied the second half of the user-supplied crash guide (advanced debugging, testing methodology,
  exception codes, validation checklist) within the standing scope rules.
- Code: hkPresent's SEH handler logged a static reason - guide 6.3 asks for the exception code and
  faulting address.  The handler now reads GetExceptionInformation() and FaultDisable receives
  "code 0x........ at 0x................", so an EBengin.log paste alone can name the fault class
  (access violation vs /GS hit vs heap corruption) before any dump is opened.
- New tool: tools/dll_probe/probe.cpp - the guide's minimal test DLL (6.4), built standalone, not
  part of EBengin UI.sln.  Deviation from the guide on purpose: no MessageBoxA inside DllMain
  (loader-lock deadlock risk, and row 236's minimal-DllMain rule); the probe logs one line to
  %TEMP%\dll_probe.log from a worker thread instead.  Inject it with the same method as the real
  DLL: log line = injection works, crash lives in the full DLL; no line = method/timing/protection.
- Docs (BUILD.md): gflags page-heap enable/disable around heap-corruption dumps (6.1), ProcMon
  Load-Image filters for "DLL never appeared" (6.2), the outside-in isolation order (7), the
  exception-code table with the first check per code (8, native-only - the .NET row is marked
  impossible), and the final validation checklist (10).
- Out of scope, recorded as such (9 + 11): no detection-mechanics work list, no manual mapping or
  CFG handling, no DLL proxying, no external game-memory access - the README scope note stands;
  the documented exit is official mod support where it exists, or not injecting.
- Validation: renders 8/8 unchanged (no UI surface touched), probe.cpp balanced and standalone
  (nothing includes it), api_sweep clean, main.cpp balance delta 0 vs HEAD.

## Stale editor coordinates after SetText can no longer read past the buffer (row 242)
- TextEditor::SetText replaced the lines but kept cursor and selection: load a 100-line file, SelectAll,
  load a 5-line file, press Copy - and GetText's reserve loop read mLines[5..99] out of bounds
  (GetCharacterIndex answers -1 for vanished lines, which the copy loop took as line[-1]; DeleteRange
  erased at begin() - 1).  Four clicks, undefined behaviour, no log line.
- SetText now resets cursor and selection to the top of the new buffer (the natural reading of a replaced
  buffer), GetText clamps stale or hand-built ranges into the buffer, and DeleteRange refuses impossible
  indices.  Vendored ImGuiColorTextEdit, not frozen - all three sites carry row comments.

## Hygiene sweep finished, conventions written down (row 241)
- The sweep (BOM, EOF newline, trailing whitespace over every project-owned file) found one defect:
  font_awesome.h ended without a final newline.  The .sln UTF-8 BOM is MSVC's own convention and stays -
  Visual Studio re-adds it on every save, so removing it would only create diff noise.
- font_awesome.h ends with exactly one newline now, and BUILD.md section 4 carries the written hygiene
  conventions: CRLF for sources/.bat/.sln (BOM kept on purpose), LF for docs/renderer/yml, vendored trees
  untouched, one EOF newline and no trailing whitespace anywhere.

## Endings census finished: clipboard.h joins the CRLF convention (row 240)
- The row-239 census over the whole tree found one more project-owned outlier: clipboard.h was entirely
  LF while every other project source, the .bat files and the .sln are CRLF.  Docs, renderer and
  ci/build.yml stay LF by convention; the vendored trees (stb_image.h with 7988 LF lines, imgui/,
  kiero/) keep their upstream endings on purpose.
- clipboard.h is normalized to CRLF; BUILD.md records the census and the vendored exception, so the
  convention now has a written definition.  A duplicate-`##id` sweep over every tab renderer found only
  the intended OpenPopup/BeginPopup pair (exec.cpp recent-files popup).

## The HUD plate carries the owner-set credit wording (row 239)
- Row 124 put "MADE BY EB" on the HUD plate; the standing credit directive sets the UI wording to
  "Remade : by EB" (splash card, sidebar footer, Config About - rows 94/103).  The plate was the one
  UI surface deviating from it.
- The plate second line, the renderer hud-scene constant and every current-description sentence
  (menu.h comment, README, BUILD architecture row) carry "Remade : by EB" now; plate metrics do not
  move because the plate width is the max of its two lines and the FPS line is the longer one.
- menu.cpp is normalized to CRLF: earlier patches had left 95 bare-LF lines in the otherwise CRLF
  file (repo convention: sources and .bat CRLF, docs and renderer LF).  Past row and changelog
  entries keep their historical wording.

## The two BUILD.md fix tables no longer share numbers (row 238)
- Section 4 (build fixes) numbered its entries 1-14 while section 5 (runtime/correctness, the table every
  bare "row N" reference in code comments, CHANGELOG, README and the vcxproj points at) numbers 1-237 -
  for 1-14 a reference could land in either table.
- Section 4 is renumbered B1-B14 and says why in its intro; bare "row N" is unambiguous again.  Docs only.

## C++ exceptions contained like access violations (row 237)
- Row 236 switched the build to /EHsc, so C++ exceptions unwind - into std::terminate, because nothing
  caught them.  The editor's token-regex constructors (TextEditor.cpp, SetLanguageDefinition) throw
  regex_error straight into Executor::Init and the first Present when an STL rejects a pattern, and any
  out_of_range / bad_alloc from a std:: call had the same end: process death with no log line, looking
  exactly like a game crash.  The row-236 SEH guard catches access violations only.
- The regex constructors are now guarded per pattern (a rejected pattern costs one highlight rule, not
  the session), and PresentBody / WndProcBody wrap their frames in try/catch(...) on both threads.  All
  three fault paths share one raw-Win32 degrade path (FaultDisable): overlay out of service, window proc
  restored, fault line in EBengin.log, game continues.

## Crash-guide checklist: build flags explicit, frame faults contained (row 236)
- The vcxproj never stated CRT, exception handling, /GS or ASLR: compiler defaults left a static
  CRT and no C++ unwind (a throwing std:: call terminated the process), and the security flags rode
  on implicit defaults.  All four configurations now carry /MD (dynamic CRT, matching the host),
  /EHsc, /GS and /DYNAMICBASE + /HIGHENTROPYVA explicitly.
- The Present hook wraps the frame body in an SEH guard: an access violation in our own draw code no
  longer kills the host - the handler hands the window proc back, disables the overlay for the
  session, logs a fault line to EBengin.log, and every later frame passes straight to the game.
- BUILD.md gained a "capturing the real crash" subsection (EBengin.log, procdump, WinDbg
  !analyze -v, what to paste) plus the user-side environment checks as diagnostics; no injector and
  no anti-cheat interaction - both stay out of scope by standing rule.

## Same-size resizes no longer draw through a released back buffer (row 235)
- The Present hook detected resizes by comparing buffer dimensions.  A same-size ResizeBuffers -
  borderless<->fullscreen at equal resolution, or a refresh-rate switch - swaps the back-buffer
  texture under an unchanged BufferDesc, so the cached render target view kept referencing a
  released texture and every subsequent draw went through it: a device-removed / access-violation
  path in the middle of a session.
- Buffer 0 pointer identity is now the second resize signal: CreateRenderTarget keeps one reference
  to the texture the view was built from (the held reference also stops the freed address from being
  recycled for the new texture, which would alias the comparison), CleanupRenderTarget releases it
  together with the view, and the per-frame probe recreates view and ImGui device objects whenever
  the pointer or the dimensions change.  No menu behaviour changes; kiero/MinHook untouched.

## Teardown no longer races live hook calls (row 234)
- On hosts whose window thread differs from the thread that calls Present (GTA V / FiveM pump
  messages on the main thread while Present runs on the render thread), DETACH could destroy the
  ImGui context and restore the Present hook bytes while either thread was still inside our hooks;
  CleanupImGui also handed the window proc back only after DestroyContext, so an in-flight message
  called ImGui_ImplWin32_WndProcHandler on a dead context.  Both are access-violation paths that
  surface as "the game crashes after injection" or on eject.
- hkPresent and WndProc are now thin wrappers that keep interlocked depth counters around static
  bodies; DETACH waits for both counters (capped ~2 s, loader-lock safe) before teardown, and
  CleanupImGui restores the window proc before the context dies.  The cross-thread lifetime flags
  are volatile.  No menu behaviour changes; kiero/MinHook stay untouched.

## The sidebar license plate was a claim with nothing behind it (row 233)
- "Licensed to #4278" was a hardcoded constant; no license mechanism exists in this build -
  nothing issues, stores or checks a license.  The line asserted a fact the build cannot back
  up, in the one place every open frame shows.
- The footer states the true fact in its place: "No license check."  The README records the swap
  beside the other honesty labels; the renderer's footer string follows, so - the sidebar draws
  in every scene - all eight previews change by exactly that one string, committed deliberately.

## The Menus tab joins the Resources provenance pattern (row 232)
- FreeMenus/PremMenus (luamenus.cpp) are compile-time constants, but the only hint was the Run
  button's hover tooltip ("not bundled"); until hovered, the lists read like an installed
  catalogue - the same provenance gap row 230 closed for the Resources list.
- The tab now opens with the same muted line - "Compiled-in name list - nothing here is bundled
  or launched." - and the README names the menu lists' origin next to the resource list's.
  docs/render_preview.py models the line, so preview_menus.png shifts by exactly one line; the
  changed render is committed deliberately, the other seven stay byte-identical.

## The reset dialog names everything it destroys (row 231)
- "Restore the compiled defaults?" listed four of the six things the whole-struct assignment
  resets: the stored window position and the last tab go back to defaults as well - and main.cpp
  opens the next session on the stored tab (`Menu::Tab = Settings::Current.Tab`), so the loss is
  real.  The tooltip and toast mentioned the window position; no UI text mentioned the tab.
- The confirm modal, the button tooltip and the success toast now state the complete scope: key
  bind, scale, switches and HUD corner are overwritten at once; the stored window position and
  last tab reset and apply on the next start.  Wording only - the reset behaviour itself
  (including Recent / ResourceName survival, row 153) is unchanged.

## The Resources tab names its own provenance (row 230)
- The browser, stopper and Executor combo operate on a compiled-in sample list (exec.cpp), but
  nothing in the UI said so: names like `_cfx_internal` or `esx_mafia` read as live server
  discovery.  The project's own honesty contract gives inert features visible labels in the tab
  (the Safe Mode / ScriptHook Bypass pattern), so the Resources tab now opens with a muted line -
  "Compiled-in sample list - nothing here is read from a server." - and the Executor combo repeats
  it in a hover tooltip; the README names the list's origin.
- docs/render_preview.py models the new line, so preview_resources.png shifts by exactly one line;
  the changed render is committed deliberately, the other seven stay byte-identical.

## The _cfx_internal error boxes were capability claims in refusal form (row 229)
- Execute answered the internal FiveM context with a red error modal, "You can't execute in
  _cfx_internal." - which implies execution works for every other resource.  It does not: this
  build reports only, and the general path says so out loud ("Execution is not implemented in this
  build", target and script size named).  A real click screenshot showed the misleading box.
- The resource stopper refused the same entry with "You can't stop _cfx_internal", implying stopping
  works elsewhere; the honest action is only the confirmed removal from the local list, which the
  confirmation states ("This does not stop it on the server") - safe for every entry, since the
  internal one is re-seeded on the next inject anyway.
- Both special cases are removed; `_cfx_internal` now takes the same honest paths as every other
  entry.  No handler signature changed and no payload was added - the refusals that implied
  capabilities were the defect.

## The splash hint wraps instead of clipping (row 228)
- A real inject screenshot showed the second splash hint running off the right card edge, clipped
  mid-word: a long localised key name (German numpad: "0 (ZEHNERTASTATUR)") or the qualified
  printable sentence ("... (not while typing in a text field).") makes the line wider than the
  460px card, and the old single centred TextColored only clamped the start cursor - the overflow
  still drew past the edge.
- RenderSplash now wraps the hints at word boundaries and centres each line; single-line strings
  render exactly as before, so the default-binding preview is unchanged.
- docs/render_preview.py models the same wrap and extends its asserts: beyond the depicted INSERT
  binding it proves the worst case (qualified printable sentence, two lines) fits the card width -
  and that assert caught a second defect on its first real run: the fixed 288px card was too short
  for a two-line hint, which would have hung the Continue button off the card edge.  The card now
  grows by one line advance per extra wrapped line, and the renderer's budget assert proves the
  worst case ends inside the grown card.

## The Docs & render row counts the graph it describes (row 227)
- Row 226's two new includes grew the regenerated README include graph from 79 to 81 edges (the
  generator rewrites the mermaid block itself, so the README moved in the same commit).  The BUILD
  Docs & render row kept quoting 79 - the silent-stale-doc-row class row 215 had already repaired
  once.  It now says 81, matching the README and a fresh generator run.
- Documentation-only batch; all eight previews stay byte-identical.

## The project compiles again: three latent compile blockers fixed (row 226)
- A real MSVC build reported what no static check here could see until it looked structurally:
  (a) the row-164 capture block (`static bool g_capturing` + `IsCapturing()`) had been inserted
  INSIDE `Config::Render()` - C2601 "local function definitions are illegal", plus "g_capturing:
  undeclared identifier" inside IsCapturing; the block now sits at namespace scope between
  ValidToggleKey() and Render().
  (b) `menu_style.h` writes `Settings::StyleMetricK()` in ApplyStyle() but never included
  settings.h, and gui.h includes menu_style.h one line before settings.h - so every translation
  unit died with "Settings: is not a class or namespace" / "StyleMetricK: identifier not found".
  menu_style.h now includes settings.h and is self-contained.
  (c) menu.cpp's key gate pushes Activity::Push (rows 155/157) without including activity.h.
- Verified without a Windows compiler: a brace-depth trace puts both capture symbols at namespace
  depth with file depth 0, and a comment-stripped include-closure sweep reports zero translation
  units using a project namespace without an include path to its header; settings.h includes
  nothing local, so the new menu_style.h include creates no cycle.
- The explorer.exe/Windhawk lines in the submitted log are the debugger session around a failed
  build (the row-98 F5 scenario), not a second defect.

## The docs tables render whole on GitHub again (rows 224-225)
- Two rows of the README file-inventory table were wrapped over physical newlines (the config.cpp
  row broke after "switches", the kiero row after "(vendored,").  GFM tables are one line per row,
  so on GitHub the table ended mid-inventory and the continuation lines rendered as loose paragraph
  text.  Both rows are single lines again.
- Row 106's fix description quoted the diagnostics suffix as `| settings: <path>` - an unescaped
  pipe inside a table cell splits it in GFM and shifted every later column of that row.  Escaped
  now, like the other literal-pipe quotes in the log.
- An escape-aware column-count sweep over every markdown table in README, BUILD, CHANGELOG and
  ci/README reports zero mismatches after these two fixes.
- Documentation-only batch; all eight previews stay byte-identical.

## The CI enable steps no longer send anyone to a merged branch (row 223)
- Four instruction sites hard-coded the merged PR-#1 branch name as the push target: BUILD.md
  section 7's bat block, ci/README.md step 4, its local-git bat block, and the echo inside
  ci/enable.bat.  Following them would commit and push the workflow enablement to a branch that is
  merged and gone.  All four are branch-agnostic now (YOUR-BRANCH / "the branch your checkout is
  on", named via `git rev-parse --abbrev-ref HEAD`), and ci/README.md's blob link points at `main`,
  which carries the merged file.
- The CHANGELOG header (previous commit in this row) claimed every entry lives on the old branch;
  it now names both session branches with the merge fact (PR #1 merged as 8d8ea8e, current since).
- The verbatim quote of GitHub's `remote rejected` error keeps its original branch name on purpose:
  it is a historical record of what the service answered, not an instruction.
- Documentation-only batch; all eight previews stay byte-identical.

## The DETACH flowchart shows the modal wait row 201 added (row 222)
- The README flowchart's DLL_PROCESS_DETACH node still read "g_unloading = true, Sleep 50 ms,
  Settings::Save flush" - the pre-row-201 sequence.  Row 201 inserted a bounded wait for an open
  modal dialog (80 x 25 ms, capped ~2 s) between g_unloading and the Sleep, because destroying the
  ImGui context under a live dialog left the rest of that frame drawing with no context; the
  flowchart never picked the step up.  The U1 node now lists it in code order.
- Documentation-only batch; all eight previews stay byte-identical.

## The two cards are compact again, and supported bindings survive Config (rows 220-221)
- A real inject screenshot showed the splash card filling the whole shell with ~300px of dead
  canvas under its content: SetNextWindowSize(460x288) loses to style.WindowMinSize (900x540 times
  scale) because ImGui clamps every window without AlwaysAutoResize in CalcWindowMinSize
  (imgui.cpp:6899-6900).  The key gate card had the same clamp.  Both cards now push their own size
  as ImGuiStyleVar_WindowMinSize around their Begin/End; the shell keeps the global minimum.  The
  previews always modelled the compact cards, so this fix makes reality match them - all eight
  renders stay byte-identical.
- The row-142 stored-key reset used !ValidToggleKey() alone, which includes printable keys - but a
  printable binding is a supported ini-only binding (rows 166/194/210).  Opening Config once
  destroyed it, with a toast claiming it "flips while typing" that the row-164 input guard
  disproves every frame.  The reset now uses Load's predicate (!printable && !ValidToggleKey), and
  both sites name the two failure classes honestly: no scancode = can never fire, a real key
  outside both allow-lists (numpad, modifiers) = flips while typing.

## The scale row-log entries point at 1.93 and at real imgui.cpp lines (rows 218-219)
- BUILD.md row 76 still described the interface scale as applied "through ImGui 1.92's
  style.FontScaleMain" and "because 1.92 bakes a font" - present-tense mechanism claims about the
  vendored copy, which is 1.93.0 (row 214 fixed the same drift in config.cpp, README and the
  troubleshooting section but missed this row-log entry).  Both corrected to 1.93.
- Its vendored line pointers had drifted with the upgrade: imgui.cpp:9104 and :9214 are blank lines
  in 1.93.0.  Re-verified against the vendored file - FontSizeBase is initialised from
  font->LegacySize at imgui.cpp:9218, and the FontScaleMain factor is applied at :9327.
- Row 91's FontScaleDpi pointer (imgui.cpp:9215, also blank in 1.93.0) corrected to :9328, and the
  README interface-scale bullet no longer contradicts itself across two lines ("1.93's
  FontScaleMain" ... "1.93 bakes a font").
- Documentation-only batch; all eight previews stay byte-identical.

## The runtime files are named where readers look for them (rows 216-217)
- BUILD.md's runtime-assets section listed only what the DLL reads (EBengin.png, Bahnschrift.ttf).
  The two files it writes next to itself were unnamed: settings.ini surfaced only in the toggle
  paragraph, and EBengin.log (rows 188-193) not at all - in the very document whose troubleshooting
  entry (row 209) now sends readers to that log first.  Section 3 gains a written-at-runtime
  paragraph naming both (one timestamped line per inject, 64 KB cap, safe to delete, neither needs
  shipping), and section 9's Hook & lifetime row names the trace.
- The README docs table listed seven previews; preview_key.png - embedded in the prose since
  row 182 - never got its table row, so the file inventory and the prose disagreed about how many
  previews exist.  The table lists all eight, with the key gate card described like its siblings.
- Documentation-only batch; all eight previews stay byte-identical.

## Present-tense version tags say 1.93.0, and the Docs & render row is whole again (rows 214-215)
- Three present-tense texts still named ImGui 1.92/1.92.9 while the vendored tree is 1.93.0: the
  config.cpp interface-scale comment, the README interface-scale bullet, and the BUILD
  troubleshooting answer to a stale-imgui.h compile error ("make sure the vendored tree really is
  1.92.9" - advice that would fail against the shipped tree).  All three now say 1.93/1.93.0; the
  migration-history mentions ("upgraded from 1.75 via 1.92.9") are correct history and stay.
- The BUILD "Docs & render" row listed seven previews (preview_key.png, added in row 182, was
  missing), fused two sentences into one ungrammatical run ("The Triggers pane is never depicted
  README include graph regenerated..."), and carried a stale "(35 nodes, 63 edges)" count three
  regenerations behind the actual 79.  The row lists all eight previews, the sentence is split,
  and the count matches the graph it describes.
- No UI surface involved; all eight previews stay byte-identical.

## Comments document their own functions, and tooltips stop printf-ing user paths (rows 212-213)
- PushRecent had been inserted between ReadFileText's contract comment and ReadFileText itself,
  so the "Returns 0 on success, 1 open failure, 2 empty, 3 oversized" block documented a function
  with no return codes at all, while the actual shared read path - whose codes every caller
  branches on - carried none.  Each comment sits on its own function again.
- Three tooltips passed user-controlled file paths as the printf format string: the Save button's
  "Save to <name> (Ctrl+S; ...)", the Revert button's "Revert to <name>", and the Recent popup's
  "<full path>\nRight-click removes the entry."  A '%' in a filename is legal on Windows, making
  each of them the same undefined-behaviour family runtime-fix #15 removed from the list texts -
  reintroduced on rows added later.  All three now build the string first and pass it through
  SetTooltip("%s", ...); a full sweep verified every remaining SetTooltip site is literal or
  already "%s".
- No UI surface changed in its default state; all eight previews stay byte-identical.

## The splash hint says exactly what the toggle does, and dialogs belong to the game window (rows 210-211)
- The splash hint claimed "<key> toggles this overlay at any time." - overstating the one binding
  class the present loop deliberately suppresses: a printable key (possible only via hand-edited
  ini; the capture refuses them since row 166) does not flip the overlay while a text field
  consumes input (rows 155/164).  The hint keeps the confident sentence for non-printable
  bindings and names the qualifier for printable ones.  The preview still depicts the default
  INSERT binding, so preview_splash.png is byte-identical.
- Every dialog the UI raised was unowned or title-guessed: Gui::ModalBoxA passed NULL to
  MessageBoxA, and both file dialogs used FindWindowA(NULL, "FiveM").  A NULL answer - any host
  window titled differently - or the wrong top-level window leaves the dialog BEHIND a fullscreen
  game while the modal loop has already stalled the render thread: what the user sees is a hang
  with no visible cause, the exact family the trace log (rows 188-193) exists to shorten.  All
  dialogs are now owned by the real host window: main.cpp resolves it from the swap chain
  (sd.OutputWindow) into its global `window`, gui.h declares it extern, and ModalBoxA plus both
  ofn.hwndOwner assignments use it.  No title guess remains in the codebase.
- All eight previews stay byte-identical (no scene depicts a dialog).

## The OS floor is named, and "nothing appears in-game" starts at the trace log (rows 208-209)
- The requirements table said "Windows 10 / 11 x64", but GetDpiForWindow - called in InitImGui
  (row 154) and per frame in hkPresent (row 187) - exists in user32.dll only since Windows 10
  1607.  On older builds the DLL fails to load with an entry-point error that no document
  mentioned.  The BUILD.md OS row now names 1607+ with the reason; the README quick-version line
  follows.
- The Troubleshooting entry "Builds fine but nothing appears in-game" predated the hook trace log
  and still claimed kiero::init "loops until it succeeds" - the retry has been bounded (~60 s)
  since the runtime-fix table's own row 8.  The entry now starts at `<DLL folder>\EBengin.log`
  and lists its five outcome lines, uses a missing log as the DLL-never-loaded signal (wrong
  architecture / wrong injector target), and drops the stale wording.
- Documentation-only batch; all eight previews stay byte-identical.

## The README architecture prose agrees with the regenerated graph (rows 206-207)
- Two bullets contradicted main.cpp's actual includes: one listed four includes and "no tab body"
  while rows 164/188/201 had grown the list to nine project includes plus <fstream> - config.h
  among them - and one claimed tabs.cpp "is the only file that includes the five tab headers"
  with main.cpp including "neither".  Both now name reality: the nine includes and what each
  reaches (Init calls, Config::IsCapturing in the toggle poll, Gui::ModalActive in the DETACH
  wait), and tabs.cpp as the only file including ALL FIVE tab headers, with config.h as
  main.cpp's single named exception.
- The header-edge bullet claimed "only three headers include anything" with a ten-name edgeless
  list the graph right above refutes: five headers have outgoing project edges (config.h, exec.h,
  gui.h, menu.h, resources.h), and gui.h - listed as edgeless - has three.  The bullet now lists
  the five with their exact edges and names gen_include_graph.py as the authority.  The
  obsolete-API bullet also still excused demo.h for using Columns(), although demo.cpp moved to
  BeginTable and no Columns()/NextColumn()/SetColumnOffset() call exists in any project file; the
  parenthetical records the migration instead.
- Documentation-only batch; all eight previews stay byte-identical.

## The session trace confirms before it dies, and the position save stops rewriting the ini (rows 204-205)
- The session log is the only timestamped trace the session leaves - in memory only, and
  unrecoverable once cleared - but its Clear button fired immediately, while every other loss of
  the same family (editor Clear/Revert, resource stopper, interface reset) confirms first.  One
  mis-click erased the trace without a word.  Clear now asks through the shared Gui::ModalBoxA
  (YESNO + warning icon) and only clears on IDYES.
- SaveWindowPos went through the full Save(): roughly twenty WritePrivateProfileStringA calls -
  each its own open/write/close of the same file - for what is a two-integer update.  The
  position saver throttles by time OR movement, so a fast drag or an Aero-Snap fling rewrote the
  entire ini on nearly every frame.  It now writes just [window] x/y directly, keeping the
  once-only save-failure latch that row 191 retires on the next successful Save.
- No preview scene shows a dialog, and the landing scene is unchanged: all eight previews stay
  byte-identical.

## The last pure-red surfaces join the ramp, and diagnostics name the trace log (rows 202-203)
- Two shipped surfaces still used pure red (255,0,0) after rows 157/175/178 moved the project to
  the lighter ramp red: the warn toast kind bar - ~2.6:1 on the card navy, under the 3:1 WCAG AA
  ask for non-text marks, with a comment claiming "same red as the error texts" while those had
  already moved - and the resources "doesn't have any files." line, ~3.4:1 on the list navy,
  under AA for normal text.  Both now use (255,107,107): one red ramp in the project.  The
  preview renderer mirrors the constant, so preview_config.png changes with this row - the only
  intended pixel change in the batch.  demo.cpp keeps its sample red on purpose: it is the
  unwired widget cheat-sheet, not shipped UI.
- The hook trace log (rows 188-193) is the primary artifact for "the menu never opened" reports,
  but Copy diagnostics - the snapshot a report from another machine carries - never mentioned it.
  The diagnostics line now ends with "log: <path>" plus " [missing]" when the file does not
  exist: missing means either an old build or a folder nobody can write to, and both are answers
  the report needs.  The include graph was regenerated for the new config.cpp -> assets.h edge
  (78 to 79 edges).

## No stray imgui.ini in the game folder, and DETACH waits for open dialogs (rows 200-201)
- io.IniFilename was never set, so ImGui's default context auto-saved its own window state to
  imgui.ini in the HOST process' working directory - a stray file in the game folder (silently
  failing on write-protected installs) duplicating what settings.ini next to the DLL already
  persists deliberately.  InitImGui now sets io.IniFilename = NULL; settings.ini stays the one
  persistence route.
- DllMain DETACH mitigated the unload race with a fixed Sleep(50), but a modal dialog
  (Gui::ModalBoxA) blocks the render thread inside a frame for as long as the user leaves it
  open.  Past 50 ms the ImGui context was destroyed under a live dialog, and once it closed the
  rest of the frame drew with no context.  gui.h now exposes Gui::ModalActive() (an
  inline-function static set around MessageBoxA), and DETACH waits for it in 25 ms ticks - capped
  at ~2 s so the loader lock cannot stall indefinitely; past the cap it proceeds exactly as
  before, honestly commented.
- The include graph was regenerated for the new main.cpp -> gui.h edge (77 to 78 edges).
- No UI surface involved; all eight previews stay byte-identical.

## The include graph sees commented includes, and the uptime counter is named honestly (rows 198-199)
- docs/gen_include_graph.py demanded end-of-line right after the closing include quote, so every
  include carrying a trailing "// why" comment - the house style in main.cpp and six other files -
  was invisible to it.  The graph was missing 14 edges, among them all six of main.cpp's commented
  includes and the settings.cpp -> config.h edge row 194 added, while the README claimed the graph
  was "generated from the actual #include lines".  INCLUDE_RE now accepts a trailing line comment;
  re-running the script takes the graph from 63 to 77 edges and makes the claim true again.
- README and the default.cpp comment sold the landing uptime counter as "session uptime", but
  Perf::Sample() ticks only while the overlay renders - perf.h itself says "menu-open uptime" and
  the card label says "Menu open".  Both README mentions and the comment now say menu-open uptime,
  matching the header, the label and the actual behaviour.
- No UI surface involved; all eight previews stay byte-identical.

## The settings-folder button opens Explorer again, and the frame flowchart caught up (rows 196-197)
- "Open settings folder" passed lpFile = NULL with "/select,..." in lpParameters whenever
  settings.ini existed.  ShellExecute needs an object to open, so the call failed in exactly the
  normal returning-user case and every click answered with the warn toast - "Explorer could not be
  started" - instead of opening anything.  The highlight route now launches explorer.exe with a
  quoted /select, command line (DLL folders with spaces work); the first-start fallback keeps
  verb "explore" on the folder itself, unchanged.
- The README hkPresent flowchart predated several shipped rows: the InitImGui node omitted the
  DPI sample / Settings::Load / ApplyScale order, the per-frame node omitted the live DPI
  re-sample (row 187), the toggle node omitted the capture and text-input suppressions
  (rows 155/164), and the DETACH chain omitted the Settings::Save flush.  All four mermaid nodes
  now mirror the shipped code.
- No UI surface changed in its default state; all eight previews stay byte-identical.

## A dead toggle key can no longer brick the overlay, and a failed session no longer overwrites the ini (rows 194-195)
- A hand-edited settings.ini could carry a VK that never fires on any keyboard (mouse buttons,
  unassigned codes).  The load clamp only range-checked 1..254, so the toggle stayed dead, the
  menu could not open, and Config's one-time invalid-key reset - which needs an open menu - never
  ran: the overlay was bricked with no way back short of hand-editing the file.  Load() now
  validates with the same predicate Config uses, exported from config.h as
  Config::ValidToggleKey.  Printable bindings stay allowed at load: the present loop already
  guards them while a text widget consumes input (row 164) and Config retires them on first visit
  (row 142).  An unassignable key resets to INSERT with an activity line naming the dead key.
- DllMain DETACH flushes settings unconditionally.  When the hook never attached (non-D3D11 host,
  bind refused, gave up after retries), Load() had never run and Current still held the compiled
  defaults - unloading then overwrote a perfectly good settings.ini with them.  Save() refuses to
  write before Load() has run; a failed session leaves the file exactly as it found it.
- No UI surface involved; all eight previews stay byte-identical.

## The trace log documents itself - and closes its sessions (rows 192-193)
- README documented settings.ini in three places but never mentioned EBengin.log, the trace file
  row 188 put next to it, and the startup / DETACH flowcharts predated the trace lines.  The
  main.cpp table row, the give-up and hook-live flowchart nodes, the DETACH chain and the
  persistence bullet now name the log: one timestamped line per inject - attach, the failure that
  stopped it, or the clean unload - capped at 64 KB.
- The trace recorded the attach and every failure outcome but never a clean end, so a report
  could not tell an orderly unload from a crash or a task kill.  DllMain DETACH now closes the
  session with a "session ended (clean unload)" line after kiero::shutdown - present in the log
  only when DETACH actually ran, which is exactly the distinction the line exists to draw.
- No UI surface involved; all eight previews stay byte-identical.

## Trace honesty on unload, and a retireable save-failure flag (rows 190-191)
- The hook retry loop also ends when an unload arrives mid-retry, and that path fell through to
  the "gave up after 120 attempts" log line - a lie in the trace for what was a clean shutdown.
  The unload exit now returns before the gave-up line, which belongs to a full 120-attempt
  exhaustion only.
- The settings save-failure flag latched for the whole session: a one-off folder lock (scanner,
  sync client) kept the About block reporting "settings not writable - changes are lost on
  restart" long after the next save had succeeded.  A successful write now retires the flag, with
  an activity line naming the recovery, so the permanent About warning matches reality again.
- No UI surface changed in its default state; all eight previews stay byte-identical.

## Hook outcomes leave a trace, and the trace stays bounded (rows 188-189)
- A failed hook attach was a silent no-op: the inject looked like "did nothing", with no trace
  anywhere to tell "not a D3D11 host" from "present bind refused" from "gave up after 120
  retries".  MainThread now writes one timestamped line per outcome - attach, bind refused, not a
  D3D11 renderer, swap chain module missing, gave up - to EBengin.log next to the DLL, the same
  folder convention settings.ini already uses.  Deterministic outcomes return straight after
  logging, so the gave-up line belongs to the retry path alone.
- The trace log survives injects, so it would grow without bound on a session-heavy machine.
  LogHookLine caps it at 64 KB: past that it starts over instead of appending forever.
- No UI surface is involved; all eight previews stay byte-identical.

## Scale-consistent defaults and live DPI (rows 186-187)
- WindowPadding, ItemSpacing, IndentSpacing, ScrollbarSize and GrabMinSize keep ImGui's compiled
  defaults - ApplyStyle never sets them, so the row-185 compiled-base rescale could not cover
  them, and at any scale other than 100% they stayed at 100% size: cramped rhythm around doubled
  text, a hairline scrollbar under a doubled frame.  ApplyScale now scales exactly those five by
  the ratio between the previous and the new ScaleFactor (a no-op while k is unchanged, so calls
  stay idempotent), and ApplyStyle publishes k = 1 through Settings::StyleMetricK() so the
  tracking survives a restyle.
- The host DPI was sampled once at init, so dragging the game window to a monitor with a
  different DPI left fonts and every DPI-folded metric at the old factor until re-inject.
  hkPresent re-samples GetDpiForWindow per frame (one cheap call) before NewFrame and re-runs
  ApplyScale only when the factor actually changed; the font rebake, the WindowMinSize change and
  the row-186 metric ratio ride the existing scale-change paths, and the shell's reachability
  re-clamp keys off WindowMinSize anyway.
- Renders run at 100% interface scale and DPI 1, where both changes are no-ops, so all eight
  previews stay byte-identical.

## Key scene in the renderer docstring, scale-safe style metrics (rows 184-185)
- The render_preview.py docstring never mentioned the row-155 key scene: neither the opening
  paragraph nor the usage block listed --key / preview_key.png, and the constants list skipped
  the key card.  All three now carry it alongside the other seven scenes.
- Roundings and FramePadding are absolute style values, not font-derived: FontScaleMain doubles
  text and frame heights while a 6px radius on a doubled frame reads as a bevel and 8x6 padding
  squeezes 40px text - and since ApplyStyle's bases carry no DPI, a 144% DPI host showed the same
  squeeze at 100% interface scale.  Settings::ApplyScale() now rescales the ApplyStyle bases
  (WindowRounding, ChildRounding, FrameRounding, PopupRounding, TabRounding, ScrollbarRounding,
  GrabRounding, FramePadding) with the same ScaleFactor() the rest of the shell uses; recomputed
  from the compiled constants on every call, so it stays idempotent, and at k == 1 it reproduces
  ApplyStyle exactly - which is why every preview (rendered at 100%, DPI 1) stays byte-identical.

## README key-render placement and open-header hover feedback (rows 182-183)
- The row-155 README sentence about the key render sat inside the HUD paragraph - directly above
  the HUD image - and called --key the seventh render, although the landing tab is numbered
  seventh and the key card is the eighth; the key image was never embedded either.  The sentence
  moved into its own paragraph after the landing-tab render, renumbered eighth, with
  docs/preview_key.png embedded like every other scene.
- An open collapsing header rests on the solid Header colour, and ImGuiCol_HeaderHovered used the
  same RGB at 100 alpha - the hover overlay blended to exactly the colour underneath, i.e. zero
  hover feedback on open headers (the resources dump preview sits in one).  HeaderHovered now
  uses the hover ramp blue (55,120,181) at the same alpha: subtle over the window background on
  closed headers, clearly readable over the solid header.
- No preview scene depicts a hover state, so all eight previews stay byte-identical.

## Two unthemed tokens: table borders and the scroll groove (rows 180-181)
- The shell's two-column divider is a table inner border, and table borders read
  ImGuiCol_TableBorderLight/Strong - neither was themed, so the one vertical rule in the
  project drew in ImGui's default grey instead of the navy every other rule uses (and the
  preview renderer models).  Both tokens now carry the Border navy (42,74,123).
- The scroll groove of any overflowing child - the activity log once it fills - kept ImGui's
  near-black default track under the themed grab.  ImGuiCol_ScrollbarBg now uses the rail navy
  (16,31,52), reading as part of the same surface family.
- The renderer already drew the divider in navy and no preview scene shows a scrollbar, so all
  eight previews stay byte-identical.

## Perf graph: AA-readable bad frames, right-pinned now (rows 178-179)
- The perf graph's bad-frame bars used pure red at 200 alpha on the navy canvas, which measures
  ~2.6:1 - under the 3:1 WCAG AA asks of non-text marks.  They now use the lighter red
  (255,107,107) of the row-157/175 ramp at the same alpha, ~3.7:1 on the canvas, keeping a single
  red in the project.
- While the frame-time history was shorter than the 120-slot ring, the bars grew from the left
  edge, so "now" sat mid-canvas for the first two minutes of every session - and the hover
  readout mapped to that left-pinned layout.  The newest bar is now pinned to the right edge
  (offset by the missing slots), the way every ring-buffer graph reads, with the hover index
  shifted by the same offset.
- The preview renderer models the graph as layout only (no bars), so all eight previews stay
  byte-identical.

## Clipboard writer: bounded retry and a Unicode format (rows 176-177)
- The shared clipboard writer (Config's "Copy diagnostics", the landing tab's session-log copy,
  the trigger and dump-path copies) tried OpenClipboard exactly once, so a single transiently
  busy viewer - Excel mid-copy, a clipboard manager syncing - produced a "clipboard busy" error
  although the same copy a moment later would have succeeded.  It now retries up to five times at
  15ms intervals before reporting failure; the whole budget stays far below anything a user
  perceives as a stall.
- The writer offered CF_TEXT only, i.e. ANSI: text outside the active code page (an umlaut in a
  path, a non-ASCII Lua string) arrived mangled in Unicode-aware paste targets, which is most of
  them.  It now also offers CF_UNICODETEXT via MultiByteToWideChar(CP_UTF8); the second global
  follows the same free-on-refuse rule as the first, and either format being accepted counts as a
  successful copy.
- No visual surface is involved; all eight previews stay byte-identical.

## Gate focus on every open, AA red in the About block (rows 174-175)
- The key field focus was a one-shot static: it fired on the very first open only.  Close the
  overlay while the gate is still locked and reopen it, and the window took focus but the field
  did not - Enter did nothing until a click or Tab moved into the field.  The gate now consumes
  the reopen flag (the sidebar, which normally clears it, never runs while gated) and drives
  both handoffs from it, window and field; a successful unlock hands the flag on to the splash
  so its Continue button keeps the keyboard handoff every other open path gives it.
- The Config About block drew the "settings not writable - changes are lost on restart" line in
  pure red, which measures ~3.7:1 on the pane and sat under WCAG AA for normal text - the same
  defect family the row-157 wrong-key line had.  It now uses the lighter red (255,107,107,
  ~5.2:1) established there, keeping one red ramp in the project.
- Both are conditional states (a reopen, a save failure), so the default-state previews - all
  eight - stay byte-identical.

## Resources list consistency: one background, explained empty (rows 172-173)
- The stopper list box used a brighter frame background (31,58,97) than the very same list
  family on the other two Resources sub-tabs (20,38,63), so switching sub-tab visibly re-lit the
  box for no reason.  It now uses the shared list background.
- A resource filter that matched nothing left an empty list box with no explanation on both
  filtered lists (file browser and stopper) - an empty box reads as a broken list, not as a
  filter result.  Both lists now count their visible rows and draw a muted "No resource matches
  the filter." line inside the list box when the count is zero; keeping the hint in-list means it
  costs no layout height in a shell that has no scrollbar to give.  The genuinely empty case
  ("No resources left to stop.") keeps its own distinct message.
- The preview resources scene shows the file list with an empty filter, so all eight previews
  stay byte-identical.

## Toast preference consistency (rows 170-171)
- The key card rendered its toast stack regardless of the "Toast notifications" preference,
  while the shell gates its stack on exactly that pref; with the pref off, a wrong-key attempt
  still popped a card over the gate.  The key card now gates RenderScreen() on
  Settings::Current.Toasts, exactly like the shell gates Render().
- With the pref off, nothing rendered and therefore nothing aged: pushes kept storing, and up
  to four unseen cards burst onto the screen the moment the pref was switched back on.
  Toasts::Push now discards entries while the pref is off - the activity log keeps the trace
  either way, and Config's switch-off Clear() remains as the second guard.
- Both are preference-path fixes; nothing visual changed in the default state, and all eight
  previews stay byte-identical.

## Modal dialogs no longer leak time or key edges (rows 168-169)
- A modal dialog stops the frame loop for its whole lifetime, so both gap heuristics - the
  shell's reopen fade and the row-167 capture retirement - read the dialog duration as idle
  time: every confirmation dialog longer than half a second replayed the open fade and reset
  the keyboard nav cursor when it closed, on top of a shell the user never left, and a reset
  confirmation opened on the Config tab could retire an armed capture on its way out.
  Gui::ModalBoxA now measures its wall-clock duration (GetTickCount64, because ImGui::GetTime
  freezes with the frame loop) and publishes it through Gui::ModalSeconds(); both heuristics
  subtract what is new since their last frame before applying the 0.5s test.
- The row-165 drain covered only the toggle edge, but an armed key-capture scan reads every
  key - so a press during a dialog survived as a stale edge and bound itself on the first frame
  after the dialog closed.  The wrapper now drains the whole keyboard range (vk 7..254; 1..6 are
  mouse buttons, which neither the capture scan nor the toggle poll reads) after every dialog.
  Presses inside a dialog belong to the dialog.
- Both are timing/input-path fixes; nothing visual changed, and all eight previews stay
  byte-identical.

## Capture behaviour: silent navigation, retired on leave (rows 166-167)
- While a key capture was armed, every press that was neither ESC nor bindable hit the refuse
  branch - including pure navigation keys (Tab, Enter, arrows, modifiers).  Keyboard users
  moving around during a capture therefore collected a warn toast and an activity entry per
  press.  The refuse branch now only answers printable keys (A-Z, 0-9, space), the ones a user
  could plausibly expect to bind; everything else is navigation and is ignored silently, with
  the capture staying armed.
- An armed capture survived tab switches although its scan only runs while the Config tab
  renders - and since row 164 the present loop suppresses the toggle flip while any capture is
  armed, so on every other tab, with no capture UI visible, the toggle key simply looked dead.
  Config::Render now retires the capture when more than half a second passes without a Config
  frame (the same gap heuristic the shell uses for its reopen fade), which restores the toggle
  key the moment the user leaves the tab.
- Both are capture-path fixes; nothing visual changed, and all eight previews stay
  byte-identical.

## Toggle-edge hygiene: capture guard and modal drain (rows 164-165)
- A toggle-key press while the Config key capture was armed flipped the overlay shut
  mid-capture: the present-loop poll runs before Menu::Render() and consumed the key edge, so
  the capture never saw the press and the menu just closed.  The capture flag moved from a
  function-local static to namespace scope behind a Config::IsCapturing() accessor, and the
  present loop suppresses the flip - while still draining the edge - whenever a capture is
  armed, so one press can no longer both bind and close.
- MessageBoxA spins a modal message loop, which stops the present loop and its toggle-edge poll
  for as long as the dialog is open; a toggle press during any confirmation dialog therefore
  survived as a pending edge and flipped the menu on the first frame after the dialog closed.
  All 23 dialog sites in the project (exec, config, resources, luamenus) now route through a
  Gui::ModalBoxA wrapper that drains the toggle edge on the way out.  Dialog texts, flags and
  return handling are untouched.
- Both are input-path fixes; nothing visual changed, and all eight previews stay byte-identical.

## Toast anchor for small surfaces and card drop shadows (rows 162-163)
- Toasts::Render() anchored the stack to the calling window's bottom-right corner, which is
  right for the 900x540 shell but put the toast cards on top of the key card's own Unlock button
  and hint line inside its 460x232 window.  The stack painter is a shared helper now: Render()
  keeps the shell anchor, and the new RenderScreen() draws the same stack - same ageing, same
  click-to-dismiss rules - anchored to the display's corner on the foreground draw list.  The
  key card calls RenderScreen(); nothing else changes call sites.
- The shell's soft three-rect drop shadow was missing on the two floating cards (splash, key),
  which read as pasted onto bright game scenes.  Both cards draw the same recipe one frame late
  on the background draw list, with the rect captured right after Begin where the fixed window
  size is final.  The preview renderer already modelled both shadows, so all eight previews
  remain byte-identical.

## Key paste tolerance and a scale-safe sidebar logo (rows 160-161)
- A pasted key carries invisible trailing whitespace - a space, tab or line end - and a
  password field shows dots, so such an attempt failed the compare with no clue why.  The
  compare now trims leading/trailing whitespace around the buffer (a small helper working on a
  local copy); the buffer itself stays as typed and is zeroed on success, the wrong-key path is
  untouched, and the check remains the local plaintext compare it was.
- The sidebar logo was placed with the raw constants 74 (image) and 58 (text fallback) while the
  sidebar width and the logo both scale with the interface scale, so at 70% or 200% the logo sat
  visibly left of centre in the rail.  Both offsets are derived from the scaled widths now; at
  100% the image offset becomes 74.5, i.e. exactly centred.  The renderer mirror moved with it,
  so the five sidebar previews shifted one pixel (intentional), while HUD, splash and key card
  stay byte-identical.

## Key-card nudge and logo-click cursor handoff (rows 158-159)
- Enter or Unlock on an empty password field used to produce the full wrong-key answer - red
  verdict line, "Key refused." activity entry, warn toast - for something that was never an
  attempt.  Empty submits now answer with a muted inline nudge in the verdict's line slot
  ("Type the key first - then Enter or Unlock."); no verdict, no log entry, no toast, no card
  height change, and the nudge retires on the next keystroke exactly like the verdict does.
- The sidebar logo home button (row 150) switched to the landing tab but left the keyboard nav
  cursor on whatever widget held it, so the next Tab/arrow jump started from the old tab - the
  row-152 defect on the home path.  The logo is an image item rather than a widget, so a click
  never moved the cursor by itself; it now sets the same handoff flag Ctrl+Tab uses, consumed
  by the sidebar loop in the same frame (first entry, since the landing tab has no row).
- Both are display/feedback-level; gate logic and tab persistence are untouched, and no
  preview changes (the nudge is a conditional state like the verdict and the Caps hint).

## Key-card feedback follow-ups (rows 156-157)
- The row-155 key is lower-case and compared case-sensitively, so with Caps Lock on every
  attempt failed looking exactly like a wrong key - nothing told the user why.  The card now
  reads the Caps-Lock toggle state (GetKeyState; no entered text is inspected), grows by one
  line while it is on and states "Caps Lock is on - the key is lower-case." in warn amber.
- The wrong-key line inherited pure red, which measures ~3.7:1 on the card background and so
  sat under WCAG AA for normal text; it now uses a lighter red at ~5.2:1.  It also answered the
  previous attempt even while the field was already being edited again - a fresh keystroke now
  clears the verdict (IsItemEdited, read directly after the input, before any later widget).
- Both changes are display-only; the gate logic (plaintext local compare, unlock once per
  inject) is untouched, and the default-state preview `docs/preview_key.png` is unchanged.

## Session key gate in front of splash and shell (row 155)
- The overlay opened straight into its splash and shell for anyone who could press the toggle
  key; the owner wanted a key in front of it.  A centred key card now replaces both until the
  key matches: password field (dots, keyboard-focused on first frame), Enter or the Unlock
  button submit, a wrong key answers inline in red plus an Activity entry and a warn toast,
  and success unlocks once per inject, zeroes the buffer and logs "Key accepted".
- The key is the owner's "eb", compared in plaintext against a local constant: this build talks
  to no server, so the card is an access gate for the overlay, not a security claim - and it
  says exactly that ("Local gate - no server is contacted.").  Nothing game-affecting sits
  behind it either; the gate is interface-level like everything else here.
- New preview render `python docs/render_preview.py --key` depicts the card (empty password
  field) as `docs/preview_key.png`; the seven existing previews are untouched by the change.

## Reset scope and a persistent dump target (rows 153-154)
- The settings reset assigned the whole Prefs struct, which silently dropped the recent-files
  list - editor history, not an interface preference, and exactly the kind of thing the buffer
  itself survives.  The reset now preserves Recent (and the dump-target name) across the
  assignment, and its confirm text names the HUD corner among what gets overwritten.
- The dump-target resource combo fell back to _cfx_internal on every inject, so anyone dumping
  into the same resource re-picked it every session.  The selection now persists by name in
  `[menu] resource` - by name, not index, because the stopper erases list entries and a stored
  index could otherwise name a stranger.  Resources::Init restores it when the list still
  carries it; combo click, list click and the stopper's erase all keep the stored name in step,
  and a reset leaves it untouched.

## Heading weight and Ctrl+Tab focus follow (rows 151-152)
- Every string in the shell rendered in one weight - Bahnschrift 20 regular - so tab headings
  carried hierarchy by colour alone.  A real bold instance of the variable font needs the
  FreeType builder (not vendored), and a larger header font would have cost every pane four
  pixels of vertical budget (the Config left pane runs at 530 of 532).  Gui::SectionTitle now
  fakes the weight: a second draw-list pass of the same string, offset by a scale-aware 0.7px,
  same font and colour - zero layout cost.  The five shell previews model the doubled draw.
- Ctrl+Tab cycling left the keyboard-nav cursor on the old tab's widget, so the next Enter or
  Space activated something the user could not see in the new tab.  The cycle handler now sets
  the same one-shot flag the reopen path uses: the nav cursor lands on the new tab's sidebar
  entry next frame.

## A way home: logo button and Ctrl+Tab tab cycling (row 150)
- The landing tab - diagnostics graph, uptime line, session activity - was a one-way street:
  once a sidebar tab was open, nothing in the shell returned to Tab 0 for the rest of the
  session.  The sidebar logo is now a home button: hover shows the hand cursor and a tooltip,
  a click returns to the landing tab and persists it like any sidebar click; the text-logo
  fallback (missing EBengin.png) works too, since its Dummy is an item as well.
- Ctrl+Tab / Ctrl+Shift+Tab now cycle all five tabs, landing included, persisted the same way.
  Safe while typing: the vendored TextEditor consumes plain Tab for indent but checks !ctrl,
  so the chord reaches the shell untouched.  The Config nav hint grew the sentence
  "Ctrl+Tab switches tabs." and wraps to two lines in the right pane; the renderer's
  single-line assert was replaced with a wrap-width check, since TextWrapped wraps by design.

## HUD plate corner setting and Recent-entry removal (rows 148-149)
- The FPS HUD plate sat pinned top-left, the one corner game HUDs (minimap, score strips)
  almost always own.  `[ui] hudcorner` (0=top left .. 3=bottom right, clamped on load) and a
  "Corner" combo under the FPS HUD switch - greyed together with the HUD - now place the plate
  in any screen corner; the plate math keeps the 10px margin and the AA backing from row 147.
  The three-line keyboard-nav hint moved from the Interface pane to the bottom of the right
  pane (one line at that width): the combo row exhausted the left pane's third-of-column
  budget, and the preview renderer's fit-assert refused to render the overflow, so the layout
  was fixed before any compiler saw it.  Label is "Corner", not "Plate corner" - at the shell's
  20px font the longer label plus the widest value clip the pane; the tooltip says the rest.
- Right-click removes an entry from the Recent popup: a renamed or moved file used to linger
  in `[recent]` until picking it failed with the unreadable-file dialog.  Removal persists
  immediately and lands in Activity plus an info toast; both tooltips name the interaction.

## AA contrast pass: muted ink and the HUD plate (row 147)
- Measured audit (WCAG relative luminance, not eyeballed): the muted secondary ink - brand
  accent at 60% alpha, i.e. blend (64,109,154) - held 2.8:1 on the canvas and 3.1:1 on the rail,
  below the 4.5:1 AA line, at 18 call sites plus ImGui's TextDisabled.  The HUD plate fared worse:
  with backing alpha 160 a bright game scene washed the credit line out to ~1.0:1, and the accent
  FPS line sat at 4.4:1 on the plate.
- Phase 2 token: ColMuted #7DAFE1 (a lighter tint of the accent) in menu_style.h - 6.6:1 on canvas,
  7.2:1 on rail, 5.0:1 on panel, so the visual hierarchy survives the fix.  It replaces all 18
  hardcoded sites and TextDisabled; the HUD backing goes 160 to 220 and the FPS line accent to
  white (12.7:1 worst case).  Accent #5D9CD6 stays reserved for the interactive ramp and surfaces
  where it measures >=4.5:1.
- Every change carries a `// UI:` marker per the upgrade prompt; the preview renderer models the
  new token and backing, all seven previews re-rendered, HUD and shell visually checked.

## Recent files: one click back to the last scripts (row 146)
- Nothing remembered which files the editor had used, so every session retreaded the file
  dialog.  The five last loaded/saved paths now persist in `[recent]` of settings.ini, newest
  first and de-duplicated; Load success and Save success both push into the list.
- The Executor's button row grows a "Recent" button (grey while the list is empty) whose popup
  lists base names with full-path tooltips; picking an entry reloads through the very same
  guarded path as the dialog - unreadable/empty/oversized files report as before, and a
  non-empty buffer still asks before being replaced.
- settings.ini carries `[recent] 0..4`; a hand-edited or truncated list cannot break the load,
  because every entry fails at use with its own message.  The executor preview shows the button.

## The shell grows a close button and a talking title bar (rows 144-145)
- `Begin` passed `p_open = NULL`, so the title bar had no close cross and the toggle key was
  the only way out of the overlay - a mouse-only user had none.  `p_open` is now `&IsMenuOpen`,
  and the cross closes the shell exactly like the bound key does.
- The title bar read "EBengin" in every tab; it now names the open tab ("EBengin - Executor",
  "… - Resources", "… - Menus", "… - Config"; the landing tab stays "EBengin"), so an
  alt-tabbed user sees where they are.  A `###ebengin_shell` suffix keeps the window id stable
  across tab switches, so ImGui state and the persisted window position survive them.
- The preview renderer draws both affordances (title per tab, cross as two strokes - the
  renderer font has no multiplication sign); the five shell previews were re-rendered, HUD and
  splash previews stay byte-identical.

## F5 opens the output folder instead of the invalid-Win32 dialog (row 143)
- A DLL has no executable entry point, so Visual Studio's F5 / Start Debugging answered with
  "…\EBengin UI.dll is not a valid Win32 application" - row 98 documented that away, but the
  dialog kept scaring people.  The tracked `.vcxproj.user` now configures the local debugger
  (WindowsLocalDebugger, command `explorer.exe`, arguments `"$(TargetDir)"`, attach off), so
  starting the project opens the folder that holds the fresh DLL; nothing executes or loads
  the library, and real debugging still goes through Attach to Process or a host command.
  The warning under build option C, the §8 troubleshooting entry and the README heads-up
  were rewritten from "dialog expected" to the new behaviour.

## The toggle-key capture refuses keys that flip while typing (row 142)
- The toggle poll sits in the present loop, outside any widget focus, so a letter, digit or
  modifier bound as toggle key flipped the overlay on every keystroke of it - and the capture
  happily accepted any of them.  It now takes only keys nobody types (F1-F24, INSERT, DELETE,
  HOME, END, PGUP/PGDN, the lock keys), refuses everything else with a reason while staying
  armed for the next press, and a settings.ini that already carries such a key is reset to
  INSERT on the first Config frame, named in a warn toast.

## Save asks before burying edits made outside the menu (row 141)
- If Notepad or a second session wrote the file behind the buffer after it was loaded, a
  one-click Save buried those edits without a word - the dialog's overwrite prompt only covers
  names picked in the dialog, not the file the buffer already belongs to.  Every successful
  Load, Save and Revert now stores the file's last-write time, and a Save onto that same file
  compares the stamp first: changed on disk means a question, and a cancel lands in the
  activity trace.  The stamp rides on GetFileAttributesEx - no open, no handle, no polling.

## The server pane stops pretending and opens the Dumps folder (row 140)
- "Server IP: not connected" was a phantom state: nothing in this build connects anywhere, and
  reading a real server IP would mean reading game state - outside this menu by design.  The
  pane now says exactly that in two muted lines, and offers the one server-facing artifact the
  build does produce: an "Open dumps folder" button that creates <DLL folder>\Dumps if it is
  missing and opens it in Explorer, with the same failure reporting as the settings-folder
  button and a tooltip naming the Resources tab's Save All as its writer.
- The Config preview follows the new pane (lines, button, fit asserts), and its About block's
  pinned ImGui tag was corrected to the vendored 1.93.0 while the renderer was open.

## Pin status recorded for TextEditor and the Font Awesome blob (docs only)
- The vendored freeze note in BUILD.md §9 now covers all three pinned trees: kiero/MinHook
  (frozen by decision, hook layer), TextEditor (upstream's only later delta targets ImGui
  <= 1.92.5 while this copy is verified clean against 1.93, and the successor project is a
  rewrite with a different API), and the Font Awesome blob (regeneration needs external
  tooling plus a glyph audit of every icon in use).  Upgrade requests for any of the three
  are answered with the note, not with code; nothing but docs changed.

## Vendored refresh: ImGui 1.93.0 and stb_image 2.30
- The upgrade request, minus the frozen hook layer: the vendored ImGui tree moves 1.92.9 to
  1.93.0 (unmodified upstream swap, DX11/Win32 backends included, demo still excluded from the
  build) and stb_image jumps 2.26 to 2.30, four years of loader fixes behind the logo decode.
  The 1.93 ImDrawList rework was audited against every custom draw call in the repo: no
  renamed enums or flags in use, all closed shapes at thickness 1.0 render identically, and
  the editor's three indent-guide lines are the only half-pixel delta candidates.
- Not upgraded, with reason: kiero and its bundled MinHook stay frozen at 1.2.6/1.3.3 by the
  recorded decision (hook layer); TextEditor has no upstream upgrade path - the original
  project is superseded by a rewrite, and the vendored copy compiles clean against 1.93; the
  Font Awesome blob stays pinned because regenerating it needs tooling this repo does not
  carry.

## Kiero freeze recorded in the docs (docs only)
- The vendored hook library stays at 1.2.6 by decision, and the docs now say so wherever the
  vendored trees are described: upstream Rebzzel/kiero's last code commit is May 2022, tag
  1.2.12, everything since is README-only, and the 1.2.6→1.2.12 delta is hook-layer code this
  project does not carry.  The README file table, the README Notes and BUILD.md §9 carry the
  note; no line of code changed, and the hook layer remains exactly as vendored.

## The empty-buffer check stops copying the buffer per frame (row 139)
- Row 127's `bufEmpty` sample answered "is the buffer empty?" by copying the entire editor
  text into a temporary `std::string` on every frame - a heap allocation per frame in the
  steady state, for a question whose answer only changes when the text does.  It is now a
  cached flag that the existing text-changed poll refreshes in the frame a change happens,
  whether that change is typing, a paste, or one of the UI's own SetText calls; the toolbar,
  the placeholder hint and the context menu read the cache.
- The remaining `GetText()` calls in the file all sit in click handlers (save, clear, revert,
  load confirm, execute report), where a copy is the work itself, not overhead.

## Brace restored in the stopper guard (row 138)
- The row-137 splice consumed the outer `else` closing brace of the stop block while only
  re-closing the inner one, so `resources.cpp` sat in the committed state with an unbalanced
  brace - the next MSVC build would have failed on it before reaching any UI.  The brace is
  back at its nesting level, and the verification battery now includes a string- and
  comment-aware brace/paren balance check over every root source file, clean across all 32.

## The stopper confirms before it removes (row 137)
- Stop was the one visible state change among the stubs and the last action in the menu that
  discarded something on a single click: a mis-click cost the list entry until the next
  start, while Clear, Load, Revert and Reset all ask first.  It now confirms - naming the
  entry and repeating that nothing is stopped on the server - and a cancel lands in the
  activity trace.  The _cfx_internal refusal still answers before any question, and the
  button tooltip says it asks first.

## The file dialogs remember where they worked (row 136)
- Save As opened on an empty name in the process working directory even when the buffer came
  from a file: renaming meant retyping the name and re-navigating to the folder, and Load
  started the same trek on every use.  Both dialogs now open on the last file's folder via
  `lpstrInitialDir`, and Save As prefills the current base name - on the Shift path only,
  since a first save has neither a name nor a folder to remember.

## Config's three action buttons get tooltips too (row 135)
- Row 132 closed the tooltip family on the Executor, Resources and Menus surfaces and called
  it complete - Config's Copy diagnostics, Open settings folder and reset buttons were the
  remainder.  Each now says on hover what it does: the clipboard line that carries the
  environment into a report, the Explorer window on the settings folder (file highlighted
  once the ini exists), and a reset that asks first while the window position waits for the
  next start.

## Build fix: missing <string> and a helper outside the namespace (row 134)
- The first MSVC build since the row-125/128 batches reported two breaks.  `settings.h`
  declared `std::string IniPath()` without including `<string>`, so every translation unit
  that includes the header before its own `<string>` failed on the declaration - five in
  total.  And the row-128 `WriteFileText` helper had been appended after `namespace Executor`
  closed: the splice anchored on a column-0 closing brace, which is the namespace's, while the
  function's own brace is tab-indented like everything inside the namespace.
- The header now includes `<string>`, and the helper sits inside the namespace right after
  `ReadFileText`, its read-path twin.  The anchor mistake is recorded here and in BUILD.md so
  the same splice is not repeated against this file's indentation.

## Reset confirms before it overwrites (row 133)
- "Reset interface settings" was the one control in the menu that discarded user state on a
  single click: a mis-click cost a custom key bind, the interface scale and every switch,
  while Load, Clear and Revert all confirm first.  It now asks - "Restore the compiled
  defaults?" - and a cancel leaves everything, including settings.ini, exactly as it was.
- The cancel lands in the activity trace ("Reset cancelled - settings kept."), the same
  pattern the other guarded actions use, so the trace shows the question was asked and
  declined rather than nothing at all.

## The last unlabeled controls get tooltips (row 132)
- Execute, Save All Resources, Copy dump path, the stopper's Stop button and the Menus Run
  buttons were the last controls without tooltips - and three of them are stubs whose honesty
  arrived only after the click, inside a message box.  Each now says before the click what it
  is: reports only, list-only removal, or a dump into the Dumps folder.  The whitespace toggle
  reports its current on/off state, matching the inert-toggle tooltips Config already had.
- The README's stub paragraph now names the tooltip as the pre-click surface, and the executor
  row in the file table - stale since row 126 - lists revert, the whitespace toggle, the
  shortcuts, the right-click menu and the empty-state hint.

## Right-click context menu in the editor (row 131)
- Toolbar buttons and shortcuts covered undo/redo, cut/copy/paste and select-all, but the
  mouse-native route was absent: a right click in the editor did nothing.  It now opens a
  context menu with exactly those actions - greyed by the same rules as the toolbar (no
  selection, no clipboard text), every label naming the shortcut it mirrors, so the menu
  teaches the keyboard while it serves the mouse.

## The empty buffer names its ways in (row 130)
- A fresh editor buffer was a blank rectangle: nothing told a first-time user that typing,
  Ctrl+V or the new Ctrl+O all land text in it.  While the buffer is empty, a muted
  placeholder line in the editor's own top-left corner now names the three routes - drawn on
  the window draw list so it cannot eat a click, and gone with the frame the buffer fills.
- The hint reuses the per-frame `bufEmpty` sample row 127 introduced for the clear button, so
  it costs no extra text copy and cannot disagree with the toolbar state.

## Keyboard mirrors finish the file workflow (row 129)
- Ctrl+S mirrored Save since row 128, but Load stayed mouse-only and the Save As dialog had no
  shortcut of its own.  Ctrl+O now opens the load dialog (the handler body moved into a
  `loadFile` lambda shared by button and shortcut), and Ctrl+Shift+S forces the dialog as
  Save As - falling out of the Shift read the save path already had, not a second code path.
- The Load and Save tooltips name their shortcuts, so the keyboard routes are discoverable
  from the mouse routes instead of living in a changelog.

## One-click Save, Ctrl+S, and a shared write path (row 128)
- Saving reopened the file dialog every time, even to write the buffer back to the very file
  it was loaded from.  The dialog is now Save As - Shift+click, or the first save before any
  file reference exists - while a plain click writes to the last file in one go, and Ctrl+S
  mirrors the button for keyboard flow.  The tooltip names the file the click would write to.
- The open/write/short-write sequence moved into a `WriteFileText()` helper, the write-side
  counterpart to row 126's `ReadFileText()`, so the button and the shortcut share one write
  path and one set of failure messages.

## Paste and clear grey out instead of clicking into nothing (row 127)
- Paste with an empty clipboard and clear on an empty buffer were the last toolbar buttons
  that answered a click with silence.  Both join the disabled family the undo/redo/copy
  buttons started: grey while there is nothing to work on, tooltip naming which side is
  missing - clipboard or buffer - so a dead control reads as waiting, not broken.

## Revert: the way back from the dirty mark (row 126)
- The dirty mark (row 119) said "unsaved edits" but offered no way back except undo keystrokes
  or a hand reload through the Load dialog.  A revert icon now ends the editor toolbar: it
  reloads the file behind the buffer, grey while no file reference exists, answering with a
  plain info toast when the buffer already matches the file and asking only when the dirty
  mark shows - so the question appears exactly when there is something to lose.
- The read path (4 MB cap, empty check, UTF-8 BOM strip) moved out of the Load handler into a
  `ReadFileText()` helper shared by Load and Revert, so the two cannot drift apart the way the
  old duplicated menu-list copies did.

## Diagnostics line carries HUD state and write state (row 125)
- A report that says "the FPS plate is missing" or "my settings reset every start" used to need
  a follow-up question: the clipboard line carried the environment but not whether the HUD was
  switched on, nor whether the settings file is writable at all.  It now appends `| hud: on/off`
  and, when a write failed this session, ` [NOT WRITABLE]` right after the settings path - the
  same two facts the About block shows visually, now in the copyable line.

## Docs & render cell lists all seven preview files
- BUILD section 9's Docs & render row enumerated six preview PNGs in its file column while the
  branch renders seven since row 121; `docs/preview_default.png` now sits in the list next to
  the others, so the cell, the README file table, the figure captions and the renderer docstring
  all name the same seven outputs.

## Row-124 comment sync
- The three places that describe `RenderHud()` in one line - BUILD section 9's Shell row, the
  `menu.h` declaration comment and the `--hud` branch comment in the renderer - now name the
  `MADE BY EB` credit line alongside the FPS line, so the credit is visible in the architecture
  notes too, not only in the code and the figure captions.

## The HUD carries the credit: MADE BY EB (row 124)
- The FPS HUD plate was the one surface that stays visible while the overlay is closed - and
  the one place the owner credit did not appear (splash card, sidebar footer and Config About
  all carry it).  `Menu::RenderHud()` now draws `MADE BY EB` as a second plate line in the
  footer's muted brand alpha; the plate sizes to the wider of the two lines and both line
  heights.
- The `--hud` render mirrors the two-line plate including the ink-over-plate blend (accent at
  alpha 153 over the 63%-alpha plate over the scene stand-in); preview_hud.png re-rendered.

## Tooling regression: seven renders byte-identical, graph generator idempotent
- Full re-run of the docs tooling after the sandbox reset: all seven preview renders reproduce
  their committed bytes exactly (executor 789c95b8, config dc2c17a0, resources c4011405, menus
  7b58708a, default 51470a8b, splash e5e09f8b, hud 5c810d43), every renderer assert passes, and
  `api_sweep.py` reports clean - the working tree stays untouched by the run.
- The re-run exposed that `gen_include_graph.py` was not idempotent: its counts regex expected
  the sentence to end right after "N edges", but the script's own output appends the
  "(regenerate with ...)" parenthesis, so a second run died with "counts paragraph not found".
  The regex now tolerates its own parenthetical; two consecutive runs leave README unchanged.

## api_sweep.py: an executable compile-risk check without MSVC
- `docs/api_sweep.py` greps every root .cpp/.h for ImGui API names the vendored 1.92.9 no
  longer provides - removed outright or left only as commented-out obsolete redirects - and
  exits nonzero on any hit; comment text is stripped first, so explanatory comments do not
  count.  The first run is clean.
- The token list is verified against the vendored imgui.h rather than memory: CalcListClipping,
  GetOverlayDrawList, ImDrawCornerFlags and the ListBoxHeader/Footer pair are commented out,
  SetNextFrameWantTextInput never existed - while PushTabStop/PopTabStop survive as live inline
  wrappers and are therefore deliberately not listed.

## Filters companion and lifecycle view catch up
- The row-122 audit covered the vcxproj but not `ImGui DirectX 11 Kiero Hook.vcxproj.filters`:
  `clipboard.h` now also has its filter entry (`Source Files\menu`, same folder as `assets.h`),
  so Solution Explorer shows it where the other shared helpers live.  The combined audit of both
  project files finds every root file placed and nothing stale.
- The README lifecycle mermaid's `Menu::Render` node names the first-start notice alongside the
  splash card - the view is kept in sync by hand, and rows 113/117 changed exactly that path.

## clipboard.h joins the project file (row 122)
- A static audit of `ImGui DirectX 11 Kiero Hook.vcxproj` against the root file list found one
  gap: `clipboard.h` was the only root header missing from the `ClInclude` list (the build never
  needs it - includes resolve by path - but Solution Explorer and header search skipped it).
  It now sits in alphabetical position.
- The same audit confirms every root `.cpp` is a `ClCompile` entry, `font_awesome.cpp` carries
  `ExcludedFromBuild` as intended (main.cpp includes it), and no stale entries point at files
  that no longer exist.

## Caption and docstring say what the landing render guarantees
- The README fidelity caption already named the Lua snippet as illustrative; the landing
  render's sparkline and log entries are illustrative runtime data in exactly the same way, and
  the caption now says so - what the renderer guarantees is layout (panel height, the Copy/Clear
  row, line widths against the asserts), not values.
- The renderer docstring's source list gains `default.cpp` (graph height, button metrics, log
  order) and names the About block lines and buttons under `config.cpp`.

## The landing tab gets its preview render (row 121)
- `docs/render_preview.py --default` renders the one tab body that had no picture: the landing
  tab with its Diagnostics sparkline panel, the uptime/actions/FPS line, the session activity
  log (three entries, newest first) and the Copy/Clear pair that shares its line with the
  heading - no sidebar highlight (the landing has no sidebar row) and no toast (a card would
  cover the log this view exists to show).
- The branch asserts the hint line, the uptime line and the heading against the content column,
  the Copy/Clear row against the heading, and the whole block against the cell bottom - the
  same protection the Config pane has had since row 84.
- README gains the seventh figure and file-table row; the docstring usage block finally lists
  all seven outputs instead of four.

## Include graph regenerated by a script, not by hand
- The README include graph claimed to be "generated from the actual #include lines" but had
  fallen behind: no `clipboard.h`, and the `settings`, `toasts`, `activity` and `perf` modules
  were missing entirely (23 nodes / 33 edges versus 35 / 63 today).
- `docs/gen_include_graph.py` now regenerates the mermaid block and the counts paragraph from
  the quoted root includes - one subgraph per module, standalone boxes for unowned headers and
  header-less translation units, nodes numbered in alphabetical module order so diffs stay
  readable.  The README paragraph names the command; BUILD section 9's docs cell lists it.

## README file table catches up with the codebase
- The path table listed the tabs and vendored code but not the support modules every tab stands
  on: `settings`, `toasts`, `activity`, `perf` and the new header-only `clipboard.h` now have
  rows saying what each one owns.
- The Executor, Config and Landing rows describe what those tabs actually contain today
  (toolbar with save, status line with file and modified mark, confirmations; About block with
  version/build/host/warning/both buttons; diagnostics card and copyable log), instead of their
  early-branch state.

## Row-119 flags hoisted above their first use (row 120)
- The clear and save toolbar handlers sit above the editor render in `Executor::Render()`, so
  the dirty flag declared below them was used before declaration - a compile error, caught in
  review since the sandbox has no MSVC.  Both `static` declarations (and the comment explaining
  the frame-scoped widget flag and the suppression) now open the function, above every handler
  that reads or resets them.

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
