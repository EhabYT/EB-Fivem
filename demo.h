#pragma once

namespace Demo
{
	// An annotated ImGui widget cheat-sheet - kept as a reference for editing the
	// real tabs.  It is deliberately wired to no sidebar entry; call Demo::Render()
	// from Tabs::Render() if you want it on screen.
	//
	// Implemented in demo.cpp.  Its state used to be defined in this header
	// (including two non-static bools), which only links while exactly one
	// translation unit includes it.
	void Render();
}
