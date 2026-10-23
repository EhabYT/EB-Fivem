#pragma once

#include "imgui/imgui.h"

// Applies the EBengin colour scheme / metrics.
//
// Called once from InitImGui() rather than every frame: the style is static, and
// re-writing ~30 ImStyleVars + colours 60-144 times a second is pure waste.
// The only value that depends on live state (MouseDrawCursor) is still updated
// every frame, from hkPresent().
inline void ApplyStyle()
{
	ImGui::SetMouseCursor(ImGuiMouseCursor_Arrow);

	ImGuiStyle& style = ImGui::GetStyle();

	// Base shell size at 100% interface scale.  Settings::ApplyScale() rescales
	// WindowMinSize from the same base, and InitImGui() runs it *after* this, so
	// the persisted [ui] scale is not overwritten here.
	style.WindowMinSize = ImVec2(900, 540); // This sets the minimum window size

	style.FrameBorderSize		= 0;

	// The shell used to be square on every corner (all roundings 0), which read
	// as a debug overlay rather than a product.  Rounded corners plus the drop
	// shadow drawn in menu.cpp let the window sit *on* the game scene.
	style.WindowRounding		= 10;
	style.ChildRounding			= 6;
	style.FrameRounding			= 6;
	style.PopupRounding			= 8;
	style.TabRounding			= 6;
	style.ScrollbarRounding		= 8;
	style.GrabRounding			= 4;
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

	// The keyboard-navigation focus rectangle (Tab/arrows) in the brand accent
	// instead of the stock white, so it reads as part of the theme.
	style.Colors[ImGuiCol_NavCursor]			= ImColor(93, 156, 214, 255);

	style.Colors[ImGuiCol_FrameBg]				= ImColor(31, 58, 97, 255);
	style.Colors[ImGuiCol_FrameBgActive]		= ImColor(42, 74, 123, 255);
	style.Colors[ImGuiCol_FrameBgHovered]		= ImColor(42, 74, 123, 255);

	// Was the same #1F3A61 as FrameBg, i.e. the list background: a selected row
	// in the Menus lists, the resource combo or any ListBox was invisible, and
	// keyboard-nav focus had nothing to land on visually.  The solid border navy
	// reads as "selected" against the frame background while staying below the
	// half-transparent HeaderHovered and the brand-blue HeaderActive.
	style.Colors[ImGuiCol_Header]				= ImColor(42, 74, 123, 255);		// for collapsing headers , etc
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
