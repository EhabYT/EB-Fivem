#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "font_awesome.h"
#include "menu_style.h"   // ColMuted token for every Gui consumer
#include "settings.h"

#include <Windows.h>
#include <cstring>
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

	// Linear blend of two packed colours - used by Toggle() to ease the track
	// between the panel navy and the brand blue.
	inline ImU32 MixColor(ImU32 a, ImU32 b, float t)
	{
		const ImVec4 fa = ImGui::ColorConvertU32ToFloat4(a);
		const ImVec4 fb = ImGui::ColorConvertU32ToFloat4(b);
		return ImGui::ColorConvertFloat4ToU32(ImVec4(
			fa.x + (fb.x - fa.x) * t, fa.y + (fb.y - fa.y) * t,
			fa.z + (fb.z - fa.z) * t, fa.w + (fb.w - fa.w) * t));
	}

	// The one primary action per tab.  Stock ImGui buttons are all the same
	// navy, so "Execute" looked exactly as important as "Copy"; this fills the
	// button with the brand ramp (rest #3778B5, hover #5D9CD6, pressed #1D5188)
	// plus a soft top highlight, and centres the label.  Input still goes
	// through InvisibleButton, so hover/active/ID semantics stay ImGui's.
	inline bool PrimaryButton(const char* label, const ImVec2& size = ImVec2(0, 0))
	{
		const ImGuiStyle& style = ImGui::GetStyle();
		const ImVec2 labelSize = ImGui::CalcTextSize(label, NULL, true);
		const ImVec2 sz = ImGui::CalcItemSize(size,
			labelSize.x + style.FramePadding.x * 2.0f,
			labelSize.y + style.FramePadding.y * 2.0f);

		const bool clicked = ImGui::InvisibleButton(label, sz);
		const ImVec2 mn = ImGui::GetItemRectMin();
		const ImVec2 mx = ImGui::GetItemRectMax();
		const bool hovered = ImGui::IsItemHovered();
		const bool active = ImGui::IsItemActive();

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImU32 base = active ? IM_COL32(29, 81, 136, 255)
			: hovered ? IM_COL32(93, 156, 214, 255)
			: IM_COL32(55, 120, 181, 255);
		dl->AddRectFilled(mn, mx, base, style.FrameRounding);
		dl->AddRectFilled(ImVec2(mn.x + 1.0f, mn.y + 1.0f),
			ImVec2(mx.x - 1.0f, mn.y + sz.y * 0.5f),
			IM_COL32(255, 255, 255, active ? 14 : 26), style.FrameRounding);
		dl->AddText(ImVec2(mn.x + (sz.x - labelSize.x) * 0.5f,
			mn.y + (sz.y - labelSize.y) * 0.5f), IM_COL32(255, 255, 255, 255), label);
		return clicked;
	}

	// Animated on/off switch, replacing the stock square checkboxes in Config.
	// The knob position is eased per widget through ImGui's own state storage,
	// so it survives across frames without any global widget table.
	inline bool Toggle(const char* label, bool* v)
	{
		ImGui::PushID(label);
		const ImGuiStyle& style = ImGui::GetStyle();
		const float rowH = ImGui::GetFrameHeight();
		const float trackW = Settings::Px(40.0f), trackH = Settings::Px(20.0f);

		// The whole row (track + label) is the hit area, like a stock checkbox;
		// the first version only accepted clicks on the 40px track.
		const char* hash = strstr(label, "##");
		const int labelLen = hash ? (int)(hash - label) : (int)strlen(label);
		const ImVec2 labelSize = ImGui::CalcTextSize(label, label + labelLen, true);
		const ImVec2 mn = ImGui::GetCursorScreenPos();
		const bool clicked = ImGui::InvisibleButton("##switch",
			ImVec2(trackW + style.ItemSpacing.x + labelSize.x, rowH));
		if (clicked)
			*v = !*v;

		ImGuiStorage* st = ImGui::GetStateStorage();
		const ImGuiID animId = ImGui::GetID("##switch_anim");
		const float target = *v ? 1.0f : 0.0f;
		float t = st->GetFloat(animId, target);
		if (Settings::Current.Animations)
		{
			t += (target - t) * (ImGui::GetIO().DeltaTime * 12.0f < 1.0f ? ImGui::GetIO().DeltaTime * 12.0f : 1.0f);
			if (t - target < 0.001f && target - t < 0.001f)
				t = target;
		}
		else
			t = target;   // animations off means no motion, not even here
		st->SetFloat(animId, t);

		ImDrawList* dl = ImGui::GetWindowDrawList();
		const ImVec2 trackMin(mn.x, mn.y + (rowH - trackH) * 0.5f);
		dl->AddRectFilled(trackMin, ImVec2(trackMin.x + trackW, trackMin.y + trackH),
			MixColor(IM_COL32(31, 58, 97, 255), IM_COL32(55, 120, 181, 255), t), trackH * 0.5f);
		const float knobR = trackH * 0.5f - Settings::Px(3.0f);
		dl->AddCircleFilled(ImVec2(trackMin.x + trackH * 0.5f + (trackW - trackH) * t,
			trackMin.y + trackH * 0.5f), knobR, IM_COL32(255, 255, 255, 255));
		dl->AddText(ImVec2(mn.x + trackW + style.ItemSpacing.x, mn.y + (rowH - labelSize.y) * 0.5f),
			ImGui::GetColorU32(ImGuiCol_Text), label, label + labelLen);
		ImGui::PopID();
		return clicked;
	}
}
