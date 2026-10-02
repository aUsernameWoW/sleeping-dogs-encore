#pragma once

// The gun vendor the cut favour "Piece of Work" (F_PW) was built around: Granny Annie sends Wei to a shady
// vendor who sells him a .45 (`_wait_for_scripted_social_dialogue('eFACEACTION_PURCHASE', ...,
// 'eINVENTORY_ITEM_PISTOL_45CAL')`). The engine's purchase path knows guns (a "gun" prompt icon for seven of them,
// the CashSpentOnWeapons / WeaponsPurchased stats) but nothing shipped uses it. Here a vendor stands at each spot
// of the ini while the player is near and sells that spot's gun the way street vendors sell food; the gun itself
// is handed over with Character.equip_firearm, so it comes loaded and works however the exchange animation goes.

#include <string>

namespace vendor
{
	void Install();
}
