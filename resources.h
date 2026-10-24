#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "TextEditor.h"
#include "font_awesome.h"

#include <Windows.h>
#include <string>
#include <vector>

#include "exec.h"   // resources / selectedResource are shared with the Executor tab
#include "gui.h"

// Defined in resources.cpp.
extern std::vector<const char*> Triggers;
extern std::vector<const char*> BlockedEvents;

namespace Resources
{
	extern TextEditor dumper;

	// One-time widget setup, called from InitImGui().
	void Init();

	// Dumps the selected resource list + fxmanifest to disk.
	void SaveResources();

	void Render();
}
