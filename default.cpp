#include "default.h"
#include "gui.h"
#include "activity.h"

#include "imgui/imgui.h"

namespace Default
{
	void Render()
	{
		Gui::SectionTitle("Default", "##default_seperator_1");
		ImGui::Text("No section has been selected. Click a button on the left navbar.");

		// Session action log: actions whose only other feedback is a message box
		// (loads, dumps, copies, stop/run requests) leave a timestamped trace
		// here, newest first.
		ImGui::Spacing();
		Gui::Seperator("##default_seperator_2");
		ImGui::Text("Session activity");

		const std::vector<std::string>& entries = Activity::Entries();
		if (entries.empty())
		{
			ImGui::TextColored(ImColor(93, 156, 214, 153), "Nothing yet this session.");
		}
		else
		{
			ImGui::BeginChild("##activity",
				ImVec2(ImGui::GetContentRegionAvail().x, ImGui::GetContentRegionAvail().y),
				ImGuiChildFlags_None);
			for (size_t i = 0; i < entries.size(); ++i)
				ImGui::TextWrapped("%s", entries[i].c_str());
			ImGui::EndChild();
		}
	}
}
