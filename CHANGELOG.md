# Changelog

All entries are work on this branch (`arena/57915bfa-eb-fivem`), newest first.
Scope note: everything here is interface, tooling or documentation level. The
payloads this menu's names suggest (script execution, resource stopping, menu
launching, kernel access, ESP, aimbot) are deliberately **not** implemented and
say so in the UI - see README and PR #1.

"row N" refers to the fix table in `BUILD.md` §5.  The PR body carries a summary
and the verification anchor only; per-batch write-ups live here and in the commits.

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
