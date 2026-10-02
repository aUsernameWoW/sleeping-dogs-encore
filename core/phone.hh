#pragma once

// Phone contacts of our own: appended to the phone's contact list (UIHK_PDAPhoneContactsWidget::PopulateList)
// while available, and handled in LaunchSubOption: the call screen the game shows for a mission contact
// (LaunchCallMission's UI part), then the contact's callback. Nothing is added to the game's progression: a
// `PDA.add_contact` trigger, what the cut gameslices used, could end up in the save and outlive the mod as a
// broken "Err2" contact.

#include <cstdint>
#include <string>

namespace phone
{
	struct Contact
	{
		std::string mKey;       // unique, hashed into the contact list's symbol
		std::string mName;      // as shown; a "$KEY" is the game's text for it
		const char* (*mInfo)(int id) = nullptr; // the line under the name, asked each time the list opens
		std::string mPortrait;  // a texture of the phone contacts' pack
		bool (*mAvailable)(int id) = nullptr;
		// `callShown`: the call screen is up and the service should hang up. Called on the UI's thread: queue it.
		void (*mCall)(int id, bool callShown) = nullptr;
		int mId = 0;
	};

	// Registers a contact; before Install.
	void Add(const Contact& contact);

	// Hooks the contact list if any contact was added.
	void Install();
}
