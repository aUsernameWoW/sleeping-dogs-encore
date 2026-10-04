#include "contacts.hh"

#include <Windows.h>

#include <atomic>
#include <cstdio>
#include <cstring>
#include <string>

#include "config.hh"
#include "log.hh"
#include "phone.hh"
#include "skookum.hh"
#include "text.hh"

namespace contacts
{
	// The lines. The originals' placeholders, as written for the gameslices ("[ Placeholder ] Wei: Bring me a
	// gun."), are kept where they read fine; the failure lines say why instead of "Can't, sorry. (ERROR)", and the
	// boat contact's says what the original only hoped for ("Once I can get a random boat spawn position, this
	// will work."): the game now has World._find_boat_spawn_xform.
	static constexpr text::Line kWei = { "Wei", "沈偉", "沈伟" };
	static constexpr text::Line kSeparator = { ": ", "：", "：" };
	static constexpr text::Line kOnMyWay = { "Whatever you need. I'll be right there.", "沒問題，我馬上到。", "没问题，我马上到。" };
	static constexpr text::Line kNoWay = { "Can't get to you there, sorry.", "你那邊我過不去，抱歉。", "你那边我过不去，抱歉。" };

	struct Service
	{
		const char* mKey;        // phone contact key, log tag, override file SDEncore-<key>.sk
		const char* mNameKey;    // the cut contact's name in the game's text
		const char* mPortrait;
		text::Line mInfo;
		text::Line mAsk;         // Wei
		text::Line mAnswer;      // the contact
		text::Line mFail;        // the contact, nothing found to spawn at
		text::Line mHandover;    // the contact, on delivery
		ServiceConfig* mConfig;
		const char* mScript;
		// Runtime (game thread, except the two flags set from the phone).
		std::atomic<bool> mPending{ false };
		std::atomic<bool> mHangUp{ false };
		skookum::Run* mRun = nullptr;
	};

	// The spot search, the call and the hang-up, shared by all five. {SEARCH} fills `found` and `spawn_xform`.
	static constexpr char kCall[] = R"sk(
!player !spawn_xform !found
player: World.c_player
Debug.println("[SDEncore] {TAG}: called at ", player.get_pos(), "; active master: ", GameSlice.get_active_master())
spawn_xform: Transform!()
found: false
{SEARCH}
if found.not() [
	Debug.println("[SDEncore] {TAG}: no spawn spot near the player")
	NIS.c_default._flash_subtitles({FAIL})
	if {HANGUP} [
		PDA.end_phone_call()
	]
]
else [
	Debug.println("[SDEncore] {TAG}: spawning at ", spawn_xform.get_pos(), ", ", player.get_pos().distance(spawn_xform.get_pos()), " m from the player")
	NIS.c_default._flash_subtitles({ASK})
	NIS.c_default._flash_subtitles({ANSWER})
	if {HANGUP} [
		PDA.end_phone_call()
	]
	{BODY}
]
)sk";

	// Like the originals: 20-50 m, on screen or not; then, unlike them, out of sight up to 120 m.
	static constexpr char kRoadSearch[] = R"sk(
c_world._find_vehicle_spawn_xform(player.get_pos(), 20.0, 50.0, false, 3.0, found, spawn_xform)
if found.not() [
	c_world._find_vehicle_spawn_xform(player.get_pos(), 40.0, 120.0, true, 3.0, found, spawn_xform)
]
)sk";

	static constexpr char kWaterSearch[] = R"sk(
c_world._find_boat_spawn_xform(player.get_pos(), 15.0, 150.0, false, 3.0, found, spawn_xform)
if found.not() [
	c_world._find_boat_spawn_xform(player.get_pos(), 15.0, 300.0, false, 3.0, found, spawn_xform)
]
)sk";

	// A man in a car, sent to the player (WeaponContact, GunBackup, MeleeBackup, SWAT Contact). {SERVICE} runs once
	// he's on the way; afterwards he goes ({LEAVE} if he's still up) and is despawned once out of sight, as the
	// gameslices' cleanup did (despawn(true)).
	static constexpr char kCourier[] = R"sk(
	!car !contact !met
	car: c_world.spawn_object_at_xform(spawn_xform, '{VEHICLE}', "SDEncore_{KEY}_Car")<>Vehicle
	contact: Character.create_at_pos(spawn_xform.get_pos() + spawn_xform.get_dir_left() *= 2.0, nil, '{CHARACTER}', "SDEncore_{KEY}", {BEHAVIOUR}, false)
	if car.is_nil() or contact.is_nil() [
		Debug.println("[SDEncore] {TAG}: spawning failed: car ", car, ", contact ", contact)
		car%despawn()
		contact%despawn()
	]
	else [
		contact.set_suspend_option('PedSuspendOption_NoSuspend')
		contact.minimap_add_blip("friendly", false)
		{ARM}
		contact.force_enter_vehicle(car, true)
		{FACTION}
		contact.get_properties().append_property_boolean('CanEnterExitVehicle', true)
		contact.enable_script_control(false)
		car.set_driving_role("Ally")
		contact.set_objective_and_actor("{OBJECTIVE}", player)
		Debug.println("[SDEncore] {TAG}: ", contact, " in ", car, " is on the way")
		met: false
		{SERVICE}
		contact%minimap_remove_blip()
		if contact.is_valid_simobject() [
			if contact.is_knocked_out().not() [
				contact.set_objective_and_actor("eAI_OBJECTIVE_NONE", player)
				contact.set_can_wander(true)
				{LEAVE}
			]
			contact.set_suspend_option('PedSuspendOption_SuspendAllowed')
			contact.despawn(true)
		]
		{AFTER}
		Debug.println("[SDEncore] {TAG}: done (met the player: ", met, ")")
	]
)sk";

	// Until the player is within 2 m of him (the original's _wait_near_actor), 4 minutes, or he's down.
	static constexpr char kMeet[] = R"sk(
		race [
			[
				player._wait_near_actor(contact, 2.0)
				met := true
				Debug.println("[SDEncore] {TAG}: met the player at ", player.get_pos())
			]
			[
				_wait(240.0)
				Debug.println("[SDEncore] {TAG}: not at the player after 4 minutes")
			]
			[
				loop [
					if contact.is_valid_simobject().not() [
						exit
					]
					if contact.is_knocked_out() [
						exit
					]
					_wait(1.0)
				]
				Debug.println("[SDEncore] {TAG}: the contact is down or gone")
			]
		]
)sk";

	// Back to his car and away.
	static constexpr char kDriveOff[] = R"sk(
				if car.is_valid_simobject() [
					race [
						contact._enter_vehicle(car, true)
						_wait(30.0)
					]
					if contact.is_the_driver(car) [
						car.wander(true)
					]
				]
)sk";

	// WeaponContact: the gun at the meeting, under a letterbox as the original had it.
	static constexpr char kWeaponService[] = R"sk(
		{MEET}
		if met [
			player.enable_player_script_control(true)
			NIS.show_letterbox()
			NIS.c_default._flash_subtitles({HANDOVER})
			player.equip_firearm('{WEAPON}')
			NIS.hide_letterbox()
			player.enable_player_script_control(false)
			Debug.println("[SDEncore] {TAG}: handed over {WEAPON}; the player holds ", player.get_firearm())
		]
)sk";

	// GunBackup / MeleeBackup: he fights with the player until he's down (the original's _wait_until_fight_over
	// on the backup himself); unlike the original (FOLLOW_TARGET, which only follows) as an ally that attacks. He
	// leaves if the player has been out of reach for 30 s, or after the time limit.
	static constexpr char kBackupService[] = R"sk(
		[
		!far
		far: 0
		race [
			[
				_wait_until_fight_over({contact})
				Debug.println("[SDEncore] {TAG}: the backup is down")
			]
			[
				loop [
					_wait(1.0)
					if contact.is_valid_simobject().not() [
						exit
					]
					if player.distance_actor(contact) > {LEASH} [
						far := far + 1
						if far >= 30 [
							Debug.println("[SDEncore] {TAG}: the player left the backup behind")
							exit
						]
					]
					else [
						far := 0
					]
				]
			]
			{TIMELIMIT}
		]
		]
)sk";

	// SWAT Contact: the truck at the meeting. The original left him in the driver's seat; he gets out and walks off,
	// and the truck is unlocked and marked until the player drives it.
	static constexpr char kSwatService[] = R"sk(
		{MEET}
		if met [
			player.enable_player_script_control(true)
			NIS.show_letterbox()
			NIS.c_default._flash_subtitles({HANDOVER})
			NIS.hide_letterbox()
			player.enable_player_script_control(false)
			if contact.is_the_driver(car) [
				race [
					contact._exit_vehicle()
					_wait(10.0)
				]
			]
			if car.is_valid_simobject() [
				car.set_parked(false)
				car.minimap_add_blip("friendly", false)
				Debug.println("[SDEncore] {TAG}: the truck is the player's, at ", car.get_pos())
			]
		]
)sk";

	static constexpr char kSwatAfter[] = R"sk(
		if met [
			race [
				[
					loop [
						if car.is_valid_simobject().not() [
							exit
						]
						if player.is_the_driver(car) [
							Debug.println("[SDEncore] {TAG}: the player drives the truck")
							exit
						]
						_wait(0.5)
					]
				]
				_wait(300.0)
			]
			car%minimap_remove_blip()
		]
)sk";

	// BoatContact: an empty boat at the water nearest the player (a man driving it couldn't reach a player on land),
	// unlocked and marked until the player takes it.
	static constexpr char kBoat[] = R"sk(
	!boat !boarded
	boat: c_world.spawn_object_at_xform(spawn_xform, '{VEHICLE}', "SDEncore_{KEY}_Boat")<>Vehicle
	if boat.is_nil() [
		Debug.println("[SDEncore] {TAG}: the boat didn't spawn")
	]
	else [
		boat.set_parked(false)
		boat.minimap_add_blip("friendly", false)
		Debug.println("[SDEncore] {TAG}: ", boat, " at ", boat.get_pos(), ", in water: ", boat.is_boat_in_water())
		NIS.c_default._flash_subtitles({HANDOVER})
		boarded: false
		race [
			[
				loop [
					if boat.is_valid_simobject().not() [
						exit
					]
					if player.is_the_driver(boat) [
						boarded := true
						exit
					]
					_wait(0.5)
				]
			]
			_wait(300.0)
		]
		boat%minimap_remove_blip()
		if boarded [
			Debug.println("[SDEncore] {TAG}: the player took the boat")
		]
		else [
			Debug.println("[SDEncore] {TAG}: nobody took the boat in 5 minutes")
		]
	]
)sk";

	enum Kind
	{
		kWeapon,
		kGun,
		kMelee,
		kBoatKind,
		kSwat,
		kKinds,
	};

	static Service gServices[kKinds] = {
		{ "WeaponContact", "$PDA_CONTACT_WEAPON", "Portrait_Smartphone_Unknown",
			{ "Has a gun brought to you", "派人給你送一把槍", "派人给你送一把枪" },
			{ "Bring me a gun.", "幫我送把槍過來。", "帮我送把枪过来。" }, kOnMyWay, kNoWay,
			{ "Here you go.", "拿去。", "拿去。" }, &gConfig.mWeaponContact, nullptr },
		{ "GunBackup", "$PDA_CONTACT_GUN", "Portrait_Smartphone_Unknown",
			{ "A Water Street gunman watches your back", "派一個帶槍的水街兄弟幫你撐場", "派一个带枪的水街兄弟帮你撑场" },
			{ "I need backup!", "我需要支援！", "我需要支援！" }, { "On my way.", "馬上到。", "马上到。" }, kNoWay,
			{ "", "", "" }, &gConfig.mGunBackup, nullptr },
		{ "MeleeBackup", "$PDA_CONTACT_MELEE", "Portrait_Smartphone_Unknown",
			{ "A Water Street brawler watches your back", "派一個拿刀的水街兄弟幫你撐場", "派一个拿刀的水街兄弟帮你撑场" },
			{ "I need backup!", "我需要支援！", "我需要支援！" }, { "On my way.", "馬上到。", "马上到。" }, kNoWay,
			{ "", "", "" }, &gConfig.mMeleeBackup, nullptr },
		{ "BoatContact", "$PDA_CONTACT_BOAT", "Portrait_Smartphone_Unknown",
			{ "Has a boat brought to the nearest water", "把快艇送到最近的水邊", "把快艇送到最近的水边" },
			{ "Bring me a boat.", "幫我弄艘快艇來。", "帮我弄艘快艇来。" }, kOnMyWay,
			{ "There's no water near you. Call me from the waterfront.", "你附近沒有水路，到岸邊再打給我。", "你附近没有水路，到岸边再打给我。" },
			{ "It's moored by the water. The keys are in it.", "船泊在水邊，鑰匙在船上。", "船泊在水边，钥匙在船上。" },
			&gConfig.mBoatContact, nullptr },
		{ "SwatContact", "$PDA_CONTACT_SWAT", "Portrait_Smartphone_SWAT",
			{ "Has a SWAT truck brought to you", "派人給你送一輛特警車", "派人给你送一辆特警车" },
			{ "Bring me a SWAT truck.", "幫我弄輛特警車來。", "帮我弄辆特警车来。" }, kOnMyWay,
			{ "Can't get a truck to you there, sorry.", "車開不到你那邊，抱歉。", "车开不到你那边，抱歉。" },
			{ "The truck is over there. Here are the keys.", "車就在那邊，鑰匙給你。", "车就在那边，钥匙给你。" },
			&gConfig.mSwatContact, nullptr },
	};

	static std::wstring gDir;

	// Wei's line, or the contact's under the name the game's text gives him. No parentheses around the sum: Skookum
	// has none for grouping (a `(` starting an expression opens a closure's parameter list, and the compile fails),
	// and `+` takes the rest of the argument as its operand anyway (operators are right-associative, no precedence).
	static std::string Say(const char* nameKey, const text::Line& line)
	{
		const std::string said = std::string(text::Get(kSeparator)) + text::Get(line);
		if (!nameKey) {
			return text::Literal(text::Get(kWei) + said);
		}
		return "UI.localize_string(" + text::Literal(nameKey) + ") + " + text::Literal(said);
	}

	static std::string Number(float value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.1f", value);
		return buffer;
	}

	// The placeholders an override file gets too: the lines, the hang-up and the configured property sets.
	static void Fill(std::string& script, const Service& s, bool hangUp)
	{
		text::Replace(script, "{ASK}", Say(nullptr, s.mAsk));
		text::Replace(script, "{ANSWER}", Say(s.mNameKey, s.mAnswer));
		text::Replace(script, "{FAIL}", Say(s.mNameKey, s.mFail));
		text::Replace(script, "{HANDOVER}", Say(s.mNameKey, s.mHandover));
		text::Replace(script, "{HANGUP}", hangUp ? "true" : "false");
		text::Replace(script, "{VEHICLE}", s.mConfig->mVehicle);
		text::Replace(script, "{CHARACTER}", s.mConfig->mCharacter);
		text::Replace(script, "{WEAPON}", s.mConfig->mWeapon);
		text::Replace(script, "{KEY}", s.mKey);
		text::Replace(script, "{TAG}", s.mKey);
	}

	static std::string Build(Kind kind, const Service& s, bool hangUp)
	{
		std::string script = kCall;
		text::Replace(script, "{SEARCH}", kind == kBoatKind ? kWaterSearch : kRoadSearch);
		text::Replace(script, "{BODY}", kind == kBoatKind ? kBoat : kCourier);
		if (kind != kBoatKind) {
			const bool backup = kind == kGun || kind == kMelee;
			text::Replace(script, "{SERVICE}", backup ? kBackupService : kind == kSwat ? kSwatService : kWeaponService);
			text::Replace(script, "{MEET}", kMeet);
			text::Replace(script, "{LEAVE}", kind == kSwat ? "if met.not() [\n" + std::string(kDriveOff) + "\n]" : backup ? "" : kDriveOff);
			text::Replace(script, "{AFTER}", kind == kSwat ? kSwatAfter : "");
			text::Replace(script, "{ARM}", kind == kGun ? "contact.equip_firearm('{WEAPON}')" : kind == kMelee ? "contact.equip_melee('{WEAPON}')" : "");
			text::Replace(script, "{FACTION}", kind == kSwat ? "" : "contact.set_faction('TRIAD_WINSTON')");
			text::Replace(script, "{BEHAVIOUR}", kind == kSwat ? "nil" : "\"Thug_behaviour.act\"");
			text::Replace(script, "{OBJECTIVE}", backup ? "eAI_OBJECTIVE_BE_ALLY" : "eAI_OBJECTIVE_FOLLOW_TARGET");
			text::Replace(script, "{LEASH}", Number(gConfig.mBackupLeash));
			text::Replace(script, "{TIMELIMIT}", gConfig.mBackupMinutes > 0
				? "[\n_wait(" + Number(gConfig.mBackupMinutes * 60.0f) + ")\nDebug.println(\"[SDEncore] {TAG}: time is up\")\n]" : "");
		}
		Fill(script, s, hangUp);
		return script;
	}

	// SDEncore-<key>.sk next to the .asi, read at every call, replaces the built script (development); it gets the
	// same placeholders.
	static bool ReadOverride(const Service& s, std::string& text)
	{
		const std::wstring path = gDir + L"\\SDEncore-" + std::wstring(s.mKey, s.mKey + strlen(s.mKey)) + L".sk";
		FILE* file = nullptr;
		if (_wfopen_s(&file, path.c_str(), L"rb") != 0 || !file) {
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

	static void Tick(float)
	{
		// The language is known before the first call: text.cc asks as soon as the game's text is loaded.
		text::Detect();
		for (int i = 0; i < kKinds; ++i) {
			Service& s = gServices[i];
			if (s.mRun && s.mRun->mFinished) {
				LOG("contacts: %s over, the contact is back", s.mKey);
				s.mRun = nullptr;
			}
			if (!s.mPending.exchange(false)) {
				continue;
			}
			if (s.mRun) {
				LOG("contacts: %s is already on it, call ignored", s.mKey);
				continue;
			}
			std::string source;
			const bool overridden = ReadOverride(s, source);
			if (overridden) {
				Fill(source, s, s.mHangUp);
			}
			else {
				source = Build(static_cast<Kind>(i), s, s.mHangUp);
			}
			LOG("contacts: %s (%s script, %s; vehicle %s, character %s, weapon %s)", s.mKey, overridden ? "override file" : "built-in",
				text::Name(text::Current()), s.mConfig->mVehicle.c_str(), s.mConfig->mCharacter.c_str(), s.mConfig->mWeapon.c_str());
			skookum::Run* run = skookum::Start(s.mKey, source);
			if (!run->mCompiled && s.mHangUp) {
				// The script hangs up itself; without it the call screen would stay on "Connected".
				skookum::Start("hang-up", "PDA.end_phone_call()");
			}
			if (!run->mFinished) {
				s.mRun = run;
			}
		}
	}

	static bool Available(int id)
	{
		const Service& s = gServices[id];
		return skookum::Ready() && !s.mRun && !s.mPending;
	}

	static const char* Info(int id)
	{
		return text::Get(gServices[id].mInfo);
	}

	static void Call(int id, bool callShown)
	{
		Service& s = gServices[id];
		s.mHangUp = callShown;
		s.mPending = true;
	}

	void Install(const std::wstring& dir)
	{
		gDir = dir;
		if (!skookum::Ready()) {
			LOG("contacts: no scripts, no contacts");
			return;
		}
		int added = 0;
		for (int i = 0; i < kKinds; ++i) {
			Service& s = gServices[i];
			if (!s.mConfig->mEnabled) {
				LOG("contacts: %s off", s.mKey);
				continue;
			}
			phone::Contact contact;
			contact.mKey = std::string("SDEncore_") + s.mKey;
			contact.mName = s.mNameKey;
			contact.mInfo = &Info;
			contact.mPortrait = s.mPortrait;
			contact.mAvailable = &Available;
			contact.mCall = &Call;
			contact.mId = i;
			phone::Add(contact);
			++added;
		}
		skookum::OnTick(&Tick);
		LOG("contacts: %d of %d cut contacts in the phone", added, static_cast<int>(kKinds));
	}
}
