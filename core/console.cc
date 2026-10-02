#include "console.hh"

#include <Windows.h>

#include <cstdio>
#include <string>
#include <vector>

#include "config.hh"
#include "log.hh"
#include "skookum.hh"

namespace console
{
	static std::wstring gPath;
	static bool gKeyWasDown = false;
	static DWORD gLastRun = 0;

	static constexpr char kDefaultScript[] =
		"// SDUncut console: the ConsoleKey in SDUncut.ini runs this file's blocks in order; results go to SDUncut.log.\n"
		"// Blocks start at lines beginning with //--- (the rest of the line is the block's name).\n"
		"//--- where the player is, and which mission (active master gameslice) runs\n"
		"Debug.println(\"player at \", World.c_player.get_pos(), \", facing \", World.c_player.get_dir(), \", active master \", "
		"GameSlice.get_active_master())\n";

	static bool GameHasFocus()
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(GetForegroundWindow(), &pid);
		return pid == GetCurrentProcessId();
	}

	static bool ReadFile(std::string& text)
	{
		FILE* file = nullptr;
		if (_wfopen_s(&file, gPath.c_str(), L"rb") != 0 || !file) {
			return false;
		}
		char buffer[4096];
		size_t n;
		while ((n = fread(buffer, 1, sizeof(buffer), file)) > 0) {
			text.append(buffer, n);
		}
		fclose(file);
		if (text.size() >= 3 && text.compare(0, 3, "\xEF\xBB\xBF") == 0) {
			text.erase(0, 3);
		}
		return true;
	}

	struct Block
	{
		std::string mName;
		std::string mSource;
	};

	static std::vector<Block> Split(const std::string& text)
	{
		std::vector<Block> blocks(1);
		blocks.back().mName = "block 1";
		size_t pos = 0;
		while (pos < text.size()) {
			size_t end = text.find('\n', pos);
			if (end == std::string::npos) {
				end = text.size();
			}
			std::string line = text.substr(pos, end - pos);
			if (!line.empty() && line.back() == '\r') {
				line.pop_back();
			}
			if (line.compare(0, 5, "//---") == 0) {
				std::string name = line.substr(5);
				name.erase(0, name.find_first_not_of(" \t"));
				blocks.push_back({ name.empty() ? "block " + std::to_string(blocks.size() + 1) : name, {} });
			}
			else {
				blocks.back().mSource += line;
				blocks.back().mSource += '\n';
			}
			pos = end + 1;
		}

		std::vector<Block> nonEmpty;
		for (Block& block : blocks) {
			bool code = false;
			// Only comments and blanks: nothing to run.
			size_t at = 0;
			while (at < block.mSource.size() && !code) {
				size_t end = block.mSource.find('\n', at);
				const std::string line = block.mSource.substr(at, end - at);
				const size_t first = line.find_first_not_of(" \t\r");
				code = first != std::string::npos && line.compare(first, 2, "//") != 0;
				at = end == std::string::npos ? block.mSource.size() : end + 1;
			}
			if (code) {
				nonEmpty.push_back(std::move(block));
			}
		}
		return nonEmpty;
	}

	static void Tick(float)
	{
		const bool down = (GetAsyncKeyState(gConfig.mConsoleKey) & 0x8000) != 0;
		const bool pressed = down && !gKeyWasDown;
		gKeyWasDown = down;
		if (!pressed || !GameHasFocus()) {
			return;
		}
		// Presses less than 3 s apart are one press: a test taxi dispatch pressed five times in four seconds (nothing
		// visible happened at once) spawned five taxis, each run's cleanup despawning the one before. Durational
		// blocks can't be stopped from here: aborting the game's call wrapper is unsafe once a scene reset freed it.
		const DWORD now = GetTickCount();
		if (gLastRun && now - gLastRun < 3000) {
			LOG("console: key ignored, pressed %lu ms after the last run", now - gLastRun);
			return;
		}
		gLastRun = now;

		std::string text;
		if (!ReadFile(text)) {
			LOG("console: can't read %s", logger::ToUtf8(gPath.c_str()).c_str());
			return;
		}
		const std::vector<Block> blocks = Split(text);
		LOG("console: running %zu block(s) from %s", blocks.size(), logger::ToUtf8(gPath.c_str()).c_str());
		for (const Block& block : blocks) {
			LOG("console: [%s]", block.mName.c_str());
			skookum::Start(block.mName.c_str(), block.mSource);
		}
	}

	void Install(const std::wstring& dir)
	{
		if (!gConfig.mConsole) {
			LOG("console: off");
			return;
		}
		if (!skookum::Ready()) {
			LOG("console: no scripts, console off");
			return;
		}
		gPath = dir + L"\\SDUncut-console.sk";
		if (GetFileAttributesW(gPath.c_str()) == INVALID_FILE_ATTRIBUTES) {
			FILE* file = nullptr;
			if (_wfopen_s(&file, gPath.c_str(), L"wb") == 0 && file) {
				fwrite(kDefaultScript, 1, sizeof(kDefaultScript) - 1, file);
				fclose(file);
			}
		}
		skookum::OnTick(&Tick);
		LOG("console: key 0x%02X runs SDUncut-console.sk", gConfig.mConsoleKey);
	}
}
