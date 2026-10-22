#include "config.h"

namespace Config
{
	bool Safemode = true;
	bool ShBypass = true;

	void Render()
	{
		ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 1.3f);
		ImGui::Text("Config");
		Gui::Seperator("##config_seperator_0");

		ImGui::SetCursorPosX(233);
		ImGui::BeginChild("##config_side", ImVec2(ImGui::GetWindowWidth() / 3.0f, ImGui::GetWindowHeight()), false);

		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 83);
		ImGui::Text("Safe Mode");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(255, 0, 0, 255), "(?)"); ImGui::Spacing();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Safe mode creates a private scripting environment.");

		Gui::Seperator("##config_seperator_1", ImGui::GetWindowWidth());

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));
		ImGui::Checkbox("Enabled##safemode", &Safemode);
		ImGui::PopStyleVar();

		Gui::Seperator("##config_seperator_2", ImGui::GetWindowWidth());
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 57);
		ImGui::Text("ScriptHook Bypass");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(255, 0, 0, 255), "(?)"); ImGui::Spacing();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Loads .ASI plugins through the ScriptHook interface.");

		Gui::Seperator("##config_seperator_3", ImGui::GetWindowWidth());
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));
		ImGui::Checkbox("Enabled##shbypass", &ShBypass);
		ImGui::PopStyleVar();

		Gui::Seperator("##config_seperator_4", ImGui::GetWindowWidth());

		ImGui::EndChild();
		ImGui::SameLine();
		Gui::Seperator("##splitter", 1, ImGui::GetWindowHeight());
		ImGui::SameLine();
		ImGui::BeginChild("##server_side", ImVec2(ImGui::GetWindowWidth(), ImGui::GetWindowHeight()), false);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 58);
		ImGui::Text("Server related stuff");
		ImGui::Spacing();
		Gui::Seperator("##server_seperator_1", ImGui::GetWindowWidth());
		// Was a hardcoded "127.0.0.1" string; nothing in this build connects to a
		// server, so don't imply that it does.
		ImGui::TextColored(ImColor(160, 160, 160, 255), "Server IP: not connected");
		Gui::Seperator("##server_seperator_2", ImGui::GetWindowWidth());
		ImGui::EndChild();
	}
}
