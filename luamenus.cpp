#include "luamenus.h"
#include "gui.h"
#include "font_awesome.h"
#include "activity.h"
#include "toasts.h"

#include "imgui/imgui.h"

#include <Windows.h>
#include <string>
#include <vector>

// Menu names shown on the Menus tab.  These used to be `static` definitions in
// luamenus.h, i.e. a fresh copy of both vectors for every translation unit that
// included it.  Nothing outside this tab reads them, so they live here.
static const std::vector<const char*> FreeMenus = {
	"HoaX_2.0",
	"Lynx_8.1",
	"Lynx_10.0",
	"InSec_1.0",
	"SkidMenu_1.0",
	"TiagoMenu_3.0",
	"TiagoMenu_4.0",
	"TiagoMenu_4.1",
	"Strata_1.0",
	"Lydia_Enterprise"
};

static const std::vector<const char*> PremMenus = {
	"Lux",
	"Dopamine",
	"Absolute",
	"Reaper",
	"Fallout",
};

static int SelectedFreeMenu = 0;   // was `= NULL` on an int
static int SelectedPremMenu = 0;

namespace Menus
{
	// Draws one of the two menu lists and its "Run <name>" button.
	// The two blocks were near-identical copies that had already drifted apart.
	static void RenderMenuList(const char* childId, const char* listBoxId, const char* title,
		float width, const std::vector<const char*>& items, int& selected)
	{
		ImGui::BeginChild(childId, ImVec2(width, ImGui::GetContentRegionAvail().y), ImGuiChildFlags_None);

		ImGui::Text("%s", title);
		Gui::Seperator(std::string("##") + childId + "_sep");

		// EndListBox() may only be called when BeginListBox() returned true.
		// (ListBoxHeader/ListBoxFooter were removed in ImGui 1.89.)
		if (ImGui::BeginListBox(listBoxId,
			ImVec2(ImGui::GetContentRegionAvail().x, Settings::Px(300.0f))))
		{
			for (int i = 0; i < (int)items.size(); ++i)
			{
				if (ImGui::Selectable(items[i], selected == i))
					selected = i;
			}
			ImGui::EndListBox();
		}

		if (!items.empty())
		{
			if (selected < 0 || selected >= (int)items.size())
				selected = 0;

			const std::string label = std::string("Run ") + items[selected];
			if (Gui::PrimaryButton(label.c_str(),
				ImVec2(ImGui::GetContentRegionAvail().x, Settings::Px(33.0f))))
			{
				// Empty body before: clicking "Run Lux" did nothing at all, with no
				// indication that it was a stub.
				Gui::ModalBoxA(
					(std::string("'") + items[selected] + "' is not bundled with this build.").c_str(),
					"EBengin", MB_OK | MB_ICONINFORMATION);
				Activity::Push(std::string("Run requested for '") + items[selected]
					+ "' (not bundled).");
				Toasts::Push(Toasts::Info, std::string("Run requested for '") + items[selected]
					+ "' (not bundled).");
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Reports only - this menu is not bundled\nwith this build.");
		}

		ImGui::EndChild();
	}

	void Render()
	{
		Gui::SectionTitle("Menus", "##menus_seperator_1");
		// UI: row 232 - the same provenance as the Resources list (row 230): both
		// name vectors are compile-time constants; nothing is fetched, bundled or
		// launched.  The Run tooltip says "not bundled", but only on hover - until
		// then the two lists read like an installed catalogue.  The tab itself now
		// says so up front, with the same muted token.
		ImGui::TextColored(ColMuted, "%s",
			"Compiled-in name list - nothing here is bundled or launched.");
		ImGui::NewLine();

		const float halfWidth = ImGui::GetContentRegionAvail().x / 2.0f;

		RenderMenuList("##freemenus_side", "##freemenusbox",
			ICON_FA_HOME " Free Menus " ICON_FA_HOME,
			halfWidth - 2.0f, FreeMenus, SelectedFreeMenu);

		ImGui::SameLine();
		Gui::Seperator("##splitter_2", 1, ImGui::GetContentRegionAvail().y, false);
		ImGui::SameLine();

		RenderMenuList("##premmenus_side", "##premmenusbox",
			ICON_FA_STAR " Premium Menus " ICON_FA_STAR,
			ImGui::GetContentRegionAvail().x, PremMenus, SelectedPremMenu);
	}
}
