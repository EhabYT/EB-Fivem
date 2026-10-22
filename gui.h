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
	//
	// ImGui 1.90 replaced BeginChild()'s third parameter (`bool border`) with
	// ImGuiChildFlags; the bool overload is gone in 1.92.  `bordered` maps to
	// ImGuiChildFlags_Borders, which is still == 1 for backward compatibility.
	inline void Seperator(const std::string& id, float width = ImGui::GetContentRegionAvail().x, float height = 1.0f, bool bordered = true)
	{
		ImGui::BeginChild(id.c_str(), ImVec2(width, height),
			bordered ? ImGuiChildFlags_Borders : ImGuiChildFlags_None);
		ImGui::EndChild();
	}

	// Section heading shared by every tab.  Four of the five opened with their
	// title at GetContentRegionAvail().x / 1.3f - a magic fraction that centres
	// nothing and aligns to nothing - while the Executor left it flush left, so
	// the five headings sat in five different places.  One helper keeps them
	// identical: title flush with the content column, full-width rule beneath.
	inline void SectionTitle(const char* title, const char* separatorId)
	{
		ImGui::Text("%s", title);
		Seperator(separatorId);
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
