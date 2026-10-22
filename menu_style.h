#pragma once

#include "imgui/imgui.h"

// Applies the redENGINE colour scheme / metrics.
//
// Called once from InitImGui() rather than every frame: the style is static, and
// re-writing ~30 ImStyleVars + colours 60-144 times a second is pure waste.
// The only value that depends on live state (MouseDrawCursor) is still updated
// every frame, from hkPresent().
inline void ApplyStyle()
{
	ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

	ImGuiStyle& style = ImGui::GetStyle();

	style.WindowMinSize = ImVec2(900, 540); // This sets the minimum window size

	style.FrameBorderSize		= 0;
	style.WindowRounding		= 0;
	style.TabRounding			= 0;
	style.ScrollbarRounding		= 0;
	style.FramePadding			= ImVec2(8, 6);

	// The menu shell is a 2-column ImGui table (see menu.h).  Table cells are
	// inset by CellPadding on every side, which would make the 225px sidebar
	// 217px of usable width and push the tab content 4px to the right.
	// Zeroing it reproduces the spacing the legacy Columns() API had.
	style.CellPadding			= ImVec2(0, 0);
	style.WindowTitleAlign		= ImVec2(0.0f, 0.5f);

	// Two ramps, six stops - the whole theme:
	//   interactive (light): #5D9CD6 accent/rest -> #3778B5 hover -> #1D5188 pressed
	//   surfaces   (navy)  : #2A4A7B raised/title -> #1F3A61 panel -> #14263F canvas
	style.Colors[ImGuiCol_TitleBg]				= ImColor(42, 74, 123, 255);		// The top bar color
	style.Colors[ImGuiCol_TitleBgActive]		= ImColor(42, 74, 123, 255);		// When you press in the imgui menu
	style.Colors[ImGuiCol_TitleBgCollapsed]		= ImColor(20, 38, 63, 50);		// When the title is collapsed

	style.Colors[ImGuiCol_WindowBg]				= ImColor(20, 38, 63, 255);		// Window Background

	style.Colors[ImGuiCol_Button]				= ImColor(31, 58, 97, 255);		// button color
	style.Colors[ImGuiCol_ButtonActive]			= ImColor(29, 81, 136);			// button color when pressed
	style.Colors[ImGuiCol_ButtonHovered]		= ImColor(55, 120, 181);			// button when hovered

	style.Colors[ImGuiCol_CheckMark]			= ImColor(93, 156, 214, 255);	// check mark color

	style.Colors[ImGuiCol_FrameBg]				= ImColor(31, 58, 97, 255);
	style.Colors[ImGuiCol_FrameBgActive]		= ImColor(42, 74, 123, 255);
	style.Colors[ImGuiCol_FrameBgHovered]		= ImColor(42, 74, 123, 255);

	style.Colors[ImGuiCol_Header]				= ImColor(31, 58, 97, 255);		// for collapsing headers , etc
	style.Colors[ImGuiCol_HeaderActive]			= ImColor(55, 120, 181);
	style.Colors[ImGuiCol_HeaderHovered]		= ImColor(42, 74, 123, 100);

	style.Colors[ImGuiCol_ResizeGrip]			= ImColor(42, 74, 123, 255);		// the resize grip thing bottom right
	style.Colors[ImGuiCol_ResizeGripActive]		= ImColor(93, 156, 214);			// when you hold it / active
	style.Colors[ImGuiCol_ResizeGripHovered]	= ImColor(55, 120, 181, 255);		// when you hover over the grip

	style.Colors[ImGuiCol_SliderGrab]			= ImColor(93, 156, 214, 255);
	style.Colors[ImGuiCol_SliderGrabActive]		= ImColor(55, 120, 181, 255);

	style.Colors[ImGuiCol_Border]				= ImColor(42, 74, 123);
	style.Colors[ImGuiCol_Separator]			= ImColor(42, 74, 123);
	style.Colors[ImGuiCol_SeparatorActive]		= ImColor(55, 120, 181);
	style.Colors[ImGuiCol_SeparatorHovered]		= ImColor(55, 120, 181);
}
