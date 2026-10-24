# Building EBengin UI

Windows-only C++ project. The output is a **DLL** (`ConfigurationType=DynamicLibrary`) that
hooks `IDXGISwapChain::Present` through Kiero/MinHook and renders a Dear ImGui 1.92.9 overlay
into the FiveM window.

There is no CI or prebuilt binary in this repo — you compile it yourself with MSVC.

---

## 1. Requirements

| | |
|---|---|
| OS | Windows 10 / 11 x64 |
| IDE / toolchain | Visual Studio **2019 (v142)** or **2022 (v143)** |
| VS workload | **Desktop development with C++** |
| Windows SDK | Any 10.0.x SDK that ships with VS (the project uses `WindowsTargetPlatformVersion = 10.0`, i.e. "latest installed") |
| DirectX SDK (June 2010) | **Not needed.** `d3d11.h` / `dxgi.h` come from the Windows SDK |

No NuGet packages, no submodules, no third-party downloads: ImGui, Kiero, MinHook,
`stb_image` and the Font Awesome blob are all vendored in-tree.

> **ImGui was upgraded from 1.75 to 1.92.9.** Since 1.80 the core is split across
> `imgui.cpp`, `imgui_widgets.cpp`, `imgui_draw.cpp` **and `imgui_tables.cpp`** — all four must
> be compiled. What the upgrade changed in this project is listed in section 6.

> The project pins `v142` when that toolset is installed and otherwise falls back to
> `$(DefaultPlatformToolset)`, so VS2022-only machines build without a manual retarget.

---

## 2. Build

### Option A — `build.bat` (recommended)

From a plain `cmd.exe` in the repo root:

```bat
build.bat                 REM Release | x64
build.bat Debug           REM Debug   | x64
build.bat Release Win32   REM Release | x86 (will NOT inject into FiveM)
build.bat Clean           REM delete intermediate output
```

The script locates MSBuild via `vswhere`, rebuilds, prints the DLL path and copies `EBengin.png`
next to the DLL so the menu logo resolves at runtime.

### Option B — MSBuild by hand

From a **Developer Command Prompt for VS 2022**:

```bat
msbuild "EBengin UI.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
```

Output:

```
x64\Release\EBengin UI.dll
```

(`ProjectName` is `EBengin UI`, so that is the produced file name — not
`ImGui DirectX 11 Kiero Hook.dll`, which is only the `.vcxproj` file name.)

### Option C — inside Visual Studio

1. Open `EBengin UI.sln`.
2. Pick **Release / x64** in the toolbar.
3. `Build → Build Solution` (Ctrl+Shift+B).

**Do not press F5 / "Start Debugging".**  The output of this project is a **DLL**, and
Windows cannot launch a DLL as a program - Visual Studio answers with
*"…\EBengin UI.dll" is not a valid Win32 application* (German: *"… ist keine gültige
Win32-Anwendung"*).  That dialog is not a build failure; the build already succeeded when
it appears.  The two legitimate ways to build and to debug are in §8 Troubleshooting.

---

## 3. Runtime assets

The DLL resolves its logo **relative to its own directory** (falling back to the process
working directory):

| Asset | Where it is looked up | If missing |
|---|---|---|
| `EBengin.png` | next to the injected `.dll` | the sidebar prints a light-blue `EBengin` text logo instead — no crash |
| `C:\Windows\Fonts\Bahnschrift.ttf` | fixed Windows path | falls back to ImGui's built-in default font |

So a working deployment is just:

```
<folder>\EBengin UI.dll
<folder>\EBengin.png
```

The menu is toggled with the bound key — **INSERT** by default, rebindable in Config →
Interface and persisted in `settings.ini`, together with the window position, the last open tab,
the animation/toast switches, the editor whitespace dots, the open Resources sub-tab and the
interface scale (`[ui] scale`, 70–200%).

---

## 4. Build fixes that were applied

If you are comparing against the original upstream snapshot, these are the things that
stopped it from compiling/opening, and what changed:

| # | Symptom before | Cause | Fix |
|---|---|---|---|
| 1 | `MSB3202: The project file "ImGui DirectX 11 Kiero Hook\ImGui DirectX 11 Kiero Hook.vcxproj" was not found` | `redENGINE UI.sln` referenced a nested folder that does not exist; the `.vcxproj` sits in the repo root | Corrected the project path in the `.sln` |
| 2 | `C1083: Cannot open include file: 'imgui.h'` | `imgui_impl_dx11.cpp` / `imgui_impl_win32.cpp` do `#include "imgui.h"`, but nothing put the project dir or `imgui\` on the include path | `IncludePath` now contains `$(ProjectDir);$(ProjectDir)imgui` |
| 3 | `C2397: conversion from 'double' to 'int' requires a narrowing conversion` | `icons_config.OversampleH = 2.5;` — those fields are `int` in `ImFontConfig` | Set to `3` / `1` |
| 4 | `C2664: 'ImGui::Combo': cannot convert argument 4 from 'size_t' to 'int'` | `exec.h` passed `resources.size()` | Cast to `int` |
| 5 | `MSB8020: The build tools for v142 cannot be found` on VS2022-only machines | Toolset hard-pinned to `v142` | Conditional: `v142` if present, else `$(DefaultPlatformToolset)` |
| 6 | Bogus `C:\Program Files (x86)\Microsoft DirectX SDK (June 2010)\Include` on the include path | Legacy hardcoded SDK | Removed; Windows SDK provides `d3d11.h` / `dxgi.h` |
| 7 | `RC.exe` needed for a resource that does not belong to this DLL | `kiero\minhook\dll_resources\MinHook.rc` was compiled in, stamping MinHook's version info onto redENGINE | `ResourceCompile` item removed from `.vcxproj` and `.filters` |
| 8 | Crashed the host process on every machine except the author's | `IM_ASSERT(LoadTextureFromFile("D:\\rE.png", ...))` — a hardcoded path plus a hard assert | Resolves next to the DLL, no assert, text fallback in `menu.h` |
| 9 | `C2065: 'uintptr_t': undeclared identifier` (possible, with `/permissive-`) | `PTR` typedef relied on a transitive include | `#include <cstdint>` and `<string>` in `includes.h` |
| 10 | `std::find` used without `<algorithm>` | Relied on transitive includes | Added to `resources.h` |
| 11 | Language standard depended on the toolset default | Not specified | `LanguageStandard = stdcpp17` in all four configs |
| 12 | `font_awesome.cpp` listed as `<None>` although it is a `#include`d TU payload | Ambiguous project item | Now `ClCompile` with `ExcludedFromBuild=true` |
| 13 | Mixed calling conventions on the hook | `__stdcall` is ignored on x64 | `HK_CALL` macro in `includes.h`, used by `Present` / `hkPresent` |
| 14 | `error C4996: 'sprintf': … declared deprecated` ×33, fatal because `SDLCheck` is `true` | `imgui/imgui_demo.cpp` was compiled into the DLL | Removed from the project. `ShowDemoWindow` / `ShowAboutWindow` / `ShowStyleEditor` / `ShowUserGuide` are never called anywhere in this codebase (`ShowMetricsWindow` lives in `imgui.cpp`, so it survives). The file stays on disk as vendored upstream code, it is simply not built — which also drops ~590 KB of demo source from every compile. **Do not re-add it without either dropping `SDLCheck` or defining `_CRT_SECURE_NO_WARNINGS` for that file.** |

---

## 5. Runtime / correctness fixes that were applied

Separate from the build fixes above: these are bugs that compiled fine but misbehaved.

| # | Symptom before | Fix |
|---|---|---|
| 1 | Clicking a menu button also fired/punched in-game; typing Lua in the executor walked the character. `ImGui_ImplWin32_WndProcHandler` returns `0` for mouse/key messages (it does *not* mean "consumed"), and `WndProc` forwarded everything to the game regardless | `WndProc` now consumes mouse messages while the cursor is over an ImGui window and key messages while `io.WantTextInput` / `WantCaptureKeyboard` is set |
| 2 | `io.WantCaptureMouse = Menu::IsMenuOpen;` in `ApplyStyle()` | Both `WantCapture*` flags are *outputs* computed by `NewFrame()` — assigning them was a no-op. Removed; capture is handled in `WndProc` |
| 3 | `ApplyStyle()` rewrote ~30 style vars + colours every frame | Called once from `InitImGui()`; only `MouseDrawCursor` is refreshed per frame |
| 4 | Alt-tab / resolution change drew into a stale render target | `hkPresent` compares the swap-chain desc against the cached size and rebuilds the RTV (`InvalidateDeviceObjects` → `CreateRenderTarget` → `CreateDeviceObjects`) |
| 5 | A lost or reset D3D device kept being drawn into every frame | `DXGI_ERROR_DEVICE_REMOVED` / `_RESET` from `Present` now triggers a full teardown and re-init next frame |
| 6 | `mainRenderTargetView`, the D3D device/context, the ImGui context and the logo SRV were never released; the window stayed subclassed forever | `CleanupImGui()` releases all of them and restores `GWLP_WNDPROC` |
| 7 | `DLL_PROCESS_DETACH` called `kiero::shutdown()` while the game could still be inside `hkPresent` | `g_unloading` latch + short drain before teardown; `hkPresent` early-outs once set |
| 8 | `MainThread` spun in `do {} while (!init_hook)` with no `Sleep` and no exit — a pegged core forever if the hook could never attach | Bounded retry loop (~60 s), gives up on `NotSupportedError` / `ModuleNotFoundError` |
| 9 | If `CreateRenderTarget` failed after subclassing, the next frame called `SetWindowLongPtr` again and stored *our own* `WndProc` as the "original" — an endless call chain | Subclass only when `oWndProc == NULL` |
| 10 | Resource stopper: `erase(std::find(begin, end, resources[selectedResource]))` searches a `vector<const char*>` for a **pointer** value, so it matched only by accident; on a miss `erase(end())` is UB — and the `try/catch` around it cannot catch UB | Erase by index, then clamp the selection. Also reports honestly that stopping the server resource is not implemented |
| 11 | `SaveResources()` popped "Dump successfully saved to C:\redENGINE\Dumps\127.0.0.1\" **before** writing anything, to a path that needs elevation, with a hardcoded fake IP | Writes `<DLL folder>\Dumps\fxmanifest.lua` + `resources.txt`, creates the tree, checks the stream state, and only then reports success (or failure) |
| 12 | File browser called `dumper.SetText(...)` **after** `dumper.Render(...)`, so the editor always showed the previous resource — one frame behind, forever | `SetText` before `Render` |
| 13 | Executor `SetReadOnly` / `SetShowWhitespaces` / `SetPalette` ran every frame, and the language definition was never set (a Lua editor highlighting as C++) | Moved to `Executor::Init()` / `Resources::Init()`, called once from `InitImGui()`; Lua highlighting enabled |
| 14 | Executor editor sized with `GetWindowSize()` — the whole window, measured before the sidebar column — so it overflowed the right edge and ran under the button row | Sized from `GetContentRegionAvail()` |
| 15 | `ImGui::Text(Triggers[i])` — a resource/trigger string used as a **printf format string** (any `%` is UB); same in the file-browser error text | `ImGui::Text("%s", ...)` / `TextColored(col, "%s ...", ...)` |
| 16 | Every "Copy" button in the trigger list shared one ImGui ID, so hover/active state leaked between rows | `PushID(i)` / `PopID()` |
| 17 | `if (Triggers.at(i) == NULL)` — `at()` returns a reference and the vector never holds NULL, so the guard was unreachable | Real emptiness check |
| 18 | `ListBoxHeader()` return value ignored before calling `ListBoxFooter()` | Both list sites now honour the contract |
| 19 | `SetClipboardData` failure leaked the `HGLOBAL`; `GlobalLock` was never NULL-checked | Ownership handled correctly |
| 20 | All menu state was **defined in headers** (`TextEditor editor;`, `std::vector<const char*> resources;`, `ID3D11ShaderResourceView* logo;`, `bool Menu::IsMenuOpen;` …). That only links while exactly one `.cpp` includes them | Split into `exec.cpp`, `resources.cpp`, `config.cpp`; headers now `extern`-declare, each symbol defined in exactly one TU |
| 21 | `Gui::Seperator` ignored `BeginChild()`'s return value | Documented + `inline`; child stack stays balanced |
| 22 | Sidebar was 5 copy-pasted button/separator blocks that had already drifted | Data-driven `SidebarItems[]` table + `RenderSidebar()` |
| 23 | `Menus` tab: two near-identical 40-line blocks, child windows sized to the *full* window height while nested inside the right-hand column | Single `RenderMenuList()` helper, sized from the available region |
| 24 | `if (Tab == 0) {...} if (Tab == 1) {...} else if ...` — asymmetric chain | `switch (Tab)` |
| 25 | Dead "Execute" / "Load from File" / "Run <menu>" buttons that silently did nothing | Load-from-file is now a real `GetOpenFileName` → editor flow; the remaining stubs say so in a message box |
| 26 | `using namespace std;` in `config.h` leaked into every TU that included `menu.h` | Removed |
| 27 | `"Build Date: Feb 10 2021"` hardcoded | `__DATE__` |
| 28 | No visual indication of the open tab: every sidebar entry drew with `ImGuiCol_Button` and only the transient hover/press colour differed, so the sidebar looked identical after a click — and at startup `Tab` is `TabDefault`, which is not in the sidebar, so nothing was marked at all | The selected entry holds `ButtonActive` and gets a 3px accent bar on its left edge |
| 29 | Sidebar footer pinned to `SetCursorPosY(380)` / `(399)` — 19px apart, less than one 20px text line — so the two strings drew on top of each other, and both were tied to one particular window height | Bottom-anchored from `GetWindowHeight()`, lines stacked by normal advance and centred in the column |
| 30 | Tab headings sat at `GetContentRegionAvail().x / 1.3f` in four tabs (a magic fraction that centres nothing and aligns to nothing) but flush-left in the Executor — five headings, five positions | Shared `Gui::SectionTitle()` |
| 31 | Config tab sized its panes with `GetWindowWidth()` / `GetWindowHeight()` — the 900×540 *outer* window, not the ~658×492 table cell — plus a stray `SetCursorPosX(233)`, so the right-hand pane ran 900px wide off the window | Both panes measured from `GetContentRegionAvail()` |
| 32 | Resources sub-tab buttons sized `GetWindowWidth()/4`: 3×225 + two item spacings = 691px did not fit the ~658px cell, clipping the third button; the file-preview editor used `GetWindowSize().x` (884px) inside the same cell | Cell width divided by three / `GetContentRegionAvail()` |
| 33 | Tab bodies started flush against the column divider (`CellPadding (0,0)` + `NoPadOuterX`), so text and widgets touched the rule | `Menu::ContentPadding` left inset applied once to the content column |
| 34 | `Load from File` was silent on **every** failure path — unreadable, empty or ≥4 MB files made the button do nothing, indistinguishable from the old dead stub | Each path now reports the reason in a message box |
| 35 | `CopyToClipBoard` returned `void`, so a busy clipboard (another process holding it) looked like a successful copy that copied nothing | Returns `bool`; the trigger "Copy" button reports a busy clipboard |
| 36 | `GetModuleDirectory()` ignored `GetModuleFileNameA` truncation (it returns exactly `MAX_PATH` when the path does not fit), which would resolve `rE.png`/`Dumps` against a wrong, silently-truncated directory | Truncation is treated as failure and falls back to the working directory |
| 37 | No frame-pacing feedback anywhere: "the menu is slow" and "the game is slow" were indistinguishable without an external profiler | Sidebar footer line `FPS n \| n.n ms` from ImGui's smoothed `io.Framerate`, drawn in the accent colour |
| 38 | Loads, dumps, copies and stop/run requests left no trace once their message box was clicked away | Timestamped session log on the landing tab (`activity.h`/`.cpp`, in-memory, newest first, capped at 32) |
| 39 | Window position, open tab and toggle key reset to defaults on every re-inject | `<DLL folder>\settings.ini` (`settings.h`/`.cpp`): loaded in `InitImGui`, window position restored with `ImGuiCond_Once` and re-saved throttled (~1/s plus 40px threshold, captured before `End()`), hand-edited values clamped |
| 40 | Overlay toggle key was hardcoded to `VK_INSERT` | Config capture button (click, press a key, ESC cancels); the label uses `MapVirtualKey`/`GetKeyNameText`, so it reads e.g. "Toggle key: INSERT" |
| 41 | Every piece of feedback was a modal `MessageBox`: correct, but it steals focus from the game and freezes the frame until clicked away | Non-blocking toast cards (`toasts.h`/`.cpp`) stack in the window's bottom-right corner, fade in/out over ~4s, cap 4; wired to the success/info paths while critical errors keep their boxes |
| 42 | The overlay popped in at full opacity in one frame, and the sidebar's 3px tab marker jumped straight to the clicked entry | Open fade (~0.22s ease-out cubic via `ImGuiStyleVar_Alpha`) and a marker bar that eases to the selected entry; both collapse to the old one-frame behaviour with Config → Animations off |
| 43 | Square corners on every widget (all roundings 0) made the shell read as a debug overlay sitting *in* the scene rather than on it | `WindowRounding 10` plus child/frame/popup/scrollbar/grab rounding in `ApplyStyle()`, a three-rect drop shadow on the background draw list, and a brand-gradient rule under the title bar |
| 44 | Primary actions (Execute, Save All Resources, Run <menu>) drew in the same navy as every secondary button, so nothing read as "the main thing here" | `Gui::PrimaryButton`: brand-filled button (rest #3778B5, hover #5D9CD6, pressed #1D5188) with a soft top highlight and centred label, input still via `InvisibleButton` |
| 45 | Stock square checkboxes for the Config switches | `Gui::Toggle`: animated switch whose knob eases via per-widget `ImGuiStorage` state; used for Safe Mode, ScriptHook Bypass and the two new Interface toggles |
| 46 | Motion and toast feedback were all-or-nothing with no way to opt out | Config → Interface: "Animations" and "Toast notifications" switches, persisted in `settings.ini` `[ui]` |
| 47 | The editor showed neither cursor position nor document size — a 400-line paste looked identical to an empty buffer | Status line under the editor: `Ln n, Col n \| n lines \| Lua` (+ read-only flag) from `GetCursorPosition()` / `GetTotalLines()` |
| 48 | The 34-entry resource list could only be scanned by eye, in the file browser and the stopper alike | Case-insensitive filter input (`InputTextWithHint`) above both lists; selections stay indexed into the real vector, never the filtered view |
| 49 | Frame pacing was one instantaneous number with no history, so a hitch vanished before it could be read | `perf.h`/`.cpp`: 120-frame ring of frame times drawn as a sparkline (60 FPS guide line, >33.3 ms bars in the semantic red) on the landing tab, with session uptime and action count |
| 50 | `Gui::Toggle`'s hit area was the 40px track only (a stock checkbox toggles from its label too), and the knob kept easing with animations switched off | Hit area covers track + label; knob easing is gated on Config → Animations |
| 51 | A letter or digit bound as toggle key flipped the menu on every keystroke while the user typed it into the editor or a filter box | The toggle poll ignores presses while `io.WantCaptureKeyboard` is true — but only for printable bindings, so INSERT/F-keys still close the menu with the editor focused |
| 52 | The resource preview editor was `SetText()` on every frame its header was open: selection and scroll position reset 60–144×/s and the document re-tokenised at the same rate | Text is set only when the open resource changes (`lastDumped` guard) |
| 53 | The Diagnostics uptime counted wall time since the menu first opened, so it kept rising while the menu was closed even though the graph next to it was frozen | `Perf` accumulates sampled (menu-open) time; the card reads "Menu open mm:ss" |
| 54 | A window position persisted from another monitor setup (laptop undocked, monitor swapped) could place the whole menu off-screen on the next start, unreachable | The restored position is clamped into the virtual screen so an 80px strip of the window - enough title bar to grab - always stays visible |
| 55 | The toggle-key capture scanned vk 1-254: binding a mouse button would flip the menu on every click, and the release edge of the click that armed the capture bound `VK_LBUTTON` on the first capture frame | The scan starts at vk 7, skipping VK_LBUTTON..VK_XBUTTON2 and the reserved entries |
| 56 | `Activity::Clear()` existed since the session log was introduced but nothing ever called it - the log could only grow until it capped | "Clear" button on the landing tab's Session activity heading |
| 57 | The frame-time sparkline showed a shape but no numbers | Hovering a bar reports its frame time and age ("x.x ms, n frames ago") in a tooltip |
| 58 | Docs and comments still claimed "INSERT toggles the menu" in five places although the key became configurable and persisted several commits earlier | README intro, README flow graph, README runtime notes, BUILD.md runtime note and the `menu.h` comment now say "INSERT by default, rebindable in Config → Interface" |
| 59 | The throttled (~1/s) window-position save could lose up to a second of window movement when the DLL unloaded | `DLL_PROCESS_DETACH` flushes `Settings::Save()` before teardown |
| 60 | The menu was mouse-only: with a dead mouse, or in any accessibility scenario, no control was reachable at all | `ImGuiConfigFlags_NavEnableKeyboard`: Tab/arrows move the focus, Enter/Space activates; the nav focus rectangle draws in the brand accent, and Config → Interface states the keys |
| 61 | The printable-toggle-key typing guard tested `io.WantCaptureKeyboard`, which is also true whenever keyboard navigation is active — enabling nav would have killed printable bindings outright | The guard tests `io.WantTextInput` (a text widget is actually consuming input), which is the precise condition and nav-safe |
| 62 | Editor actions were keyboard-shortcut-only: with the focus elsewhere, or under keyboard nav, undo/copy/paste were unreachable, and nothing made them discoverable | Icon toolbar above the editor (undo / redo / copy / paste / clear); tooltips name the shortcut each button mirrors |
| 63 | The status line showed where the cursor is but not what a selection covers | With an active selection the line appends "n selected" |
| 64 | Toasts could only expire on their own; a stack covering a widget had to be waited out | Clicking a card dismisses it; clicks on widgets under the stack are never stolen (`IsAnyItemHovered` / `IsAnyItemActive` guard) |
| 65 | Whitespace-dot visibility in the editor was hardcoded off in `Executor::Init()` | "Whitespace" switch at the end of the editor toolbar, persisted as `[ui] whitespace` |
| 66 | The Resources sub-tab (File Browser / Triggers / Stopper) fell back to the empty default view on every re-inject | Sub-tab persisted as `[menu] resourcetab`, clamped to 0–3 on load |
| 67 | A bad key binding or a window dragged off-screen could only be undone by hand-editing or deleting `settings.ini` | Config → Interface "Reset interface settings" restores the compiled defaults and rewrites the ini in one click, with a toast saying what applies when |
| 68 | The window-position saver persisted on the first frame after open even for a window that never moved — so after "Reset interface settings" it re-persisted the current position within one frame and silently undid the reset | The saver now arms only on a real inter-frame drag (>4px) and disarms whenever the ini says "not persisted" (`WindowX < 0`); an unmoved window stays unpersisted and a reset survives until the next drag |
| 69 | With keyboard nav enabled a freshly opened menu had no focus target: the first Tab/arrow either went nowhere or landed on an arbitrary widget | On reopen the menu takes window focus (`SetNextWindowFocus`) and `SetKeyboardFocusHere` puts the nav cursor on the sidebar entry of the open tab (first entry on the landing tab) |
| 70 | A frame hitch of >250 ms re-triggered the open fade mid-session, because the call-gap heuristic mistook the hitch for a reopen | Gap threshold raised to 0.5 s — far above any real frame time, far below any human close/reopen |
| 71 | `TextEditor.h` declared `static class TextEditor { … };` — MSVC answers with C4091 ("'static ': ignored on left of 'TextEditor' when no variable is declared") once per including TU (5×), and had the `static` been honoured the out-of-line member definitions in `TextEditor.cpp` would no longer have belonged to the same class | `static` dropped: the class has external linkage, which is what a widget with out-of-line definitions needs |
| 72 | `Toasts::Render()` drew its text with `font->FontSize`; `ImFont::FontSize` does not exist in 1.92 (C2039), which in turn left no matching `ImDrawList::AddText` overload (C2665) | `AddText(NULL, 0.0f, …)`: the draw list supplies its own bound font and size, so toast text matches the window exactly and follows any window font scale |
| 73 | `Activity::Push(… + std::to_string(Triggers[i].size()) …)` — `Triggers` is a `std::vector<const char*>`, so the element is a pointer and has no `.size()` (C2228 "left of '.size' must have class/struct/union") | `strlen(Triggers[i])`, which is what the log line always meant |
| 74 | `GetPrimerosePalette()` passed `ImColor(0.30,0.30,0.30, 120)`: three bare double literals next to an int alpha made MSVC bind the `ImColor(int,int,int,int)` overload (3× C4244 "conversion from 'double' to 'int'"), so `0.30` truncated to `0` and the selection highlight came out pure black | `ImColor(77,77,77,120)` — 77 = round(0.30 × 255), alpha unchanged |
| 75 | `AddFontFromFileTTF(path, 20)` passed an `int` to the `float size_pixels` parameter | `20.0f` |
| 76 | The interface scale was whatever the compiled font size happened to give you — no way to make the shell readable on a high-DPI screen or smaller on a cramped one | Config → Interface "Interface scale" slider, 70–200%, persisted as `[ui] scale` and applied through ImGui 1.92's `style.FontScaleMain`, which multiplies the *bound font's own* size (`style.FontSizeBase` is initialised from `font->LegacySize` on the first frame, `imgui.cpp:9104`; the factor is applied at `:9214`). `style.WindowMinSize` and the sidebar/inset metrics in `menu.h` follow the same factor, so the shell grows instead of clipping its labels. Applied on slider **release**, because 1.92 bakes a font for every distinct size it sees |
| 77 | "Reset interface settings" only wrote the prefs struct — but two prefs also live somewhere else at runtime: the scale inside ImGui's style and the whitespace flag inside the TextEditor widget. The shell kept its old size and the dots stayed on screen until the toolbar switch was clicked again | The reset now calls `Settings::ApplyScale()` and the new `Executor::ApplyPrefs()`; `Executor::Init()` and the toolbar switch use that same function, so widget and ini cannot drift apart |
| 78 | Switching toast notifications off left the stack in memory: `Menu::Render()` skips `Toasts::Render()` while the pref is false, so a card younger than ~4s reappeared mid-fade when toasts were switched back on | New `Toasts::Clear()`, called when the toggle turns off |
| 79 | The dump folder was only ever *named* in a message box — there was no way to get the path except transcribing it | "Copy dump path" button under "Save All Resources" (which the file browser and the stopper both call), with an honest toast about whether the folder exists yet. `GetFileAttributesA` is validity-checked **before** the directory bit is tested, because `INVALID_FILE_ATTRIBUTES` has every bit set |
| 80 | `Perf::DrawGraph()` divided by the bar width without checking it: on a zero-width canvas that is a division by zero, and the `(int)` cast of the resulting infinity is undefined — the index check after it would not reliably catch that | `bw > 0.0f` guard ahead of the hover readout |
| 81 | Row 76's scale slider could push the shell off-screen: growing it resizes the window (900×540 → 1800×1080 at 200%), but the persisted position was clamped only on the first frame after open — and with `NoResize` a title bar outside the virtual screen leaves nothing to grab, recoverable only by editing `settings.ini` | `ClampToVirtualScreen()` extracted in `menu.cpp` and shared by the restore path and a new guard that re-clamps whenever `style.WindowMinSize` differs from the previous frame — exactly the one thing `ApplyScale()` writes, so there are no per-frame Win32 calls and no false triggers from tab switches (those change content, not the minimum size). The correction is picked up by the position saver, which is the point: the reachable position is the one worth persisting |
| 82 | `TextEditor.cpp` (vendored ImGuiColorTextEdit) floods a `/W3` build with C4018/C4267 from its `int`-vs-`size_t` comparisons — upstream style, ~30 sites, none of them a defect, and editing them blind with no compiler in this environment is the riskier move | Per-file `<DisableSpecificWarnings>4018;4267</DisableSpecificWarnings>` in the `.vcxproj`, with a comment saying why. Every project-owned translation unit stays unsuppressed at `/W3`; the diagnostics that *did* point at real bugs (rows 71–75) were fixed in the sources instead |
| 83 | Config's "Safe Mode" and "ScriptHook Bypass" sections described behaviour that does not exist — the tooltips promised "a private scripting environment" and ".ASI plugins through the ScriptHook interface", while `config.h` documents both bools as UI state that nothing reads and nothing persists. Two switches that look functional but are inert is the same defect the dead Execute/Run stubs had | Both headings are greyed via `TextDisabled`, both switches read `Enabled (no effect)`, the tooltips say plainly that the switch only stores a value, and every flip writes an activity entry plus a warn toast — the honest pattern the execute/run/dump paths already use. The bools stay (dropping them would change the tab's layout for no gain); nothing reads them either way. Why the marker sits in the switch rather than in the heading: see row 84 |
| 84 | The Config tab's left pane is a third of the content column (~216px at the 900px shell) and `ImGui::Text`/`TextDisabled` do not wrap, so four strings were silently clipped at the pane edge — measured with the preview renderer's own font metric: row 83's `(not implemented)` heading suffixes (269px / 338px), the click-to-capture button label while capturing (377px) and the keyboard hint (448px, cut after "Tab/arrows"). The capture label was the worst case: it hid exactly the "ESC cancels" information a user needs mid-capture | The honesty marker moved into the switch label (`Enabled (no effect)`, 213px), the capture label shortened to `press any key... (ESC)` with the full sentence in a hover tooltip, the keyboard hint wrapped via `TextWrapped`, and `docs/render_preview.py` gained a `--config` branch that renders `docs/preview_config.png` and asserts that every Config-pane string fits its pane and the pane fits its height — so the next label that is too wide fails the render instead of shipping clipped |
| 85 | The 70—200 % interface scale (row 76) scaled everything font-derived (`GetFrameHeight()`, text, default frames) plus the shell metrics in `menu.h` — but every *explicit* pixel size in the tabs stayed at its 100 % value: at 200 % a 40 px glyph sat on the 30 px "Execute" button, the 40x20 switch track was dwarfed by its own label, the Menus list box (fixed 300 px) pushed the "Run" button below the child's fold at 70 %, and toast cards stayed 320 px wide around 40 px text | `Settings::Px()` in `settings.h` multiplies explicit sizes by `ScaleFactor()`; applied to every fixed size in `exec.cpp`, `config.cpp`, `resources.cpp`, `luamenus.cpp`, `default.cpp`, `menu.cpp` (logo), `toasts.cpp` (card metrics) and `Gui::Toggle`'s track. At the default 100 % it is the identity, so nothing moves for anyone who never touched the slider |
| 86 | In the Resources tab the list box reserved 48 px for everything rendered below it, but `SaveResources()` needs 76 px (separator 1+4, the 33 px primary +4, `Spacing()` 4, the 26 px copy button +4): "Copy dump path" sat ~24 px below the visible content area in the File Browser and Triggers sub-tabs — and because the window is created with `NoScrollbar` it could not even be scrolled into view, just clipped and unreachable. Found by modelling the tab for `docs/preview_resources.png` | The reserve is now a named `saveBlockH = Settings::Px(76.0f)` shared by both sub-tabs that call `SaveResources()`; the Stopper keeps its 48 px because its block below the list needs 42 px. The new `--resources` render asserts the whole column fits |
| 87 | The editor toolbar's undo/redo/copy buttons looked exactly the same whether or not they could do anything: on a fresh buffer all three clicked into no-ops, indistinguishable from working buttons — the same "dead control reads as broken" family as the old silent stubs, just smaller. And after the stopper erased the last entry, the Executor's resource row went silently blank where a combo used to be | `BeginDisabled(!editor.CanUndo()/CanRedo()/HasSelection())` around the three, with tooltips that state the reason while disabled (`ImGuiHoveredFlags_AllowWhenDisabled`, since `IsItemHovered()` ignores disabled items by default); paste and clear stay live because clipboard state is not polled per frame and clearing an empty buffer is a harmless no-op. The combo row falls back to a muted "No resources left to select." The Executor preview draws the three icons with the disabled alpha of 0.60, so `docs/preview.png` shows the honest state |
| 88 | The selected row was invisible wherever ImGui shows one: `ImGuiCol_Header` (the Selectable/Combo selection fill) was the same #1F3A61 as `ImGuiCol_FrameBg`, the list background — so in the Menus lists, the Executor resource combo and every ListBox the only cue for which entry was selected sat elsewhere (the Run button label), and keyboard-nav focus had nothing to land on visually | `ImGuiCol_Header` raised to the solid border navy #2A4A7B in `menu_style.h`, distinct from both the frame background and the half-transparent `HeaderHovered`; `HeaderActive` (brand blue) unchanged. Caught while modelling the tab for `docs/preview_menus.png` |
| 89 | The file browser's dump preview is a single shared read-only `TextEditor`, but nothing kept more than one `CollapsingHeader` open: with two headers open, the second `SetText` retargeted the first header's preview to the wrong resource — the box under "esx_mafia" could show "--successfully dumped esx_jail." | A one-open invariant is re-asserted per frame: `SetNextItemOpen(openRes == i)` ahead of each header with the open index tracked; clicks still toggle both ways, the previous header closes on the frame after a new one opens, and the existing `lastDumped` guard then always refers to the one open header |
| 90 | Two small truthfulness/presentation defects: a loaded file with a UTF-8 byte-order mark put a junk glyph in front of the first editor line (and the "Loaded ... (N bytes)" toast counted the marker as content), and seven Resources message boxes titled themselves "rE" while every other box in the menu says "redENGINE" | The three BOM bytes are stripped after the read and the reported size is what the editor holds; the seven titles are unified to "redENGINE" |
| 91 | On a per-monitor-DPI-aware host the window is sized in physical pixels, but the whole layout was compiled against 96 DPI: at 150% the 900x540 shell and every metric kept their logical size and the menu rendered visibly small inside its own window | `Settings::DpiScale` (`GetDpiForWindow(window) / 96`, read once in `InitImGui`, 1.0 on unaware hosts or on failure) folds into `ScaleFactor()`, so `Settings::Px()`, the shell metrics in `menu.h` and `style.FontScaleDpi` (multiplied into every font size at imgui.cpp:9215) all follow; `ApplyScale()` now sizes `WindowMinSize` from `ScaleFactor()` |
| 92 | A `settings.ini` in a folder without write permission failed silently: `WritePrivateProfileStringA`'s return value was ignored, so every preference looked saved and nothing survived a re-inject — indistinguishable from working persistence | `WriteInt` reports success, `Save()` aggregates it, and the first failure pushes an activity entry plus a warn toast ("is the DLL folder writable?"); `Settings::SaveFailed()` exposes the state |
| 93 | The product still carried its upstream name everywhere a user looks - window title, 14 message-box captions, the sidebar text-logo fallback, the logo asset `rE.png`, the solution / project / output file names and the docs prose - although the project is EBengin | Rebranded end to end: new `EBengin.png` (EB monogram in the user's blue ramp + two-tone "EBengin" wordmark, same frame geometry and canvas as before), window title, fallback text, all 14 captions and the renderer strings say "EBengin", the solution is `EBengin UI.sln` with `ProjectName`/DLL `EBengin UI`, `build.bat` and `ci/build.yml` copy the new logo; historical mentions (recolour note, earlier fix rows; the credit comment was re-credited afterwards, row 94) were deliberately left untouched |
| 94 | After the rebrand the menu.cpp credit comment still carried the upstream line `//redENGINE Remade : by xo1337` | Re-credited on owner request: the comment now reads `//Remade : by EB`; the README credits line followed in row 103 |
| 95 | The rebrand stayed invisible in the running UI beyond the title bar: no credit line, no start card, no brand block where a user actually looks | `RenderSplash()` shows a branded start card on the first open per inject (logo, name, "Remade : by EB", two honest hints, Continue); the sidebar footer gains a muted fourth credit line and Config's right pane an About block (name, credit, ImGui version); the renderer mirrors all three and adds a `--splash` preview |
| 96 | The splash card replaces the shell on first open but never took keyboard focus: the shell's open-time `SetNextWindowFocus` handoff is skipped while the splash is up, so Continue was unreachable by keyboard until the first click | `RenderSplash()` performs the same handoff while `g_justOpened` is set (the flag stays alive so the sidebar still places the nav cursor after Continue); the renderer docstring now lists all five preview branches instead of only Executor/Config |
| 97 | The start card's second hint hardcoded "INSERT" although the toggle key is user-bindable (Config -> Toggle key), so after a rebind the card's own instruction was wrong | The hint is formatted with `Settings::KeyName(Settings::Current.ToggleKey)` - the same helper the Config button and the key-change toast use; the renderer mirror keeps the default binding and says so in a comment; README file table and the Docs & render row now list the `--splash` render |
| 98 | Pressing F5 in Visual Studio pops a scary dialog - "…\EBengin UI.dll is not a valid Win32 application" / "… ist keine gültige Win32-Anwendung" - which reads like a broken build although the build succeeded: a DLL has no executable entry point, so Windows refuses to launch it | Documented in three places: a warning under Option C, a Troubleshooting entry with the two legitimate workflows (build-only via Ctrl+Shift+B / build.bat; real debugging via Debugging → Command or Attach to Process), and a README note; no project file changed, the per-machine debugger command stays untracked |
| 99 | The Config About block listed name, credit and ImGui version but not which configuration the loaded DLL was built with - a Debug DLL behaves visibly differently in a host (asserts on, optimisations off) and nothing in the UI said so | The About block ends with an `#ifdef _DEBUG` line: `Build: Debug (asserts on, slower)` versus `Build: Release`; the renderer mirrors the Release wording and width-asserts both variants |
| 100 | The About build marker said Debug or Release but not the bitness - a Win32 DLL cannot load into a 64-bit host (section 8 says so in prose), yet nothing in the UI distinguished x64 from Win32 | The marker now has four variants via `_DEBUG` / `_WIN64`: `Build: Debug x64 (asserts on, slower)`, `Build: Debug Win32 (asserts on, slower)`, `Build: Release x64`, `Build: Release Win32 (x86)`; the renderer mirrors Release x64 and width-asserts all four |
| 101 | The theme left several ImGui defaults in stock grey / near-black (TextDisabled, PopupBg, scrollbar grabs) and the sidebar floated as button blocks on the canvas with no panel hierarchy; the selected nav row differed from a pressed button only by the 3px bar | Style refinement, no layout change: sidebar cell painted one notch darker via `TableSetBgColor(CellBg)` (z-safe, under the widgets), selected row gets a left-to-right accent wash (alpha 22->0), pill grabs (`GrabRounding` 6), scrollbar grabs on the raised/hover/pressed ramp, `TextDisabled` = muted accent, popups on canvas navy; the renderer mirrors every stop and re-rendered four previews (splash has no sidebar) |
| 102 | The FPS/frame-time line existed only in the sidebar footer, i.e. only while the overlay was open - with the overlay closed there was no frame-rate feedback at all | `Menu::RenderHud()` draws the same string as a top-left plate (dark rounded backing for legibility, accent text, background draw list: over the scene, under any window) whenever the overlay is closed; switchable via Config -> `FPS HUD` (default on, persisted as `[ui] hud`).  The switch first overflowed the Config left pane by 18px; the renderer's overflow assert caught it pre-ship and the keyboard hint was compacted from three wrapped lines to two (same information) to make room; new `--hud` preview |
| 103 | The README credits line still attributed the menu UI to the upstream author although the owner had re-credited the project twice (source comment, then this line on offer) | The credits line now reads "Menu UI made by EB" - ImGui credit and the rest of the line untouched; the UI credit wording stays "Remade : by EB" as explicitly set earlier; the changelog keeps the provenance record |
| 104 | On a high-DPI host the shell scales by host contents scale times interface scale, but nothing in the UI reported either factor - a size report from another machine could not be placed without asking for screenshots of settings | The Config About block ends with a `Host: DPI %.2f, interface %d%%` line formatted from `Settings::DpiScale` and `Settings::Current.ScalePercent`; the renderer mirrors the 96-DPI / 100% default and width-asserts the string |
| 105 | A size or behaviour report from another machine arrived without its environment: build configuration, host DPI scale, interface scale and toggle key live in four places (About block, settings.ini, key binding) and had to be asked for one by one | The About block gains a `Copy diagnostics` button that puts one line - name, build config, DPI and interface scale, ImGui version, toggle key - on the Windows clipboard, with success/warn toast and activity entry; the renderer mirrors the button and asserts its label |
| 106 | The About block and the diagnostics clipboard line hardcoded the ImGui version string, so a vendored imgui update would leave both lying, and the diagnostics line named no file to attach | Both strings now format `ImGui::GetVersion()` (the version the linked imgui.cpp was compiled with) and the diagnostics line appends `| settings: <path>` from a now-public `Settings::IniPath()`; buffer grew 256 to 640 for MAX_PATH; the renderer pins the vendored tag and says so in a comment |
| 107 | `Load from File` had no counterpart - a script buffer could only leave the editor through the clipboard, so anything written or pasted in the menu could not be kept as a file from the menu itself | A save icon at the end of the editor toolbar opens `GetSaveFileNameA` with the loader's filter, `lua` default extension and the dialog's own overwrite prompt; open/write/incomplete-write failures each report via message box, activity entry and warn toast, success reports the byte count; the renderer mirrors the sixth toolbar icon |
| 108 | A failed settings.ini write answered once - a warn toast and an activity entry at the moment of failure - and then nothing: seconds later nothing on screen said that every further change would be lost on restart | While `Settings::SaveFailed()` is true, the Config About block shows a permanent red `Settings not writable - changes are lost on restart.` line in the project's error red; the renderer comments that the writable default draws nothing |
| 109 | The session activity log - the only timestamped trace of failures, refusals and file operations - could be watched and cleared but not taken with you: a report meant retyping it line by line | A `Copy` button beside `Clear` on the landing tab joins the entries as displayed (newest first, CRLF) and puts them on the clipboard via a new header-only `clipboard.h` shared with Config's Copy diagnostics; success and clipboard-unavailable paths toast and log themselves |
| 110 | After a load or save nothing on screen said which file the buffer came from or went to - a second load replaced the text and the origin was gone, and the status line named only cursor, line count and language | `exec.cpp` remembers the last successful load/save path in a file-static and the status line appends `\| file: <base name>`; the renderer comments that the no-file default draws nothing |
| 111 | Diagnostics could be copied, but attaching `settings.ini` (or a dump) to a report still meant finding the DLL folder by hand | A second About button, `Open settings folder`, runs `ShellExecuteA("explore", "/select,<ini path>")` so Explorer opens with the file highlighted; a return code of 32 or less (Explorer refused) answers with an activity entry carrying the code plus a warn toast; `shell32.lib` is linked via pragma |
| 112 | `Load from File` and the toolbar Clear replaced the buffer through `TextEditor::SetText`, which clears the undo buffer - one mis-click on an unsaved buffer was unrecoverable, with no question asked | Both paths confirm via `MB_OKCANCEL` naming the byte count whenever the buffer is non-empty (empty buffer: no question); Cancel keeps the buffer and logs itself, a confirmed Clear also drops the status line's file reference and logs the cleared size |
| 113 | On a first start without `settings.ini` nothing said that the compiled defaults were in effect and that the file only appears with the first saved change - a "why is there no settings.ini?" report had no answer on screen | `Settings::Load()` checks `GetFileAttributesA` and, when the file is absent, pushes one activity entry naming the path plus an info toast, so the landing log and the first open both carry the explanation |
| 114 | The codebase carried three clipboard writers: the shared `clipboard.h` plus a file-static legacy helper in `resources.cpp` (with an `HWND` parameter and an `extern HWND window` solely to feed it) that duplicated the whole OpenClipboard/GlobalAlloc/CF_TEXT sequence and its own failure wording | Both `resources.cpp` call sites (dump path, trigger rows) now use `CopyTextToClipboard`; the legacy helper and the extern are gone, so the sequence and its failure behaviour exist exactly once |
| 115 | A successful stopper removal - the one honest stub that visibly changes state - answered only with a modal message box; once closed, neither the activity log nor a toast said it happened, unlike the Execute and Run requests | The removal now also pushes an activity entry and an info toast naming the resource and repeating that the server stop is not implemented |
| 116 | `Open settings folder` always passed `/select,<ini path>`, but on a first start the ini does not exist yet and Explorer then has nothing to highlight | When `GetFileAttributesA` reports the ini missing, the button explores the folder itself instead of selecting a file that is not there |
| 117 | The row-113 first-start toast was pushed from `Settings::Load()`, but toasts live four seconds and the overlay opens later than that - the notice expired unseen on every first start, leaving only the log entry | `Load()` sets a flag instead; a new `Settings::ConsumeFirstStartNotice()` returns and clears it, and `Menu::Render()` consumes it on the first frame the menu is actually visible, so the info toast now arrives when it can be read |
| 118 | The diagnostics clipboard line named the scale factors but not what they produce - a report from a 200% machine still left the reader computing 900 times 2 by hand | The line carries `\| shell: WxH` computed from `900/540 * Settings::ScaleFactor()`, the same factor `ApplyScale()` writes into `style.WindowMinSize` |
| 119 | Nothing showed whether the editor buffer held unsaved edits - with Save and Load present, closing the menu or loading over a worked-on buffer gave no on-screen hint that work existed only in memory | `exec.cpp` polls `TextEditor::IsTextChanged()` right after the widget render (the frame-scoped edit flag) and accumulates it into a session dirty flag that load/save/clear reset; while set, the status line appends `\| modified`; `SetText` from our own load/clear is suppressed for one frame so those do not count as edits |
| 120 | The row-119 dirty flag was declared after the toolbar handlers that reset it (clear and save sit above the editor render in `Executor::Render()`), i.e. used before declaration - the translation unit would not have compiled | The two `static` declarations and their comment moved to the top of `Render()`, above every handler that reads or resets them |
| 121 | The landing tab was the only tab body without a preview render: its hint line, uptime line and the Copy/Clear row that shares a line with the heading were neither documented in the README figure set nor protected by a renderer assert | `docs/render_preview.py --default` renders the landing tab (sparkline panel, uptime line, three log entries, Copy/Clear pair, no sidebar highlight, no toast) into `docs/preview_default.png` and asserts every single-line text against the content column plus the button row against the heading; README gains the seventh figure and file-table row, the docstring usage block lists all seven outputs |
| 122 | The new header-only `clipboard.h` was the only root header absent from the project's `ClInclude` list - the build never needs it (includes resolve by path), but Solution Explorer and header search skipped the one header four translation units share | `<ClInclude Include="clipboard.h" />` added in alphabetical position; a static audit of the vcxproj against the root file list confirms every other root `.cpp` is compiled (with `font_awesome.cpp` correctly `ExcludedFromBuild`) and every other root header listed |
| 123 | The row-122 audit had checked the vcxproj but not its `.filters` companion: `clipboard.h` had no filter entry either, so Solution Explorer would have shown it outside every folder while all other root headers sit in theirs | `<ClInclude Include="clipboard.h">` with `Source Files\menu`, the same filter as `assets.h`; the audit now covers both files and finds no stale entries |
| 124 | The HUD plate was the one surface visible while the overlay is closed, and the one place the owner credit did not appear - splash, footer and About carry it, the closed-menu view did not | `Menu::RenderHud()` draws a second plate line, `MADE BY EB`, in the footer's muted brand alpha (93,156,214,153); the plate grows to two lines (width = max of both lines, height = line advance + credit line) and the `--hud` render mirrors plate, both lines and the ink-over-plate blend |
| 125 | The diagnostics clipboard line carried the environment but not two states a size/behaviour report routinely needs: whether the FPS HUD was even switched on, and whether the settings file those prefs persist to is writable at all | The line appends `\| hud: on/off` from `Settings::Current.Hud` and, when `Settings::SaveFailed()` is true, ` [NOT WRITABLE]` directly after the settings path - the same two facts the About block shows visually, now copyable |
| 126 | Once the buffer carried unsaved edits (row 119), the only ways back were undo keystrokes or reloading by hand through the Load dialog - and the read path (size cap, empty check, BOM strip) existed once, inside the Load handler, where a second consumer could not reach it | A `ReadFileText()` helper in `exec.cpp` owns the read path for both consumers; a revert icon at the end of the editor toolbar reloads the file behind the buffer - grey without a file reference, an info toast when the buffer already matches, a confirm only when the dirty mark shows - and resets the dirty flag; the renderer mirrors the eighth toolbar icon in its disabled state |
| 127 | Paste and clear were the last toolbar buttons that could click into a silent no-op - paste on an empty clipboard, clear on an empty buffer - while undo/redo/copy already greyed out with an explanation | Paste greys out while the clipboard holds no text (`IsClipboardFormatAvailable`, which needs no OpenClipboard), clear while the buffer is empty; both tooltips name which side is missing, and the preview scene keeps them live because its buffer and clipboard hold text |
| 128 | Saving always reopened the file dialog, even to write the buffer back to the file it came from, and the open/write/short-write sequence lived inline in the one handler that needed it | `WriteFileText()` single-sources the write path the way `ReadFileText()` does the read path; the Save button writes to the last file in one click (dialog becomes Save As via Shift+click or first save), Ctrl+S mirrors it, and the tooltip names the target file |
| 129 | After row 128 gave Save a Ctrl+S mirror, Load was the last file-workflow action reachable only by mouse, and the Save As dialog had no shortcut of its own | The Load handler body became a `loadFile` lambda the button and a new Ctrl+O check both call; Ctrl+Shift+S reaches the dialog as Save As through saveBuffer's existing KeyShift read, and the Load/Save tooltips now name their shortcuts |







---

## 9. Architecture overview

How the pieces fit together, top to bottom:

| Layer | Files | Responsibility |
|---|---|---|
| Hook & lifetime | `main.cpp`, `includes.h`, `kiero/` | `DllMain` spawns `MainThread`; kiero hooks `IDXGISwapChain::Present` (vtable 8). `hkPresent` owns device/render-target lifecycle, resize & device-lost rebuild, the edge-triggered toggle poll and the frame pump |
| Shell | `menu.cpp`, `menu.h`, `menu_style.h` | One ImGui window: sidebar on a darker nav-rail cell (logo, tab buttons, animated marker, accent wash on the selected row, FPS footer plus credit line), two-column table shell, open fade, drop shadow, brand rule, window-position persistence; first-open splash start card; `RenderHud()` FPS plate with the `MADE BY EB` credit line over the scene while the overlay is closed; `ApplyStyle()` sets the theme once (six-stop ramps plus the cohesion stops) |
| Routing | `tabs.cpp`, `tabs.h` | `switch (Menu::Tab)` into the five tab bodies; the shell never learns which tabs exist |
| Tab bodies | `default.cpp`, `exec.cpp`, `resources.cpp`, `luamenus.cpp`, `config.cpp` | Landing (activity log, diagnostics card), executor (TextEditor + toolbar incl. save-to-file + status line), resources (browser/triggers/stopper + filters + dump), menus (two lists), config (switches incl. FPS HUD, key capture, reset, About block with build/host info, not-writable flag, Copy diagnostics and Open settings folder buttons) |
| Widgets & feedback | `gui.h`, `toasts.*`, `activity.*`, `perf.*`, `settings.*` | `SectionTitle`/`Seperator`/`PrimaryButton`/`Toggle`; toast stack; session log; frame-time ring; `settings.ini` persistence — single-purpose modules, each with its own header |
| Vendored | `imgui/` (1.92.9), `TextEditor.*`, `stb_image.h`, `font_awesome.*`, `kiero/`, MinHook | In-tree, no submodules; `imgui_demo.cpp` excluded (C4996 under `SDLCheck`) |
| Docs & render | `README.md`, `BUILD.md`, `CHANGELOG.md`, `docs/render_preview.py`, `docs/preview.png`, `docs/preview_config.png`, `docs/preview_resources.png`, `docs/preview_menus.png`, `docs/preview_splash.png`, `docs/preview_hud.png`, `docs/preview_default.png` | The renderer re-draws the shell from the repo's own constants (theme ramps, sidebar metrics, editor palette, the real embedded FA font) — a render, not a screenshot. `--config` renders the Config tab and asserts that every label fits its pane (row 84); `--resources` renders the File Browser sub-tab and caught row 86, `--menus` the Menus tab and caught row 88, `--splash` the first-open start card (row 95), `--hud` the FPS plate over a scene stand-in (row 102), `--default` the landing tab with its Copy/Clear row (row 121). The Triggers pane is never depicted README include graph regenerated by `docs/gen_include_graph.py` from the real `#include` lines (35 nodes, 63 edges); `docs/api_sweep.py` greps the root sources for ImGui APIs the vendored 1.92.9 no longer provides (one of the few executable compile checks without MSVC) |

Data flow per frame: `hkPresent` → NewFrame trio → toggle poll → `Menu::Render()` (fade,
shadow, shell table) → `Tabs::Render()` → tab body → `Toasts::Render()` → position saver →
`ImGui::Render()` → DX11 draw data → original `Present`.

Nothing in the menu reads or writes game state: every "action" button (execute, run, stop)
reports that it is not implemented in this build, and the only file-system writes are the
resource dump the user explicitly saves and `settings.ini`.

---

## 6. ImGui 1.75 → 1.92.9 migration

Six years of API churn. The old names are **actually gone** in 1.92.9 — the
`IMGUI_DISABLE_OBSOLETE_FUNCTIONS` redirections at the bottom of `imgui.h` are commented out
(`//static inline …`), so they do not even compile as deprecated shims.

Vendored: the 15 upstream files (`imgui.h/.cpp`, `imgui_internal.h`, `imgui_draw.cpp`,
`imgui_widgets.cpp`, **`imgui_tables.cpp`** (new TU), `imgui_demo.cpp`, `imconfig.h`, the three
`imstb_*` headers, and `imgui_impl_dx11` + `imgui_impl_win32` flattened out of `backends/` so
the include layout is unchanged). `imgui/` stays byte-identical to upstream.

| Old (1.75) | New (1.92.9) | Where |
|---|---|---|
| `ListBoxHeader(label, size)` / `ListBoxFooter()` | `BeginListBox` / `EndListBox` — removed in 1.89 | `luamenus.h`, `resources.cpp` (3 sites) |
| `GetContentRegionAvailWidth()` | `GetContentRegionAvail().x` — obsoleted in 1.70, redirection now gone | `resources.cpp` (2 sites) |
| `BeginChild(id, size, bool border, windowFlags)` | `BeginChild(id, size, ImGuiChildFlags, windowFlags)` — 1.90 inserted a parameter | `gui.h`, `luamenus.h`, `config.cpp` ×2, `exec.cpp`, `TextEditor.cpp` |
| `ImGui::Image((void*)logo, …)` | `ImTextureID` is now `ImU64` and drawing takes `ImTextureRef` (1.92) → `(ImTextureID)(intptr_t)logo` | `menu.h` |
| `IsKeyPressed(GetKeyIndex(ImGuiKey_X))` | `IsKeyPressed(ImGuiKey_X)` — `GetKeyIndex()` removed; keys are direct since 1.87 | `TextEditor.cpp` (25 sites) |
| `PushAllowKeyboardFocus` / `Pop…` | `PushItemFlag(ImGuiItemFlags_NoTabStop, false)` / `PopItemFlag()` — the `PushTabStop` replacement is itself obsoleted in 1.91.0 | `TextEditor.cpp` |
| `GetWindowContentRegionMax()` | `GetContentRegionAvail()` — obsoleted in 1.91.0, and its meaning changed in 1.90 (now includes `WindowPadding`) | `TextEditor.cpp` |
| `io.WantCaptureKeyboard = true`<br>`io.WantTextInput = true`<br>`io.WantCaptureMouse = true` | `SetNextFrameWantCaptureKeyboard(true)` + `SetKeyboardFocusHere(-1)` / `SetNextFrameWantCaptureMouse(true)`. These are **outputs** of `NewFrame()`; the assignments were always no-ops. There is no `SetNextFrameWantTextInput` — focus is what drives it | `TextEditor.cpp` (3 sites) |
| `OversampleH = 3` | `ImS8`, `0 == auto`, and ignored entirely when `PixelSnapH` is set → `0` | `main.cpp` |
| `font->FontSize` | member removed: the size handed to `AddFont()` is now `ImFont::LegacySize`, and for drawing you let the draw list decide — `AddText(NULL, 0.0f, …)` uses `_Data->Font` / `_Data->FontSize` | `toasts.cpp` |
| `AddFontFromFileTTF(path, 20)` | second parameter is `float size_pixels` → `20.0f` | `main.cpp` |

Unchanged and verified present: all 61 enum constants the project uses, all 88 `ImGui::` calls,
the 5 `ImDrawList` methods `TextEditor.cpp` draws with, `Columns`/`NextColumn`/`SetColumnOffset`
(legacy but still supported), the font-*loading* API (`AddFontFromFileTTF`, `AddFontFromMemoryCompressedTTF`,
`AddFontDefault` — but not `ImFont::FontSize`, see the table above), and `ImGui_ImplWin32_WndProcHandler`
including its `0 == not handled` contract.

`imconfig.h` was replaced with the upstream one: the project's copy had no local `#define`s, only
older comments, so nothing was lost.

### Legacy `Columns()` → `Tables`

`ImGui::Columns` / `NextColumn` / `SetColumnOffset` still compile in 1.92.9 but have been
deprecated since 1.80, and the two-column menu shell was the last place using them. It is now a
table:

```cpp
if (ImGui::BeginTable("##maincolumn", 2,
    ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
{
    ImGui::TableSetupColumn("##sidebar", ImGuiTableColumnFlags_WidthFixed,   SidebarWidth);
    ImGui::TableSetupColumn("##content", ImGuiTableColumnFlags_WidthStretch);
    ImGui::TableNextRow();
    ImGui::TableNextColumn(); RenderSidebar();
    ImGui::TableNextColumn(); /* tab content */
    ImGui::EndTable();
}
```

Flag choices, because the defaults are not equivalent to the old API:

| Choice | Why |
|---|---|
| `BordersInnerV` (not `BordersV`) | `BordersV` = `BordersInnerV \| BordersOuterV`, i.e. it would add an outer frame. `Columns(2, id, true)` drew only the divider between the two columns. |
| `SizingFixedFit` | Required for `ImGuiTableColumnFlags_WidthFixed` to be honoured. Without an explicit sizing policy the table auto-fits to contents and the 225px sidebar drifts. |
| `NoPadOuterX` | Content starts flush with the window edge, as it did before. |
| `style.CellPadding = ImVec2(0,0)` in `ApplyStyle()` | Table cells are inset by `CellPadding` (default `4,1`) on every side, which would leave the 225px sidebar with 217px of usable width and shift the tab content 4px right. `Columns()` had no such inset. This is the one change outside `menu.h`. |

Two things this also cleaned up:

* `SetCursorPosY(GetWindowHeight() - 10); Text("");` at the end of the sidebar — whose only job
  was to stretch the legacy column cell to full height ("so don't remove :)"). A table row is
  already as tall as its tallest cell, so the hack is gone.
* Each table cell now has its own layout context, so `GetContentRegionAvail()` inside a tab
  reports that cell instead of the whole window — which is what the tab code already assumed.

`demo.cpp` was migrated too (`SizingStretchSame` reproduces the two even columns), so **the project
code now uses no deprecated ImGui API at all** and `#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS` in
`imgui/imconfig.h` compiles cleanly. That macro is the recommended way to stay ahead of the next
upgrade: it turns every future removal into a compile error at the point of use instead of a
silent behaviour change.

### Shell / tabs split

`menu.h` was the whole UI in one header: window, sidebar, tab routing, *and* the includes of all five
tab headers. It is now three units with one direction of dependency:

```
main.cpp ──> menu.h ──> (nothing but imgui.h + d3d11.h)
    │           ▲
    │           │ Menu::Render / IsMenuOpen / TabId
    │        menu.cpp ──> tabs.h ──> (declares Tabs::Render only)
    │                        ▲
    └──> exec.h, resources.h  └── tabs.cpp ──> default.h exec.h resources.h luamenus.h config.h
         (Init() only)
```

| File | Responsibility | Includes |
|---|---|---|
| `menu.h` | `Menu::Render` declaration, `Menu::TabId`, `extern IsMenuOpen/Tab/logo/LogoLoaded`, `SidebarWidth` | `imgui.h`, `d3d11.h` |
| `menu.cpp` | the window, the table, `SidebarItems[]`, `RenderSidebar()` (file-local) | `menu.h`, `tabs.h`, `gui.h`, `font_awesome.h` |
| `tabs.h` | `Tabs::Render` declaration | nothing |
| `tabs.cpp` | the `switch` over `Menu::Tab` | `tabs.h`, `menu.h`, the five tab headers |

What that buys concretely:

* `main.cpp` no longer sees any tab header. It only calls `Executor::Init()` / `Resources::Init()`,
  so it includes just those two.
* `menu.cpp` does not know which tabs exist — it calls `Tabs::Render()`. Editing a tab body cannot
  force the shell to recompile.
* `RenderSidebar()` became file-local (`static`), which it always should have been.
* The same treatment went through the tab headers: `default.h`, `luamenus.h` and `demo.h` are now
  declarations only, with their bodies in `default.cpp`, `luamenus.cpp` and `demo.cpp`. `luamenus.h`
  had been defining two `static std::vector`s and two selection ints, and `demo.h` two **non-static**
  bools plus four char arrays — all of which were one include away from `LNK2005`.

Adding a tab is now three edits: an enumerator in `Menu::TabId`, a row in `SidebarItems[]`, and a
`case` in `Tabs::Render()`.

---

## 7. Continuous integration

> **Status: written, validated, committed at `ci/build.yml` — but not enabled.**
>
> The workflow below is a complete GitHub Actions file. It is committed at **[`ci/build.yml`](ci/build.yml)**,
> not at `.github/workflows/build.yml`, because the GitHub App token used to produce this branch is
> refused the push to that path:
>
> ```
> ! [remote rejected] (refusing to allow a GitHub App to create or update workflow
>                      `.github/workflows/build.yml` without `workflows` permission)
> ```
>
> Creating or updating `.github/workflows/*` needs the `workflows` permission. Parking the file under
> `ci/` keeps it in the repository, in the PR diff and reviewable as an ordinary file instead of
> surviving only in somebody's working tree. It has no effect there — GitHub only reads
> `.github/workflows/`.
>
> **To enable it**, copy the file into place and commit that from an account holding the permission:
>
> ```bat
> ci\enable.bat
> git commit -m "ci: enable the Windows build workflow"
> git push origin arena/57915bfa-eb-fivem
> ```
>
> `enable.bat` copies `ci\build.yml` to `.github\workflows\build.yml` and runs `git add` on it, then
> stops — it deliberately does not commit or push, so the commit lands under your identity with your
> permissions. The web-UI route and the trade-offs are described in [`ci/README.md`](ci/README.md).
>
> Until that happens this repository has **no CI and no compile proof**; every check recorded in this
> document was static. Everything else is committed and pushed — only this one file is displaced.

The workflow builds **Release\|x64** and **Debug\|x64** on `windows-latest` for every push to
`main`, every pull request, and on demand (`workflow_dispatch`). Each job:

1. `microsoft/setup-msbuild@v2` — MSBuild is installed on the runner but is not on `PATH`.
2. prints the resolved VS installation, its version and the MSBuild version, so a toolchain
   difference shows up in the log rather than as a mystery failure;
3. `msbuild "EBengin UI.sln" /t:Rebuild /p:Configuration=… /p:Platform=… /m`;
4. **fails the job if `x64\<Config>\EBengin UI.dll` does not exist**, then prints its size and
   `/exports` — a `DynamicLibrary` that lost its entry point would still link, so the export table
   is checked rather than trusted;
5. uploads the DLL, its PDB and `EBengin.png` as an artifact (14 days).

`Win32` is deliberately not in the matrix: the DLL is injected into FiveM, which is 64-bit only, so
a Win32 artifact could never be loaded. The configuration still exists in the `.vcxproj`.

Until it is enabled, the equivalent local command is `build.bat`, or:

```bat
msbuild "EBengin UI.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
```

Two things worth knowing:

* `windows-latest` ships VS2022/v143, not v142 — so every run also exercises the conditional
  `PlatformToolset` fallback added in section 4.
* `SDLCheck` is `true` in all four configurations, which makes `C4996` a hard **error** rather than a
  warning. See the note on `imgui_demo.cpp` below before adding any vendored file to the project.

---

## 8. Troubleshooting

**F5 / "Start Debugging" pops "…\EBengin UI.dll is not a valid Win32 application"
(German: "… ist keine gültige Win32-Anwendung")**
Expected, and not a build error: the output of this project is a DLL, and `CreateProcess`
cannot launch a library - it has no executable entry point.  The build succeeded when this
dialog appears (check `x64\<Config>\EBengin UI.dll` exists).  Two legitimate workflows:

1. **Build only** - Ctrl+Shift+B in the IDE, or `build.bat` / the MSBuild line above.  This
   is what the CI job does as well.
2. **Debug for real** - either point *Project properties → Debugging → Command* at the host
   process that loads the DLL, or load the DLL first and then use
   *Debug → Attach to Process…* on that process.  Neither is preconfigured on purpose: the
   host is a per-machine choice and belongs in your local `.user` file, not in the tracked
   project files.

   Confirmed on a real Windows machine (2026-10-23, Visual Studio, Release x64):
   `1 erfolgreich, 0 Fehler` - the first real MSVC compile of this branch.  Reader's note for
   the build log: the compile list shows 26 translation units because `font_awesome.cpp` is
   `ExcludedFromBuild` **on purpose** - `main.cpp` includes it directly (`#include
   "font_awesome.cpp"`), so its `static` blob is compiled inside `main.cpp`.  Do not "fix" the
   exclusion; the LTCG "IPDB not found" lines are first-link information, not warnings.

**`error LNK2019: unresolved external symbol ... d3d11`**
`d3d11.lib` and `dxgi.lib` are in `AdditionalDependencies` for all four configs; if you
added a config by hand, add them there too.

**`fatal error C1083: Cannot open include file: 'd3d11.h'`**
The "Desktop development with C++" workload (and with it a Windows SDK) is not installed.
Run the VS Installer and add it.

**Builds fine but nothing appears in-game**
Confirm you built **x64** — a Win32 DLL cannot be loaded by FiveM. Also check the injector
targets the `FiveM.exe` / `FiveM_GTAProcess.exe` process that actually owns the D3D11
swap chain; `kiero::init` loops until it succeeds, so a wrong target looks like a hang.

**Menu opens but has no icons (squares instead of Font Awesome glyphs)**
The merged icon font failed to load; `main.cpp` falls back to `AddFontDefault()`, so this only
shows up if `font_awesome_data` is corrupt. The icon range `0xf000`–`0xf3ff` still fits `ImWchar`
(16-bit by default).

**`LNK2019` mentioning `ImGui::TableNextRow`, `TableGetSortSpecs` or similar**
`imgui/imgui_tables.cpp` is not in the project. It became a separate translation unit in 1.80
and is required even if you never call a `Table*` function yourself.

**`error C4996` on a vendored file (e.g. `imgui_demo.cpp`)**
`SDLCheck` is `true`, which turns `C4996` into an error. Only add upstream files that are clean;
`imgui_demo.cpp` uses bare `sprintf()` in ~33 places and is therefore excluded from the project.

**`C2065: 'ImGuiChildFlags_None': undeclared identifier`**
An old `imgui/imgui.h` is still on the include path. `ImGuiChildFlags` arrived in 1.90; make sure
the vendored tree really is 1.92.9 (`IMGUI_VERSION` at the top of `imgui/imgui.h`).

**`MSB8036: The Windows SDK version 10.0.xxxxx.0 was not found`**
Right-click the project → *Retarget solution*, or install that SDK. The project asks for
`10.0` (= latest installed), so this only happens with a stale `Directory.Build.props` or
a custom `WindowsTargetPlatformVersion` in `%LOCALAPPDATA%\Microsoft.Cpp.*.user.props`.
