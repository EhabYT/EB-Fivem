#pragma once

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "font_awesome.h"
#include "menu_style.h"   // ColMuted token for every Gui consumer
#include "settings.h"

#include <Windows.h>

// UI: row 211 - defined in main.cpp: the host window the hook resolved from
// the swap chain (sd.OutputWindow).  Every dialog this UI raises belongs to
// it - an unowned dialog can sit BEHIND a fullscreen game while the modal
// loop has already stalled the render thread, which reads as a hang with
// no visible cause.
extern HWND window;
#include <cstring>
#include <string>
#include <iostream>
#include <vector>

namespace Gui
{
	// UI: row 168 - wall-clock seconds spent inside modal dialogs since start.
	// The frame loop (and ImGui::GetTime with it) freezes while a dialog is
	// open, so the two gap heuristics - the shell's reopen fade and the
	// capture retirement - would read the dialog duration as idle time and
	// misfire on close.  They subtract what this counter newly reports.
	// Adder-and-reader in one: the wrapper adds, consumers just read.
	inline double ModalSeconds(double add = 0.0)
	{
		static double total = 0.0;
		total += add;
		return total;
	}

	// UI: row 165 - MessageBoxA spins a modal message loop: the present loop -
	// and with it the toggle-edge poll - stops while the dialog is open, so a
	// toggle press during the dialog survives as a pending edge and flips the
	// menu on the first frame after it closes.  Every dialog in the project
	// routes through this wrapper, which drains key edges on the way out.
	// UI: row 169 - not just the toggle edge: an armed key-capture scan reads
	// every key, so a press during the dialog (F5 out of habit, say) used to
	// bind itself on the first frame after close.  Presses inside a dialog
	// belong to the dialog; drain the whole keyboard range (vk 7+; 1..6 are
	// mouse buttons, which neither the capture scan nor the toggle poll read).
	// UI: row 201 - true while a modal dialog blocks the render thread.
	// DllMain's DETACH waits (capped) for this before destroying the ImGui
	// context; the inline-function static is one instance program-wide,
	// same trick as ModalSeconds below.
	inline bool& ModalActiveRef()
	{
		static bool s_active = false;
		return s_active;
	}
	inline bool ModalActive() { return ModalActiveRef(); }

	// UI: row 254 - shared modal lifecycle for MessageBoxA and the Win32 file
	// dialogs (GetOpenFileNameA / GetSaveFileNameA), which spin the same modal
	// message loop on the render thread.
	inline ULONGLONG BeginModal()
	{
		ModalActiveRef() = true;
		return ::GetTickCount64();
	}

	inline void EndModal(ULONGLONG t0)
	{
		ModalActiveRef() = false;
		ModalSeconds((double)(::GetTickCount64() - t0) / 1000.0);
		for (int vk = 7; vk < 255; ++vk)
			(void)::GetAsyncKeyState(vk);
	}

	inline int ModalBoxA(const char* text, const char* title, UINT flags)
	{
		const ULONGLONG t0 = BeginModal();
		// UI: row 211 - was NULL: unowned boxes can open behind the game window.
		const int r = ::MessageBoxA(window, text, title, flags);
		EndModal(t0);
		return r;
	}

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
		// UI: row 151 - type hierarchy by weight.  The shell ships exactly one
		// font (Bahnschrift 20; no bold instance is reachable without the
		// FreeType builder), and a larger header font would cost every pane
		// four pixels of budget - so the heading fakes its weight with a
		// sub-pixel second pass over itself.  Zero layout cost, and the
		// draw-list text rides the same font/scale as the first pass.
		if (ImGui::IsItemVisible())
		{
			const ImVec2 mn = ImGui::GetItemRectMin();
			ImGui::GetWindowDrawList()->AddText(
				ImVec2(mn.x + Settings::Px(0.7f), mn.y),
				ImGui::GetColorU32(ImGuiCol_Text), title);
		}
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
