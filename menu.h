#pragma once

#include "imgui/imgui.h"
#include "settings.h"   // ScaleFactor() for the layout metrics below

#include <d3d11.h>

// Defined in main.cpp.  These used to be defined *in the header*, which is only
// legal while exactly one .cpp includes it - a second include anywhere would be
// a LNK2005 duplicate symbol.
extern ID3D11ShaderResourceView* logo;
extern bool LogoLoaded;

namespace Menu
{
	// Tabs are addressed by name rather than by bare int, so the sidebar in
	// menu.cpp and the routing in tabs.cpp cannot drift apart.  Menu::Tab itself
	// stays an int (defined in main.cpp) because ImGui widgets take int*.
	enum TabId
	{
		TabDefault   = 0,   // landing page
		TabExecutor  = 1,
		TabResources = 2,
		TabMenus     = 3,
		TabConfig    = 4,
	};

	// Both defined in main.cpp.
	extern bool IsMenuOpen;   // toggled with the bound key (Settings::Current.ToggleKey, INSERT by default)
	extern int Tab;

	// Layout of the shell, shared with nothing else - the sidebar buttons are the
	// only consumer of SidebarWidth.
	//
	// All three are the metrics at 100% interface scale.  The inline accessors
	// multiply by the persisted scale (settings.ini [ui] scale) so the sidebar and
	// the tab inset grow with the font instead of clipping their labels; they have
	// to be functions rather than constants because Config can change the scale
	// while the menu is open.
	static const float SidebarWidthBase        = 225.0f;
	static const float SidebarButtonHeightBase = 46.0f;
	static const float ContentPaddingBase      = 8.0f;

	inline float SidebarWidth()        { return SidebarWidthBase        * Settings::ScaleFactor(); }
	inline float SidebarButtonHeight() { return SidebarButtonHeightBase * Settings::ScaleFactor(); }

	// Left inset of the tab column.  The shell table uses NoPadOuterX and a
	// (0,0) CellPadding so the sidebar keeps exactly SidebarWidth of usable
	// width; the same flags leave the content column starting flush against the
	// column divider, which this pushes the tab bodies away from.
	inline float ContentPadding()      { return ContentPaddingBase      * Settings::ScaleFactor(); }

	// The brand accent, #5D9CD6 - the top of the light ramp in menu_style.h.
	// The sidebar's selected-tab marker, the editor line numbers (TextEditor
	// palette) and the logo fallback text all share it.
	static const ImU32 Accent              = IM_COL32(93, 156, 214, 255);

	// Draws the whole menu window: sidebar column + tab content column.
	// Implemented in menu.cpp.
	void Render();
	void RenderHud();   // FPS/frame-time plate over the scene, overlay closed
}
