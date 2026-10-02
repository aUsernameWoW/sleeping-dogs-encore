#pragma once

// The five phone contacts cut from the game, each a SkookumScript coroutine ported from its gameslice
// (Taylor's test definitions: WeaponContact, GunBackup, MeleeBackup, BoatContact, E_SC "SWAT Contact"). The
// originals waited 5 s, added themselves with PDA.add_contact, waited for the call and showed
// "[ Placeholder ]" subtitles; here the phone contact is ours (core/phone.cc) and the lines are real ones.
// A contact is hidden while its service runs (its definition's repeatableinterval is 5 s: no real cooldown).

#include <string>

namespace contacts
{
	// Registers the enabled contacts with the phone and the service tick with the scripts.
	void Install(const std::wstring& dir);
}
