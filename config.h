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
}
