# Building redENGINE UI

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

The script locates MSBuild via `vswhere`, rebuilds, prints the DLL path and copies `rE.png`
next to the DLL so the menu logo resolves at runtime.

### Option B — MSBuild by hand

From a **Developer Command Prompt for VS 2022**:

```bat
msbuild "redENGINE UI.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
```

Output:

```
x64\Release\redENGINE UI.dll
```

(`ProjectName` is `redENGINE UI`, so that is the produced file name — not
`ImGui DirectX 11 Kiero Hook.dll`, which is only the `.vcxproj` file name.)

### Option C — inside Visual Studio

1. Open `redENGINE UI.sln`.
2. Pick **Release / x64** in the toolbar.
3. `Build → Build Solution` (Ctrl+Shift+B).

---

## 3. Runtime assets

The DLL resolves its logo **relative to its own directory** (falling back to the process
working directory):

| Asset | Where it is looked up | If missing |
|---|---|---|
| `rE.png` | next to the injected `.dll` | the sidebar prints a red `redENGINE` text logo instead — no crash |
| `C:\Windows\Fonts\Bahnschrift.ttf` | fixed Windows path | falls back to ImGui's built-in default font |

So a working deployment is just:

```
<folder>\redENGINE UI.dll
<folder>\rE.png
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
| 83 | Config's "Safe Mode" and "ScriptHook Bypass" sections described behaviour that does not exist — the tooltips promised "a private scripting environment" and ".ASI plugins through the ScriptHook interface", while `config.h` documents both bools as UI state that nothing reads and nothing persists. Two switches that look functional but are inert is the same defect the dead Execute/Run stubs had | Both headings now read `(not implemented)` in `TextDisabled`, the tooltips say plainly that the switch only stores a value, and every flip writes an activity entry plus a warn toast — the honest pattern the execute/run/dump paths already use. The bools stay (dropping them would change the tab's layout for no gain); nothing reads them either way |

---

## 9. Architecture overview

How the pieces fit together, top to bottom:

| Layer | Files | Responsibility |
|---|---|---|
| Hook & lifetime | `main.cpp`, `includes.h`, `kiero/` | `DllMain` spawns `MainThread`; kiero hooks `IDXGISwapChain::Present` (vtable 8). `hkPresent` owns device/render-target lifecycle, resize & device-lost rebuild, the edge-triggered toggle poll and the frame pump |
| Shell | `menu.cpp`, `menu.h`, `menu_style.h` | One ImGui window: sidebar (logo, tab buttons, animated marker, FPS footer), two-column table shell, open fade, drop shadow, brand rule, window-position persistence; `ApplyStyle()` sets the six-stop theme once |
| Routing | `tabs.cpp`, `tabs.h` | `switch (Menu::Tab)` into the five tab bodies; the shell never learns which tabs exist |
| Tab bodies | `default.cpp`, `exec.cpp`, `resources.cpp`, `luamenus.cpp`, `config.cpp` | Landing (activity log, diagnostics card), executor (TextEditor + toolbar + status line), resources (browser/triggers/stopper + filters + dump), menus (two lists), config (switches, key capture, reset) |
| Widgets & feedback | `gui.h`, `toasts.*`, `activity.*`, `perf.*`, `settings.*` | `SectionTitle`/`Seperator`/`PrimaryButton`/`Toggle`; toast stack; session log; frame-time ring; `settings.ini` persistence — single-purpose modules, each with its own header |
| Vendored | `imgui/` (1.92.9), `TextEditor.*`, `stb_image.h`, `font_awesome.*`, `kiero/`, MinHook | In-tree, no submodules; `imgui_demo.cpp` excluded (C4996 under `SDLCheck`) |
| Docs & render | `README.md`, `BUILD.md`, `docs/render_preview.py`, `docs/preview.png` | The renderer re-draws the shell from the repo's own constants (theme ramps, sidebar metrics, editor palette, the real embedded FA font) — a render, not a screenshot |

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
3. `msbuild "redENGINE UI.sln" /t:Rebuild /p:Configuration=… /p:Platform=… /m`;
4. **fails the job if `x64\<Config>\redENGINE UI.dll` does not exist**, then prints its size and
   `/exports` — a `DynamicLibrary` that lost its entry point would still link, so the export table
   is checked rather than trusted;
5. uploads the DLL, its PDB and `rE.png` as an artifact (14 days).

`Win32` is deliberately not in the matrix: the DLL is injected into FiveM, which is 64-bit only, so
a Win32 artifact could never be loaded. The configuration still exists in the `.vcxproj`.

Until it is enabled, the equivalent local command is `build.bat`, or:

```bat
msbuild "redENGINE UI.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
```

Two things worth knowing:

* `windows-latest` ships VS2022/v143, not v142 — so every run also exercises the conditional
  `PlatformToolset` fallback added in section 4.
* `SDLCheck` is `true` in all four configurations, which makes `C4996` a hard **error** rather than a
  warning. See the note on `imgui_demo.cpp` below before adding any vendored file to the project.

---

## 8. Troubleshooting

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
