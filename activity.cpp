#include "activity.h"

#include <Windows.h>
#include <cstdio>

namespace Activity
{
	static const size_t kMaxEntries = 32;
	static std::vector<std::string> g_entries;   // newest first
	static unsigned g_total = 0;

	void Push(const std::string& text)
	{
		SYSTEMTIME t;
		GetLocalTime(&t);
		char stamp[32];
		snprintf(stamp, sizeof(stamp), "%02u:%02u:%02u",
			(unsigned)t.wHour, (unsigned)t.wMinute, (unsigned)t.wSecond);

		g_entries.insert(g_entries.begin(), std::string("[") + stamp + "] " + text);
		++g_total;
		if (g_entries.size() > kMaxEntries)
			g_entries.resize(kMaxEntries);
	}

	unsigned TotalCount()
	{
		return g_total;
	}

	const std::vector<std::string>& Entries()
	{
		return g_entries;
	}

	void Clear()
	{
		g_entries.clear();
		// UI: row 256 - the counter behind "N actions" in the Diagnostics line
		// kept counting the cleared entries, so the tab showed an empty
		// session log next to a counter that still reported the old total.
		// The counter belongs to the session, the same as the log.
		g_total = 0;
	}
}
