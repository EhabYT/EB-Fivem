#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "font_awesome.h"

#include <Windows.h>
#include <string>
#include <iostream>
#include <vector>

namespace Gui
{
	// Thin full-width rule used as a visual separator between menu sections.
	//
	// NOTE: BeginChild() returns false when the child is collapsed or fully
	// clipped, but EndChild() must be called for *every* BeginChild()
	// regardless of the return value - otherwise ImGui's window stack
	// unbalances and later frames assert.  The previous version ignored the
	// return value entirely, which happened to work only because a 1px child is
	// never collapsed.
	inline void Seperator(const std::string& id, float width = ImGui::GetContentRegionAvail().x, float height = 1.0f, bool visible = true)
	{
		ImGui::BeginChild(id.c_str(), ImVec2(width, height), visible);
		ImGui::EndChild();
	}

	inline void SetGetPosX(float value)
	{
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + value);
	}

	inline void SetGetPosY(float value)
	{
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + value);
	}

	inline void SetPosX(float value)
	{
		ImGui::SetCursorPosX(value);
	}

	inline void SetPosY(float value)
	{
		ImGui::SetCursorPosY(value);
	}
}
