#pragma once

// Development console: a key runs SkookumScript from a file next to the .asi, so the script API can be tried in
// the running game without rebuilding. The file is split into blocks at lines starting with "//---" (the rest of
// the line names the block); each block is compiled and run in turn, and the log gets its compile errors and
// its value (or when it finished, for durational blocks).

#include <string>

namespace console
{
	void Install(const std::wstring& dir);
}
