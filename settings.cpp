#include "settings.h"
#include "assets.h"
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
			Toasts::Push(Toasts::Info, "No settings.ini yet - running on compiled defaults.");
		}
		Current.WindowX   = GetPrivateProfileIntA("window", "x", -1, path.c_str());
		Current.WindowY   = GetPrivateProfileIntA("window", "y", -1, path.c_str());
		Current.Tab       = GetPrivateProfileIntA("menu", "tab", 0, path.c_str());
		Current.ToggleKey = GetPrivateProfileIntA("menu", "togglekey", 0x2D, path.c_str());
		Current.Animations = GetPrivateProfileIntA("ui", "anim", 1, path.c_str()) != 0;
		Current.Toasts     = GetPrivateProfileIntA("ui", "toasts", 1, path.c_str()) != 0;
		Current.Hud        = GetPrivateProfileIntA("ui", "hud", 1, path.c_str()) != 0;
		Current.ShowWhitespace = GetPrivateProfileIntA("ui", "whitespace", 0, path.c_str()) != 0;
		Current.ResourceTab    = GetPrivateProfileIntA("menu", "resourcetab", 0, path.c_str());
		Current.ScalePercent   = GetPrivateProfileIntA("ui", "scale", 100, path.c_str());

		// A hand-edited or truncated ini must not produce a tab or key the menu
		// cannot use; clamp both back to their defaults.
		if (Current.Tab < 0 || Current.Tab > 4)
			Current.Tab = 0;
		if (Current.ToggleKey <= 0 || Current.ToggleKey > 254)
			Current.ToggleKey = 0x2D;
		if (Current.ResourceTab < 0 || Current.ResourceTab > 3)
			Current.ResourceTab = 0;
		// Below 70% the shell is unreadable, above 200% it no longer fits a 1080p
		// screen; a hand-edited ini falls back to the compiled default.
		if (Current.ScalePercent < 70 || Current.ScalePercent > 200)
			Current.ScalePercent = 100;
	}

	static bool WriteInt(const char* section, const char* key, int value)
	{
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", value);
		return WritePrivateProfileStringA(section, key, buf, IniPath().c_str()) != 0;
	}

	void Save()
	{
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
		ok = WriteInt("ui", "whitespace", Current.ShowWhitespace ? 1 : 0) && ok;
		ok = WriteInt("menu", "resourcetab", Current.ResourceTab) && ok;
		ok = WriteInt("ui", "scale", Current.ScalePercent) && ok;
		if (!ok && !g_saveFailed)
		{
			g_saveFailed = true;
			Activity::Push("Settings could not be written to " + IniPath()
				+ " - is the folder writable?");
			Toasts::Push(Toasts::Warn, "Settings could not be saved - is the DLL folder writable?");
		}
	}

	bool SaveFailed() { return g_saveFailed; }

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
