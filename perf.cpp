#include "perf.h"

#include <cstdio>

namespace Perf
{
	namespace
	{
		const int kSlots = 120;
		float  g_ring[kSlots] = { 0 };
		int    g_head  = 0;
		int    g_count = 0;
		double g_open  = 0.0;   // accumulated sampled (menu-open) seconds
	}

	void Sample(float ms)
	{
		g_open += ms / 1000.0;
		g_ring[g_head] = ms;
		g_head = (g_head + 1) % kSlots;
		if (g_count < kSlots)
			++g_count;
	}

	void DrawGraph(const ImVec2& size)
	{
		const ImVec2 mn = ImGui::GetCursorScreenPos();
		const ImVec2 mx(mn.x + size.x, mn.y + size.y);
		ImDrawList* dl = ImGui::GetWindowDrawList();

		// Canvas in the window's own navy so the bars carry the colour.
		dl->AddRectFilled(mn, mx, IM_COL32(20, 38, 63, 255));
		dl->AddRect(mn, mx, IM_COL32(42, 74, 123, 255));

		// Guide line at 16.7ms (60 FPS); the scale tops out at 33.3ms (30 FPS),
		// so anything at the ceiling is a bad frame even by console standards.
		const float gy = mn.y + size.y - (16.7f / 33.3f) * size.y;
		dl->AddLine(ImVec2(mn.x, gy), ImVec2(mn.x + size.x, gy), IM_COL32(93, 156, 214, 90));

		const float bw = size.x / (float)kSlots;
		for (int i = 0; i < g_count; ++i)
		{
			const float ms = g_ring[(g_head - g_count + i + kSlots) % kSlots];  // old -> new
			const float h = (ms < 33.3f ? ms : 33.3f) / 33.3f * (size.y - 2.0f);
			const ImU32 col = ms > 33.3f ? IM_COL32(255, 0, 0, 200)
				: IM_COL32(93, 156, 214, 220);
			dl->AddRectFilled(ImVec2(mn.x + i * bw, mn.y + size.y - 1.0f - h),
				ImVec2(mn.x + (i + 1) * bw, mn.y + size.y - 1.0f), col);
		}
		// Hover readout: the bars show the shape, the tooltip the numbers.
		// bw > 0 first: on a zero-width canvas the division below is by zero and
		// the (int) cast of the resulting infinity is undefined, which the index
		// check after it would not reliably catch.
		if (bw > 0.0f && ImGui::IsMouseHoveringRect(mn, mx) && g_count > 0)
		{
			const int i = (int)((ImGui::GetMousePos().x - mn.x) / bw);
			if (i >= 0 && i < g_count)
			{
				const float ms = g_ring[(g_head - g_count + i + kSlots) % kSlots];
				const int ago = g_count - 1 - i;
				char tip[48];
				snprintf(tip, sizeof(tip), "%.1f ms, %d frame%s ago", ms, ago, ago == 1 ? "" : "s");
				ImGui::SetTooltip("%s", tip);
			}
		}
		ImGui::Dummy(size);
	}

	const char* UptimeString()
	{
		static char buf[16];
		const int sec = (int)g_open;
		const int h = sec / 3600, m = (sec / 60) % 60, s = sec % 60;
		if (h > 0)
			snprintf(buf, sizeof(buf), "%d:%02d:%02d", h, m, s);
		else
			snprintf(buf, sizeof(buf), "%02d:%02d", m, s);
		return buf;
	}
}
