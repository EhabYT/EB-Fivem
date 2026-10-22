#pragma once

// Persisted interface preferences, stored as <DLL folder>\settings.ini.
// Nothing in here touches the game: it only remembers where the window sat,
// which tab was open and which key toggles the overlay, so a re-inject
// restores the previous session's layout instead of resetting it.

namespace Settings
{
	struct Prefs
	{
		int WindowX   = -1;      // -1 => never persisted, let ImGui place it
		int WindowY   = -1;
		int Tab       = 0;
		int ToggleKey = 0x2D;    // VK_INSERT
		bool Animations = true;  // open fade, sliding tab marker, switch easing
		bool Toasts     = true;  // non-blocking notifications (toasts.h)
	};

	extern Prefs Current;        // defined in settings.cpp

	void Load();
	void Save();
	void SaveWindowPos(float x, float y);

	// Human-readable name for a virtual key, for the capture button.
	const char* KeyName(int vk);
}
