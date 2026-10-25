#include "menu.h"
#include "tabs.h"
#include "gui.h"
#include "font_awesome.h"
#include "settings.h"
#include "toasts.h"
#include "perf.h"

#include <cstdio>
#include <cmath>
#include <string>

namespace Menu
{
	struct SidebarItem
	{
		const char* label;
		int         tab;
	};

	// Sidebar layout, kept in one place so the buttons and the separators cannot
	// drift apart (they used to be 5 near-identical copy-pasted blocks, each with
	// its own hand-maintained cursor offsets).
	// Set when the render-call stream gaps (menu was closed, just reopened).
	// Consumed once by RenderSidebar() to hand keyboard-nav focus to the
	// sidebar entry of the open tab.
	static bool g_justOpened = false;
	// The branded start card shows on the first open after inject, once.
	static bool g_splashDone = false;

	static const SidebarItem SidebarItems[] =
	{
		{ ICON_FA_CODE     " Executor", TabExecutor  },
		{ ICON_FA_DOWNLOAD " Resources", TabResources },
		{ ICON_FA_BOOK     " Menus",     TabMenus     },
		{ ICON_FA_COGS     " Config",    TabConfig    },
	};

	// Draws the left-hand column: logo, the tab buttons, and the footer text.
	// File-local - nothing outside this translation unit has a reason to call it.
	// Same as CenteredInSidebar but in a chosen colour (the FPS readout).
	static void CenteredInSidebarColored(const char* text, ImU32 col)
	{
		const float w = ImGui::CalcTextSize(text).x;
		ImGui::SetCursorPosX(ImMax(0.0f, (SidebarWidth() - w) * 0.5f));
		ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s", text);
	}

	// Draws one line of the sidebar footer centred in the sidebar column.
	static void CenteredInSidebar(const char* text)
	{
		const float w = ImGui::CalcTextSize(text).x;
		ImGui::SetCursorPosX(ImMax(0.0f, (SidebarWidth() - w) * 0.5f));
		ImGui::Text("%s", text);
	}

	static void RenderSidebar()
	{
		// Nav rail: the sidebar cell sits one notch darker than the canvas (WindowBg
		// darkened 18%), so the panel buttons read as a rail instead of floating
		// blocks.  CellBg is painted under the cell content, so no widget is covered.
		ImGui::TableSetBgColor(ImGuiTableBgTarget_CellBg, ImColor(16, 31, 52, 255));

		// Animated selected-tab marker: current bar rect (kept across frames)
		// and this frame's target, i.e. the selected entry's button rect.
		static float indX = 0.0f, indY = 0.0f, indH = 0.0f;
		float tgtX = indX, tgtY = indY, tgtH = 0.0f;

		ImGui::SetCursorPosX(74);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 9);
		if (LogoLoaded)
		{
			// ImTextureID became ImU64/ImTextureRef in 1.92: cast the SRV explicitly.
			ImGui::Image((ImTextureID)(intptr_t)logo,
				ImVec2(Settings::Px(76.0f), Settings::Px(87.0f)));
		}
		else
		{
			// EBengin.png was not found next to the DLL: keep the sidebar the same height.
			ImGui::SetCursorPosX(58);
			ImGui::TextColored(ImColor(93, 156, 214, 255), "EBengin");
			ImGui::Dummy(ImVec2(Settings::Px(76.0f), Settings::Px(68.0f)));
		}
		// UI: row 150 - once a sidebar tab was open, the landing tab (diagnostics
		// + session activity) had no way back for the whole session; the logo is
		// the home button now.  Works for the text-logo fallback too, since the
		// Dummy/Text above are items as well.
		if (ImGui::IsItemHovered())
		{
			ImGui::SetMouseCursor(ImGuiMouseCursor_Hand);
			ImGui::SetTooltip("Back to the landing tab.");
		}
		if (ImGui::IsItemClicked())
		{
			Tab = TabDefault;
			Settings::Current.Tab = Tab;
			Settings::Save();
		}
		ImGui::NewLine();

		for (size_t i = 0; i < IM_ARRAYSIZE(SidebarItems); ++i)
		{
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
			ImGui::SetCursorPosX(0);
			Gui::Seperator(std::string("##main_seperator_") + std::to_string(i + 1), SidebarWidth(), 1.0f);
			ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 3);
			ImGui::SetCursorPosX(0);

			// Nothing used to mark the open tab: every entry drew with
			// ImGuiCol_Button and only the transient hover/press colour differed,
			// so after a click the sidebar looked exactly as before and the user
			// could not tell which tab was open.  At startup Tab is TabDefault,
			// which is not in the sidebar at all, so nothing was marked then
			// either.  The selected entry now holds the pressed colour and gets a
			// 3px accent bar on its left edge.
			const bool active = (Tab == SidebarItems[i].tab);
			if (active)
				ImGui::PushStyleColor(ImGuiCol_Button, ImGui::GetStyleColorVec4(ImGuiCol_ButtonActive));

			// Place the nav cursor on the entry of the tab that is actually
			// open; the landing tab has no sidebar row, so it gets the first.
			if (g_justOpened && (active || (Tab == TabDefault && i == 0)))
				ImGui::SetKeyboardFocusHere();

			if (ImGui::Button(SidebarItems[i].label, ImVec2(SidebarWidth(), SidebarButtonHeight())))
			{
				Tab = SidebarItems[i].tab;
				Settings::Current.Tab = Tab;
				Settings::Save();
			}

			// The 3px marker no longer jumps straight to the clicked entry: its
			// target rect is recorded here and the bar below eases towards it.
			if (active)
			{
				const ImVec2 min = ImGui::GetItemRectMin();
				tgtX = min.x;
				tgtY = min.y;
				tgtH = SidebarButtonHeight();
				// Soft accent wash fading left -> right over the pressed colour:
				// ~9% alpha at the marker edge, gone by the label end, so the row
				// reads lit from the side without tinting the label.
				ImGui::GetWindowDrawList()->AddRectFilledMultiColor(min,
					ImGui::GetItemRectMax(),
					ImColor(93, 156, 214, 22), ImColor(93, 156, 214, 0),
					ImColor(93, 156, 214, 22), ImColor(93, 156, 214, 0));
				ImGui::PopStyleColor();
			}
		}

		// With animations off (Config -> Interface) k is 1 and the bar lands in
		// one frame, i.e. exactly the old behaviour.
		const float dt = ImGui::GetIO().DeltaTime;
		const float k = Settings::Current.Animations
			? (dt * 14.0f < 1.0f ? dt * 14.0f : 1.0f) : 1.0f;
		if (indH <= 0.5f && tgtH > 0.0f)
		{
			indX = tgtX;   // appearing: snap to the entry instead of flying in
			indY = tgtY;
		}
		indX += (tgtX - indX) * k;
		indY += (tgtY - indY) * k;
		indH += (tgtH - indH) * k;
		if (indH > 0.5f)
			ImGui::GetWindowDrawList()->AddRectFilled(
				ImVec2(indX, indY), ImVec2(indX + 3.0f, indY + indH), Accent);

		g_justOpened = false;   // handoff done, however it resolved

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() - 4);
		ImGui::SetCursorPosX(0);
		Gui::Seperator("##main_seperator_5", SidebarWidth(), 1.0f);

		// The footer was pinned to SetCursorPosY(380) and (399): 19px apart, less
		// than one 20px text line, so the two strings drew on top of each other,
		// and both were tied to one particular window height.  Anchor the block to
		// the bottom of the sidebar and centre each line in the column instead.
		char buildDate[64];
		snprintf(buildDate, sizeof(buildDate), "Build Date: %s", __DATE__);

		ImGui::SetCursorPosY(ImGui::GetWindowHeight() - ImGui::GetStyle().WindowPadding.y
			- 4.0f * ImGui::GetTextLineHeightWithSpacing());

		// Live frame rate / frame time from ImGui's own smoothed counter, so the
		// overlay can answer "is it the menu or the game?" without a profiler.
		const float fps = ImGui::GetIO().Framerate;
		char fpsLine[48];
		snprintf(fpsLine, sizeof(fpsLine), "FPS %.0f  |  %.1f ms",
			fps, fps > 0.0f ? 1000.0f / fps : 0.0f);
		CenteredInSidebarColored(fpsLine, Accent);

		CenteredInSidebar("Licensed to #4278");
		CenteredInSidebar(buildDate);

		// Visible brand credit, same wording as the source comment.
		CenteredInSidebarColored("Remade : by EB", ColMuted);

		// The old layout ended with
		//     SetCursorPosY(GetWindowHeight() - 10); Text("");
		// whose only job was to stretch the legacy Columns() cell to the full
		// window height ("so don't remove :)").  A table row is already as tall as
		// its tallest cell, so that hack is gone.
	}

	// Clamps a window position so that at least an 80px strip - enough of the
	// title bar to grab - stays inside the virtual screen, for a shell of the
	// given size.  Shared by the restore path and the scale-change guard below:
	// both answer the same question ("is this window still reachable?") about a
	// window the user cannot resize.
	static void ClampToVirtualScreen(float& x, float& y, float w, float h)
	{
		const float vx = (float)GetSystemMetrics(SM_XVIRTUALSCREEN);
		const float vy = (float)GetSystemMetrics(SM_YVIRTUALSCREEN);
		const float vw = (float)GetSystemMetrics(SM_CXVIRTUALSCREEN);
		const float vh = (float)GetSystemMetrics(SM_CYVIRTUALSCREEN);
		if (vw <= 0.0f || vh <= 0.0f)
			return;   // no usable metrics (headless / RDP startup): leave it alone
		const float strip = 80.0f;
		if (x < vx - w + strip) x = vx - w + strip;
		if (x > vx + vw - strip) x = vx + vw - strip;
		if (y < vy - h + strip) y = vy - h + strip;
		if (y > vy + vh - strip) y = vy + vh - strip;
	}

	// Branded start card in front of the shell: shown on the first open after
	// inject until the user presses Continue.  Pure presentation - logo, name,
	// credit and two honest hints; it touches no state beyond g_splashDone.
	static void RenderSplash(float fade)
	{
		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);

		const ImVec2 disp = ImGui::GetIO().DisplaySize;
		const ImVec2 card(Settings::Px(460.0f), Settings::Px(288.0f));
		ImGui::SetNextWindowSize(card, ImGuiCond_Always);
		ImGui::SetNextWindowPos(ImVec2(disp.x * 0.5f, disp.y * 0.5f),
			ImGuiCond_Always, ImVec2(0.5f, 0.5f));

		// Keyboard users: the shell hands focus over with SetNextWindowFocus
		// when it opens; while the splash replaces the shell it does the same
		// handoff, so Continue is reachable by keyboard from the first frame.
		// g_justOpened stays set: the sidebar still places the nav cursor on
		// the open tab on the shell's first frame after Continue.
		if (g_justOpened)
			ImGui::SetNextWindowFocus();

		ImGui::Begin("EBengin Splash", NULL,
			ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize |
			ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoScrollbar |
			ImGuiWindowFlags_NoSavedSettings);

		auto Centered = [](const char* text, ImU32 col)
		{
			const float w = ImGui::CalcTextSize(text).x;
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
				ImMax(0.0f, (ImGui::GetContentRegionAvail().x - w) * 0.5f));
			ImGui::TextColored(ImGui::ColorConvertU32ToFloat4(col), "%s", text);
		};

		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + Settings::Px(18.0f));
		if (LogoLoaded)
		{
			const float lw = Settings::Px(110.0f);
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
				ImMax(0.0f, (ImGui::GetContentRegionAvail().x - lw) * 0.5f));
			// 110 x 127 keeps the 76:87 aspect of EBengin.png.
			ImGui::Image((ImTextureID)(intptr_t)logo,
				ImVec2(lw, Settings::Px(127.0f)));
		}
		else
		{
			Centered("EBengin", ImColor(93, 156, 214, 255));
		}

		Centered("Remade : by EB", ColMuted);

		ImGui::Spacing();
		Gui::Seperator("##splash_seperator");
		ImGui::Spacing();

		Centered("Interface-level: no game state is read or written.",
			ImColor(255, 255, 255, 255));
		// The toggle key is user-bindable (Config -> Toggle key), so the hint
		// names whatever is actually bound; KeyName is the same helper the
		// Config button and the key-change toast use.
		char hint2[96];
		snprintf(hint2, sizeof(hint2), "%s toggles this overlay at any time.",
			Settings::KeyName(Settings::Current.ToggleKey));
		Centered(hint2, ImColor(255, 255, 255, 255));

		ImGui::Spacing();
		const float bw = Settings::Px(140.0f);
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() +
			ImMax(0.0f, (ImGui::GetContentRegionAvail().x - bw) * 0.5f));
		if (Gui::PrimaryButton("Continue", ImVec2(bw, Settings::Px(30.0f))))
			g_splashDone = true;

		ImGui::End();
		ImGui::PopStyleVar();
	}

	// FPS / frame-time HUD over the game scene while the overlay is closed: the
	// same string as the sidebar footer line, drawn on the background draw list
	// (over the scene, under any ImGui window) with a dark plate behind it so it
	// stays legible on bright frames.  Interface-level diagnostics only - the
	// numbers are ImGui's own smoothed present-loop counters, not game state.
	void RenderHud()
	{
		const float fps = ImGui::GetIO().Framerate;
		char line[48];
		snprintf(line, sizeof(line), "FPS %.0f  |  %.1f ms",
			fps, fps > 0.0f ? 1000.0f / fps : 0.0f);

		// The HUD is the one surface that stays visible while the overlay
		// is closed, so it is the one place the owner credit survives a
		// closed menu: second plate line, muted brand alpha like the
		// footer's credit line.
		const char* credit = "MADE BY EB";

		const ImVec2 ts = ImGui::CalcTextSize(line);
		const ImVec2 cs = ImGui::CalcTextSize(credit);
		const float w = ts.x > cs.x ? ts.x : cs.x;
		const float adv = ImGui::GetTextLineHeightWithSpacing();
		const float pad = Settings::Px(6.0f);
		// UI: row 148 - the plate used to be pinned top-left, exactly where game
		// HUDs (minimap, score strips) commonly sit; the corner is a setting now.
		const ImVec2 disp = ImGui::GetIO().DisplaySize;
		const float m = Settings::Px(10.0f);
		ImVec2 pos(m, m);
		switch (Settings::Current.HudCorner)
		{
		case 1: pos = ImVec2(disp.x - m - w, m); break;
		case 2: pos = ImVec2(m, disp.y - m - adv - cs.y); break;
		case 3: pos = ImVec2(disp.x - m - w, disp.y - m - adv - cs.y); break;
		default: break;
		}
		ImDrawList* bg = ImGui::GetBackgroundDrawList();
		bg->AddRectFilled(ImVec2(pos.x - pad, pos.y - pad),
			ImVec2(pos.x + w + pad, pos.y + adv + cs.y + pad),
			// UI: row 147 - alpha 160 let a bright game scene wash the plate out
			// (worst-case ~1.0:1 for the credit line); 220 keeps the scene visible
			// behind it while both plate lines hold WCAG AA worst-case.
			IM_COL32(12, 18, 28, 220), Settings::Px(6.0f));
		bg->AddText(ImGui::GetFont(), ImGui::GetFontSize(), pos,
			// UI: row 147 - the FPS line was brand accent (4.4:1 on the plate);
			// primary white gives 12.7:1 worst-case and keeps accent for the
			// interactive ramp only.
			ImColor(255, 255, 255, 255), line);
		bg->AddText(ImGui::GetFont(), ImGui::GetFontSize(),
			ImVec2(pos.x, pos.y + adv), ColMuted, credit);
	}

	void Render()
	{
		//Remade : by EB

		// The first-start notice toast cannot be pushed from Settings::Load():
		// toasts live four seconds and the overlay opens later than that, so
		// the toast expired unseen.  Load() leaves a flag instead; the first
		// frame the menu is actually visible consumes it.
		static bool firstStartChecked = false;
		if (!firstStartChecked)
		{
			firstStartChecked = true;
			if (Settings::ConsumeFirstStartNotice())
				Toasts::Push(Toasts::Info,
					"No settings.ini yet - running on compiled defaults.");
		}

		// Open fade.  Render() only runs while the overlay is open, so a gap in
		// the call stream means the menu was closed and has just come back.
		const double tNow = ImGui::GetTime();
		static double lastRender = -1.0;
		static double openT = -1.0;
		// 0.5s: far above any real frame hitch (which must not re-trigger the
		// fade mid-session) and far below any human close/reopen.
		if (lastRender < 0.0 || (tNow - lastRender) > 0.5)
		{
			openT = tNow;
			g_justOpened = true;
		}
		lastRender = tNow;

		// Frame-time history for the Diagnostics card on the landing tab.
		Perf::Sample(ImGui::GetIO().DeltaTime * 1000.0f);

		float fade = 1.0f;
		if (Settings::Current.Animations)
		{
			double t = (tNow - openT) / 0.22;
			if (t < 0.0)
				t = 0.0;
			if (t < 1.0)
			{
				const float u = (float)t;
				fade = 1.0f - (1.0f - u) * (1.0f - u) * (1.0f - u);   // ease-out cubic
			}
		}

		// First open per session: the branded start card replaces the shell
		// until dismissed; nothing else runs, so no state is touched.
		// Side effect worth keeping: toast ageing lives in Toasts::Render(),
		// which this skip bypasses - a warn toast queued at load (e.g. unwritable
		// settings.ini) therefore waits behind the card at full lifetime instead
		// of expiring unseen.  Do not "fix" that by ageing toasts here.
		if (!g_splashDone)
		{
			RenderSplash(fade);
			return;
		}

		// Soft drop shadow under the rounded window, drawn one frame late (the
		// rect is captured at the end of the previous frame) onto the background
		// list, which sits under every ImGui window but over the game scene.
		static ImVec2 shadowMin, shadowMax;
		if (shadowMax.x > shadowMin.x)
		{
			ImDrawList* bg = ImGui::GetBackgroundDrawList();
			const float off[3] = { 5.0f, 11.0f, 19.0f };
			const int   alp[3] = { (int)(70 * fade), (int)(46 * fade), (int)(26 * fade) };
			for (int i = 0; i < 3; ++i)
				bg->AddRectFilled(ImVec2(shadowMin.x - 2.0f, shadowMin.y + off[i] * 0.4f),
					ImVec2(shadowMax.x + 2.0f, shadowMax.y + off[i]),
					IM_COL32(0, 0, 0, alp[i]), 12.0f);
		}

		ImGui::PushStyleVar(ImGuiStyleVar_Alpha, fade);

		// The moment the menu opens it takes keyboard focus, so Tab/arrows work
		// immediately instead of first focusing whatever window was in front.
		if (g_justOpened)
			ImGui::SetNextWindowFocus();
		// NoCollapse/NoResize: the layout below is pixel-positioned, so a
		// user-resized or collapsed window just produced broken frames.
		// Restore the persisted position on the first frame only; afterwards the
		// window is free to move and its new position is saved below.
		if (Settings::Current.WindowX >= 0)
		{
			// A position persisted from another monitor setup (laptop undocked,
			// monitor swapped) can be entirely off-screen today, which would
			// leave the menu unreachable.
			float px = (float)Settings::Current.WindowX;
			float py = (float)Settings::Current.WindowY;
			ClampToVirtualScreen(px, py,
				ImGui::GetStyle().WindowMinSize.x, ImGui::GetStyle().WindowMinSize.y);
			ImGui::SetNextWindowPos(ImVec2(px, py), ImGuiCond_Once);
		}

		// Two affordances the shell used to lack.  p_open gives the title bar a
		// close button - with the toggle key as the only way out, a mouse-only
		// user had none - and the title names the open tab, so a alt-tabbed user
		// sees where they are.  The ### suffix keeps the window id stable across
		// tab switches, so ImGui state and the persisted position survive them.
		static const char* const titles[] = {
			"EBengin", "EBengin - Executor", "EBengin - Resources",
			"EBengin - Menus", "EBengin - Config",
		};
		char title[64];
		snprintf(title, sizeof(title), "%s###ebengin_shell",
			titles[Tab >= TabDefault && Tab <= TabConfig ? Tab : TabDefault]);
		ImGui::Begin(title, &IsMenuOpen,
			ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

		// Reachability guard for a shell that changes size mid-session.  Config's
		// interface scale resizes the window (900x540 at 100% -> 1800x1080 at
		// 200%), but the persisted position above is clamped only on the first
		// frame after open - so growing the shell could push its title bar off
		// screen, and with NoResize there would be nothing left to grab.  Re-clamp
		// whenever style.WindowMinSize differs from the previous frame, which is
		// exactly the one thing Settings::ApplyScale() writes: no Win32 calls in
		// the common frame, and no false triggers from tab switches (those change
		// the content, not the minimum size).
		//
		// The move is picked up by the position saver at the end of this function,
		// which is the point: the corrected position is the one worth persisting.
		static ImVec2 lastMinSize(0.0f, 0.0f);
		const ImVec2 minSize = ImGui::GetStyle().WindowMinSize;
		if (lastMinSize.x > 0.0f
			&& (minSize.x != lastMinSize.x || minSize.y != lastMinSize.y))
		{
			const ImVec2 shell = ImMax(ImGui::GetWindowSize(), minSize);
			ImVec2 live = ImGui::GetWindowPos();
			ClampToVirtualScreen(live.x, live.y, shell.x, shell.y);
			ImGui::SetWindowPos(live);
		}
		lastMinSize = minSize;

		// Thin brand rule under the title bar: accent -> pressed blue, L->R.
		{
			ImDrawList* dl = ImGui::GetWindowDrawList();
			const ImVec2 wp = ImGui::GetWindowPos();
			const float titleH = ImGui::GetCurrentWindowRead()->TitleBarHeight;
			const float ww = ImGui::GetWindowSize().x;
			dl->AddRectFilledMultiColor(ImVec2(wp.x, wp.y + titleH),
				ImVec2(wp.x + ww, wp.y + titleH + 2.0f),
				IM_COL32(93, 156, 214, 255), IM_COL32(29, 81, 136, 255),
				IM_COL32(29, 81, 136, 255), IM_COL32(93, 156, 214, 255));
		}

		// UI: row 150 - Ctrl+Tab / Ctrl+Shift+Tab cycle all five tabs, landing
		// included.  The editor consumes plain Tab for indent but leaves the
		// Ctrl chord alone (TextEditor checks !ctrl), so this stays safe while
		// the editor has focus.
		if (ImGui::GetIO().KeyCtrl && ImGui::IsKeyPressed(ImGuiKey_Tab, false))
		{
			const int dir = ImGui::GetIO().KeyShift ? -1 : 1;
			Tab = (Tab + dir + 5) % 5;
			Settings::Current.Tab = Tab;
			Settings::Save();
			// UI: row 152 - keyboard users cycling tabs keep the nav cursor on
			// the old tab's widget otherwise; the same flag the reopen path
			// uses hands focus to the new tab's sidebar entry next frame.
			g_justOpened = true;
		}

		// Two-column shell: fixed sidebar on the left, tab content on the right.
		//
		// This used to be the legacy Columns() API (ImGui::Columns / NextColumn /
		// SetColumnOffset), deprecated since 1.80 in favour of Tables.  Unlike
		// Columns(), a table gives every cell its own layout context, so
		// GetContentRegionAvail() inside a tab reports that cell rather than the
		// whole window.
		//
		//   BordersInnerV  - the divider between the two columns only, no outer
		//                    frame.  This is exactly what Columns(2, id, true)
		//                    drew.  (BordersV would add outer edges as well.)
		//   SizingFixedFit - required for WidthFixed to be honoured; without a
		//                    sizing policy the table would auto-fit to contents.
		//   NoPadOuterX    - content starts flush with the window edge, as before.
		//
		// style.CellPadding is set to (0,0) in ApplyStyle() so the 225px sidebar
		// stays exactly 225px of usable width, matching the old column semantics.
		if (ImGui::BeginTable("##maincolumn", 2,
			ImGuiTableFlags_SizingFixedFit | ImGuiTableFlags_BordersInnerV | ImGuiTableFlags_NoPadOuterX))
		{
			ImGui::TableSetupColumn("##sidebar", ImGuiTableColumnFlags_WidthFixed, SidebarWidth());
			ImGui::TableSetupColumn("##content", ImGuiTableColumnFlags_WidthStretch);

			ImGui::TableNextRow();

			ImGui::TableNextColumn();
			RenderSidebar();

			ImGui::TableNextColumn();
			// See Menu::ContentPadding(): keeps the tab bodies off the divider.
			ImGui::SetCursorPosX(ImGui::GetCursorPosX() + ContentPadding());
			Tabs::Render();

			ImGui::EndTable();
		}

		// Non-blocking feedback stack in the window's bottom-right corner.
		if (Settings::Current.Toasts)
			Toasts::Render();

		// Remember where the window ended up so the next session opens there -
		// but only once the user actually dragged it.  The old logic saved on
		// the first frame after open even for a window that never moved, which
		// was pointless at best: after Config -> "Reset interface settings"
		// (WindowX < 0) it re-persisted the current position within one frame
		// and silently undid the reset.  Captured before End() (after it there
		// is no current window to ask) and throttled to ~1/s plus a distance
		// threshold: writing an .ini every frame would be wasteful, and the
		// value is at most a second stale when the DLL unloads.
		const ImVec2 pos = ImGui::GetWindowPos();
		static ImVec2 lastPos(-1.0f, -1.0f);
		static ImVec2 lastSaved(-1.0f, -1.0f);
		static double lastSaveTime = -1.0;
		static bool armed = false;   // a real drag happened since the last reset
		const double now = ImGui::GetTime();
		if (Settings::Current.WindowX < 0)
			armed = false;           // never persisted / just reset: wait for a drag
		if (lastPos.x >= 0.0f &&
			(fabsf(pos.x - lastPos.x) > 4.0f || fabsf(pos.y - lastPos.y) > 4.0f))
			armed = true;            // moved between frames => user drag, not layout
		lastPos = pos;
		if (armed && ((int)pos.x != Settings::Current.WindowX || (int)pos.y != Settings::Current.WindowY))
		{
			const bool movedFar = fabsf(pos.x - lastSaved.x) > 40.0f || fabsf(pos.y - lastSaved.y) > 40.0f;
			if (lastSaveTime < 0.0 || (now - lastSaveTime) > 1.0 || movedFar)
			{
				Settings::SaveWindowPos(pos.x, pos.y);
				lastSaved = pos;
				lastSaveTime = now;
			}
		}

		// Remember the window rect for next frame's drop shadow; captured before
		// End(), like the position save above.
		shadowMin = pos;
		shadowMax = ImVec2(pos.x + ImGui::GetWindowSize().x, pos.y + ImGui::GetWindowSize().y);

		ImGui::End();
		ImGui::PopStyleVar();   // the open-fade alpha
	}
}
