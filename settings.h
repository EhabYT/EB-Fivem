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
		bool ShowWhitespace = false;  // editor whitespace dots
		int  ResourceTab    = 0;      // Resources sub-tab: 0 none, 1 browser, 2 triggers, 3 stopper
		int  ScalePercent   = 100;    // interface scale 70..200 (%), applied as style.FontScaleMain
	};

	extern Prefs Current;        // defined in settings.cpp

	void Load();
	void Save();
	void SaveWindowPos(float x, float y);

	// Interface scale as a factor (1.0f == the compiled layout).  Inline because
	// the shell metrics in menu.h multiply their 100% base by it every frame.
	inline float ScaleFactor() { return (float)Current.ScalePercent / 100.0f; }

	// Writes ScalePercent into ImGui's global font scale factor and the window's
	// minimum size; the shell metrics in menu.h follow through ScaleFactor().
	// Clamps to 70..200 itself.  Call after Load() and whenever the value changes.
	void ApplyScale();

	// Human-readable name for a virtual key, for the capture button.
	const char* KeyName(int vk);
}
