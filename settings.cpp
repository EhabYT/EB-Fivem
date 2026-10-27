#include "settings.h"
#include "assets.h"
#include "config.h"     // UI: row 194 - ValidToggleKey() for the load clamp
#include "activity.h"
#include "toasts.h"

#include "imgui/imgui.h"

#include <Windows.h>
#include <cstdio>

namespace Settings
{
	Prefs Current;
	float DpiScale = 1.0f;
	static bool g_saveFailed = false;
	static bool g_firstStart = false;
	// UI: row 195 - true once Load() has run; Save() refuses to write before
	// that (see Save() below for why).
	static bool g_loaded = false;

	std::string IniPath()
	{
		return ResolveAssetPath("settings.ini");
	}

	void Load()
	{
		const std::string path = IniPath();
		// First start on a machine: no ini means compiled defaults, and the
		// file only appears with the first saved change.  Nothing said so -
		// a "why is there no settings.ini?" report had no answer on screen.
		if (GetFileAttributesA(path.c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			Activity::Push("No settings.ini at " + path
				+ " - compiled defaults in use; the file appears with the first saved change.");
			g_firstStart = true;
		}
		Current.WindowX   = GetPrivateProfileIntA("window", "x", -1, path.c_str());
		Current.WindowY   = GetPrivateProfileIntA("window", "y", -1, path.c_str());
		Current.Tab       = GetPrivateProfileIntA("menu", "tab", 0, path.c_str());
		Current.ToggleKey = GetPrivateProfileIntA("menu", "togglekey", 0x2D, path.c_str());
		Current.Animations = GetPrivateProfileIntA("ui", "anim", 1, path.c_str()) != 0;
		Current.Toasts     = GetPrivateProfileIntA("ui", "toasts", 1, path.c_str()) != 0;
		Current.Hud        = GetPrivateProfileIntA("ui", "hud", 1, path.c_str()) != 0;
		Current.HudCorner  = GetPrivateProfileIntA("ui", "hudcorner", 0, path.c_str());
		Current.ShowWhitespace = GetPrivateProfileIntA("ui", "whitespace", 0, path.c_str()) != 0;
		Current.ResourceTab    = GetPrivateProfileIntA("menu", "resourcetab", 0, path.c_str());
		Current.ScalePercent   = GetPrivateProfileIntA("ui", "scale", 100, path.c_str());

		// [recent] 0..4, newest first; empty slots are simply absent.  A hand-
		// edited ini cannot break anything here: unreadable paths fail at use.
		Current.Recent.clear();
		for (int i = 0; i < 5; ++i)
		{
			char buf[MAX_PATH] = { 0 };
			GetPrivateProfileStringA("recent", std::to_string(i).c_str(), "", buf, MAX_PATH, path.c_str());
			if (buf[0])
				Current.Recent.push_back(buf);
		}
		char resBuf[128] = { 0 };
		GetPrivateProfileStringA("menu", "resource", "", resBuf, sizeof(resBuf), path.c_str());
		Current.ResourceName = resBuf;

		// A hand-edited or truncated ini must not produce a tab or key the menu
		// cannot use; clamp both back to their defaults.
		if (Current.Tab < 0 || Current.Tab > 4)
			Current.Tab = 0;
		// UI: row 194 - the old range check (1..254) still let through VKs that
		// can never fire on any keyboard (mouse buttons, unassigned codes): the
		// toggle stayed dead, the menu could not open, and Config's one-time
		// reset - which needs an open menu - never ran.  A hand-edited ini
		// bricked the overlay with no way back.  Validate with the same
		// predicate Config uses; printable bindings stay allowed here because
		// the present loop guards them while text input is focused (row 164)
		// and Config retires them on first visit (row 142).
		{
			const int vk = Current.ToggleKey;
			const bool printable = (vk >= 'A' && vk <= 'Z')
				|| (vk >= '0' && vk <= '9') || vk == VK_SPACE;
			if (!printable && !Config::ValidToggleKey(vk))
			{
				Activity::Push(std::string("Stored toggle key ") + KeyName(vk)
					+ " can never fire; reset to INSERT.");
				Current.ToggleKey = 0x2D;
			}
		}
		if (Current.HudCorner < 0 || Current.HudCorner > 3)
			Current.HudCorner = 0;
		if (Current.ResourceTab < 0 || Current.ResourceTab > 3)
			Current.ResourceTab = 0;
		// Below 70% the shell is unreadable, above 200% it no longer fits a 1080p
		// screen; a hand-edited ini falls back to the compiled default.
		if (Current.ScalePercent < 70 || Current.ScalePercent > 200)
			Current.ScalePercent = 100;

		g_loaded = true;   // UI: row 195 - Save() may write from here on
	}

	static bool WriteInt(const char* section, const char* key, int value)
	{
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", value);
		return WritePrivateProfileStringA(section, key, buf, IniPath().c_str()) != 0;
	}

	static bool WriteStr(const char* section, const char* key, const char* value)
	{
		return WritePrivateProfileStringA(section, key, value, IniPath().c_str()) != 0;
	}

	void Save()
	{
		// UI: row 195 - DllMain's DETACH flushes settings unconditionally.  When
		// the hook never attached (non-D3D11 host, bind refused, gave up),
		// Load() never ran and Current still held the compiled defaults -
		// unloading then overwrote a perfectly good settings.ini with them.
		// Refuse to write before Load(): a failed session leaves the file as it
		// found it.
		if (!g_loaded)
			return;
		// A DLL folder without write permission (program files, read-only media)
		// used to fail silently here: every preference looked saved and nothing
		// survived a re-inject.  Report the first failure once, loudly.
		bool ok = true;
		ok = WriteInt("window", "x", Current.WindowX) && ok;
		ok = WriteInt("window", "y", Current.WindowY) && ok;
		ok = WriteInt("menu", "tab", Current.Tab) && ok;
		ok = WriteInt("menu", "togglekey", Current.ToggleKey) && ok;
		ok = WriteInt("ui", "anim", Current.Animations ? 1 : 0) && ok;
		ok = WriteInt("ui", "toasts", Current.Toasts ? 1 : 0) && ok;
		ok = WriteInt("ui", "hud", Current.Hud ? 1 : 0) && ok;
		ok = WriteInt("ui", "hudcorner", Current.HudCorner) && ok;
		ok = WriteInt("ui", "whitespace", Current.ShowWhitespace ? 1 : 0) && ok;
		ok = WriteInt("menu", "resourcetab", Current.ResourceTab) && ok;
		ok = WriteInt("ui", "scale", Current.ScalePercent) && ok;
		for (size_t i = 0; i < 5; ++i)
			ok = WriteStr("recent", std::to_string(i).c_str(),
				i < Current.Recent.size() ? Current.Recent[i].c_str() : "") && ok;
		ok = WriteStr("menu", "resource", Current.ResourceName.c_str()) && ok;
		if (!ok && !g_saveFailed)
		{
			g_saveFailed = true;
			Activity::Push("Settings could not be written to " + IniPath()
				+ " - is the folder writable?");
			Toasts::Push(Toasts::Warn, "Settings could not be saved - is the DLL folder writable?");
		}
		// UI: row 191 - the flag latched for the whole session: a one-off lock
		// (scanner, sync client) kept the About block reporting "settings not
		// writable" long after the next save had succeeded.  A successful write
		// retires it, named, so the report matches reality again.
		else if (ok && g_saveFailed)
		{
			g_saveFailed = false;
			Activity::Push("Settings writable again - the warning line is gone.");
		}
	}

	bool SaveFailed() { return g_saveFailed; }

	bool ConsumeFirstStartNotice()
	{
		const bool had = g_firstStart;
		g_firstStart = false;
		return had;
	}

	void ApplyScale()
	{
		if (Current.ScalePercent < 70)
			Current.ScalePercent = 70;
		if (Current.ScalePercent > 200)
			Current.ScalePercent = 200;

		// ImGui 1.92 has a first-class global scale factor: FontScaleMain
		// multiplies style.FontSizeBase, and everything derived from the font size
		// (frame height, item spacing, text, the editor) follows it.  Fonts are
		// re-baked on demand at the new size, so this is one assignment rather than
		// a restyle pass.
		ImGuiStyle& style = ImGui::GetStyle();
		style.FontScaleMain = (float)Current.ScalePercent / 100.0f;
		// Per-monitor DPI: 1.92 multiplies FontScaleMain * FontScaleDpi into every
		// font size (imgui.cpp:9215), and ScaleFactor() folds the same factor into
		// every shell metric and Settings::Px(), so the whole layout follows.
		style.FontScaleDpi = DpiScale;

		// menu.cpp sizes the window from style.WindowMinSize, so that has to follow
		// the scale too - a 200% font inside a fixed 900x540 shell would clip.
		// Recomputed from the compiled base, so repeated calls are idempotent.
		// ApplyStyle() resets it to the 100% base, hence the call order in
		// InitImGui(): ApplyStyle() -> Load() -> ApplyScale().
		style.WindowMinSize = ImVec2(900.0f * ScaleFactor(), 540.0f * ScaleFactor());

		// UI: row 185 - roundings and frame padding are absolute style values,
		// not font-derived: FontScaleMain doubles text and frame heights, but a
		// 6px radius on a doubled frame reads as a bevel and 8x6 padding
		// squeezes 40px text (the same squeeze a 144% DPI host produced at
		// 100% interface scale, since ApplyStyle's bases carry no DPI).  Rescale
		// the ApplyStyle bases with the same factor the rest of the shell uses;
		// recomputed from the compiled constants, so calls stay idempotent and
		// k == 1 reproduces ApplyStyle exactly.
		const float k = ScaleFactor();
		style.WindowRounding   = 10.0f * k;
		style.ChildRounding    = 6.0f * k;
		style.FrameRounding    = 6.0f * k;
		style.PopupRounding    = 8.0f * k;
		style.TabRounding      = 6.0f * k;
		style.ScrollbarRounding = 8.0f * k;
		style.GrabRounding     = 6.0f * k;
		style.FramePadding     = ImVec2(8.0f * k, 6.0f * k);

		// UI: row 186 - WindowPadding, ItemSpacing, IndentSpacing, ScrollbarSize
		// and GrabMinSize keep ImGui's compiled defaults; ApplyStyle never sets
		// them, so the compiled-base rescale above cannot cover them, and at
		// k != 1 they stayed at 100% size - cramped rhythm around doubled text,
		// a hairline scrollbar under a 2x frame.  Scale them by the ratio
		// between the previous and the new factor: no knowledge of the defaults
		// needed, idempotent, and a no-op whenever k does not change.
		float& lastK = StyleMetricK();
		if (k != lastK)
		{
			const float r = k / lastK;
			style.WindowPadding = ImVec2(style.WindowPadding.x * r, style.WindowPadding.y * r);
			style.ItemSpacing   = ImVec2(style.ItemSpacing.x * r, style.ItemSpacing.y * r);
			style.IndentSpacing *= r;
			style.ScrollbarSize *= r;
			style.GrabMinSize   *= r;
			lastK = k;
		}
	}

	void SaveWindowPos(float x, float y)
	{
		Current.WindowX = (int)x;
		Current.WindowY = (int)y;
		Save();
	}

	const char* KeyName(int vk)
	{
		static char buf[64];
		const UINT scan = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
		if (scan != 0 && GetKeyNameTextA((LONG)(scan << 16), buf, (int)sizeof(buf)) != 0)
			return buf;
		snprintf(buf, sizeof(buf), "VK %d", vk);
		return buf;
	}
}
