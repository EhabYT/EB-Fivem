#pragma once

#include <string>
#include <vector>

// Session action log, shown on the landing tab.  In-memory only - nothing here
// is written to disk - and it exists so that an action whose only visible
// effect is a message box still leaves a trace of what happened and when.
namespace Activity
{
	void Push(const std::string& text);      // prepends "[HH:MM:SS] text"
	const std::vector<std::string>& Entries();   // newest first
	void Clear();
}
