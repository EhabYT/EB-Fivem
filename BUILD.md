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

The menu is toggled with **INSERT**.

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

Unchanged and verified present: all 61 enum constants the project uses, all 88 `ImGui::` calls,
the 5 `ImDrawList` methods `TextEditor.cpp` draws with, `Columns`/`NextColumn`/`SetColumnOffset`
(legacy but still supported), the font API (`AddFontFromFileTTF`, `AddFontFromMemoryCompressedTTF`,
`AddFontDefault`), and `ImGui_ImplWin32_WndProcHandler` including its `0 == not handled` contract.

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

> **Status: written, not enabled.** The workflow below lives at `.github/workflows/build.yml` in the
> working tree but is **not committed**, because the GitHub App token used here is refused the push:
>
> ```
> ! [remote rejected] (refusing to allow a GitHub App to create or update workflow
>                      `.github/workflows/build.yml` without `workflows` permission)
> ```
>
> Adding a workflow file needs the `workflows` permission. To enable it, either commit that file
> yourself, or paste the contents into a new file at `.github/workflows/build.yml` via the GitHub
> web UI. Everything else in this repository is committed and pushed; only the workflow is not.

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
