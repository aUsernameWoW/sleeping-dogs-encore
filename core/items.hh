#pragma once

// The guns in the game's item profiles (UFG::ItemProfiles, read from propprofiles-manifest at start): what a
// vendor's purchase prompt shows (name, description, price) and what buying charges. The guns' profiles are
// unfinished: HK$10 each and English names written in place of text keys (the shipped goods use $ITEMS_TITLE_*),
// some guns have none. SetGun fills a gun's entry in memory, for the vendor to sell it.

#include <string>

namespace items
{
	struct Gun
	{
		const char* mItem;      // eINVENTORY_ITEM_<mItem>
		int mEnum;              // its UFG::eInventoryItemEnum value
		const char* mFirearm;   // the type Character.equip_firearm takes (Firearm.type_to_propset_name)
		int mClass;             // 0 pistol, 1 SMG, 2 shotgun, 3 rifle (the ini's prices)
	};

	// The gun called `item` (case-insensitive, with or without the eINVENTORY_ITEM_ prefix), or null.
	const Gun* Find(const std::string& item);

	void Install();

	// Writes the gun's price and text into its item profile. Game thread, once the profiles exist (the scripts
	// running is late enough). Returns false if the profiles aren't there.
	bool SetGun(const Gun& gun, int price);
}
