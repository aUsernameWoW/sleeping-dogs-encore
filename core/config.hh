#pragma once

#include <string>
#include <vector>

// One of the cut phone contacts (core/contacts.cc). Property sets as the game names them; empty = not used.
struct ServiceConfig
{
	bool mEnabled = true;
	std::string mVehicle;
	std::string mCharacter;
	std::string mWeapon;  // Firearm.type_to_propset_name / Melee type (e.g. smg_machine_pistol, cleaver)
};

// A spot where the gun vendor stands (core/vendor.cc).
struct VendorSpot
{
	float mX = 0, mY = 0, mZ = 0;
	float mHeading = 0;  // degrees, the way the vendor faces: 0 = +Y (north), 90 = +X
	std::string mItem;   // the gun sold there (an eINVENTORY_ITEM_ name without the prefix, e.g. PISTOL_45CAL)
};

struct Config
{
	// "auto" (the game's text decides: a Chinese translation pack means Chinese), "en", "zh-Hant", "zh-Hans".
	std::string mLanguage = "auto";

	// The cut contacts, as Taylor's test definitions and the scripts have them, with their rough edges fixed.
	ServiceConfig mWeaponContact{ true, "object-physical-vehicle-625MHCGTS01", "object-physical-character-Thug_Wsg_quick",
		"smg_machine_pistol" };
	ServiceConfig mGunBackup{ true, "object-physical-vehicle-625MHCGTS01", "object-physical-character-Thug_Wsg_quick",
		"smg_machine_pistol" };
	ServiceConfig mMeleeBackup{ true, "object-physical-vehicle-625MHCGTS01", "object-physical-character-Thug_Wsg_quick", "cleaver" };
	ServiceConfig mBoatContact{ true, "object-physical-vehicle-jet620v01", "", "" };
	ServiceConfig mSwatContact{ true, "object-physical-vehicle-RidgeSportSWAT01",
		"object-physical-character-jobCharacters-CJ_SWAT01_Character", "" };

	// Backup leaves when the player has been this far away for 30 s (taxi, fast travel); minutes = 0: no time limit.
	float mBackupLeash = 250.0f;
	int mBackupMinutes = 0;

	// The gun vendor: buy guns from him like food from a street vendor.
	bool mVendor = true;
	std::string mVendorCharacter = "object-physical-character-Thug_Wsg_quick";
	std::vector<VendorSpot> mVendorSpots;
	// Prices in HK$ (the game's item profiles have a placeholder of 10 for every gun).
	int mPricePistol = 1500;
	int mPriceSmg = 3000;
	int mPriceShotgun = 4000;
	int mPriceRifle = 6000;

	bool mLogging = true;

	// Log the scripts' Debug.print/println (ours and the game's; empty in this build).
	bool mScriptPrints = true;

	// Development console (core/console.cc): ConsoleKey runs SDEncore-console.sk.
	bool mConsole = false;
	int mConsoleKey = 0x7A; // VK_F11
	// Development: puts a gun vendor where the player stands and logs the spot in the ini's format (0 = off).
	int mVendorHereKey = 0;
};

extern Config gConfig;

namespace config
{
	// Reads SDEncore.ini from `dir`, writing a commented default first if there is none.
	void Load(const std::wstring& dir);
}
