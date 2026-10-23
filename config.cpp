#include "config.h"
#include "activity.h"
#include "settings.h"
#include "toasts.h"
#include "exec.h"      // Executor::ApplyPrefs() for the settings reset

#include <Windows.h>
#include <string>

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

		// Both sections below are inert.  Safemode/ShBypass are UI state only (see
		// config.h): nothing in this build reads them and nothing persists them.
		// The labels and tooltips used to describe behaviour that does not exist
		// ("a private scripting environment", ".ASI plugins through the ScriptHook
		// interface"), which reads as two working modes - the same defect the dead
		// Execute/Run buttons had.  So the heading is greyed, the switch itself says
		// "(no effect)", the tooltip spells it out, and every flip is answered with
		// an activity entry plus a warn toast.
		//
		// The wording has to stay short: this pane is a third of the content column
		// (~216px at the 900px shell) and ImGui::Text does not wrap, so
		// "Safe Mode   (not implemented)" (~270px) was silently clipped at the pane
		// edge.  docs/render_preview.py measures these strings and asserts the fit.
		ImGui::TextDisabled("Safe Mode");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(93, 156, 214, 255), "(?)"); ImGui::Spacing();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Inert in this build: the switch only stores a value\n"
				"that nothing reads, and it is not written to settings.ini.");

		Gui::Seperator("##config_seperator_1");

		if (Gui::Toggle("Enabled (no effect)##safemode", &Safemode))
		{
			Activity::Push(std::string("Safe Mode switched ") + (Safemode ? "on" : "off")
				+ " (not implemented - no effect).");
			Toasts::Push(Toasts::Warn, "Safe Mode is not implemented; the switch has no effect.");
		}

		Gui::Seperator("##config_seperator_2");
		ImGui::TextDisabled("ScriptHook Bypass");
		ImGui::SameLine();
		ImGui::TextColored(ImColor(93, 156, 214, 255), "(?)"); ImGui::Spacing();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Inert in this build: no .ASI plugin is loaded; the\n"
				"switch only stores a value that nothing reads.");

		Gui::Seperator("##config_seperator_3");
		if (Gui::Toggle("Enabled (no effect)##shbypass", &ShBypass))
		{
			Activity::Push(std::string("ScriptHook Bypass switched ") + (ShBypass ? "on" : "off")
				+ " (not implemented - no effect).");
			Toasts::Push(Toasts::Warn, "ScriptHook Bypass is not implemented; the switch has no effect.");
		}

		Gui::Seperator("##config_seperator_4");

		// Interface preferences: which key toggles the overlay.  Click the button,
		// then press the key; ESC cancels.  Persisted with the rest of settings.
		ImGui::Text("Interface");
		Gui::Seperator("##config_seperator_5");

		static bool capturing = false;
		// "press any key...  (ESC cancels)" measured ~377px against a ~216px pane and
		// was clipped at the edge - so while capturing, the button never actually
		// told the user that ESC cancels, the one thing they need to know then.
		// Shortened to fit; the full sentence moved into the tooltip.
		const std::string label = capturing
			? std::string("press any key... (ESC)")
			: std::string("Toggle key:  ") + Settings::KeyName(Settings::Current.ToggleKey);
		if (ImGui::Button(label.c_str(), ImVec2(ImGui::GetContentRegionAvail().x, 30)))
			capturing = !capturing;
		if (capturing && ImGui::IsItemHovered())
			ImGui::SetTooltip("Press the key you want to bind.\nESC cancels the capture.");

		if (capturing)
		{
			// vk 1..6 are the mouse buttons and reserved entries.  Binding one
			// would flip the menu on every click - and the release edge of the
			// click that armed the capture used to bind VK_LBUTTON on the very
			// first capture frame.  Scan printable/keyboard keys only.
			for (int vk = 7; vk < 255; ++vk)
			{
				if (GetAsyncKeyState(vk) & 1)
				{
					if (vk != VK_ESCAPE)
					{
						Settings::Current.ToggleKey = vk;
						Settings::Save();
						Activity::Push(std::string("Toggle key set to ") + Settings::KeyName(vk));
						Toasts::Push(Toasts::Success, std::string("Toggle key set to ") + Settings::KeyName(vk));
					}
					capturing = false;
					break;
				}
			}
		}

		// Motion and feedback are taste, so both are switchable - and, like the
		// rest of the interface prefs, persisted in settings.ini ([ui]).
		ImGui::Spacing();
		if (Gui::Toggle("Animations##ui_anim", &Settings::Current.Animations))
			Settings::Save();
		if (Gui::Toggle("Toast notifications##ui_toasts", &Settings::Current.Toasts))
		{
			// Switching the stack off must also empty it: Menu::Render() skips
			// Toasts::Render() while the pref is false, so a card younger than ~4s
			// would otherwise reappear mid-fade when toasts are switched back on.
			if (!Settings::Current.Toasts)
				Toasts::Clear();
			Settings::Save();
		}

		// Interface scale, 70..200%.  Applied through ImGui 1.92's global font
		// scale factor (style.FontScaleMain), so the whole shell - fonts, frames,
		// sidebar, window size - grows together instead of text overflowing fixed
		// widgets.  Persisted as [ui] scale.
		//
		// Applied on release rather than while dragging: 1.92 bakes the font for
		// every distinct size it sees, so a live drag from 70 to 200 would bake
		// ~130 sizes into the atlas for nothing.
		ImGui::Spacing();
		ImGui::PushItemWidth(ImGui::GetContentRegionAvail().x);
		ImGui::SliderInt("##ui_scale", &Settings::Current.ScalePercent, 70, 200,
			"Interface scale %d%%");
		const bool scaleDone = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::PopItemWidth();
		if (ImGui::IsItemHovered())
			ImGui::SetTooltip("Font and layout scale - applies when you release the slider");
		if (scaleDone)
		{
			Settings::ApplyScale();
			Settings::Save();
			const std::string pct = std::to_string(Settings::Current.ScalePercent) + "%";
			Activity::Push("Interface scale set to " + pct);
			Toasts::Push(Toasts::Info, "Interface scale set to " + pct + ".");
		}

		ImGui::Spacing();
		// TextColored does not wrap, and this hint is ~448px against a ~216px pane:
		// it was clipped after "Tab/arrows".  There is no TextWrappedColored, so the
		// colour goes through the style stack.  The cast is required - ImColor
		// converts to both ImU32 and ImVec4, which PushStyleColor would find
		// ambiguous.
		ImGui::PushStyleColor(ImGuiCol_Text, (ImVec4)ImColor(93, 156, 214, 153));
		ImGui::TextWrapped("Keyboard: Tab/arrows move, Enter/Space activates.");
		ImGui::PopStyleColor();

		ImGui::Spacing();
		if (Gui::PrimaryButton("Reset interface settings", ImVec2(ImGui::GetContentRegionAvail().x, 30)))
		{
			// One click back to the compiled defaults - before this, a bad key
			// bind or a window dragged off-screen meant hand-editing or
			// deleting settings.ini.  Window position and tab apply on the next
			// start (the position is restored with ImGuiCond_Once); the key,
			// the switches, the scale and the whitespace dot take effect at once.
			Settings::Current = Settings::Prefs();
			Settings::Save();
			// Two of the prefs also live somewhere else at runtime - the scale in
			// ImGui's style and the whitespace flag inside the TextEditor widget -
			// so writing the struct alone is not enough.  Push both out again,
			// otherwise the shell keeps its old size and the dots stay on screen
			// until the toolbar switch is clicked.
			Settings::ApplyScale();
			Executor::ApplyPrefs();
			Activity::Push("Interface settings reset to defaults.");
			Toasts::Push(Toasts::Info, "Interface settings reset; window position applies on next start.");
		}

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
		ImGui::TextColored(ImColor(93, 156, 214, 153), "Server IP: not connected");
		Gui::Seperator("##server_seperator_2");
		ImGui::EndChild();
	}
}
