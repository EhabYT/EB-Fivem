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
	// UI: row 176 - the clipboard is one global lock: a single busy viewer
	// (Excel mid-copy, a clipboard manager syncing) made OpenClipboard fail
	// and the UI reported "clipboard busy" although a moment later the same
	// copy would have succeeded.  A short bounded retry turns that common
	// transient case into a copy instead of an error; five tries at 15ms
	// stay far below anything a user perceives as a stall.
	bool opened = false;
	for (int attempt = 0; attempt < 5 && !opened; ++attempt)
	{
		if (attempt)
			Sleep(15);
		opened = OpenClipboard(NULL) != FALSE;
	}
	if (!opened)
		return false;
	bool okA = false, okW = false;
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
			okA = SetClipboardData(CF_TEXT, g) != NULL;
		}
		if (!okA)
			GlobalFree(g);
	}
	// UI: row 177 - CF_TEXT alone is ANSI: anything outside the active code
	// page (an umlaut in a path, a non-ASCII lua string) arrived mangled in
	// Unicode-aware targets, which is most of them.  Offer CF_UNICODETEXT as
	// well; a handle belongs to the clipboard once SetClipboardData accepts
	// it, so the second global follows the same free-on-refuse rule.
	const int wlen = MultiByteToWideChar(CP_UTF8, 0, text, -1, NULL, 0);
	if (wlen > 0)
	{
		HGLOBAL gu = GlobalAlloc(GMEM_MOVEABLE, (SIZE_T)wlen * sizeof(wchar_t));
		if (gu)
		{
			wchar_t* pw = (wchar_t*)GlobalLock(gu);
			if (pw)
			{
				MultiByteToWideChar(CP_UTF8, 0, text, -1, pw, wlen);
				GlobalUnlock(gu);
				okW = SetClipboardData(CF_UNICODETEXT, gu) != NULL;
			}
			if (!okW)
				GlobalFree(gu);
		}
	}
	CloseClipboard();
	return okA || okW;
}
