#pragma once

#include "imgui/imgui.h"
#include "TextEditor.h"
#include "font_awesome.h"

#include <string>
#include <vector>

// Known FiveM resource names offered by the Executor / Resource stopper combos.
// Defined in exec.cpp (this header used to define them, which only links while a
// single .cpp includes it).
extern std::vector<const char*> resources;
extern int selectedResource;

namespace Executor
{
	extern TextEditor editor;

	// One-time widget setup (language definition, palette, read-only state).
	// Called from InitImGui(); doing this inside Render() re-tokenised the whole
	// document on every single frame.
	void Init();

	void Render();
}
