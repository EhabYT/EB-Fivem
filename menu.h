#pragma once

#include "imgui/imgui.h"

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
	extern bool IsMenuOpen;   // toggled with INSERT
	extern int Tab;

	// Layout of the shell, shared with nothing else - the sidebar buttons are the
	// only consumer of SidebarWidth.
	static const float SidebarWidth        = 225.0f;
	static const float SidebarButtonHeight = 46.0f;

	// Draws the whole menu window: sidebar column + tab content column.
	// Implemented in menu.cpp.
	void Render();
}
