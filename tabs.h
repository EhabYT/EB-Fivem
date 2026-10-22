#pragma once

// Menu::Tab (the selected tab) and the Menu::TabId enumeration live in menu.h;
// tabs.cpp includes it.  This header only declares the routing entry point, so
// that menu.cpp does not have to know which tabs exist.

namespace Tabs
{
	// Draws the body of the currently selected tab.  Owns the routing that used
	// to sit at the bottom of Menu::Render(), and is the only place that needs to
	// know which tab headers exist.
	void Render();
}
