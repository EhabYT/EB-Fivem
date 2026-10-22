#pragma once

#include <string>

// Resolve "file" relative to the directory this DLL was loaded from, falling
// back to the bare relative name (i.e. the process working directory).
// Defined in main.cpp.
std::string ResolveAssetPath(const char* file);
