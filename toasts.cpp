#include "toasts.h"
#include "settings.h"

#include "imgui/imgui.h"

#include <vector>

namespace Toasts
{
	namespace
	{
		struct Entry
		{
			Kind        kind;
			std::string text;
			double      born;
		};

		std::vector<Entry> g_entries;
		const double kLife = 4.0;      // seconds on screen
		const size_t kMax  = 4;        // stacked cards

		ImU32 KindColor(Kind kind, float alpha)
		{
			const int a = (int)(255.0f * alpha);
			switch (kind)
			{
			case Success: return IM_COL32(93, 156, 214, a);   // #5D9CD6 accent
			case Warn:    return IM_COL32(255, 0, 0, a);      // same red as the error texts
			default:      return IM_COL32(55, 120, 181, a);   // #3778B5 hover blue
			}
		}
	}

	void Push(Kind kind, const std::string& text)
	{
		const Entry e = { kind, text, ImGui::GetTime() };
		g_entries.push_back(e);
		if (g_entries.size() > kMax)
			g_entries.erase(g_entries.begin());
	}

	void Clear()
	{
		g_entries.clear();
	}

	// UI: row 162 - shared stack painter.  The shell anchors the stack to its
	// own bottom-right corner on the window draw list; surfaces smaller than
	// the stack pass their own anchor instead (see RenderScreen below).
	static void RenderStack(ImDrawList* dl, const ImVec2& wp, const ImVec2& ws)
	{
		const double now = ImGui::GetTime();

		// Card metrics scale with the interface: at 200% a fixed 320px card
		// around 40px text would wrap every toast into a tower.
		const float pad     = Settings::Px(10.0f);
		const float maxW    = Settings::Px(320.0f);
		const float barW    = Settings::Px(3.0f);
		const float inner   = Settings::Px(8.0f);
		const float gap     = Settings::Px(6.0f);

		// Click a card to dismiss it - but never steal a click that belongs to
		// a widget sitting under the stack.
		const bool dismissClick = ImGui::IsMouseClicked(0)
			&& !ImGui::IsAnyItemHovered() && !ImGui::IsAnyItemActive();
		const ImVec2 mouse = ImGui::GetMousePos();

		// Newest sits lowest; walk backwards so the stack grows upwards.
		float cursorY = wp.y + ws.y - pad;
		for (int i = (int)g_entries.size() - 1; i >= 0; --i)
		{
			const double age = now - g_entries[i].born;
			if (age >= kLife)
			{
				g_entries.erase(g_entries.begin() + i);
				continue;
			}

			// Ease in over the first 0.18s, out over the last 0.4s.
			float a = 1.0f;
			if (age < 0.18)
				a = (float)(age / 0.18);
			else if (age > kLife - 0.4)
				a = (float)((kLife - age) / 0.4);

			const Entry& e = g_entries[i];
			const float wrapW = maxW - barW - inner * 3.0f;
			const ImVec2 ts = ImGui::CalcTextSize(e.text.c_str(), NULL, false, wrapW);
			const float h = ts.y + inner * 2.0f;
			const float w = (ts.x + barW + inner * 3.0f < maxW) ? ts.x + barW + inner * 3.0f : maxW;
			cursorY -= h;

			// Slide in from the right while fading.
			const float slide = (1.0f - a) * Settings::Px(14.0f);
			const ImVec2 mn(wp.x + ws.x - pad - w + slide, cursorY);
			const ImVec2 mx(mn.x + w, mn.y + h);

			if (dismissClick && mouse.x >= mn.x && mouse.x <= mx.x
				&& mouse.y >= mn.y && mouse.y <= mx.y)
			{
				g_entries.erase(g_entries.begin() + i);
				continue;
			}

			dl->AddRectFilled(mn, mx, IM_COL32(31, 58, 97, (int)(242.0f * a)), Settings::Px(6.0f));
			dl->AddRect(mn, mx, IM_COL32(93, 156, 214, (int)(110.0f * a)), Settings::Px(6.0f));
			dl->AddRectFilled(ImVec2(mn.x, mn.y), ImVec2(mn.x + barW, mx.y),
				KindColor(e.kind, a), Settings::Px(6.0f), ImDrawFlags_RoundCornersLeft);
			// ImFont::FontSize was removed in 1.92 (renamed to LegacySize, which is the
			// size passed to AddFont()).  NULL + 0.0f makes AddText() take the font and
			// size already bound to this draw list, i.e. exactly what the rest of the
			// window renders with - including any window font scale.
			dl->AddText(NULL, 0.0f,
				ImVec2(mn.x + barW + inner, mn.y + inner),
				IM_COL32(255, 255, 255, (int)(255.0f * a)),
				e.text.c_str(), NULL, wrapW);

			cursorY -= gap;
		}
	}

	void Render()
	{
		if (g_entries.empty())
			return;
		RenderStack(ImGui::GetWindowDrawList(), ImGui::GetWindowPos(), ImGui::GetWindowSize());
	}

	// UI: row 162 - screen-anchored variant for surfaces smaller than the stack:
	// the key card calls this instead of Render(), because a 460x232 window
	// would put the toast cards on top of its own button and hint line.  Same
	// stack, same ageing, same dismiss rules - anchored to the display and
	// drawn on the foreground list, over every window instead of inside one.
	void RenderScreen()
	{
		if (g_entries.empty())
			return;
		const ImVec2 disp = ImGui::GetIO().DisplaySize;
		RenderStack(ImGui::GetForegroundDrawList(), ImVec2(0.0f, 0.0f), disp);
	}
}
