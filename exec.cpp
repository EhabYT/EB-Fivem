#include "exec.h"
#include "gui.h"

#include <Windows.h>
#include <commdlg.h>   // GetOpenFileNameA
#include <cstdio>
#include <cstring>
#include <string>

#pragma comment(lib, "comdlg32.lib")

// Shared with the Resources tab (file browser + stopper), which selects into the
// same `resources` / `selectedResource` pair.
#include "resources.h"

std::vector<const char*> resources = {
	"_cfx_internal","fivem","hardcap","anti-cheese","esx_mafia","esx_gopostal","esx_jail","mysql-async","es_admin2",
	"async","es_extended","esx_barbershop","esx_clotheshop","esx_datastore","esx_garage","esx_identity","esx_lscustom",
	"esx_menu_default","esx_menu_dialog","esx_menu_list","esx_property","esx_shops","esx_sit","esx_vehicleshop","esx_weashops",
	"es_camera","instance","skinchanger","mellotrainer","bob74_ipl","coordsaver","loadingscreen"
};

int selectedResource = 0;   // was `static int selectedResource = NULL;`

namespace Executor
{
	TextEditor editor;

	void Init()
	{
		editor.SetReadOnly(false);
		editor.SetShowWhitespaces(false);
		editor.SetPalette(TextEditor::GetDarkPalette());

		// This is a Lua editor; the widget defaults to C++.  Setting it here (once)
		// rather than in Render() also stops the document being re-tokenised every
		// frame, which was the single biggest per-frame cost in the menu.
		editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
	}

	void Render()
	{
		Gui::SectionTitle("Executor", "##under_text1");

		// The Resource stopper can shrink `resources`, so keep the selection in range.
		if (!resources.empty() && selectedResource >= (int)resources.size())
			selectedResource = (int)resources.size() - 1;
		if (selectedResource < 0)
			selectedResource = 0;

		// Was ImVec2(GetWindowWidth(), GetWindowHeight()) - the *whole window*,
		// measured before the sidebar column was accounted for, so the editor
		// overflowed past the right edge and under the button row.
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		editor.Render("##Null", ImVec2(avail.x - 16.0f, avail.y - 52.0f), true);
		ImGui::Spacing();

		if (ImGui::Button(ICON_FA_CODE " Execute", ImVec2(116, 30)))
		{
			if (resources.empty())
			{
				MessageBoxA(NULL, "No resource selected.", "redENGINE", MB_OK | MB_ICONERROR);
			}
			else if (strcmp(resources[selectedResource], "_cfx_internal") == 0)
			{
				MessageBoxA(NULL, "You can't execute in _cfx_internal.", "redENGINE", MB_OK | MB_ICONERROR);
			}
			else
			{
				// Not implemented in this build.  The button used to be silently dead,
				// which reads as a crash/hang to the user.
				const std::string script = editor.GetText();
				MessageBoxA(NULL,
					(std::string("Execution is not implemented in this build.\n\nTarget: ")
						+ resources[selectedResource]
						+ "\nScript: " + std::to_string(script.size()) + " bytes").c_str(),
					"redENGINE", MB_OK | MB_ICONINFORMATION);
			}
		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_FILE " Load from File", ImVec2(180, 30)))
		{
			// Was an empty `// load file code` stub.  Wire up a real Win32 file
			// dialog: picking a .lua/.txt file loads it into the editor.
			char file[MAX_PATH] = { 0 };
			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = FindWindowA(NULL, "FiveM");
			ofn.lpstrFilter = "Lua / text files\0*.lua;*.txt\0All files\0*.*\0";
			ofn.lpstrFile = file;
			ofn.nMaxFile = MAX_PATH;
			ofn.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

			if (GetOpenFileNameA(&ofn))
			{
				FILE* fp = NULL;
				if (fopen_s(&fp, file, "rb") == 0 && fp != NULL)
				{
					fseek(fp, 0, SEEK_END);
					const long size = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					if (size > 0 && size < 4 * 1024 * 1024)
					{
						std::string text((size_t)size, '\0');
						const size_t read = fread(&text[0], 1, (size_t)size, fp);
						text.resize(read);
						editor.SetText(text);
					}
					fclose(fp);
				}
			}
		}

		ImGui::SameLine();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		// items_count is an int: size_t has no implicit conversion (C2664).
		if (!resources.empty())
			ImGui::Combo("##resources", &selectedResource, resources.data(), (int)resources.size());
		ImGui::PopItemWidth();
	}
}
