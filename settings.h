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

	// Host window contents scale (GetDpiForWindow / 96), set once in InitImGui.
	// 1.0 on DPI-unaware hosts, i.e. the identity everywhere; 1.5/2.0/... on
	// per-monitor-aware ones, where the window is sized in physical pixels and
	// the compiled 900x540 layout would otherwise render small.  Environment-
	// derived, so it is not part of the persisted Prefs.
	extern float DpiScale;

	void Load();
	void Save();
	void SaveWindowPos(float x, float y);

	// Interface scale as a factor (1.0f == the compiled layout).  Inline because
	// the shell metrics in menu.h multiply their 100% base by it every frame.
	inline float ScaleFactor() { return (float)Current.ScalePercent / 100.0f * DpiScale; }

	// Explicit widget sizes (ImVec2(116, 30) and friends) do NOT follow
	// style.FontScaleMain on their own - only font-derived metrics such as
	// GetFrameHeight() do.  Every hardcoded pixel size in the tabs goes through
	// here so the layout stays proportional across the 70..200 % range; at the
	// compiled default (100 %) this is the identity and nothing moves.
	inline float Px(float v) { return v * ScaleFactor(); }

	// Writes ScalePercent into ImGui's global font scale factor and the window's
	// minimum size; the shell metrics in menu.h follow through ScaleFactor().
	// Clamps to 70..200 itself.  Call after Load() and whenever the value changes.
	void ApplyScale();

	// True once any settings.ini write has failed this session (read-only or
	// missing folder).  The first failure is answered with an activity entry
	// and a warn toast instead of staying silent.
	bool SaveFailed();

	// Human-readable name for a virtual key, for the capture button.
	const char* KeyName(int vk);
}
