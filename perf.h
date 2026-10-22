#pragma once

#include "imgui/imgui.h"

// Frame-time history and session uptime for the Diagnostics card on the
// landing tab.  Sample() is called once per rendered frame from Menu::Render();
// the ring keeps the last 120 frame times in milliseconds - long enough to see
// a hitch that the instantaneous FPS number in the sidebar footer averages away.
namespace Perf
{
	void Sample(float ms);            // one frame's frame time
	void DrawGraph(const ImVec2& size);   // sparkline, oldest left; >33.3ms in red
	const char* UptimeString();       // "mm:ss" / "h:mm:ss", static buffer
}
