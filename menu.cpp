#include "menu.h"
#include "tabs.h"
#include "gui.h"
#include "font_awesome.h"
#include "settings.h"

#include <cstdio>
#include <cmath>
#include <string>

namespace Menu
{
	struct SidebarItem
	{
		const char* label;
		int         tab;
	};

	// Sidebar layout, kept in one place so the buttons and the separators cannot
	// drift apart (they used to be 5 near-identical copy-pasted blocks, each with
	// its own hand-maintained cursor offsets).
	static const SidebarItem SidebarItems[] =
	{
		{ ICON_FA_CODE     " Executor", TabExecutor  },
		{ ICON_FA_DOWNLOAD " Resources", TabResources },
		{ ICON_FA_BOOK     " Menus",     TabMenus     },
		{ ICON_FA_COGS     " Config",    TabConfig    },
	};

	// Draws the left-hand column: logo, the tab buttons, and the footer text.
	// File-local - nothing outside this translation unit has a reason to call it.
	// Same as CenteredInSidebar but in a chosen colour (the FPS readout).
	static void CenteredInSidebarColored(const char* text, ImU32 col)
	{
		const float w = ImGui::CalcTextSize(text).x;
		ImGui::SetCursorPosX(ImMax(0.0f, (SidebarWidth - w) * 0.5f));
		ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s", text);
	}

	// Draws one line of the sidebar footer centred in the sidebar column.
	static void CenteredInSidebar(const char* text)
	{
		const float w = ImGui::CalcTextSize(text).x;
		ImGui::SetCursorPosX(ImMax(0.0f, (SidebarWidth - w) * 0.5f));
		ImGui::Text("%s", text);
	}

	static void RenderSidebar()
	{
		ImGui::SetCursorPosX(74);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 9);
		if (LogoLoaded)
		{
			// ImTextureID became ImU64/ImTextureRef in 1.92: cast the SRV explicitly.
			ImGui::Image((ImTextureID)(intptr_t)logo, ImVec2(76, 87));
		}
		else
		{
			// rE.png was not found next to the DLL: keep the sidebar the same height.
			ImGui::SetCursorPosX(58);
			ImGui::TextColored(ImColor(93, 156, 214, 255), "redENGINE");
			ImGui::Dummy(ImVec2(76, 68));
		}
		ImGui::NewLine();

		for (size_t i = 0; i < IM_ARRAYSIZE(SidebarItems); ++i)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
			ImGui::SetCursorPosX(0);
			Gui::Seperator(std::string("##main_seperator_") + std::to_string(i + 1), SidebarWidth, 1.0f);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
			ImGui::SetCursorPosX(0);

			// Nothing used to mark the open tab: every entry drew with
			// ImGuiCol_Button and only the transient hover/press colour differed,
			// so after a click the sidebar looked exactly as before and the user
			// could not tell which tab was open.  At startup Tab is TabDefault,
			// which is not in the sidebar at all, so nothing was marked then
			// either.  The selected entry now holds the pressed colour and gets a
			// 3px accent bar on its left edge.
			const bool active = (Tab == SidebarItems[i].tab);
			if (active)
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

			if (ImGui::Button(SidebarItems[i].label, ImVec2(SidebarWidth, SidebarButtonHeight)))
			{
				Tab = SidebarItems[i].tab;
				Settings::Current.Tab = Tab;
				Settings::Save();
			}

			if (active)
			{
				const ImVec2 min = ImGui::GetItemRectMin();
				ImGui::GetWindowDrawList()->AddRectFilled(
					ImVec2(min.x, min.y), ImVec2(min.x + 3.0f, min.y + SidebarButtonHeight), Accent);
				ImGui::PopStyleColor();
			}
		}

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4);
		ImGui::SetCursorPosX(0);
		Gui::Seperator("##main_seperator_5", SidebarWidth, 1.0f);

		// The footer was pinned to SetCursorPosY(380) and (399): 19px apart, less
		// than one 20px text line, so the two strings drew on top of each other,
		// and both were tied to one particular window height.  Anchor the block to
		// the bottom of the sidebar and centre each line in the column instead.
		char buildDate[64];
		snprintf(buildDate, sizeof(buildDate), "Build Date: %s", __DATE__);

		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y
			- 3.0f * ImGui::GetTextLineHeightWithSpacing());

		// Live frame rate / frame time from ImGui's own smoothed counter, so the
		// overlay can answer "is it the menu or the game?" without a profiler.
		const float fps = ImGui::GetIO().Framerate;
		char fpsLine[48];
		snprintf(fpsLine, sizeof(fpsLine), "FPS %.0f  |  %.1f ms",
			fps, fps > 0.0f ? 1000.0f / fps : 0.0f);
		CenteredInSidebarColored(fpsLine, Accent);

		CenteredInSidebar("Licensed to #4278");
		CenteredInSidebar(buildDate);

		// The old layout ended with
		//     SetCursorPosY(GetWindowHeight() - 10); Text("");
		// whose only job was to stretch the legacy Columns() cell to the full
		// window height ("so don't remove :)").  A table row is already as tall as
		// its tallest cell, so that hack is gone.
	}

	void Render()
	{
		//redENGINE Remade : by xo1337
		// NoCollapse/NoResize: the layout below is pixel-positioned, so a
		// user-resized or collapsed window just produced broken frames.
		// Restore the persisted position on the first frame only; afterwards the
		// window is free to move and its new position is saved below.
		if (Settings::Current.WindowX >= 0)
			ImGui::SetNextWindowPos(
				ImVec2((float)Settings::Current.WindowX, (float)Settings::Current.WindowY),
				ImGuiCond_Once);

		ImGui::Begin("redENGINE", NULL,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		// Two-column shell: fixed sidebar on the left, tab content on the right.
		//
		// This used to be the legacy Columns() API (ImGui::Columns / NextColumn /
		// SetColumnOffset), deprecated since 1.80 in favour of Tables.  Unlike
		// Columns(), a table gives every cell its own layout context, so
		// GetContentRegionAvail() inside a tab reports that cell rather than the
		// whole window.
		//
		//   BordersInnerV  - the divider between the two columns only, no outer
		//                    frame.  This is exactly what Columns(2, id, true)
		//                    drew.  (BordersV would add outer edges as well.)
		//   SizingFixedFit - required for WidthFixed to be honoured; without a
		//                    sizing policy the table would auto-fit to contents.
		//   NoPadOuterX    - content starts flush with the window edge, as before.
		//
		// style.CellPadding is set to (0,0) in ApplyStyle() so the 225px sidebar
		// stays exactly 225px of usable width, matching the old column semantics.
		if (ImGui::BeginTable("##maincolumn", 2,
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
		{
			ImGui::TableSetupColumn("##sidebar", ImGuiTableColumnFlags_WidthFixed, SidebarWidth);
			ImGui::TableSetupColumn("##content", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			RenderSidebar();

			ImGui::TableNextColumn();
			// See Menu::ContentPadding: keeps the tab bodies off the divider.
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ContentPadding);
			Tabs::Render();

			ImGui::EndTable();
		}

		// Remember where the window ended up so the next session opens there.
		// Captured before End() (after it there is no current window to ask) and
		// throttled to ~1/s plus a distance threshold: writing an .ini every
		// frame would be wasteful, and the value is at most a second stale when
		// the DLL unloads.
		const ImVec2 pos = ImGui::GetWindowPos();
		static ImVec2 lastSaved(-1.0f, -1.0f);
		static double lastSaveTime = -1.0;
		const double now = ImGui::GetTime();
		if ((int)pos.x != Settings::Current.WindowX || (int)pos.y != Settings::Current.WindowY)
		{
			const bool movedFar = fabsf(pos.x - lastSaved.x) > 40.0f || fabsf(pos.y - lastSaved.y) > 40.0f;
			if (lastSaveTime < 0.0 || (now - lastSaveTime) > 1.0 || movedFar)
			{
				Settings::SaveWindowPos(pos.x, pos.y);
				lastSaved = pos;
				lastSaveTime = now;
			}
		}

		ImGui::End();
	}
}
