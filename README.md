# EB-Fivem — redENGINE UI

An in-game **Dear ImGui overlay for FiveM**, delivered as a Windows x64 DLL.

`main.cpp` hooks `IDXGISwapChain::Present` (vtable index 8) with [Kiero](kiero/kiero.h) +
MinHook, subclasses the window procedure, initialises ImGui with the DX11/Win32 backends and
draws the **redENGINE** menu on top of the game. Press **INSERT** to toggle it.

Everything is vendored in-tree — no submodules, no package manager, no external downloads.

## Layout

| Path | What it is |
|---|---|
| [`main.cpp`](main.cpp) | `DllMain`, Present hook, ImGui init/teardown, texture + font loading, input routing |
| [`includes.h`](includes.h) | Windows / D3D11 / DXGI / Kiero / ImGui includes, hook typedefs, `HK_CALL` |
| [`assets.h`](assets.h) | `ResolveAssetPath()` — finds files next to the DLL |
| [`menu.h`](menu.h) | Menu shell: data-driven sidebar + tab routing |
| [`menu_style.h`](menu_style.h) | `ApplyStyle()` — colours, rounding, frame padding (applied once) |
| [`gui.h`](gui.h) | Small helpers (`Gui::Seperator`, cursor setters) |
| [`exec.h`](exec.h) · [`exec.cpp`](exec.cpp) | Executor tab — Lua editor, Execute / Load-from-file, resource combo |
| [`resources.h`](resources.h) · [`resources.cpp`](resources.cpp) | Resources tab — file browser, trigger list, resource stopper, dumps |
| [`config.h`](config.h) · [`config.cpp`](config.cpp) | Config tab — Safe Mode / ScriptHook Bypass toggles |
| [`luamenus.h`](luamenus.h) | Menus tab — free / premium menu lists |
| [`demo.h`](demo.h) | Annotated ImGui widget cheat-sheet (not wired to a tab) |
| [`default.h`](default.h) | Landing tab |
| [`TextEditor.cpp`](TextEditor.cpp) · [`TextEditor.h`](TextEditor.h) | Vendored ImGui code editor widget |
| [`imgui/`](imgui) | Dear ImGui **1.92.9** + `imgui_impl_dx11` / `imgui_impl_win32` (vendored, unmodified) |
| [`kiero/`](kiero) | Kiero 1.2.6 (D3D11 enabled) + bundled MinHook 1.3.3 sources (vendored, unmodified) |
| [`font_awesome.h`](font_awesome.h) · [`font_awesome.cpp`](font_awesome.cpp) | Font Awesome icon macros + compressed TTF blob |
| [`stb_image.h`](stb_image.h) | PNG decoding for the menu logo |
| [`rE.png`](rE.png) | Menu logo, loaded from beside the DLL at runtime |

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
* The code deliberately avoids everything behind `IMGUI_DISABLE_OBSOLETE_FUNCTIONS`, so defining
  that macro in `imgui/imconfig.h` is a supported way to stay ahead of the next upgrade.
* What is **implemented**: the full UI, the Lua editor with syntax highlighting, Load-from-file,
  the trigger clipboard list, resource dumps, and the resource-list stopper.
  What is **still a stub** (and now says so in a message box instead of doing nothing silently):
  actually executing a script, actually stopping a server resource, launching the listed menus,
  and the Safe Mode / ScriptHook Bypass toggles.
* `font_awesome.cpp` is `#include`d by `main.cpp` rather than compiled as its own translation
  unit; it is marked `ExcludedFromBuild` in the project so nobody adds it twice.
* Every menu global is now declared `extern` in its header and defined in exactly one `.cpp`, so
  adding a second translation unit that includes `menu.h` cannot produce `LNK2005` duplicates.

## Credits

Menu UI remade by xo1337 · ImGui by [Omar Cornut](https://github.com/ocornut/imgui) ·
Kiero by [Rebzzel](https://github.com/Rebzzel/kiero) · MinHook by Tsuda Kageyu ·
`stb_image` by Sean Barrett · ImGuiColorTextEdit by BalazsJako.
