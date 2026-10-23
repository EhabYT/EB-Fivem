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
#include "activity.h"
#include "toasts.h"

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

	// Last file the buffer came from or went to.  After a second load the
	// origin of the text on screen was simply gone; the status line keeps it.
	static std::string g_lastFile;

	static std::string BaseName(const std::string& path)
	{
		const size_t i = path.find_last_of("\\/");
		return i == std::string::npos ? path : path.substr(i + 1);
	}

	void Init()
	{
		editor.SetReadOnly(false);
		// Was hardcoded false; the visibility of whitespace dots is a preference
		// and is persisted with the rest of the interface settings.
		ApplyPrefs();
		editor.SetPalette(TextEditor::GetDarkPalette());

		// This is a Lua editor; the widget defaults to C++.  Setting it here (once)
		// rather than in Render() also stops the document being re-tokenised every
		// frame, which was the single biggest per-frame cost in the menu.
		editor.SetLanguageDefinition(TextEditor::LanguageDefinition::Lua());
	}

	void ApplyPrefs()
	{
		editor.SetShowWhitespaces(Settings::Current.ShowWhitespace);
	}

	void Render()
	{
		Gui::SectionTitle("Executor", "##under_text1");

		// Mouse-reachable editor actions.  The Ctrl+Z/Y/C/V shortcuts work
		// inside the editor, but with keyboard-nav focus a button is the only
		// path, and icon buttons make the actions discoverable in the first
		// place.  The tooltips name the shortcut each button mirrors.
		const float tb = Settings::Px(26.0f);
		// The three actions that need something to work on now say so: on a fresh
		// buffer undo/redo/copy are greyed instead of clicking into a no-op, the
		// same "dead control reads as broken" family as the old silent stubs, just
		// smaller.  BeginDisabled is ImGui's own state (alpha + item flags), so
		// hover, click and keyboard focus all stop on their own; the tooltip
		// explains why, which needs AllowWhenDisabled because IsItemHovered()
		// ignores disabled items by default.
		ImGui::BeginDisabled(!editor.CanUndo());
		if (ImGui::Button(ICON_FA_UNDO "##ed_undo", ImVec2(tb, tb)))
			editor.Undo();
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip(editor.CanUndo() ? "Undo (Ctrl+Z)" : "Nothing to undo");
		ImGui::SameLine();
		ImGui::BeginDisabled(!editor.CanRedo());
		if (ImGui::Button(ICON_FA_REDO "##ed_redo", ImVec2(tb, tb)))
			editor.Redo();
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip(editor.CanRedo() ? "Redo (Ctrl+Y)" : "Nothing to redo");
		ImGui::SameLine();
		ImGui::BeginDisabled(!editor.HasSelection());
		if (ImGui::Button(ICON_FA_COPY "##ed_copy", ImVec2(tb, tb)))
			editor.Copy();
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip(editor.HasSelection()
				? "Copy selection (Ctrl+C)" : "Nothing selected");
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_PASTE "##ed_paste", ImVec2(tb, tb)))
			editor.Paste();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Paste (Ctrl+V)");
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_ERASER "##ed_clear", ImVec2(tb, tb)))
			editor.SetText("");
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Clear the buffer");
		ImGui::SameLine();
		// Load from File had no counterpart: a buffer could only leave the
		// editor through the clipboard.  Same dialog family as the loader -
		// the overwrite prompt is the dialog's own, every failure path says
		// what happened instead of doing nothing.
		if (ImGui::Button(ICON_FA_SAVE "##ed_save", ImVec2(tb, tb)))
		{
			char file[MAX_PATH] = { 0 };
			OPENFILENAMEA ofn;
			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.hwndOwner = FindWindowA(NULL, "FiveM");
			ofn.lpstrFilter = "Lua / text files\0*.lua;*.txt\0All files\0*.*\0";
			ofn.lpstrFile = file;
			ofn.nMaxFile = MAX_PATH;
			ofn.lpstrDefExt = "lua";
			ofn.Flags = OFN_OVERWRITEPROMPT | OFN_PATHMUSTEXIST | OFN_NOCHANGEDIR;

			if (GetSaveFileNameA(&ofn))
			{
				const std::string text = editor.GetText();
				FILE* fp = NULL;
				if (fopen_s(&fp, file, "wb") != 0 || fp == NULL)
				{
					MessageBoxA(NULL, (std::string("Could not write:\n") + file).c_str(),
						"EBengin", MB_OK | MB_ICONERROR);
					Activity::Push(std::string("Save failed: ") + file);
					Toasts::Push(Toasts::Warn, std::string("Save failed: ") + file);
				}
				else
				{
					const size_t written = fwrite(text.data(), 1, text.size(), fp);
					fclose(fp);
					if (written != text.size())
					{
						MessageBoxA(NULL, (std::string("Write stopped after "
							+ std::to_string(written) + " of "
							+ std::to_string(text.size()) + " bytes:\n") + file).c_str(),
							"EBengin", MB_OK | MB_ICONERROR);
						Activity::Push(std::string("Save incomplete: ") + file);
						Toasts::Push(Toasts::Warn, "Save incomplete - disk full?");
					}
					else
					{
						g_lastFile = file;
						Activity::Push("Saved " + std::to_string(written)
							+ " bytes to " + file + ".");
						Toasts::Push(Toasts::Success, "Saved "
							+ std::to_string(written) + " bytes.");
					}
				}
			}
		}
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Save the buffer to a file");
		ImGui::SameLine();
		if (Gui::Toggle("Whitespace##ed_ws", &Settings::Current.ShowWhitespace))
		{
			ApplyPrefs();
			Settings::Save();
		}
		ImGui::Spacing();

		// The Resource stopper can shrink `resources`, so keep the selection in range.
		if (!resources.empty() && selectedResource >= (int)resources.size())
			selectedResource = (int)resources.size() - 1;
		if (selectedResource < 0)
			selectedResource = 0;

		// Was ImVec2(GetWindowWidth(), GetWindowHeight()) - the *whole window*,
		// measured before the sidebar column was accounted for, so the editor
		// overflowed past the right edge and under the button row.
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float statusH = ImGui::GetTextLineHeightWithSpacing();
		editor.Render("##Null", ImVec2(avail.x - 16.0f,
			avail.y - Settings::Px(52.0f) - statusH - tb - Settings::Px(4.0f)), true);

		// Status line: the editor widget shows neither cursor position nor
		// document size, so a 400-line paste looked identical to an empty buffer.
		const TextEditor::Coordinates cur = editor.GetCursorPosition();
		char status[96];
		snprintf(status, sizeof(status), "Ln %d, Col %d  |  %d lines  |  Lua%s",
			cur.mLine + 1, cur.mColumn + 1, editor.GetTotalLines(),
			editor.IsReadOnly() ? "  |  read-only" : "");
		std::string statusLine = status;
		if (!g_lastFile.empty())
			statusLine += "  |  file: " + BaseName(g_lastFile);
		if (editor.HasSelection())
			statusLine += "  |  " + std::to_string(editor.GetSelectedText().size()) + " selected";
		ImGui::TextColored(ImColor(93, 156, 214, 153), "%s", statusLine.c_str());
		ImGui::Spacing();

		if (Gui::PrimaryButton(ICON_FA_CODE " Execute",
			ImVec2(Settings::Px(116.0f), Settings::Px(30.0f))))
		{
			if (resources.empty())
			{
				MessageBoxA(NULL, "No resource selected.", "EBengin", MB_OK | MB_ICONERROR);
			}
			else if (strcmp(resources[selectedResource], "_cfx_internal") == 0)
			{
				MessageBoxA(NULL, "You can't execute in _cfx_internal.", "EBengin", MB_OK | MB_ICONERROR);
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
					"EBengin", MB_OK | MB_ICONINFORMATION);
				Activity::Push(std::string("Execute requested for ")
					+ resources[selectedResource] + " (not implemented).");
				Toasts::Push(Toasts::Info, std::string("Execute requested for ")
					+ resources[selectedResource] + " (not implemented).");
			}
		}

		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_FILE " Load from File",
			ImVec2(Settings::Px(180.0f), Settings::Px(30.0f))))
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
				// Every failure path used to be silent: pick an unreadable, empty
				// or oversized file and the button simply did nothing, which is
				// indistinguishable from the old dead stub.  Say what happened.
				FILE* fp = NULL;
				if (fopen_s(&fp, file, "rb") != 0 || fp == NULL)
				{
					MessageBoxA(NULL, (std::string("Could not open:\n") + file).c_str(),
						"EBengin", MB_OK | MB_ICONERROR);
					Activity::Push(std::string("Load failed: ") + file);
				}
				else
				{
					fseek(fp, 0, SEEK_END);
					const long size = ftell(fp);
					fseek(fp, 0, SEEK_SET);
					if (size <= 0)
					{
						MessageBoxA(NULL, "The selected file is empty.",
							"EBengin", MB_OK | MB_ICONERROR);
						Activity::Push(std::string("Load failed (empty): ") + file);
					}
					else if (size >= 4 * 1024 * 1024)
					{
						MessageBoxA(NULL, "The selected file is larger than 4 MB and was not loaded.",
							"EBengin", MB_OK | MB_ICONERROR);
						Activity::Push(std::string("Load refused (>4 MB): ") + file);
					}
					else
					{
						std::string text((size_t)size, '\0');
						const size_t read = fread(&text[0], 1, (size_t)size, fp);
						text.resize(read);
						// A UTF-8 byte-order mark would land in the editor as a junk
						// glyph in front of the first line.  The editor holds UTF-8
						// text; the marker itself carries no information.
						if (read >= 3 && (unsigned char)text[0] == 0xEF
							&& (unsigned char)text[1] == 0xBB && (unsigned char)text[2] == 0xBF)
						{
							text.erase(0, 3);
						}
						editor.SetText(text);
						g_lastFile = file;
						// Reported size is what the editor holds, i.e. after the BOM
						// strip, not what the file occupied on disk.
						Activity::Push("Loaded " + std::string(file) + " ("
							+ std::to_string(text.size()) + " bytes) into the editor.");
						Toasts::Push(Toasts::Success, "Loaded " + std::string(file)
							+ " (" + std::to_string(text.size()) + " bytes).");
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
		else
			// The stopper can erase the last entry; without a fallback the row
			// would just be empty where a selection used to be.
			ImGui::TextColored(ImColor(93, 156, 214, 153), "No resources left to select.");
		ImGui::PopItemWidth();
	}
}
