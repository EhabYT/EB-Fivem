#include "settings.h"
#include "assets.h"

#include "imgui/imgui.h"

#include <Windows.h>
#include <cstdio>

namespace Settings
{
	Prefs Current;

	static std::string IniPath()
	{
		return ResolveAssetPath("settings.ini");
	}

	void Load()
	{
		const std::string path = IniPath();
		Current.WindowX   = GetPrivateProfileIntA("window", "x", -1, path.c_str());
		Current.WindowY   = GetPrivateProfileIntA("window", "y", -1, path.c_str());
		Current.Tab       = GetPrivateProfileIntA("menu", "tab", 0, path.c_str());
		Current.ToggleKey = GetPrivateProfileIntA("menu", "togglekey", 0x2D, path.c_str());
		Current.Animations = GetPrivateProfileIntA("ui", "anim", 1, path.c_str()) != 0;
		Current.Toasts     = GetPrivateProfileIntA("ui", "toasts", 1, path.c_str()) != 0;
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

	static void WriteInt(const char* section, const char* key, int value)
	{
		char buf[16];
		snprintf(buf, sizeof(buf), "%d", value);
		WritePrivateProfileStringA(section, key, buf, IniPath().c_str());
	}

	void Save()
	{
		WriteInt("window", "x", Current.WindowX);
		WriteInt("window", "y", Current.WindowY);
		WriteInt("menu", "tab", Current.Tab);
		WriteInt("menu", "togglekey", Current.ToggleKey);
		WriteInt("ui", "anim", Current.Animations ? 1 : 0);
		WriteInt("ui", "toasts", Current.Toasts ? 1 : 0);
		WriteInt("ui", "whitespace", Current.ShowWhitespace ? 1 : 0);
		WriteInt("menu", "resourcetab", Current.ResourceTab);
		WriteInt("ui", "scale", Current.ScalePercent);
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

		// menu.cpp sizes the window from style.WindowMinSize, so that has to follow
		// the scale too - a 200% font inside a fixed 900x540 shell would clip.
		// Recomputed from the compiled base, so repeated calls are idempotent.
		// ApplyStyle() resets it to the 100% base, hence the call order in
		// InitImGui(): ApplyStyle() -> Load() -> ApplyScale().
		style.WindowMinSize = ImVec2(900.0f * style.FontScaleMain, 540.0f * style.FontScaleMain);
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
