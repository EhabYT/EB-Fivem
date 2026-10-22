#include "default.h"
#include "gui.h"

#include "imgui/imgui.h"

namespace Default
{
	void Render()
	{
		Gui::SectionTitle("Default", "##default_seperator_1");
		ImGui::Text("No section has been selected. Click a button on the left navbar.");
	}
}
