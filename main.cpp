#include "includes.h"

#include "menu.h"
#include "settings.h"          // Menu::Render / IsMenuOpen / logo
#include "menu_style.h"    // ApplyStyle()
#include "assets.h"        // ResolveAssetPath()
#include <fstream>         // row 188: the hook trace log
#include "exec.h"          // Executor::Init()
#include "config.h"         // Config::IsCapturing() (row 164)
#include "gui.h"            // UI: row 201 - Gui::ModalActive() for the DETACH wait
#include "resources.h"     // Resources::Init()

#define STB_IMAGE_IMPLEMENTATION

#include "stb_image.h"
#include "font_awesome.cpp"
#include "font_awesome.h"

extern LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// ---------------------------------------------------------------------------
// Definitions of the globals that the menu headers only declare.  Everything
// lives in this single translation unit, so no header-defined object can ever
// turn into a LNK2005 duplicate symbol if another .cpp starts including them.
// ---------------------------------------------------------------------------
Present oPresent = NULL;
HWND window = NULL;
HMODULE g_hModule = NULL;              // set in DllMain, used to resolve assets beside the DLL
WNDPROC oWndProc = NULL;
ID3D11Device* pDevice = NULL;
ID3D11DeviceContext* pContext = NULL;
ID3D11RenderTargetView* mainRenderTargetView = NULL;

ID3D11ShaderResourceView* logo = NULL;  // declared in menu.h
bool LogoLoaded = false;                // declared in menu.h

namespace Menu
{
	bool IsMenuOpen = true;
	int Tab = 0;
}

// ---------------------------------------------------------------------------
// Frame / lifetime state
// ---------------------------------------------------------------------------
static volatile bool  init = false;           // ImGui + render target are up
static volatile bool  g_unloading = false;    // DLL_PROCESS_DETACH started
static volatile bool  g_frameActive = false;  // an ImGui frame has run; read by WndProc
// UI: row 234 - teardown fence: on hosts whose window thread is not the thread
// that calls Present (GTA V pumps messages on its main thread while Present
// runs on the render thread), DETACH used to destroy the ImGui context and
// unhook Present with no way to know whether either thread was still inside
// our hooks.  Both hooks now keep an interlocked depth counter so DETACH can
// wait for in-flight calls before tearing anything down.
static volatile LONG g_presentDepth = 0;
static volatile LONG g_wndprocDepth = 0;
static volatile LONG g_hookFaulted = 0;    // UI: row 236 - overlay out of service
// UI: row 246/247 - per-process single-instance guard ("Local\\EBengin_SingleInstance_<pid>").
// Acquired in DLL_PROCESS_ATTACH before CreateThread so a duplicate mapping is
// rejected immediately with FALSE (and g_mainThreadEntered also blocks a second
// MainThread entry in the same mapping).  g_ownsSession gates DLL_PROCESS_DETACH
// so a rejected duplicate never tears down the live first instance, and
// g_mainThreadActive lets dynamic DETACH wait for MainThread before unmap.
static HANDLE g_instanceMutex = NULL;
static volatile LONG g_ownsSession = 0;
static volatile LONG g_mainThreadActive = 0;
static volatile LONG g_mainThreadEntered = 0;

static void ReleaseInstanceMutex()
{
	HANDLE m = (HANDLE)InterlockedExchangePointer((PVOID*)&g_instanceMutex, NULL);
	if (m != NULL)
		CloseHandle(m);
}

// UI: row 236 - forward: LogHookLine is defined below the hooks, and both
// fault paths need it.
static void LogHookLine(const char* text);

// UI: row 237 - the one degrade path shared by the SEH guard (row 236) and the
// C++ exception containment below: hand the window proc back so no message can
// reach the misfired state, stop re-entering the frame body, log the outcome.
// Raw Win32/atomics only - freeing half-dead ImGui or CRT state from a fault
// path is how a recoverable frame becomes a dead process.
static void FaultDisable(const char* reason)
{
	g_hookFaulted = 1;
	init = false;
	g_frameActive = false;
	if (window != NULL && oWndProc != NULL)
	{
		SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
		oWndProc = NULL;
	}
	LogHookLine(reason);
}
static UINT  g_rtWidth = 0;
static UINT  g_rtHeight = 0;


bool LoadTextureFromFile(const char* filename, ID3D11ShaderResourceView** out_srv, int* out_width, int* out_height)
{
	if (filename == NULL || out_srv == NULL || pDevice == NULL)
		return false;

	// Load from disk into a raw RGBA buffer
	int image_width = 0;
	int image_height = 0;
	unsigned char* image_data = stbi_load(filename, &image_width, &image_height, NULL, 4);
	if (image_data == NULL || image_width <= 0 || image_height <= 0)
	{
		if (image_data != NULL)
			stbi_image_free(image_data);
		return false;
	}

	// Create texture
	D3D11_TEXTURE2D_DESC desc;
	ZeroMemory(&desc, sizeof(desc));
	desc.Width = image_width;
	desc.Height = image_height;
	desc.MipLevels = 1;
	desc.ArraySize = 1;
	desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	desc.SampleDesc.Count = 1;
	desc.SampleDesc.Quality = 0;
	desc.Usage = D3D11_USAGE_DEFAULT;
	desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
	desc.CPUAccessFlags = 0;

	ID3D11Texture2D* pTexture = NULL;
	D3D11_SUBRESOURCE_DATA subResource;
	ZeroMemory(&subResource, sizeof(subResource));
	subResource.pSysMem = image_data;
	subResource.SysMemPitch = desc.Width * 4;
	subResource.SysMemSlicePitch = 0;

	HRESULT hr = pDevice->CreateTexture2D(&desc, &subResource, &pTexture);
	if (FAILED(hr) || pTexture == NULL)
	{
		stbi_image_free(image_data);
		return false;
	}

	// Create texture view
	D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc;
	ZeroMemory(&srvDesc, sizeof(srvDesc));
	srvDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	srvDesc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
	srvDesc.Texture2D.MipLevels = desc.MipLevels;
	srvDesc.Texture2D.MostDetailedMip = 0;
	hr = pDevice->CreateShaderResourceView(pTexture, &srvDesc, out_srv);
	pTexture->Release();
	stbi_image_free(image_data);

	if (FAILED(hr))
	{
		*out_srv = NULL;
		return false;
	}

	if (out_width != NULL)
		*out_width = image_width;
	if (out_height != NULL)
		*out_height = image_height;

	return true;
}


// Directory this DLL was loaded from, without a trailing backslash.
static std::string GetModuleDirectory()
{
	char path[MAX_PATH] = { 0 };
	// GetModuleFileNameA signals truncation by returning exactly MAX_PATH.  A
	// truncated path would resolve assets against the wrong directory, so treat
	// it the same as failure and fall back to the working directory.
	const DWORD len = (g_hModule != NULL) ? GetModuleFileNameA(g_hModule, path, MAX_PATH) : 0;
	if (len != 0 && len < MAX_PATH)
	{
		char* last = strrchr(path, '\\');
		if (last != NULL)
		{
			*last = '\0';
			return std::string(path);
		}
	}
	return std::string(".");
}

// Build "<dir of this DLL>\<file>", falling back to the bare relative file name
// so the asset can also just sit in the process working directory.
std::string ResolveAssetPath(const char* file)
{
	return GetModuleDirectory() + "\\" + file;
}


// UI: row 249 - release the render target view before returning from Present so
// zero references to swap-chain buffer 0 (neither ID3D11RenderTargetView nor
// ID3D11Texture2D, nor an OMSetRenderTargets binding on pContext) survive
// between frames.  Because this DLL hooks Present only and not ResizeBuffers,
// holding any buffer-0 reference across frames makes the host's own
// IDXGISwapChain::ResizeBuffers fail with DXGI_ERROR_INVALID_CALL (0x887A0001).
static void CleanupRenderTarget()
{
	if (mainRenderTargetView != NULL)
	{
		mainRenderTargetView->Release();
		mainRenderTargetView = NULL;
	}
}

// Acquire a render target view for the current frame's swap-chain buffer 0 and
// release the temporary ID3D11Texture2D reference immediately.
static bool CreateRenderTarget(IDXGISwapChain* pSwapChain)
{
	if (pSwapChain == NULL || pDevice == NULL)
		return false;

	CleanupRenderTarget();

	ID3D11Texture2D* pBackBuffer = NULL;
	if (FAILED(pSwapChain->GetBuffer(0, __uuidof(ID3D11Texture2D), (LPVOID*)&pBackBuffer)) || pBackBuffer == NULL)
		return false;

	D3D11_TEXTURE2D_DESC bbDesc;
	ZeroMemory(&bbDesc, sizeof(bbDesc));
	pBackBuffer->GetDesc(&bbDesc);

	HRESULT hr = pDevice->CreateRenderTargetView(pBackBuffer, NULL, &mainRenderTargetView);
	pBackBuffer->Release();
	if (FAILED(hr) || mainRenderTargetView == NULL)
	{
		mainRenderTargetView = NULL;
		return false;
	}

	g_rtWidth = bbDesc.Width;
	g_rtHeight = bbDesc.Height;
	return true;
}


void InitImGui()
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// UI: row 200 - without this, ImGui's default context auto-saves its own
	// window state to imgui.ini in the HOST process' working directory: a
	// stray file in the game folder (silently failing on write-protected
	// installs) duplicating what settings.ini next to the DLL already
	// persists deliberately.
	io.IniFilename = NULL;
	static const ImWchar icons_ranges[] = { 0xf000, 0xf3ff, 0 }; //ICON_MIN_FA , ICON_MAX_FA
	ImFontConfig icons_config;

	icons_config.MergeMode = true;
	icons_config.PixelSnapH = true;
	// Since 1.90 these are ImS8 with 0 == auto, and they are ignored entirely
	// when PixelSnapH is set.  (Upstream 1.75 had them as int, and the original
	// code assigned 2.5 - a narrowing error under /permissive-.)
	icons_config.OversampleH = 0;
	icons_config.OversampleV = 0;

	io.ConfigFlags |= ImGuiConfigFlags_NoMouseCursorChange;

	// Keyboard navigation: the menu is operable without a mouse at all
	// (Tab/arrows move the focus, Enter/Space activates).  Every widget takes
	// part natively, including the Gui::PrimaryButton / Gui::Toggle widgets
	// built on InvisibleButton.
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	// Fall back to the built-in default font instead of asserting when the
	// Windows font is unavailable (older Windows / stripped font sets).
	ImFont* mainFont = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\Bahnschrift.ttf", 20.0f);
	if (mainFont == NULL)
		mainFont = io.Fonts->AddFontDefault();
	if (io.Fonts->AddFontFromMemoryCompressedTTF(font_awesome_data, font_awesome_size, 19.0f, &icons_config, icons_ranges) == NULL)
		io.Fonts->AddFontDefault();

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(pDevice, pContext);

	// Ship EBengin.png next to the DLL (or in the working directory).  A missing logo
	// must not take the whole host process down, so no IM_ASSERT here.
	int LogoWidth = 16;
	int LogoHeight = 16;
	LogoLoaded = LoadTextureFromFile(ResolveAssetPath("EBengin.png").c_str(), &logo, &LogoWidth, &LogoHeight);

	// Style is static: apply it once here instead of on every single frame.
	ApplyStyle();

	// Interface preferences (window position, last tab, toggle key) live next to
	// the DLL so a re-inject restores the previous session instead of resetting
	// the window to the top-left and the tab to the landing page.
	// Host window contents scale, before ApplyScale() reads it: 96 DPI on an
	// unaware host means every factor stays the identity; a per-monitor-aware
	// host sizes the window in physical pixels, where the compiled layout would
	// otherwise render small.
	const UINT dpi = GetDpiForWindow(window);
	Settings::DpiScale = dpi > 0 ? (float)dpi / 96.0f : 1.0f;
	Settings::Load();
	// Font/window scale: must follow Load() (it reads the persisted value) and
	// ApplyStyle() above (which resets WindowMinSize to the 100% base).
	Settings::ApplyScale();
	Menu::Tab = Settings::Current.Tab;

	// One-time widget setup for the tabs that own a TextEditor.
	Executor::Init();
	Resources::Init();
}


// Tear everything down in the right order.  Safe to call more than once.
void CleanupImGui()
{
	if (!init)
		return;
	init = false;
	g_frameActive = false;

	// UI: row 234 - give the window proc back to the game first: a message that
	// enters WndProc after DestroyContext() would call ImGui_ImplWin32_
	// WndProcHandler (and read GetIO()) on a dead context - an access violation
	// on any host whose window thread differs from the render thread.
	if (window != NULL && oWndProc != NULL)
	{
		SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)oWndProc);
		oWndProc = NULL;
	}
	window = NULL;

	if (ImGui::GetCurrentContext() != NULL)
	{
		ImGui_ImplDX11_Shutdown();
		ImGui_ImplWin32_Shutdown();
		ImGui::DestroyContext();
	}

	CleanupRenderTarget();
	g_rtWidth = 0;
	g_rtHeight = 0;

	if (logo != NULL)
	{
		logo->Release();
		logo = NULL;
		LogoLoaded = false;
	}
	// UI: row 248 - release the cached device/context references here too so a
	// dynamic unload (DLL_PROCESS_DETACH) drops them just like the device-reset
	// path below.
	if (pContext != NULL)
	{
		pContext->Release();
		pContext = NULL;
	}
	if (pDevice != NULL)
	{
		pDevice->Release();
		pDevice = NULL;
	}
}


// UI: row 234 - body/wrapper split: the wrapper keeps the teardown fence count
// so DETACH can wait for in-flight messages before the context they read dies.
static LRESULT WndProcFrame(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (g_unloading || g_hookFaulted || !init)
		return (oWndProc != NULL) ? CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam)
								  : DefWindowProc(hWnd, uMsg, wParam, lParam);

	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
		return 0;

	// Feed ImGui first, then decide whether the game may also see the message.
	// Previously every message was passed straight through, so clicking a menu
	// button also punched/shoot/fired in-game and typing Lua in the executor
	// walked the character around.
	if (g_frameActive && ImGui::GetCurrentContext() != NULL)
	{
		ImGuiIO& io = ImGui::GetIO();

		const bool overImGui = io.WantCaptureMouse
			|| ImGui::IsWindowHovered(ImGuiHoveredFlags_AnyWindow | ImGuiHoveredFlags_ChildWindows)
			|| ImGui::IsAnyItemActive()
			|| ImGui::IsAnyItemHovered();

		switch (uMsg)
		{
		case WM_LBUTTONDOWN: case WM_LBUTTONUP: case WM_LBUTTONDBLCLK:
		case WM_RBUTTONDOWN: case WM_RBUTTONUP: case WM_RBUTTONDBLCLK:
		case WM_MBUTTONDOWN: case WM_MBUTTONUP: case WM_MBUTTONDBLCLK:
		case WM_XBUTTONDOWN: case WM_XBUTTONUP:
		case WM_MOUSEWHEEL: case WM_MOUSEHWHEEL:
		case WM_MOUSEMOVE:
			if (overImGui)
				return 0;
			break;

		default:
			// Anything ImGui is typing into (executor, inputs) must not reach the game.
			if (io.WantTextInput || io.WantCaptureKeyboard)
			{
				if ((uMsg >= WM_KEYFIRST && uMsg <= WM_KEYLAST) ||
					(uMsg >= WM_IME_STARTCOMPOSITION && uMsg <= WM_IME_KEYLAST))
					return 0;
			}
			break;
		}
	}

	return (oWndProc != NULL) ? CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam)
							  : DefWindowProc(hWnd, uMsg, wParam, lParam);
}


// UI: row 237 - the same containment on the window thread: an exception inside
// ImGui's message handler (the editor's input paths) used to terminate the
// process here, again with no trace anywhere.
static LRESULT WndProcBody(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	try
	{
		return WndProcFrame(hWnd, uMsg, wParam, lParam);
	}
	catch (...)
	{
		FaultDisable("window proc threw - overlay disabled, game continues");
		return 0;
	}
}


LRESULT CALLBACK WndProc(const HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (g_unloading || g_hookFaulted || !init)
		return (oWndProc != NULL) ? CallWindowProc(oWndProc, hWnd, uMsg, wParam, lParam)
								  : DefWindowProc(hWnd, uMsg, wParam, lParam);

	InterlockedIncrement(&g_wndprocDepth);
	const LRESULT r = WndProcBody(hWnd, uMsg, wParam, lParam);
	InterlockedDecrement(&g_wndprocDepth);
	return r;
}


// UI: row 234 - same split as WndProc: the hooked Present keeps a depth count
// so DETACH never restores hook bytes or frees the context under a live frame.
static HRESULT PresentFrame(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (g_unloading || pSwapChain == NULL)
		return oPresent(pSwapChain, SyncInterval, Flags);

	if (!init)
	{
		if (pContext != NULL) { pContext->Release(); pContext = NULL; }
		if (pDevice != NULL) { pDevice->Release(); pDevice = NULL; }
		if (FAILED(pSwapChain->GetDevice(__uuidof(ID3D11Device), (void**)&pDevice)) || pDevice == NULL)
			return oPresent(pSwapChain, SyncInterval, Flags);

		pDevice->GetImmediateContext(&pContext);
		if (pContext == NULL)
		{
			pDevice->Release();
			pDevice = NULL;
			return oPresent(pSwapChain, SyncInterval, Flags);
		}

		DXGI_SWAP_CHAIN_DESC sd;
		if (FAILED(pSwapChain->GetDesc(&sd)) || sd.OutputWindow == NULL)
		{
			pContext->Release();
			pContext = NULL;
			pDevice->Release();
			pDevice = NULL;
			return oPresent(pSwapChain, SyncInterval, Flags);
		}
		window = sd.OutputWindow;

		if (!CreateRenderTarget(pSwapChain))
		{
			window = NULL;
			pContext->Release();
			pContext = NULL;
			pDevice->Release();
			pDevice = NULL;
			return oPresent(pSwapChain, SyncInterval, Flags);
		}

		// UI: row 248 - finish InitImGui() (CreateContext, font bake, Win32/DX11
		// backend init, settings load) BEFORE installing WndProc: on a host whose
		// window thread is not the render thread (GTA V / FiveM), subclassing
		// first let the window thread enter WndProc -> ImGui_ImplWin32_WndProcHandler
		// while InitImGui() was still constructing the context on the render thread.
		InitImGui();
		init = true;
		if (oWndProc == NULL)
			oWndProc = (WNDPROC)SetWindowLongPtr(window, GWLP_WNDPROC, (LONG_PTR)WndProc);
	}
	else
	{
		// UI: row 249 - CreateRenderTarget acquires a fresh view of buffer 0 for
		// this frame and CleanupRenderTarget releases it before oPresent, so
		// same-size and different-size ResizeBuffers both work without holding
		// any buffer-0 reference between frames.  When the dimensions change,
		// also cycle ImGui's DX11 device objects.
		DXGI_SWAP_CHAIN_DESC sd;
		const bool resized = SUCCEEDED(pSwapChain->GetDesc(&sd)) &&
			(sd.BufferDesc.Width != g_rtWidth || sd.BufferDesc.Height != g_rtHeight);
		if (resized)
		{
			ImGui_ImplDX11_InvalidateDeviceObjects();
			ImGui_ImplDX11_CreateDeviceObjects();
		}
		if (!CreateRenderTarget(pSwapChain))
			return oPresent(pSwapChain, SyncInterval, Flags);
	}

	// UI: row 187 - DPI was sampled once at init: dragging the host window to
	// a monitor with a different DPI left fonts and every DPI-folded metric
	// at the old factor until re-inject.  Re-sample per frame (one cheap
	// call) before NewFrame and re-run ApplyScale only when the factor
	// actually changed; the font rebake, the WindowMinSize change and the
	// row-186 metric ratio then ride the existing scale-change paths, and the
	// shell's reachability re-clamp keys off WindowMinSize anyway.
	if (window != NULL)
	{
		const UINT dpiNow = GetDpiForWindow(window);
		const float dpiF = dpiNow > 0 ? (float)dpiNow / 96.0f : 1.0f;
		if (dpiF != Settings::DpiScale)
		{
			Settings::DpiScale = dpiF;
			Settings::ApplyScale();
		}
	}

	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	g_frameActive = true;

	// Draw our own cursor while the menu is up (the backend's cursor changes are
	// disabled via ImGuiConfigFlags_NoMouseCursorChange).  This used to live in
	// ApplyStyle(), which is now called once at init instead of every frame.
	ImGui::GetIO().MouseDrawCursor = Menu::IsMenuOpen;

	// Edge-triggered toggle.  The key is user-configurable (Config tab) and
	// persisted, so read it from settings instead of assuming INSERT.
	//
	// The capture button accepts letters and digits, and a letter bound as
	// toggle key used to flip the menu on every keystroke while the user typed
	// it into the editor or a filter box.  Printable bindings are therefore
	// ignored while a text widget is actually consuming input
	// (io.WantTextInput); non-printable ones (INSERT, F-keys, ...) still close
	// the menu with the editor focused, as before.  WantCaptureKeyboard would
	// be the wrong test here: it is also true whenever keyboard navigation is
	// active, which would kill printable bindings outright.
	const int toggleVk = Settings::Current.ToggleKey;
	const bool printable = (toggleVk >= 'A' && toggleVk <= 'Z')
		|| (toggleVk >= '0' && toggleVk <= '9') || toggleVk == VK_SPACE;
	// UI: row 164 - while the Config capture is armed, every key press is a
	// binding candidate; flipping the overlay shut on the toggle key would
	// swallow the capture mid-press.  The poll still reads (and so drains)
	// the edge, keeping capture and toggle from both acting on one press.
	if (toggleVk > 0 && (!printable || !ImGui::GetIO().WantTextInput)
		&& !Config::IsCapturing()
		&& (GetAsyncKeyState(toggleVk) & 1))
		Menu::IsMenuOpen = !Menu::IsMenuOpen;

	if (Menu::IsMenuOpen)
		Menu::Render();
	else if (Settings::Current.Hud)
		Menu::RenderHud();

	ImGui::Render();

	// UI: row 249 - ImGui_ImplDX11_RenderDrawData backs up and restores shaders,
	// states, viewports and buffers, but NOT the Output-Merger render target.
	// Leaving mainRenderTargetView bound on pContext (and alive in a global)
	// clobbered the host's bound RTV/DSV and kept an outstanding reference to
	// swap-chain buffer 0 across Present, causing host ResizeBuffers calls to
	// fail with DXGI_ERROR_INVALID_CALL.  Save/restore the host's OM target and
	// release mainRenderTargetView before handing the frame to oPresent.
	ID3D11RenderTargetView* oldRTV = NULL;
	ID3D11DepthStencilView* oldDSV = NULL;
	pContext->OMGetRenderTargets(1, &oldRTV, &oldDSV);
	pContext->OMSetRenderTargets(1, &mainRenderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	pContext->OMSetRenderTargets(1, &oldRTV, oldDSV);
	if (oldRTV != NULL)
		oldRTV->Release();
	if (oldDSV != NULL)
		oldDSV->Release();
	CleanupRenderTarget();

	HRESULT hr = oPresent(pSwapChain, SyncInterval, Flags);

	// Lost/reset device (driver crash, alt-tab on exclusive fullscreen, ...).
	// Drop our D3D references and rebuild them on the next frame rather than
	// drawing into a dead swap chain every frame.
	if (hr == DXGI_ERROR_DEVICE_REMOVED || hr == DXGI_ERROR_DEVICE_RESET)
	{
		CleanupImGui();
		if (pContext != NULL) { pContext->Release(); pContext = NULL; }
		if (pDevice != NULL) { pDevice->Release(); pDevice = NULL; }
	}

	return hr;
}


// UI: row 237 - /EHsc (row 236) unwinds C++ exceptions, but nothing anywhere
// caught them: a regex_error from the editor's token patterns, an out_of_range
// or bad_alloc from any std:: call ran out of handlers into std::terminate -
// process death with no log line, indistinguishable from a game crash.  Contain
// it exactly like the SEH guard: one faulty frame costs the overlay, not the
// session.  (The SEH guard above catches access violations; under /EHsc a
// catch(...) does not, hence both.)
static HRESULT PresentBody(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	try
	{
		return PresentFrame(pSwapChain, SyncInterval, Flags);
	}
	catch (...)
	{
		FaultDisable("hook threw inside a frame - overlay disabled, game continues");
		return S_OK;
	}
}


// UI: row 236 - crash guard: an access violation inside our own frame code used
// to take the host process down with us.  Catch it, put the overlay out of
// service and let every later frame pass straight to the game: one faulty
// frame costs the menu, not the session.  The wrapper holds no C++ objects, so
// __try is legal here; the handler touches raw Win32/atomics only (never ImGui -
// its state is exactly what misfired) and writes a line to EBengin.log like
// every other hook outcome.
HRESULT HK_CALL hkPresent(IDXGISwapChain* pSwapChain, UINT SyncInterval, UINT Flags)
{
	if (g_unloading || g_hookFaulted || pSwapChain == NULL)
		return oPresent(pSwapChain, SyncInterval, Flags);

	// UI: row 246 - re-entrancy guard: if Present is ever re-entered while a
	// frame is already inside PresentBody (stacked hook trampoline or recursive
	// swap-chain call), pass straight through to oPresent instead of calling
	// ImGui::NewFrame() inside an open frame.
	if (InterlockedIncrement(&g_presentDepth) > 1)
	{
		InterlockedDecrement(&g_presentDepth);
		return oPresent(pSwapChain, SyncInterval, Flags);
	}
	HRESULT hr;
	__try
	{
		hr = PresentBody(pSwapChain, SyncInterval, Flags);
	}
	__except (EXCEPTION_EXECUTE_HANDLER)
	{
		// UI: row 243 - guide 6.3: a fault line without the exception code and
		// faulting address cannot be triaged from EBengin.log alone.  Both come
		// from GetExceptionInformation(), valid only inside the handler; the reads
		// stay raw Win32/POD like everything else on this path (no C++ objects in
		// hkPresent, so __try stays legal).
		DWORD code = 0;
		void* addr = NULL;
		PEXCEPTION_POINTERS xps = GetExceptionInformation();
		if (xps != NULL && xps->ExceptionRecord != NULL)
		{
			code = xps->ExceptionRecord->ExceptionCode;
			addr = xps->ExceptionRecord->ExceptionAddress;
		}
		char detail[160];
		snprintf(detail, sizeof(detail),
			"hook faulted inside a frame (code 0x%08lX at 0x%016llX) - overlay disabled, game continues",
			(unsigned long)code, (unsigned long long)(uintptr_t)addr);
		FaultDisable(detail);
		hr = S_OK;   // this frame's draw is lost; the next Present passes through
	}
	InterlockedDecrement(&g_presentDepth);
	return hr;
}


// UI: row 188 - a failed hook attach used to be a silent no-op: the inject
// looked like "did nothing", with no trace anywhere to tell "not a D3D11
// host" from "bind refused" from "gave up".  One line per outcome next to
// the DLL, the same folder convention settings.ini uses, turns the silent
// case into a diagnosable one.  UI: row 189 - the file survives injects, so
// cap it: past 64 KB start over instead of appending forever.
static void LogHookLine(const char* text)
{
	const std::string path = ResolveAssetPath("EBengin.log");
	WIN32_FILE_ATTRIBUTE_DATA attr = {};
	if (GetFileAttributesExA(path.c_str(), GetFileExInfoStandard, &attr) != 0
		&& attr.nFileSizeHigh == 0 && attr.nFileSizeLow > 64u * 1024u)
		DeleteFileA(path.c_str());
	std::ofstream log(path.c_str(), std::ios::app);
	if (!log.is_open())
		return;
	SYSTEMTIME t;
	GetLocalTime(&t);
	// UI: row 246 - include pid, tid and module base on every line so two
	// attach entries immediately separate double-injection (same pid) from a
	// crash+restart (new pid), and so any row-243 fault address converts to an
	// RVA (addr - base) under /DYNAMICBASE (row 236).
	char stamp[96];
	snprintf(stamp, sizeof(stamp),
		"%04u-%02u-%02u %02u:%02u:%02u [pid=%lu tid=%lu base=0x%016llX]",
		(unsigned)t.wYear, (unsigned)t.wMonth, (unsigned)t.wDay,
		(unsigned)t.wHour, (unsigned)t.wMinute, (unsigned)t.wSecond,
		(unsigned long)GetCurrentProcessId(),
		(unsigned long)GetCurrentThreadId(),
		(unsigned long long)(uintptr_t)g_hModule);
	log << stamp << " " << text << "\n";
}

DWORD WINAPI MainThread(LPVOID lpReserved)
{
	// UI: row 250 - silence C4100 / -Wunused-parameter on lpReserved.
	(void)lpReserved;
	// UI: row 247 - guard against a second MainThread entry in the same module
	// (while DLL_PROCESS_ATTACH below guards against a second module mapping).
	if (InterlockedCompareExchange(&g_mainThreadEntered, 1, 0) != 0)
	{
		LogHookLine("hook skipped: instance already active in this process");
		return FALSE;
	}

	// The old version spun in a tight `do {} while (!init_hook)` loop with no
	// sleep and no exit condition, burning a core forever if the hook could
	// never attach.  Retry on a timer and give up on a hard "not supported".
	const DWORD kMaxAttempts = 120;   // ~60 s at 500 ms per attempt
	for (DWORD attempt = 0; attempt < kMaxAttempts && !g_unloading; ++attempt)
	{
		kiero::Status::Enum status = kiero::init(kiero::RenderType::D3D11);
		if (status == kiero::Status::Success)
		{
			if (kiero::bind(8, (void**)&oPresent, (void*)hkPresent) == kiero::Status::Success)
			{
				LogHookLine("hook attached (D3D11 present)");
				InterlockedExchange(&g_mainThreadActive, 0);
				return TRUE;
			}

			kiero::shutdown();
			LogHookLine("hook failed: present bind refused");
			g_ownsSession = 0;
			g_mainThreadEntered = 0;
			ReleaseInstanceMutex();
			InterlockedExchange(&g_mainThreadActive, 0);
			return FALSE;   // logged outcome; the gave-up line below stays for retries only
		}

		if (status == kiero::Status::NotSupportedError ||
			status == kiero::Status::ModuleNotFoundError)
		{
			// not a D3D11 renderer / no swap chain: retrying cannot help
			LogHookLine(status == kiero::Status::NotSupportedError
				? "hook failed: host is not a D3D11 renderer"
				: "hook failed: swap chain module not found");
			g_ownsSession = 0;
			g_mainThreadEntered = 0;
			ReleaseInstanceMutex();
			InterlockedExchange(&g_mainThreadActive, 0);
			return FALSE;   // logged outcome; the gave-up line below stays for retries only
		}

		// UI: row 247 - sleep in 50 ms slices checking !g_unloading so a dynamic
		// unload wakes MainThread within <=50 ms instead of waiting out 500 ms.
		for (int s = 0; s < 10 && !g_unloading; ++s)
			Sleep(50);
	}
	// UI: row 190 - the loop also ends when an unload arrives mid-retry; that
	// is not a failure, and logging "gave up" for it would put a lie in the
	// trace.  The gave-up line belongs to a full 120-attempt exhaustion only.
	if (g_unloading)
	{
		InterlockedExchange(&g_mainThreadActive, 0);
		return FALSE;
	}
	LogHookLine("hook failed: attach gave up after 120 attempts");
	g_ownsSession = 0;
	g_mainThreadEntered = 0;
	ReleaseInstanceMutex();
	InterlockedExchange(&g_mainThreadActive, 0);
	return FALSE;
}


BOOL WINAPI DllMain(HMODULE hMod, DWORD dwReason, LPVOID lpReserved)
{
	switch (dwReason)
	{
	case DLL_PROCESS_ATTACH:
	{
		// UI: row 247 - acquire the per-process single-instance mutex right in
		// DLL_PROCESS_ATTACH (guide 4.1) before spawning MainThread: a duplicate
		// injection is rejected immediately with FALSE (so the loader unmaps the
		// second copy without creating a thread), and g_ownsSession/g_mainThreadActive
		// are set before CreateThread so an immediate DETACH cannot race MainThread
		// startup.  Also closes the thread HANDLE that used to leak on every attach.
		char mutexName[64];
		wsprintfA(mutexName, "Local\\EBengin_SingleInstance_%lu",
			(unsigned long)GetCurrentProcessId());
		HANDLE hMutex = CreateMutexA(nullptr, FALSE, mutexName);
		if (hMutex != nullptr && GetLastError() == ERROR_ALREADY_EXISTS)
		{
			CloseHandle(hMutex);
			if (g_hModule == NULL)
				g_hModule = hMod;
			OutputDebugStringA("EBengin: already injected, aborting second instance\n");
			LogHookLine("hook skipped: instance already active in this process");
			return FALSE;
		}
		g_hModule = hMod;
		g_instanceMutex = hMutex;
		g_ownsSession = 1;
		g_mainThreadActive = 1;
		DisableThreadLibraryCalls(hMod);
		HANDLE hThread = CreateThread(nullptr, 0, MainThread, hMod, 0, nullptr);
		if (hThread == nullptr)
		{
			g_mainThreadActive = 0;
			g_ownsSession = 0;
			ReleaseInstanceMutex();
			g_hModule = NULL;
			return FALSE;
		}
		CloseHandle(hThread);
		break;
	}

	case DLL_PROCESS_DETACH:
		// UI: row 246 - if this mapping was a rejected duplicate (or never started
		// a session), do not touch settings, ImGui, kiero or the trace log.
		if (!g_ownsSession)
			break;
		// Stop rendering before anything is torn down, otherwise the game can be
		// sitting inside hkPresent while we destroy the ImGui context under it.
		g_unloading = true;
		// UI: row 247 - when lpReserved != nullptr, the host process is exiting:
		// Windows has already terminated other threads (so depth counters could
		// wait on dead threads, and d3d11.dll/dxgi.dll/heap locks may be held by
		// terminated threads).  Skip fence waits, D3D11/ImGui COM releases and
		// vtable patching during process termination; only flush settings, release
		// the mutex and log the exit.  Full teardown runs on dynamic unload
		// (FreeLibrary, lpReserved == nullptr).
		if (lpReserved != nullptr)
		{
			Settings::Save();
			ReleaseInstanceMutex();
			g_ownsSession = 0;
			LogHookLine("session ended (process exit)");
			break;
		}
		// Wait for MainThread if detach arrived while the attach loop was still running.
		for (int i = 0; i < 200 && g_mainThreadActive > 0; ++i)
			Sleep(10);
		// UI: row 201 - a modal dialog (Gui::ModalBoxA) blocks the render thread
		// inside a frame for as long as the user leaves it open, and the fixed
		// 50 ms below would then destroy the ImGui context under the live
		// dialog: once it closed, the rest of the frame drew with no context.
		// Wait for the dialog, capped at ~2 s so DETACH cannot stall the loader
		// lock indefinitely - past the cap we proceed exactly as before.
		for (int i = 0; i < 80 && Gui::ModalActive(); ++i)
			Sleep(25);
		Sleep(50);
		// UI: row 234 - the render thread and the window thread can both be inside
		// our hooks here; the old fixed sleep was not a fence.  Wait for both
		// depth counters, capped at ~2 s so DETACH cannot stall the loader lock
		// indefinitely - past the cap we proceed exactly as before.
		for (int i = 0; i < 200 && (g_presentDepth > 0 || g_wndprocDepth > 0); ++i)
			Sleep(10);
		// The position save in Menu::Render() is throttled to ~1/s; flush the
		// final window position / tab / toggle key so the next start restores
		// exactly where this session ended.
		Settings::Save();
		CleanupImGui();
		kiero::shutdown();
		ReleaseInstanceMutex();
		g_ownsSession = 0;
		g_mainThreadEntered = 0;
		// UI: row 193 - the trace log (row 188) records the attach and every
		// failure outcome, but never a clean end: a report could not tell an
		// orderly unload from a crash or a task kill.  One line closes the
		// session - present in the log only when DETACH actually ran.
		LogHookLine("session ended (clean unload)");
		break;
	}
	return TRUE;
}
