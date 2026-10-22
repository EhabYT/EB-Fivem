# EB-Fivem — redENGINE UI

An in-game **Dear ImGui overlay for FiveM**, delivered as a Windows x64 DLL.

`main.cpp` hooks `IDXGISwapChain::Present` (vtable index 8) with [Kiero](kiero/kiero.h) +
MinHook, subclasses the window procedure, initialises ImGui with the DX11/Win32 backends and
draws the **redENGINE** menu on top of the game. Press **INSERT** to toggle it.

Everything is vendored in-tree — no submodules, no package manager, no external downloads.

![redENGINE menu preview — Executor tab](docs/preview.png)

> **This is a render, not a screenshot.** The DLL cannot be executed on the machine that produced
> this branch (no MSVC, no Windows), so the image is drawn from the repository's own constants
> instead of being captured from a running game: every colour from [`menu_style.h`](menu_style.h),
> the 225 px sidebar and 46 px buttons from [`menu.h`](menu.h), the window title, logo placement,
> tab labels and footer strings from [`menu.cpp`](menu.cpp), the editor palette and Lua token
> colours from [`TextEditor.cpp`](TextEditor.cpp), and the sidebar/button glyphs from the project's
> **own embedded Font Awesome font** (the stb-compressed blob in `font_awesome.cpp`, decompressed for
> this render). Two things are substituted and cannot be faithful: the text face — `Bahnschrift.ttf`
> is a Windows-only font, so a horizontally condensed DejaVu Sans stands in for its DIN proportions —
> and ImGui's exact layout engine, which is not running here. The Lua snippet in the editor is
> illustrative; on first launch the menu opens on the empty landing tab.

## Layout

| Path | What it is |
|---|---|
| [`main.cpp`](main.cpp) | `DllMain`, Present hook, ImGui init/teardown, texture + font loading, input routing |
| [`includes.h`](includes.h) | Windows / D3D11 / DXGI / Kiero / ImGui includes, hook typedefs, `HK_CALL` |
| [`assets.h`](assets.h) | `ResolveAssetPath()` — finds files next to the DLL |
| [`menu.h`](menu.h) · [`menu.cpp`](menu.cpp) | Menu **shell**: the window, the 2-column `ImGui::Table`, the sidebar, `Menu::TabId` |
| [`tabs.h`](tabs.h) · [`tabs.cpp`](tabs.cpp) | **Tab routing**: the only place that knows which tabs exist |
| [`menu_style.h`](menu_style.h) | `ApplyStyle()` — colours, rounding, frame/cell padding (applied once) |
| [`gui.h`](gui.h) | Small helpers (`Gui::Seperator`, cursor setters) |
| [`exec.h`](exec.h) · [`exec.cpp`](exec.cpp) | Executor tab — Lua editor, Execute / Load-from-file, resource combo |
| [`resources.h`](resources.h) · [`resources.cpp`](resources.cpp) | Resources tab — file browser, trigger list, resource stopper, dumps |
| [`config.h`](config.h) · [`config.cpp`](config.cpp) | Config tab — Safe Mode / ScriptHook Bypass toggles |
| [`luamenus.h`](luamenus.h) · [`luamenus.cpp`](luamenus.cpp) | Menus tab — free / premium menu lists |
| [`demo.h`](demo.h) · [`demo.cpp`](demo.cpp) | Annotated ImGui widget cheat-sheet (not wired to a tab) |
| [`default.h`](default.h) · [`default.cpp`](default.cpp) | Landing tab |
| [`TextEditor.cpp`](TextEditor.cpp) · [`TextEditor.h`](TextEditor.h) | Vendored ImGui code editor widget |
| [`imgui/`](imgui) | Dear ImGui **1.92.9** + `imgui_impl_dx11` / `imgui_impl_win32` (vendored, unmodified). `imgui_demo.cpp` is on disk but not built — see [BUILD.md](BUILD.md) |
| [`kiero/`](kiero) | Kiero 1.2.6 (D3D11 enabled) + bundled MinHook 1.3.3 sources (vendored, unmodified) |
| [`ci/`](ci) | The GitHub Actions build workflow, staged as [`ci/build.yml`](ci/build.yml) + [`ci/enable.bat`](ci/enable.bat) — written and validated, **not yet enabled**. See [`ci/README.md`](ci/README.md) |
| [`font_awesome.h`](font_awesome.h) · [`font_awesome.cpp`](font_awesome.cpp) | Font Awesome icon macros + compressed TTF blob |
| [`stb_image.h`](stb_image.h) | PNG decoding for the menu logo |
| [`rE.png`](rE.png) | Menu logo, loaded from beside the DLL at runtime |
| [`docs/preview.png`](docs/preview.png) | The preview render shown above, with its caption explaining exactly what is and is not faithful |

## Architecture

Two views of the same code: what happens at runtime, and what depends on what at compile time.
Both are kept in sync with the source by hand — if you change the hook, the frame loop or an
`#include`, change these too.

### Lifecycle and frame loop

From injection to the first rendered frame, and what happens on resize, device loss and unload.

```mermaid
flowchart TD
    A(["Injector loads redENGINE UI.dll"]) --> B["DllMain: DLL_PROCESS_ATTACH"]
    B --> B1["g_hModule = hMod<br/>DisableThreadLibraryCalls"]
    B1 --> C["CreateThread → MainThread"]

    C --> D{"kiero::init D3D11<br/>== Success?"}
    D -- "no: UnknownError" --> D1["Sleep 500 ms"] --> D2{"120 attempts<br/>or unloading?"}
    D2 -- "keep retrying" --> D
    D2 -- "give up" --> D3(["thread exits FALSE"])
    D -- "no: NotSupported /<br/>ModuleNotFound" --> D3
    D -- "yes" --> E{"kiero::bind index 8<br/>IDXGISwapChain::Present"}
    E -- "failed" --> E1["kiero::shutdown"] --> D3
    E -- "ok" --> F(["hook live, oPresent saved"])

    F --> G["hkPresent runs every frame"]
    G --> H{"g_unloading or<br/>pSwapChain == NULL?"}
    H -- "yes" --> Z["return oPresent"]
    H -- "no" --> I{"init?"}

    I -- "first frame" --> J["GetDevice, GetImmediateContext<br/>GetDesc → window<br/>CreateRenderTarget"]
    J --> K{"all succeeded?"}
    K -- "no" --> Z
    K -- "yes" --> L["subclass WndProc<br/>only when oWndProc == NULL"]
    L --> M["InitImGui: fonts, rE.png beside the DLL,<br/>ApplyStyle once, Executor::Init, Resources::Init"]
    M --> N["init = true"]

    I -- "later frames" --> O{"back buffer size<br/>changed?"}
    O -- "yes" --> P["CleanupRenderTarget<br/>InvalidateDeviceObjects<br/>CreateRenderTarget<br/>CreateDeviceObjects"]
    O -- "no" --> Q
    P --> Q
    N --> Q

    Q["NewFrame for DX11, Win32 and ImGui<br/>MouseDrawCursor = Menu::IsMenuOpen"] --> R{"INSERT pressed?<br/>edge triggered"}
    R -- "yes" --> S["toggle Menu::IsMenuOpen"] --> T
    R -- "no" --> T{"IsMenuOpen?"}
    T -- "yes" --> U["Menu::Render<br/>sidebar column + Tabs::Render"] --> V
    T -- "no" --> V

    V["ImGui::Render<br/>OMSetRenderTargets<br/>RenderDrawData"] --> W["hr = oPresent"]
    W --> X{"DEVICE_REMOVED<br/>or DEVICE_RESET?"}
    X -- "yes" --> Y["CleanupImGui, release device<br/>and context → re-init next frame"] --> Z2(["return hr"])
    X -- "no" --> Z2

    G -.-> WP
    subgraph WP["WndProc, on every Windows message"]
        direction TB
        W1["ImGui_ImplWin32_WndProcHandler<br/>feeds ImGui"] --> W2{"over an ImGui window<br/>or a widget wants text?"}
        W2 -- "yes" --> W3(["return 0, the game never sees it"])
        W2 -- "no" --> W4(["CallWindowProc → game"])
    end

    UN(["DLL_PROCESS_DETACH"]) --> U1["g_unloading = true, Sleep 50 ms"] --> U2["CleanupImGui: backends, context,<br/>render target, logo, restore WndProc"] --> U3["kiero::shutdown"]
```

The two details that are easy to get wrong in a Present hook, and are therefore explicit in the
code: the window is subclassed **only** when `oWndProc == NULL` (a second `SetWindowLongPtr` would
store our own `WndProc` as the "original" and build an endless call chain), and `g_unloading` is
latched *before* anything is torn down so the game cannot be inside `hkPresent` while the ImGui
context disappears under it.

### Include graph

Rectangles are translation units, rounded boxes are headers, each subgraph is one `.cpp` plus the
header it owns. **Generated from the actual `#include` lines** — 23 nodes, 33 edges.

```mermaid
flowchart LR
    subgraph sg_TextEditor["TextEditor"]
        n0["TextEditor.cpp"]
        n1(["TextEditor.h"])
    end
    subgraph sg_config["config"]
        n3["config.cpp"]
        n4(["config.h"])
    end
    subgraph sg_default["default"]
        n5["default.cpp"]
        n6(["default.h"])
    end
    subgraph sg_demo["demo"]
        n7["demo.cpp"]
        n8(["demo.h"])
    end
    subgraph sg_exec["exec"]
        n9["exec.cpp"]
        n10(["exec.h"])
    end
    subgraph sg_luamenus["luamenus"]
        n13["luamenus.cpp"]
        n14(["luamenus.h"])
    end
    subgraph sg_main["main"]
        n15["main.cpp"]
    end
    subgraph sg_menu["menu"]
        n16["menu.cpp"]
        n17(["menu.h"])
    end
    subgraph sg_resources["resources"]
        n19["resources.cpp"]
        n20(["resources.h"])
    end
    subgraph sg_tabs["tabs"]
        n21["tabs.cpp"]
        n22(["tabs.h"])
    end
    n2(["assets.h"])
    n11(["gui.h"])
    n12(["includes.h"])
    n18(["menu_style.h"])

    n0 --> n1
    n3 --> n4
    n4 --> n11
    n5 --> n6
    n5 --> n11
    n7 --> n8
    n9 --> n10
    n9 --> n11
    n9 --> n20
    n10 --> n1
    n13 --> n11
    n13 --> n14
    n15 --> n2
    n15 --> n10
    n15 --> n12
    n15 --> n17
    n15 --> n18
    n15 --> n20
    n16 --> n11
    n16 --> n17
    n16 --> n22
    n19 --> n2
    n19 --> n20
    n20 --> n1
    n20 --> n10
    n20 --> n11
    n21 --> n4
    n21 --> n6
    n21 --> n10
    n21 --> n14
    n21 --> n17
    n21 --> n20
    n21 --> n22

    classDef tu fill:#2b2b2b,stroke:#e30028,stroke-width:2px,color:#ffffff
    classDef hdr fill:#1c1c1c,stroke:#666666,color:#dddddd
    class n0,n3,n5,n7,n9,n13,n15,n16,n19,n21 tu
    class n1,n2,n4,n6,n8,n10,n11,n12,n14,n17,n18,n20,n22 hdr
```

What the shape of that graph is meant to show:

* **`menu.cpp` has three edges** (`gui.h`, `menu.h`, `tabs.h`). The shell does not know which tabs
  exist; it calls `Tabs::Render()`.
* **`tabs.cpp` is the only node pointing at all five tabs.** Adding a tab touches `tabs.cpp`, one
  `SidebarItems[]` row in `menu.cpp` and one `Menu::TabId` enumerator — not the hook code.
* **`main.cpp` points at `exec.h` and `resources.h` but at no tab body.** It only calls
  `Executor::Init()` / `Resources::Init()`; the includes it needs are `includes.h`, `menu.h`,
  `menu_style.h` and `assets.h`.
* Only three headers include anything at all: `config.h → gui.h`, `exec.h → TextEditor.h` and
  `resources.h → {TextEditor.h, exec.h, gui.h}`. The other ten — `assets.h`, `default.h`, `demo.h`,
  `gui.h`, `includes.h`, `luamenus.h`, `menu.h`, `menu_style.h`, `tabs.h`, `TextEditor.h` — have no
  outgoing edges. Nothing defines state in a header any more, which is what used to make a second
  including `.cpp` a `LNK2005`.

Vendored code (`imgui/`, `kiero/`, `stb_image.h`, `font_awesome.*`) is left out for readability; it
has no edges into the project's own headers.

## Building

Full instructions, requirements and a troubleshooting table: **[BUILD.md](BUILD.md)**.

Quick version — Windows 10/11, Visual Studio 2019 or 2022 with the
*Desktop development with C++* workload (the legacy DirectX SDK is **not** required):

```bat
build.bat
```

or from a Developer Command Prompt:

```bat
msbuild "redENGINE UI.sln" /t:Rebuild /p:Configuration=Release /p:Platform=x64 /m
```

Output: `x64\Release\redENGINE UI.dll`. Put `rE.png` in the same folder as the DLL, otherwise
the sidebar falls back to a text logo.

A GitHub Actions workflow that does the same on `windows-latest` (Release and Debug, x64), verifies
the DLL and its export table exist, and uploads it as an artifact is written, validated and
**committed at [`ci/build.yml`](ci/build.yml) — but not yet enabled**. It cannot live in
`.github/workflows/` because creating files there needs the `workflows` permission, which the token
that produced this branch does not have; GitHub refuses the push outright. Moving it into place is
one commit from your account: run [`ci/enable.bat`](ci/enable.bat) (it copies and stages the file,
then stops so you keep the commit), or paste it in through the web UI. Full instructions in
[`ci/README.md`](ci/README.md) and [BUILD.md §7](BUILD.md).

Until then there is **no CI and no compile proof** — every check on this branch was static.

> Build **x64 only** — a Win32 DLL cannot be loaded by FiveM.

## Runtime behaviour

* **INSERT** toggles the menu.
* While the cursor is over the menu (or you are typing in the executor), mouse and keyboard
  messages are consumed instead of being passed to the game — clicking a button no longer also
  punches/shoots in-game, and typing Lua no longer walks the character around.
* Resolution changes, alt-tab and borderless↔fullscreen transitions rebuild the render target;
  a lost/reset D3D device tears the overlay down and re-initialises on the next frame.
* Dumps are written to `<DLL folder>\Dumps\` (`fxmanifest.lua` + `resources.txt`), which needs
  no elevation.

## Notes

* The vendored ImGui is **1.92.9** (upgraded from 1.75 — see the migration table in
  [BUILD.md](BUILD.md#6-imgui-175--1929-migration)). `imgui/` and `kiero/` hold unmodified
  upstream code; keep local changes in the project's own files so those trees can be re-synced
  cleanly. Note that `imgui_tables.cpp` is a separate translation unit since 1.80 and must stay
  in the project.
* The code uses **no deprecated ImGui API**: the menu shell was moved off the legacy
  `Columns()`/`NextColumn()`/`SetColumnOffset()` API onto `BeginTable`/`TableSetupColumn`. That
  makes `#define IMGUI_DISABLE_OBSOLETE_FUNCTIONS` in `imgui/imconfig.h` a clean compile, and the
  recommended way to stay ahead of the next upgrade. (`demo.h` still uses `Columns()` — it is an
  annotated cheat-sheet wired to no tab, left alone on purpose.)
* `style.CellPadding` is `(0,0)` in `menu_style.h`. That is not cosmetic: table cells are inset by
  `CellPadding` on every side, so the default `4,1` would leave the 225px sidebar with 217px of
  usable width. Keep it at zero, or subtract the padding from `Menu::SidebarWidth`.
* What is **implemented**: the full UI, the Lua editor with syntax highlighting, Load-from-file,
  the trigger clipboard list, resource dumps, and the resource-list stopper.
  What is **still a stub** (and now says so in a message box instead of doing nothing silently):
  actually executing a script, actually stopping a server resource, launching the listed menus,
  and the Safe Mode / ScriptHook Bypass toggles.
* `font_awesome.cpp` is `#include`d by `main.cpp` rather than compiled as its own translation
  unit; it is marked `ExcludedFromBuild` in the project so nobody adds it twice.
* Every menu global is declared `extern` in its header and defined in exactly one `.cpp`, so
  adding a second translation unit cannot produce `LNK2005` duplicates.
* **Shell and tabs are separate translation units.** `menu.cpp` draws the window and the sidebar and
  calls `Tabs::Render()`; `tabs.cpp` owns the `switch` and is the only file that includes the five tab
  headers. `main.cpp` (hooks, device, ImGui lifetime) includes neither. Adding a tab therefore touches
  `tabs.cpp`, `menu.cpp` (one `SidebarItems[]` row) and `menu.h` (one `TabId` enumerator) — not the
  hook code.
* `demo.h` is compiled but wired to no sidebar entry; call `Demo::Render()` from `Tabs::Render()` if
  you want the widget cheat-sheet on screen.

## Credits

Menu UI remade by xo1337 · ImGui by [Omar Cornut](https://github.com/ocornut/imgui) ·
Kiero by [Rebzzel](https://github.com/Rebzzel/kiero) · MinHook by Tsuda Kageyu ·
`stb_image` by Sean Barrett · ImGuiColorTextEdit by BalazsJako.
