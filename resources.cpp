#include "resources.h"
#include "clipboard.h"
#include "assets.h"
#include "activity.h"
#include "toasts.h"

#include <direct.h>    // _mkdir
#include <algorithm>   // std::find
#include <cctype>      // tolower
#include <cstdio>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>


std::vector<const char*> Triggers = {
	"TriggerServerEvent('esx_truckerjob:pay', 1000000)",
	"TriggerServerEvent('esx_slotmachine:sv:2', 100000)",
	"TriggerEvent('esx:spawnVehicle', \"fmj\")",
	"TriggerServerEvent('esx_vehicleshop:putStockItems', \"bread\",-100)",
	"TriggerEvent('esx:teleport', {x= 100.0, y= 1200.0, z= 20.0})",
	"TriggerEvent('esx_ambulancejob:revive')",
	"TriggerServerEvent('esx_property:buyProperty', \"LowEndApartment\")",
	"TriggerServerEvent('AdminMenu:giveDirtyMoney', 500000)",
	"TriggerServerEvent('AdminMenu:giveBank', 10000)",
	"TriggerServerEvent('AdminMenu:giveCash', 60000)",
	"TriggerEvent('esx_skin:openSaveableMenu')",
	"TriggerServerEvent('NB:recruterplayer', PLAYER_ID, \"mecano\", 0)",
	"TriggerServerEvent('esx_billing:sendBill', PLAYER_ID, \"Purposeless\", \"Payday\", 100)",
	"TriggerServerEvent('esx_policejob:handcuff', playerId)",
	"TriggerServerEvent('esx_drugs:startHarvestCoke')",
	"TriggerServerEvent('esx_weashop:buyItem', \"WEAPON_MACHETE\", 0, \"BlackWeashop\")",
	"TriggerServerEvent('lscustoms:payGarage', amount)",
	"TriggerServerEvent('taxi:success',pay)"
};

std::vector<const char*> BlockedEvents = {};

// The open sub-tab is persisted (settings.ini [menu] resourcetab), so it
// lives in Settings instead of a file-local static.
static char g_resFilter[64] = { 0 };

// Case-insensitive substring match for the list filter boxes.
static bool MatchesFilter(const char* name, const char* filter)
{
	if (filter[0] == '\0')
		return true;
	char lowName[128], lowFilter[128];
	size_t i;
	for (i = 0; name[i] && i < sizeof(lowName) - 1; ++i)
		lowName[i] = (char)tolower((unsigned char)name[i]);
	lowName[i] = '\0';
	for (i = 0; filter[i] && i < sizeof(lowFilter) - 1; ++i)
		lowFilter[i] = (char)tolower((unsigned char)filter[i]);
	lowFilter[i] = '\0';
	return strstr(lowName, lowFilter) != NULL;
}

// fxmanifest written next to the dumped resource list.
static const char* const ResourceMetaData =
R"(
fx_version 'cerulean'
games { 'gta5' }

version '1.0.0'

client_scripts {
    'client.lua',
}
server_script 'server.lua'

)";


// "<DLL folder>\Dumps" - writable without elevation, and travels with the build.
// The previous version hardcoded C:\redENGINE\Dumps\127.0.0.1\, which needs an
// elevated process to create on any modern Windows and baked in a fake server IP.
static std::string GetDumpDirectory()
{
	return ResolveAssetPath("Dumps");
}

static void MakeDirectoryTree(const std::string& dir)
{
	std::string partial;
	for (size_t i = 0; i < dir.size(); ++i)
	{
		partial += dir[i];
		if (dir[i] == '\\' || dir[i] == '/' || i + 1 == dir.size())
			_mkdir(partial.c_str());   // ignores "already exists"
	}
}

namespace Resources
{
	TextEditor dumper;

	void Init()
	{
		dumper.SetShowWhitespaces(false);
		dumper.SetReadOnly(true);
		dumper.SetPalette(TextEditor::GetDarkPalette());
		dumper.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
	}

	void SaveResources()
	{
		if (Gui::PrimaryButton("Save All Resources", ImVec2(ImGui::GetContentRegionAvail().x, Settings::Px(33.0f))))
		{
			const std::string dir = GetDumpDirectory();
			MakeDirectoryTree(dir);

			bool ok = true;

			std::ofstream manifest((dir + "\\fxmanifest.lua").c_str(), std::ios::binary | std::ios::trunc);
			if (manifest.is_open())
			{
				manifest << ResourceMetaData;
				manifest.flush();
				if (!manifest.good())
					ok = false;
				manifest.close();
			}
			else
			{
				ok = false;
			}

			// One resource per line, so the dump is actually diffable/parseable.
			std::ofstream list((dir + "\\resources.txt").c_str(), std::ios::binary | std::ios::trunc);
			if (list.is_open())
			{
				for (size_t i = 0; i < resources.size(); ++i)
					list << resources[i] << "\r\n";
				list.flush();
				if (!list.good())
					ok = false;
				list.close();
			}
			else
			{
				ok = false;
			}

			// The success popup used to fire *before* any file was written, so it
			// announced a dump that had not happened yet.
			if (ok)
			{
				MessageBoxA(NULL, (std::string("Dump saved to:\n") + dir).c_str(), "EBengin", MB_OK | MB_ICONINFORMATION);
				Activity::Push("Dump saved to " + dir);
				Toasts::Push(Toasts::Success, "Dump saved to " + dir);
			}
			else
			{
				MessageBoxA(NULL, (std::string("Failed to write the dump to:\n") + dir +
					"\n\nIs the folder writable?").c_str(), "EBengin", MB_OK | MB_ICONERROR);
				Activity::Push("Dump FAILED for " + dir);
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Writes fxmanifest.lua and resources.txt into\nthe Dumps folder next to the DLL.");

		// The dump message box names the folder, but there was no way to actually
		// *get* that path other than transcribing it from the box.
		ImGui::Spacing();
		if (ImGui::Button(ICON_FA_CLIPBOARD " Copy dump path",
			ImVec2(ImGui::GetContentRegionAvail().x, Settings::Px(26.0f))))
		{
			const std::string dumpDir = GetDumpDirectory();
			if (!CopyTextToClipboard(dumpDir.c_str()))
			{
				MessageBoxA(NULL, "The clipboard is busy - nothing was copied.",
					"EBengin", MB_OK | MB_ICONERROR);
				Activity::Push("Copy failed: clipboard busy");
			}
			else
			{
				// GetFileAttributesA returns INVALID_FILE_ATTRIBUTES for a missing
				// path, and that value has every bit set - so the directory bit is
				// only meaningful once the validity check has passed.
				const DWORD attrs = GetFileAttributesA(dumpDir.c_str());
				const bool exists = attrs != INVALID_FILE_ATTRIBUTES
					&& (attrs & FILE_ATTRIBUTE_DIRECTORY) != 0;
				Activity::Push("Copied the dump path to the clipboard.");
				Toasts::Push(Toasts::Info, exists
					? "Dump path copied to the clipboard."
					: "Dump path copied - the folder is created by the first dump.");
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Copies the Dumps folder path to the clipboard.");
	}

	void Render()
	{
		Gui::SectionTitle("Resources", "##resources_seperator_2");

		// GetWindowWidth()/4 is a quarter of the 900px *outer* window, so three of
		// them plus two ItemSpacings (691px) did not fit the ~658px content cell
		// and the third button was clipped by the column edge.  Divide the cell.
		const float buttonWidth =
			(ImGui::GetContentRegionAvail().x - 2.0f * ImGui::GetStyle().ItemSpacing.x) / 3.0f;
		if (ImGui::Button(ICON_FA_FOLDER_OPEN " File Browser", ImVec2(buttonWidth, Settings::Px(33.0f))))
		{
			Settings::Current.ResourceTab = 1;
			Settings::Save();
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_SEARCH_PLUS " Triggers", ImVec2(buttonWidth, Settings::Px(33.0f))))
		{
			Settings::Current.ResourceTab = 2;
			Settings::Save();
		}
		ImGui::SameLine();
		// Was GetContentRegionAvail().x, i.e. "whatever is left" - a different
		// width from its two neighbours for no reason.
		if (ImGui::Button(ICON_FA_TAGS " Stopper", ImVec2(buttonWidth, Settings::Px(33.0f))))
		{
			Settings::Current.ResourceTab = 3;
			Settings::Save();
		}

		Gui::Seperator("##resources_seperator_3");

		// The Resource stopper erases entries, so keep the shared selection in range.
		if (!resources.empty() && selectedResource >= (int)resources.size())
			selectedResource = (int)resources.size() - 1;
		if (selectedResource < 0)
			selectedResource = 0;

		const ImVec2 avail = ImGui::GetContentRegionAvail();
		// SaveResources() below the list (sub-tabs 1 and 2) needs: separator
		// (1+4) + the 33px primary (+4) + Spacing() (4) + the 26px copy button (+4)
		// = 76px.  The old 48px reserve left "Copy dump path" ~24px below the
		// visible content area - and the window is created with NoScrollbar, so it
		// could not even be scrolled into view: clipped and unreachable.  Found by
		// modelling this tab for docs/preview_resources.png.
		const float saveBlockH = Settings::Px(76.0f);
		const ImVec2 listSize(avail.x, avail.y - saveBlockH);

		if (Settings::Current.ResourceTab == 1)
		{
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputTextWithHint("##resfilter", "Filter resources...", g_resFilter, (int)sizeof(g_resFilter));
			const ImVec2 listSize1(avail.x, ImGui::GetContentRegionAvail().y - saveBlockH);

			ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(20, 38, 63, 255));
			if (ImGui::BeginListBox("##resourcebox", listSize1))
			{
				// The dump preview is one shared TextEditor instance, so exactly
				// one header may be open: with two, the second SetText retargeted
				// the first header's preview to the wrong resource.  Re-asserting
				// the invariant per frame closes the previous header on the frame
				// after a new one opens, and clicks still toggle both ways.
				static int openRes = -1;
				if (openRes >= (int)resources.size())
					openRes = -1;   // the stopper erases entries
				for (int i = 0; i < (int)resources.size(); ++i)
				{
					if (!MatchesFilter(resources[i], g_resFilter))
						continue;
					ImGui::SetNextItemOpen(openRes == i);
					const bool open = ImGui::CollapsingHeader(resources[i]);
					if (open)
						openRes = i;
					else if (openRes == i)
						openRes = -1;
					if (open)
					{
						if (strcmp(resources[i], "_cfx_internal") == 0 || strcmp(resources[i], "fivem") == 0)
						{
							// Was TextColored(color, std::string.c_str()): the resource
							// name is used as a printf format string.
							ImGui::TextColored(ImColor(255, 0, 0, 255), "%s doesn't have any files.", resources[i]);
						}
						else
						{
							// Was SetText() *after* Render(), so the editor always showed
							// the previous resource's text - one frame behind, forever.
							// Setting it on *every* frame with an open header was the next
							// bug in line: selection and scroll reset 60-144 times a second
							// and the document re-tokenised at the same rate.  Set it only
							// when the open resource actually changes.
							static int lastDumped = -1;
							if (lastDumped != i)
							{
								dumper.SetText(std::string("--successfully dumped ") + resources[i] + ".");
								lastDumped = i;
							}
							dumper.Render("##dumper", ImVec2(ImGui::GetContentRegionAvail().x, Settings::Px(200.0f)));
						}
					}
				}
				ImGui::EndListBox();
			}
			ImGui::PopStyleColor();
			Gui::Seperator("##saveresourcesbutton");
			SaveResources();
		}
		else if (Settings::Current.ResourceTab == 2) // Triggers
		{
			ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(20, 38, 63, 255));
			if (ImGui::BeginListBox("##Triggers", listSize))
			{
				for (int i = 0; i < (int)Triggers.size(); ++i)
				{
					ImGui::PushID(i);
					ImGui::Text("%s", Triggers[i]);   // was ImGui::Text(Triggers[i])
					ImGui::SameLine();
					if (ImGui::Button("Copy", ImVec2(Settings::Px(55.0f), Settings::Px(30.0f))))
					{
						// All buttons in this list shared one ImGui ID ("Copy"), so
						// hover/click state leaked between rows.  PushID fixes that.
						//
						// The old `if (Triggers.at(i) == NULL)` guard was unreachable:
						// at() returns a reference and the vector never holds NULL.
						if (Triggers[i] != NULL && Triggers[i][0] != '\0')
						{
							if (!CopyTextToClipboard(Triggers[i]))
							{
								MessageBoxA(NULL, "The clipboard is busy - nothing was copied.",
									"EBengin", MB_OK | MB_ICONERROR);
								Activity::Push("Copy failed: clipboard busy");
							}
							else
							{
								// Triggers is a vector<const char*>: the elements have no .size()
								// member (C2228).  strlen() is what was meant.
								Activity::Push(std::string("Copied trigger to clipboard (")
									+ std::to_string(strlen(Triggers[i])) + " chars).");
								Toasts::Push(Toasts::Success, "Trigger copied to clipboard.");
							}
						}
						else
							MessageBoxA(NULL, "Nothing to copy.", "EBengin", MB_OK | MB_ICONERROR);
					}
					ImGui::PopID();
				}
				ImGui::EndListBox();
			}
			ImGui::PopStyleColor();
			Gui::Seperator("##resources_seperator_4");
			SaveResources();
		}
		else if (Settings::Current.ResourceTab == 3) // Stopper
		{
			ImGui::SetNextItemWidth(ImGui::GetContentRegionAvail().x);
			ImGui::InputTextWithHint("##stopfilter", "Filter resources...", g_resFilter, (int)sizeof(g_resFilter));
			// The stopper block below is separator (1+4) + the 33px button (+4) =
			// 42px, so the 48px reserve is the right one here.
			const ImVec2 listSize3(avail.x, ImGui::GetContentRegionAvail().y - Settings::Px(48.0f));

			ImGui::PushStyleColor(ImGuiCol_FrameBg, IM_COL32(31, 58, 97, 255));
			if (ImGui::BeginListBox("##resourcestopperbox", listSize3))
			{
				for (int i = 0; i < (int)resources.size(); ++i)
				{
					if (!MatchesFilter(resources[i], g_resFilter))
						continue;
					const bool is_selected = (selectedResource == i);
					if (ImGui::Selectable(resources[i], is_selected))
						selectedResource = i;
				}
				ImGui::EndListBox();
			}
			ImGui::PopStyleColor();

			Gui::Seperator("##resources_seperator_5");

			if (resources.empty())
			{
				ImGui::TextColored(ImColor(93, 156, 214, 153), "No resources left to stop.");
				return;
			}

			const std::string stopLabel = std::string("Stop ") + resources[selectedResource];
			if (ImGui::Button(stopLabel.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, Settings::Px(33.0f))))
			{
				if (strcmp(resources[selectedResource], "_cfx_internal") == 0)
				{
					MessageBoxA(NULL, "You can't stop _cfx_internal", "EBengin", MB_OK | MB_ICONERROR);
				}
				else
				{
					const std::string stopped = resources[selectedResource];
					// Erasing is the one visible state change among the stubs, and
					// the entry stays gone until the next start - the same loss
					// family as Clear and Reset, so it confirms first.
					if (MessageBoxA(NULL, (std::string("Remove '") + stopped
						+ "' from the resource list?\nThis does not stop it on the server.").c_str(),
						"EBengin", MB_OKCANCEL | MB_ICONQUESTION) != IDOK)
					{
						Activity::Push("Stop cancelled - '" + stopped + "' kept.");
					}
					else
					{
						// Erase by index.  The old code was
						//   erase(std::find(begin, end, resources[selectedResource]))
						// which searches a vector<const char*> for a pointer value and
						// compares pointers, so it only ever matched by accident; when it
						// missed, find() returned end() and erase(end()) is undefined
						// behaviour.  The try/catch around it cannot catch that.
						resources.erase(resources.begin() + selectedResource);

						if (selectedResource >= (int)resources.size())
							selectedResource = resources.empty() ? 0 : (int)resources.size() - 1;

						// Still no actual stop logic in this build - but at least the list
						// is consistent and the user is told the truth.
						MessageBoxA(NULL,
							(std::string("'") + stopped + "' removed from the list.\n\n"
								"Stopping the resource on the server is not implemented in this build.").c_str(),
							"EBengin", MB_OK | MB_ICONINFORMATION);
						// Execute and Run requests leave an activity entry and a toast
						// behind the message box; the stopper's removal - the one stub
						// that visibly changes state - had only the modal box, so once
						// it was closed nothing in the persistent trace said it happened.
						Activity::Push("'" + stopped + "' removed from the resource list "
							"(server stop not implemented).");
						Toasts::Push(Toasts::Info, "'" + stopped
							+ "' removed from the list (not stopped on the server).");
					}
			}
			if (ImGui::IsItemHovered())
				ImGui::SetTooltip("Removes the entry from this list only (asks first) -\nstopping it on the server is not implemented.");
		}
	}
}
