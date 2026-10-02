#include "items.hh"

#include <Windows.h>

#include <cstdint>
#include <cstring>

#include "log.hh"
#include "mem.hh"
#include "scan.hh"
#include "text.hh"

namespace items
{
	// UFG::ItemProfiles::mpInstance → mProfiles[NUM_INVENTORY_ITEMS], 0x38 bytes each (InitItemProfiles,
	// GetCost/GetName/GetDescription): +0x0 property flags, +0x4 ItemCost, +0x10 ItemCaptioning, +0x18 ItemName,
	// +0x20 ItemDescriptionList[3] (the purchase prompt shows [0]). The strings are the property sets'; ours replace
	// the pointers.
	static constexpr size_t kProfileSize = 0x38;
	static constexpr int kItems = 209;
	static uint8_t** gProfiles = nullptr;

	static const Gun kGuns[] = {
		{ "PISTOL_9MM", 32, "pistol_9mm", 0 },
		{ "PISTOL_SERVICE", 34, "pistol_service", 0 },
		{ "PISTOL_45CAL", 36, "pistol_45cal", 0 },
		{ "PISTOL_45CAL_TAC", 37, "pistol_45cal_taclight", 0 },
		{ "PISTOL_50CAL_GOL", 155, "pistol_50cal_gold", 0 },
		{ "PISTOL_50CAL_SIL", 156, "pistol_50cal_silver", 0 },
		{ "SMG_MACHINE_PISTOL", 55, "smg_machine_pistol", 1 },
		{ "SMG_45CAL", 59, "smg_45cal", 1 },
		{ "SMG_45CAL_TACLIG", 57, "smg_45cal_taclight", 1 },
		{ "SMG_45CAL_GOLD", 58, "smg_45cal_gold", 1 },
		{ "SHOTGUN_PUMP", 50, "shotgun_pump", 2 },
		{ "SHOTGUN_ANTIRIOT", 53, "shotgun_antiriot", 2 },
		{ "SHOTGUN_ANTI_TAC", 52, "shotgun_antiriot_taclight", 2 },
		{ "RIFLE_ASSAULT", 42, "rifle_assault", 3 },
		{ "RIFLE_ASSAUL_TAC", 41, "rifle_assault_taclight", 3 },
	};

	struct GunText
	{
		text::Line mName;
		text::Line mDescription;
	};

	// The profiles' English where there is some (".45 CAL PISTOL", "A powerful pistol"), in the shipped goods' case.
	static const GunText kTexts[] = {
		{ { "9mm Pistol", "9mm 手槍", "9mm 手枪" }, { "A light pistol", "輕便的手槍", "轻便的手枪" } },
		{ { "Service Pistol", "警用手槍", "警用手枪" }, { "A tactical pistol", "戰術手槍", "战术手枪" } },
		{ { ".45 Pistol", ".45 手槍", ".45 手枪" }, { "A powerful pistol", "威力強大的手槍", "威力强大的手枪" } },
		{ { ".45 Pistol with Red Dot", ".45 紅點瞄準手槍", ".45 红点瞄准手枪" },
			{ "A powerful pistol with a red dot sight", "裝了紅點瞄準器的強力手槍", "装了红点瞄准器的强力手枪" } },
		{ { ".50 Pistol, Gold", ".50 金色手槍", ".50 金色手枪" }, { "A very powerful pistol", "威力極強的手槍", "威力极强的手枪" } },
		{ { ".50 Pistol, Silver", ".50 銀色手槍", ".50 银色手枪" }, { "A very powerful pistol", "威力極強的手槍", "威力极强的手枪" } },
		{ { "Machine Pistol", "衝鋒手槍", "冲锋手枪" }, { "A compact automatic pistol", "小巧的全自動手槍", "小巧的全自动手枪" } },
		{ { ".45 SMG", ".45 衝鋒槍", ".45 冲锋枪" }, { "A powerful submachine gun", "威力強大的衝鋒槍", "威力强大的冲锋枪" } },
		{ { ".45 SMG with Red Dot", ".45 紅點瞄準衝鋒槍", ".45 红点瞄准冲锋枪" },
			{ "A powerful submachine gun with a red dot sight", "裝了紅點瞄準器的強力衝鋒槍", "装了红点瞄准器的强力冲锋枪" } },
		{ { ".45 SMG, Gold", ".45 金色衝鋒槍", ".45 金色冲锋枪" }, { "A powerful golden submachine gun", "威力強大的金色衝鋒槍", "威力强大的金色冲锋枪" } },
		{ { "Pump Shotgun", "泵動式霰彈槍", "泵动式霰弹枪" }, { "Devastating up close", "近距離威力驚人", "近距离威力惊人" } },
		{ { "Anti-Riot Shotgun", "防暴霰彈槍", "防暴霰弹枪" }, { "Shoots to stop its target", "專為制服目標而設", "专为制服目标而设" } },
		{ { "Anti-Riot Shotgun with Taclight", "戰術燈防暴霰彈槍", "战术灯防暴霰弹枪" },
			{ "Shoots to stop its target", "專為制服目標而設", "专为制服目标而设" } },
		{ { "Assault Rifle", "突擊步槍", "突击步枪" }, { "A very powerful rifle", "威力極強的步槍", "威力极强的步枪" } },
		{ { "Assault Rifle with Taclight", "戰術燈突擊步槍", "战术灯突击步枪" }, { "A very powerful rifle", "威力極強的步槍", "威力极强的步枪" } },
	};
	static_assert(ARRAYSIZE(kGuns) == ARRAYSIZE(kTexts));

	static const char kEmpty[] = "";

	const Gun* Find(const std::string& item)
	{
		std::string name = item;
		for (char& c : name) {
			if (c >= 'a' && c <= 'z') {
				c = static_cast<char>(c - 'a' + 'A');
			}
		}
		if (name.compare(0, 16, "EINVENTORY_ITEM_") == 0) {
			name.erase(0, 16);
		}
		for (const Gun& gun : kGuns) {
			if (name == gun.mItem) {
				return &gun;
			}
		}
		return nullptr;
	}

	void Install()
	{
		// GetCost: test ecx, ecx; jnz; xor eax, eax; ret; mov rax, ItemProfiles::mpInstance; movsxd rcx, ecx;
		// imul rcx, 38h; mov eax, [rcx+rax+4].
		uint8_t* getCost = scan::FindUnique("UFG::ItemProfiles::GetCost", "85 C9 75 03 33 C0 C3 48 8B 05 ? ? ? ? 48 63 C9 48 6B C9 38");
		if (!getCost || !scan::Matches(getCost + 0x15, "8B 44 01 04 C3")) {
			LOG("items: item profiles MISSING or not as expected, guns keep their placeholder prices");
			return;
		}
		gProfiles = static_cast<uint8_t**>(scan::RipTarget(getCost + 0xA));
	}

	bool SetGun(const Gun& gun, int price)
	{
		uint8_t* profiles = gProfiles ? *gProfiles : nullptr;
		if (!profiles || gun.mEnum <= 0 || gun.mEnum >= kItems) {
			return false;
		}
		uint8_t* entry = profiles + static_cast<size_t>(gun.mEnum) * kProfileSize;
		if (!mem::Readable(entry, kProfileSize)) {
			return false;
		}
		const GunText& texts = kTexts[&gun - kGuns];
		const char* name = text::Get(texts.mName);
		const char* description = text::Get(texts.mDescription);
		const int32_t oldPrice = mem::Read<int32_t>(entry, 0x4);
		const char* oldName = mem::Read<const char*>(entry, 0x18);
		std::memcpy(entry + 0x4, &price, sizeof(price));
		std::memcpy(entry + 0x18, &name, sizeof(name));
		for (size_t i = 0; i < 3; ++i) {
			std::memcpy(entry + 0x20 + i * sizeof(void*), &description, sizeof(description));
		}
		// Guns without a profile have no captioning; the prompt copies it.
		if (!mem::Read<const char*>(entry, 0x10)) {
			const char* empty = kEmpty;
			std::memcpy(entry + 0x10, &empty, sizeof(empty));
		}
		LOG("items: %s: HK$%d (was %d), \"%s\" (was \"%s\")", gun.mItem, price, oldPrice, name,
			oldName && mem::Readable(oldName, 1) ? oldName : "");
		return true;
	}
}
