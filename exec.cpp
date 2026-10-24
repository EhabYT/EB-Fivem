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

	// Shared read path for Load-from-file and Revert: the size cap, the
	// empty check and the UTF-8 BOM strip live in one place so the two
	// cannot drift apart the way the old menu-list copies did.
	// Returns 0 on success, 1 open failure, 2 empty, 3 oversized; on 0,
	// out holds the file text after the BOM strip.
	static int ReadFileText(const char* file, std::string& out)
	{
		FILE* fp = NULL;
		if (fopen_s(&fp, file, "rb") != 0 || fp == NULL)
			return 1;
		fseek(fp, 0, SEEK_END);
		const long size = ftell(fp);
		fseek(fp, 0, SEEK_SET);
		if (size <= 0)
		{
			fclose(fp);
			return 2;
		}
		if (size >= 4 * 1024 * 1024)
		{
			fclose(fp);
			return 3;
		}
		out.assign((size_t)size, '\0');
		const size_t read = fread(&out[0], 1, (size_t)size, fp);
		fclose(fp);
		out.resize(read);
		if (read >= 3 && (unsigned char)out[0] == 0xEF
			&& (unsigned char)out[1] == 0xBB && (unsigned char)out[2] == 0xBF)
		{
			out.erase(0, 3);
		}
		return 0;
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

		// Declared up here because the toolbar handlers below (clear, save)
		// reset them: the widget reports edits for the frame they happen in
		// (mTextChanged is cleared at the end of its own Render), so polling
		// it right after Render() and accumulating gives a session dirty
		// flag - true while the buffer holds edits the last load/save/clear
		// does not.  SetText() raises the same widget flag, hence the
		// one-frame suppression after our own load/clear calls.
		static bool dirty = false;
		static int suppress = 0;

		// Mouse-reachable editor actions.  The Ctrl+Z/Y/C/V shortcuts work
		// inside the editor, but with keyboard-nav focus a button is the only
		// path, and icon buttons make the actions discoverable in the first
		// place.  The tooltips name the shortcut each button mirrors.
		const float tb = Settings::Px(26.0f);
		// IsClipboardFormatAvailable needs no OpenClipboard, and the buffer is
		// only sampled once per frame here, so both checks stay cheap.
		const bool clipAvail = ::IsClipboardFormatAvailable(CF_UNICODETEXT) != 0
			|| ::IsClipboardFormatAvailable(CF_TEXT) != 0;
		const bool bufEmpty = editor.GetText().empty();
		// The actions that need something to work on now say so: on a fresh
		// buffer undo/redo/copy are greyed instead of clicking into a no-op, paste
		// waits for clipboard text and clear for buffer text - the
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
		ImGui::BeginDisabled(!clipAvail);
		if (ImGui::Button(ICON_FA_PASTE "##ed_paste", ImVec2(tb, tb)))
			editor.Paste();
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip(clipAvail ? "Paste (Ctrl+V)" : "Clipboard is empty");
		ImGui::SameLine();
		ImGui::BeginDisabled(bufEmpty);
		if (ImGui::Button(ICON_FA_ERASER "##ed_clear", ImVec2(tb, tb)))
		{
			// SetText wipes the undo buffer, so a mis-click on an unsaved
			// buffer is unrecoverable - ask first whenever there is anything
			// to lose, and drop the file reference with the text.
			const std::string cur = editor.GetText();
			const bool go = cur.empty()
				|| MessageBoxA(NULL, (std::string("Discard the current buffer (")
					+ std::to_string(cur.size())
					+ " bytes)?\nThis cannot be undone.").c_str(),
					"EBengin", MB_OKCANCEL | MB_ICONQUESTION) == IDOK;
			if (go)
			{
				editor.SetText("");
				g_lastFile.clear();
				dirty = false;
				suppress = 1;
				if (!cur.empty())
					Activity::Push("Editor buffer cleared ("
						+ std::to_string(cur.size()) + " bytes).");
			}
			else
				Activity::Push("Clear cancelled - buffer kept.");
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip(bufEmpty ? "Nothing to clear" : "Clear the buffer");
		ImGui::SameLine();
		// Save grew a fast path: once a file reference exists the button writes
		// straight to it - the dialog becomes Save As, reached with Shift+click or
		// on a first save - and Ctrl+S mirrors the button for keyboard flow.  The
		// write itself lives in WriteFileText above, so both call sites share one
		// open/write/short-write path the way Load and Revert share ReadFileText.
		auto saveBuffer = [&]()
		{
			std::string target = g_lastFile;
			if (target.empty() || ImGui::GetIO().KeyShift)
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
				if (!GetSaveFileNameA(&ofn))
					return;
				target = file;
			}
			const std::string text = editor.GetText();
			size_t written = 0;
			const int rc = WriteFileText(target.c_str(), text, written);
			if (rc == 1)
			{
				MessageBoxA(NULL, (std::string("Could not write:\n") + target).c_str(),
					"EBengin", MB_OK | MB_ICONERROR);
				Activity::Push(std::string("Save failed: ") + target);
				Toasts::Push(Toasts::Warn, std::string("Save failed: ") + target);
			}
			else if (rc == 2)
			{
				MessageBoxA(NULL, (std::string("Write stopped after "
					+ std::to_string(written) + " of "
					+ std::to_string(text.size()) + " bytes:\n") + target).c_str(),
					"EBengin", MB_OK | MB_ICONERROR);
				Activity::Push(std::string("Save incomplete: ") + target);
				Toasts::Push(Toasts::Warn, "Save incomplete - disk full?");
			}
			else
			{
				g_lastFile = target;
				dirty = false;
				Activity::Push("Saved " + std::to_string(written)
					+ " bytes to " + target + ".");
				Toasts::Push(Toasts::Success, "Saved "
					+ std::to_string(written) + " bytes.");
			}
		};
		if (ImGui::Button(ICON_FA_SAVE "##ed_save", ImVec2(tb, tb)))
			saveBuffer();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip(g_lastFile.empty() ? "Save the buffer to a file (Ctrl+S)"
				: ("Save to " + BaseName(g_lastFile)
					+ " (Ctrl+S; Shift+click or Ctrl+Shift+S: save as...)").c_str());
		// Ctrl+S: the editor owns Ctrl+Z/Y/C/V but not S or O, so both shortcuts
		// are free here.  Repeat off, and tab-local because the checks live in
		// this tab.  Ctrl+Shift+S falls out of saveBuffer's KeyShift read.
		if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_S, false))
			saveBuffer();
		ImGui::SameLine();
		// Revert reloads the file behind the buffer - the counterpart to
		// the dirty mark: once it shows, one click takes the edits back.
		// Grey without a file reference; with one, it compares first, so an
		// unchanged buffer gets a plain info toast, not a scary question.
		ImGui::BeginDisabled(g_lastFile.empty());
		if (ImGui::Button(ICON_FA_HISTORY "##ed_revert", ImVec2(tb, tb)))
		{
			std::string fileText;
			const int rc = ReadFileText(g_lastFile.c_str(), fileText);
			if (rc != 0)
			{
				MessageBoxA(NULL, (std::string("Revert could not read:\n")
					+ g_lastFile).c_str(), "EBengin", MB_OK | MB_ICONERROR);
				Activity::Push(std::string("Revert failed: ") + g_lastFile);
				Toasts::Push(Toasts::Warn, "Revert failed - file unreadable.");
			}
			else if (fileText == editor.GetText())
			{
				dirty = false;
				Toasts::Push(Toasts::Info, "Buffer already matches the file.");
			}
			else
			{
				const bool go = !dirty
					|| MessageBoxA(NULL, (std::string("Discard the current edits and revert to\n")
						+ g_lastFile + "?").c_str(),
						"EBengin", MB_OKCANCEL | MB_ICONQUESTION) == IDOK;
				if (go)
				{
					editor.SetText(fileText);
					dirty = false;
					suppress = 1;
					Activity::Push("Reverted to " + g_lastFile + ".");
					Toasts::Push(Toasts::Success,
						"Reverted to " + BaseName(g_lastFile) + ".");
				}
				else
					Activity::Push("Revert cancelled - buffer kept.");
			}
		}
		ImGui::EndDisabled();
		if (ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled))
			ImGui::SetTooltip(g_lastFile.empty()
				? "Nothing to revert to - no file behind the buffer"
				: ("Revert to " + BaseName(g_lastFile)).c_str());
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

		if (editor.IsTextChanged())
		{
			if (suppress > 0)
				--suppress;
			else
				dirty = true;
		}

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
		if (dirty)
			statusLine += "  |  modified";
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
		// Ctrl+O mirrors the Load button for keyboard flow, the way Ctrl+S
		// mirrors Save - and with Shift held, the same Save press routes through
		// the dialog as Save As, because saveBuffer already reads KeyShift.
		auto loadFile = [&]()
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
				std::string text;
				const int rc = ReadFileText(file, text);
				if (rc == 1)
				{
					MessageBoxA(NULL, (std::string("Could not open:\n") + file).c_str(),
						"EBengin", MB_OK | MB_ICONERROR);
					Activity::Push(std::string("Load failed: ") + file);
				}
				else if (rc == 2)
				{
					MessageBoxA(NULL, "The selected file is empty.",
						"EBengin", MB_OK | MB_ICONERROR);
					Activity::Push(std::string("Load failed (empty): ") + file);
				}
				else if (rc == 3)
				{
					MessageBoxA(NULL, "The selected file is larger than 4 MB and was not loaded.",
						"EBengin", MB_OK | MB_ICONERROR);
					Activity::Push(std::string("Load refused (>4 MB): ") + file);
				}
				else
				{
					// Same lossiness as Clear: SetText wipes undo, so replacing
					// a non-empty buffer throws away whatever was not saved.
					const std::string curBuf = editor.GetText();
					const bool replace = curBuf.empty()
						|| MessageBoxA(NULL, (std::string("Replace the current buffer (")
							+ std::to_string(curBuf.size()) + " bytes) with\n"
							+ file + "?\nUnsaved changes cannot be undone.").c_str(),
							"EBengin", MB_OKCANCEL | MB_ICONQUESTION) == IDOK;
					if (replace)
					{
						editor.SetText(text);
						g_lastFile = file;
						dirty = false;
						suppress = 1;
						// Reported size is what the editor holds, i.e. after the BOM
						// strip, not what the file occupied on disk.
						Activity::Push("Loaded " + std::string(file) + " ("
							+ std::to_string(text.size()) + " bytes) into the editor.");
						Toasts::Push(Toasts::Success, "Loaded " + std::string(file)
							+ " (" + std::to_string(text.size()) + " bytes).");
					}
					else
					{
						Activity::Push(std::string("Load cancelled - buffer kept: ") + file);
						Toasts::Push(Toasts::Info, "Load cancelled - buffer kept.");
					}
				}
			}
		};
		if (ImGui::Button(ICON_FA_FILE " Load from File",
			ImVec2(Settings::Px(180.0f), Settings::Px(30.0f))))
			loadFile();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Load from File (Ctrl+O)");
		if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_O, false))
			loadFile();

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

// The write path, single-sourced the way the read path is (ReadFileText
// above): open, fwrite, short-write check.  Returns 0 on success, 1 when the
// file would not open, 2 when the write stopped early - `written` then holds
// how far it got, for the message.
static int WriteFileText(const char* file, const std::string& text, size_t& written)
{
	written = 0;
	FILE* fp = NULL;
	if (fopen_s(&fp, file, "wb") != 0 || fp == NULL)
		return 1;
	written = fwrite(text.data(), 1, text.size(), fp);
	fclose(fp);
	return written == text.size() ? 0 : 2;
}
