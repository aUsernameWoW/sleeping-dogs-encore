#include "config.hh"

#include <Windows.h>

#include <cstdio>
#include <cwchar>

Config gConfig;

namespace config
{
	static std::wstring gPath;

	static constexpr char kDefaultIni[] =
		"; SDUncut 配置 / configuration\n"
		"; 1 = 开启 (on), 0 = 关闭 (off)\n"
		"\n"
		"[General]\n"
		"; 字幕和物品名的语言：auto（装了中文汉化就用中文）、en、zh-Hant（繁体）、zh-Hans（简体）。\n"
		"; Language of the subtitles and item names: auto (Chinese with a Chinese translation pack), en, zh-Hant, zh-Hans.\n"
		"Language = auto\n"
		"\n"
		"; ---- 被删掉的手机联系人 / The cut phone contacts ----\n"
		"; Vehicle / Character：派来的车和人（游戏的属性集名）。 / The vehicle and man sent (the game's property sets).\n"
		"; Weapon：枪械类型（smg_machine_pistol、pistol_45cal、shotgun_pump、rifle_assault……）或近战武器（cleaver……）。\n"
		"; Weapon: a firearm type (smg_machine_pistol, pistol_45cal, shotgun_pump, rifle_assault...) or a melee one (cleaver...).\n"
		"\n"
		"[WeaponContact]\n"
		"; 武器联络人：开车给你送一把枪。 / Weapons contact: drives over and hands you a gun.\n"
		"Enabled = 1\n"
		"Vehicle = object-physical-vehicle-625MHCGTS01\n"
		"Character = object-physical-character-Thug_Wsg_quick\n"
		"Weapon = smg_machine_pistol\n"
		"\n"
		"[GunBackup]\n"
		"; 武装支援：一个带枪的水街兄弟跟着你打，直到被打倒。 / Armed backup: a Water Street gunman fights at your side until he's down.\n"
		"Enabled = 1\n"
		"Vehicle = object-physical-vehicle-625MHCGTS01\n"
		"Character = object-physical-character-Thug_Wsg_quick\n"
		"Weapon = smg_machine_pistol\n"
		"\n"
		"[MeleeBackup]\n"
		"; 近战支援：一个拿刀的水街兄弟。 / Hand to hand backup: a Water Street man with a cleaver.\n"
		"Enabled = 1\n"
		"Vehicle = object-physical-vehicle-625MHCGTS01\n"
		"Character = object-physical-character-Thug_Wsg_quick\n"
		"Weapon = cleaver\n"
		"\n"
		"[Backup]\n"
		"; 你离开支援这么远（米）超过 30 秒，他就回去。 / Backup goes home after you've been this far (m) away for 30 s.\n"
		"Leash = 250\n"
		"; 支援最多跟多少分钟，0 = 不限（原版：直到被打倒）。 / Minutes backup stays at most; 0 = no limit (the original: until he's down).\n"
		"Minutes = 0\n"
		"\n"
		"[BoatContact]\n"
		"; 快艇联络人：在离你最近的水边放一艘快艇。 / Boat contact: leaves a boat at the water nearest to you.\n"
		"Enabled = 1\n"
		"Vehicle = object-physical-vehicle-jet620v01\n"
		"\n"
		"[SwatContact]\n"
		"; 特勤联络人：开一辆特警车给你。 / SWAT contact: drives a SWAT truck over for you.\n"
		"Enabled = 1\n"
		"Vehicle = object-physical-vehicle-RidgeSportSWAT01\n"
		"Character = object-physical-character-jobCharacters-CJ_SWAT01_Character\n"
		"\n"
		"; ---- 枪贩 / The gun vendor ----\n"
		"[GunVendor]\n"
		"; 走近摆摊位置时出现一个枪贩，像买小吃一样买枪。 / A vendor appears at each spot when you come near; buy a gun like street food.\n"
		"Enabled = 1\n"
		"Character = object-physical-character-Thug_Wsg_quick\n"
		"; 摊位：Spot1、Spot2……= X, Y, Z, 朝向（度，0 = +Y）, 卖的枪。 / Spots: Spot1, Spot2... = X, Y, Z, heading (degrees, 0 = +Y), the gun.\n"
		"; 枪 / Guns: PISTOL_9MM PISTOL_SERVICE PISTOL_45CAL PISTOL_45CAL_TAC PISTOL_50CAL_GOL PISTOL_50CAL_SIL\n"
		";   SMG_MACHINE_PISTOL SMG_45CAL SMG_45CAL_TACLIG SMG_45CAL_GOLD SHOTGUN_PUMP SHOTGUN_ANTIRIOT SHOTGUN_ANTI_TAC\n"
		";   RIFLE_ASSAULT RIFLE_ASSAUL_TAC\n"
		"; 价格（港币）/ Prices (HK$):\n"
		"PricePistol = 1500\n"
		"PriceSmg = 3000\n"
		"PriceShotgun = 4000\n"
		"PriceRifle = 6000\n"
		"\n"
		"[Debug]\n"
		"; 在 .asi 旁边写 SDUncut.log。 / Write SDUncut.log.\n"
		"Logging = 1\n"
		"\n"
		"; 把游戏脚本的调试输出（Debug.print/println，本来是空操作）也写进日志。\n"
		"; Also log the scripts' debug output (Debug.print/println, which do nothing in this build).\n"
		"ScriptPrints = 1\n"
		"\n"
		"; 开发用脚本控制台：按 ConsoleKey 执行 SDUncut-console.sk 里的脚本，结果写进日志。\n"
		"; Development console: ConsoleKey runs the scripts in SDUncut-console.sk; the results go to the log.\n"
		"Console = 0\n"
		"\n"
		"; 虚拟键码，0x7A = F11。 / Virtual-key code, 0x7A = F11.\n"
		"ConsoleKey = 0x7A\n"
		"\n"
		"; 开发用：按这个键在你面前放一个测试枪贩，并把位置按 Spot 格式写进日志（0 = 关，0x79 = F10）。\n"
		"; Development: this key puts a test gun vendor in front of you and logs the spot in Spot format (0 = off, 0x79 = F10).\n"
		"VendorHereKey = 0\n";

	static std::wstring ReadString(const wchar_t* section, const wchar_t* key)
	{
		wchar_t text[256] = {};
		GetPrivateProfileStringW(section, key, L"", text, ARRAYSIZE(text), gPath.c_str());
		return text;
	}

	static std::string ToUtf8(const std::wstring& text)
	{
		const int size = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, nullptr, 0, nullptr, nullptr);
		std::string result(static_cast<size_t>(size > 1 ? size - 1 : 0), '\0');
		if (size > 1) {
			WideCharToMultiByte(CP_UTF8, 0, text.c_str(), -1, result.data(), size, nullptr, nullptr);
		}
		return result;
	}

	// A text value as UTF-8, or the fallback if the key is missing or empty.
	static std::string ReadText(const wchar_t* section, const wchar_t* key, const std::string& fallback)
	{
		const std::wstring text = ReadString(section, key);
		return text.empty() ? fallback : ToUtf8(text);
	}

	static bool ReadBool(const wchar_t* section, const wchar_t* key, bool fallback)
	{
		return GetPrivateProfileIntW(section, key, fallback ? 1 : 0, gPath.c_str()) != 0;
	}

	static int ReadInt(const wchar_t* section, const wchar_t* key, int fallback)
	{
		// Accepts decimal and 0x-prefixed hex (GetPrivateProfileInt only does decimal).
		const std::wstring text = ReadString(section, key);
		wchar_t* end = nullptr;
		const long value = std::wcstol(text.c_str(), &end, 0);
		return end != text.c_str() ? static_cast<int>(value) : fallback;
	}

	static void ReadService(const wchar_t* section, ServiceConfig& service)
	{
		service.mEnabled = ReadBool(section, L"Enabled", service.mEnabled);
		service.mVehicle = ReadText(section, L"Vehicle", service.mVehicle);
		service.mCharacter = ReadText(section, L"Character", service.mCharacter);
		service.mWeapon = ReadText(section, L"Weapon", service.mWeapon);
	}

	// "X, Y, Z, heading, GUN"
	static bool ParseSpot(const std::wstring& text, VendorSpot& spot)
	{
		wchar_t item[64] = {};
		if (swscanf_s(text.c_str(), L" %f , %f , %f , %f , %63ls", &spot.mX, &spot.mY, &spot.mZ, &spot.mHeading, item,
				static_cast<unsigned>(ARRAYSIZE(item))) != 5) {
			return false;
		}
		spot.mItem = ToUtf8(item);
		return true;
	}

	void Load(const std::wstring& dir)
	{
		gPath = dir + L"\\SDUncut.ini";

		if (GetFileAttributesW(gPath.c_str()) == INVALID_FILE_ATTRIBUTES)
		{
			FILE* file = nullptr;
			if (_wfopen_s(&file, gPath.c_str(), L"wb") == 0 && file)
			{
				fwrite(kDefaultIni, 1, sizeof(kDefaultIni) - 1, file);
				fclose(file);
			}
		}

		gConfig.mLanguage = ReadText(L"General", L"Language", gConfig.mLanguage);
		ReadService(L"WeaponContact", gConfig.mWeaponContact);
		ReadService(L"GunBackup", gConfig.mGunBackup);
		ReadService(L"MeleeBackup", gConfig.mMeleeBackup);
		ReadService(L"BoatContact", gConfig.mBoatContact);
		ReadService(L"SwatContact", gConfig.mSwatContact);
		gConfig.mBackupLeash = static_cast<float>(ReadInt(L"Backup", L"Leash", static_cast<int>(gConfig.mBackupLeash)));
		gConfig.mBackupMinutes = ReadInt(L"Backup", L"Minutes", gConfig.mBackupMinutes);

		gConfig.mVendor = ReadBool(L"GunVendor", L"Enabled", gConfig.mVendor);
		gConfig.mVendorCharacter = ReadText(L"GunVendor", L"Character", gConfig.mVendorCharacter);
		gConfig.mPricePistol = ReadInt(L"GunVendor", L"PricePistol", gConfig.mPricePistol);
		gConfig.mPriceSmg = ReadInt(L"GunVendor", L"PriceSmg", gConfig.mPriceSmg);
		gConfig.mPriceShotgun = ReadInt(L"GunVendor", L"PriceShotgun", gConfig.mPriceShotgun);
		gConfig.mPriceRifle = ReadInt(L"GunVendor", L"PriceRifle", gConfig.mPriceRifle);
		for (int i = 1; i <= 32; ++i) {
			wchar_t key[16];
			swprintf_s(key, L"Spot%d", i);
			const std::wstring text = ReadString(L"GunVendor", key);
			VendorSpot spot;
			if (!text.empty() && ParseSpot(text, spot)) {
				gConfig.mVendorSpots.push_back(spot);
			}
		}

		gConfig.mLogging = ReadBool(L"Debug", L"Logging", gConfig.mLogging);
		gConfig.mScriptPrints = ReadBool(L"Debug", L"ScriptPrints", gConfig.mScriptPrints);
		gConfig.mConsole = ReadBool(L"Debug", L"Console", gConfig.mConsole);
		gConfig.mConsoleKey = ReadInt(L"Debug", L"ConsoleKey", gConfig.mConsoleKey);
		gConfig.mVendorHereKey = ReadInt(L"Debug", L"VendorHereKey", gConfig.mVendorHereKey);
	}
}
