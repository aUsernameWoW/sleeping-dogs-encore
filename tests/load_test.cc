// Loads SDEncore.asi into a process that isn't the game: it must not crash, must write its default ini and
// must report the game functions missing. argv[1] = path to the .asi (build.ps1 passes a sandbox copy).

#include <Windows.h>

#include <cstdio>
#include <fstream>
#include <sstream>
#include <string>

int main(int argc, char** argv)
{
	if (argc < 2) {
		std::printf("usage: load_test <SDEncore.asi>\n");
		return 2;
	}

	std::string dir = argv[1];
	dir = dir.substr(0, dir.find_last_of("\\/") + 1);

	// The sandbox outlives a run: start without an earlier run's files, so the defaults are what's tested.
	DeleteFileA((dir + "SDEncore.ini").c_str());
	DeleteFileA((dir + "SDEncore.log").c_str());
	DeleteFileA((dir + "SDEncore-console.sk").c_str());

	HMODULE module = LoadLibraryA(argv[1]);
	if (!module) {
		std::printf("FAIL: LoadLibrary error %lu\n", GetLastError());
		return 1;
	}

	std::ifstream ini(dir + "SDEncore.ini");
	if (!ini) {
		std::printf("FAIL: no default SDEncore.ini written\n");
		return 1;
	}

	std::ifstream log(dir + "SDEncore.log");
	std::stringstream text;
	text << log.rdbuf();
	const std::string contents = text.str();
	std::printf("%s", contents.c_str());

	const char* expected[] = {
		"SDEncore loaded (Language=auto WeaponContact=1 GunBackup=1 MeleeBackup=1 BoatContact=1 SwatContact=1 GunVendor=1 spots=0 "
		"ScriptPrints=1 Console=0 VendorHereKey=0x00)",
		"UFG::ScriptCache::GetScript: 0 matches",
		"skookum: game functions MISSING or not as expected, no scripts",
		"text: language from the game's text (auto)",
		"contacts: no scripts, no contacts",
		"items: item profiles MISSING or not as expected, guns keep their placeholder prices",
		"vendor: no scripts, no vendor",
		"phone: no contacts to add",
		"console: off",
	};
	for (const char* line : expected) {
		if (contents.find(line) == std::string::npos) {
			std::printf("FAIL: log lacks \"%s\"\n", line);
			return 1;
		}
	}

	std::printf("PASS\n");
	return 0;
}
