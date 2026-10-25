#pragma once

#include <string>

// Non-blocking in-menu notifications, drawn as a small stack of cards in the
// bottom-right corner of the menu window.
//
// Every piece of feedback in this build used to be a MessageBox: correct, but
// modal - it steals focus from the game and freezes the frame until someone
// clicks it away.  Toasts cover the non-critical half of that feedback (a load
// finished, a dump was written, a trigger reached the clipboard) without
// interrupting play; the critical errors keep their message boxes so they
// cannot be missed.
//
// Purely presentational: Push() only stores text, Render() only draws.
namespace Toasts
{
	enum Kind
	{
		Info = 0,   // left bar in the hover blue  #3778B5
		Success,    // left bar in the accent      #5D9CD6
		Warn,       // left bar in the semantic red already used for errors
	};

	// Queues one card (newest last, stack capped at four).
	void Push(Kind kind, const std::string& text);

	// Drops the whole stack.  Called when the toast preference is switched off:
	// Menu::Render() skips Render() while it is false, so a card younger than
	// ~4s would otherwise reappear mid-fade when toasts are switched back on.
	void Clear();

	// Draws the live stack.  Call from inside the menu window (menu.cpp);
	// cards fade in, live ~4s and fade out again.
	void Render();
	// UI: row 162 - same stack anchored to the display's bottom-right corner,
	// for surfaces smaller than the stack (the key card).
	void RenderScreen();
}
