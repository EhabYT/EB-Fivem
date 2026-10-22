#include "default.h"
#include "gui.h"

#include "imgui/imgui.h"

namespace Default
{
	void Render()
	{
		ImGui::SetCursorPosX(ImGui::GetContentRegionAvail().x / 1.3f);
		ImGui::Text("Default");
		Gui::Seperator("##default_seperator_1");
		ImGui::Text("No section has been selected. Click a button on the left navbar.");
	}
}
