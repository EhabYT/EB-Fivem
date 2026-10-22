#include "config.h"

namespace Config
{
	bool Safemode = true;
	bool ShBypass = true;

	void Render()
	{
		Gui::SectionTitle("Config", "##config_seperator_0");

		// GetWindowWidth()/GetWindowHeight() report the *outer* window (900x540),
		// not this table cell, so the panes were sized from the wrong box: the
		// right-hand child came out 900px wide and ran off the window, and both
		// children were a full window taller than the cell.  SetCursorPosX(233)
		// then shoved the left pane another 233px to the right on top of that.
		// Measure the cell instead.
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		const float leftWidth = avail.x / 3.0f;

		ImGui::BeginChild("##config_side", ImVec2(leftWidth, avail.y), ImGuiChildFlags_None);

		ImGui::Text("Safe Mode");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(255, 0, 0, 255), "(?)"); ImGui::Spacing();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Safe mode creates a private scripting environment.");

		Gui::Seperator("##config_seperator_1");

		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));
		ImGui::Checkbox("Enabled##safemode", &Safemode);
		ImGui::PopStyleVar();

		Gui::Seperator("##config_seperator_2");
		ImGui::Text("ScriptHook Bypass");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(255, 0, 0, 255), "(?)"); ImGui::Spacing();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Loads .ASI plugins through the ScriptHook interface.");

		Gui::Seperator("##config_seperator_3");
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(10, 7));
		ImGui::Checkbox("Enabled##shbypass", &ShBypass);
		ImGui::PopStyleVar();

		Gui::Seperator("##config_seperator_4");

		ImGui::EndChild();
		ImGui::SameLine();
		Gui::Seperator("##splitter", 1, avail.y);
		ImGui::SameLine();
		ImGui::BeginChild("##server_side", ImVec2(ImGui::GetContentRegionAvail().x, avail.y), ImGuiChildFlags_None);
		ImGui::Text("Server related stuff");
		ImGui::Spacing();
		Gui::Seperator("##server_seperator_1");
		// Was a hardcoded "127.0.0.1" string; nothing in this build connects to a
		// server, so don't imply that it does.
		ImGui::TextColored(ImColor(160, 160, 160, 255), "Server IP: not connected");
		Gui::Seperator("##server_seperator_2");
		ImGui::EndChild();
	}
}
