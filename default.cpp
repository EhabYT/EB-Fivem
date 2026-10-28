#include "default.h"
#include "gui.h"
#include "activity.h"
#include "clipboard.h"
#include "toasts.h"
#include "perf.h"

#include <cstdio>
#include <string>

#include "imgui/imgui.h"

namespace Default
{
	void Render()
	{
		Gui::SectionTitle("Default", "##default_seperator_1");
		ImGui::Text("No section has been selected. Click a button on the left navbar.");

		// Diagnostics: the history behind the sidebar's instantaneous FPS line -
		// UI: row 199 - "session uptime" overstated the counter: Sample() ticks
		// only while the overlay renders, so it is menu-open time, and the label
		// below already says "Menu open".  Comment and README now agree with it.
		// 120 frames of frame time, menu-open uptime and the action count.
		ImGui::Spacing();
		Gui::Seperator("##default_seperator_3");
		ImGui::Text("Diagnostics");
		Perf::DrawGraph(ImVec2(ImGui::GetContentRegionAvail().x, Settings::Px(60.0f)));
		char diag[128];
		snprintf(diag, sizeof(diag), "Menu open %s  |  %u actions  |  %.0f FPS now",
			Perf::UptimeString(), Activity::TotalCount(), ImGui::GetIO().Framerate);
		ImGui::TextColored(ColMuted, "%s", diag);

		// Session action log: actions whose only other feedback is a message box
		// (loads, dumps, copies, stop/run requests) leave a timestamped trace
		// here, newest first.
		ImGui::Spacing();
		Gui::Seperator("##default_seperator_2");

		const std::vector<std::string>& entries = Activity::Entries();
		ImGui::Text("Session activity");
		if (!entries.empty())
		{
			// Activity::Clear() existed since the log was introduced but
			// nothing called it - the log could only grow until it capped.
			ImGui::SameLine();
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ImGui::GetContentRegionAvail().x
			- Settings::Px(50.0f) - Settings::Px(4.0f) - Settings::Px(60.0f));
			if (ImGui::Button("Copy##activity", ImVec2(Settings::Px(50.0f), Settings::Px(20.0f))))
			{
				// The log is the only timestamped trace the session leaves -
				// failures, refusals, file operations.  Copy takes it along as
				// displayed (newest first); retyping it was the lossy path.
				std::string joined;
				for (size_t i = 0; i < entries.size(); ++i)
				{
					if (!joined.empty())
						joined += "\r\n";
					joined += entries[i];
				}
				const unsigned n = (unsigned)entries.size();
				if (CopyTextToClipboard(joined.c_str()))
				{
					Toasts::Push(Toasts::Success, "Activity log copied ("
						+ std::to_string(n) + " entries).");
					Activity::Push("Activity log copied to the clipboard.");
				}
				else
				{
					Toasts::Push(Toasts::Warn, "Clipboard unavailable - log not copied.");
					Activity::Push("Clipboard unavailable - log not copied.");
				}
			}
			ImGui::SameLine();
			if (ImGui::Button("Clear##activity", ImVec2(Settings::Px(60.0f), Settings::Px(20.0f))))
			{
				// UI: row 204 - the session log is the only timestamped trace the
				// session leaves (the Copy comment above says so), in memory only
				// and unrecoverable once cleared - the same loss family the
				// editor Clear/Revert, the stopper and the interface reset all
				// confirm for.  One mis-click used to erase it without a word.
				if (Gui::ModalBoxA("Clear the session log?\nThe trace of this session cannot be recovered.",
						"EBengin", MB_YESNO | MB_ICONWARNING) == IDYES)
					Activity::Clear();
			}
		}
		if (entries.empty())
		{
			ImGui::TextColored(ColMuted, "Nothing yet this session.");
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
