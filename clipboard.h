#pragma once

#include <Windows.h>
#include <cstring>

// One shared Win32 clipboard writer: Config's "Copy diagnostics" and the
// landing tab's "Copy" (session log) both need exactly this, and keeping it
// header-only means no project-file change for a second translation unit.
// CF_TEXT with a moveable global; the global is freed again when
// SetClipboardData refuses it (the clipboard owns it once it accepts).
inline bool CopyTextToClipboard(const char* text)
{
	if (!OpenClipboard(NULL))
		return false;
	bool ok = false;
	EmptyClipboard();
	const size_t len = std::strlen(text) + 1;
	HGLOBAL g = GlobalAlloc(GMEM_MOVEABLE, len);
	if (g)
	{
		void* p = GlobalLock(g);
		if (p)
		{
			std::memcpy(p, text, len);
			GlobalUnlock(g);
			ok = SetClipboardData(CF_TEXT, g) != NULL;
		}
		if (!ok)
			GlobalFree(g);
	}
	CloseClipboard();
	return ok;
}
