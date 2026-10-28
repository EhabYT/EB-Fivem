#pragma once

#include "imgui/imgui_internal.h"
#include "imgui/imgui.h"

#include <Windows.h>
#include <vector>
#include <iostream>
#include "font_awesome.h"

#include "gui.h"

namespace Config
{
	// Defined in config.cpp.  NOTE: these two are UI state only - nothing in this
	// build reads them and they are not persisted anywhere.  The Config tab says
	// so out loud: both headings are greyed (TextDisabled), both switches read
	// "Enabled (no effect)", the tooltips describe them as inert, and every flip
	// writes an activity entry plus a warn toast instead of implying two working
	// modes.
	extern bool Safemode;
	extern bool ShBypass;

	void Render();
	// UI: row 194 - the toggle-key predicate, exported so Settings::Load()
	// can reject ini keys that can never fire before the menu (and this tab)
	// is reachable at all.
	bool ValidToggleKey(int vk);
	// UI: row 164 - true while the toggle-key capture is armed; the present
	// loop suppresses the toggle flip while it is.
	bool IsCapturing();
}
