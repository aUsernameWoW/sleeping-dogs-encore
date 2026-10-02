#include "vendor.hh"

#include <Windows.h>

#include <cmath>
#include <cstdio>
#include <string>
#include <vector>

#include "config.hh"
#include "items.hh"
#include "log.hh"
#include "skookum.hh"
#include "text.hh"

namespace vendor
{
	// Selling, once the vendor stands: like F_PW's vendor, a scripted purchase conversation for the gun, again and
	// again. A purchase shows as money gone when the conversation ends; the gun is then equipped the way scripts
	// arm the player (equip_firearm: a new Firearm, the old one stowed or dropped), not through the purchase
	// animation's item exchange, which no shipped vendor does with a gun.
	static constexpr char kSell[] = R"sk(
				loop [
					!money
					money: player.money_get()
					vendor._wait_for_scripted_social_dialogue('eFACEACTION_PURCHASE', 0, false, 'eINVENTORY_ITEM_{ITEM}')
					Debug.println("[SDEncore] {TAG}: conversation over, money ", money, " -> ", player.money_get(), ", success: ", vendor.face_is_action_success())
					if player.money_get() < money [
						player.equip_firearm('{FIREARM}')
						Debug.println("[SDEncore] {TAG}: sold {ITEM}; the player holds ", player.get_firearm())
					]
					_wait(1.0)
				]
)sk";

	// Until the player has gone {FAR} m away, or the vendor is down or gone.
	static constexpr char kStay[] = R"sk(
				loop [
					_wait(1.0)
					if vendor.is_valid_simobject().not() [
						exit
					]
					if vendor.is_knocked_out() [
						Debug.println("[SDEncore] {TAG}: the vendor is down")
						exit
					]
					if player.distance(pos) > {FAR} [
						exit
					]
				]
)sk";

	// One spot: wait for the player to come within {NEAR} m, put the vendor there, sell until the player leaves,
	// despawn him once out of sight; after a knockout, not again until the player has left.
	static constexpr char kSpot[] = R"sk(
!pos !facing !player
pos: Vector3!xyz({X}, {Y}, {Z})
facing: Vector3!xyz({FX}, {FY}, {Z})
player: World.c_player
Debug.println("[SDEncore] {TAG}: selling {ITEM} at ", pos)
loop [
	_wait(1.0)
	if player.is_valid_simobject() [
		if player.distance(pos) < {NEAR} [
			!vendor
			vendor: Character.create_at_pos(pos, facing, '{CHARACTER}', "SDEncore_GunVendor", nil, true)
			if vendor.is_nil() [
				Debug.println("[SDEncore] {TAG}: the vendor didn't spawn")
				_wait(10.0)
			]
			else [
				Debug.println("[SDEncore] {TAG}: ", vendor, " at ", vendor.get_pos(), ", the player ", player.distance(pos), " m away")
				vendor.set_suspend_option('PedSuspendOption_NoSuspend')
				vendor.face_set_requires_greet(false)
				race [
					[
						{SELL}
					]
					[
						{STAY}
					]
				]
				vendor.set_suspend_option('PedSuspendOption_SuspendAllowed')
				vendor.despawn(true)
				loop [
					if player.distance(pos) > {FAR} [
						exit
					]
					_wait(1.0)
				]
				Debug.println("[SDEncore] {TAG}: the player left, the vendor goes")
			]
		]
	]
]
)sk";

	// The development key: a vendor 2 m in front of the player, facing them; the spot goes to the log in the ini's
	// format (core/vendor.cc reads the two tagged positions).
	static constexpr char kHere[] = R"sk(
!player !pos !facing !vendor !dir
player: World.c_player
dir: player.get_dir()
pos: player.get_pos() + (dir *= 2.0)
facing: player.get_pos()
Debug.println("[SDEncore:spot]", pos)
Debug.println("[SDEncore:facing]", facing)
vendor: Character.create_at_pos(pos, facing, '{CHARACTER}', "SDEncore_GunVendor", nil, true)
if vendor.is_nil() [
	Debug.println("[SDEncore] {TAG}: the vendor didn't spawn")
]
else [
	Debug.println("[SDEncore] {TAG}: ", vendor, " at ", vendor.get_pos())
	vendor.set_suspend_option('PedSuspendOption_NoSuspend')
	vendor.face_set_requires_greet(false)
	race [
		[
			{SELL}
		]
		[
			{STAY}
		]
	]
	vendor.set_suspend_option('PedSuspendOption_SuspendAllowed')
	vendor.despawn(true)
	Debug.println("[SDEncore] {TAG}: done")
]
)sk";

	static constexpr float kNear = 60.0f;
	static constexpr float kFar = 90.0f;
	static constexpr char kTestItem[] = "PISTOL_45CAL";

	struct Spot
	{
		VendorSpot mSpot;
		const items::Gun* mGun = nullptr;
		skookum::Run* mRun = nullptr;
		DWORD mLastStart = 0;
	};
	static std::vector<Spot> gSpots;
	static skookum::Run* gHereRun = nullptr;
	static bool gKeyWasDown = false;
	static text::Language gPricedIn = text::Language::English;
	static bool gPriced = false;
	static float gSpotPos[3] = {};
	static bool gHaveSpotPos = false;

	static std::string Number(float value)
	{
		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.2f", value);
		return buffer;
	}

	static int Price(const items::Gun& gun)
	{
		switch (gun.mClass) {
		case 1:
			return gConfig.mPriceSmg;
		case 2:
			return gConfig.mPriceShotgun;
		case 3:
			return gConfig.mPriceRifle;
		default:
			return gConfig.mPricePistol;
		}
	}

	static std::string Fill(std::string script, const items::Gun& gun, const std::string& tag)
	{
		text::Replace(script, "{SELL}", kSell);
		text::Replace(script, "{STAY}", kStay);
		text::Replace(script, "{ITEM}", gun.mItem);
		text::Replace(script, "{FIREARM}", gun.mFirearm);
		text::Replace(script, "{CHARACTER}", gConfig.mVendorCharacter);
		text::Replace(script, "{NEAR}", Number(kNear));
		text::Replace(script, "{FAR}", Number(kFar));
		text::Replace(script, "{TAG}", tag);
		return script;
	}

	// The item profiles hold the prompt's text, so they follow the language once the game has told it.
	static void ApplyPrices()
	{
		if (gPriced && gPricedIn == text::Current()) {
			return;
		}
		bool ok = true;
		for (const Spot& spot : gSpots) {
			ok = items::SetGun(*spot.mGun, Price(*spot.mGun)) && ok;
		}
		if (gConfig.mVendorHereKey) {
			ok = items::SetGun(*items::Find(kTestItem), Price(*items::Find(kTestItem))) && ok;
		}
		gPriced = ok;
		gPricedIn = text::Current();
	}

	static bool GameHasFocus()
	{
		DWORD pid = 0;
		GetWindowThreadProcessId(GetForegroundWindow(), &pid);
		return pid == GetCurrentProcessId();
	}

	// "Vector3(x, y, z)", as skookum::Describe writes it.
	static bool ParseVector(const std::string& text, float* v)
	{
		return sscanf_s(text.c_str(), "Vector3(%f, %f, %f)", &v[0], &v[1], &v[2]) == 3;
	}

	static void OnSpot(const void* instance)
	{
		gHaveSpotPos = ParseVector(skookum::Describe(instance, false), gSpotPos);
	}

	static void OnFacing(const void* instance)
	{
		float facing[3];
		if (!gHaveSpotPos || !ParseVector(skookum::Describe(instance, false), facing)) {
			LOG("vendor: couldn't read the test vendor's spot");
			return;
		}
		// Heading: degrees from +Y towards +X (the ini's convention; Install turns it back into a facing point).
		const float heading = std::atan2(facing[0] - gSpotPos[0], facing[1] - gSpotPos[1]) * 57.2957795f;
		LOG("vendor: test vendor's spot, for the ini: Spot = %.2f, %.2f, %.2f, %.0f, %s", gSpotPos[0], gSpotPos[1], gSpotPos[2],
			heading < 0 ? heading + 360.0f : heading, kTestItem);
		gHaveSpotPos = false;
	}

	static void Tick(float)
	{
		if (!skookum::Loaded()) {
			return;
		}
		ApplyPrices();
		const DWORD now = GetTickCount();
		for (size_t i = 0; i < gSpots.size(); ++i) {
			Spot& spot = gSpots[i];
			if (spot.mRun && spot.mRun->mFinished) {
				LOG("vendor: spot %zu's script ended (a load or a reset), restarting it", i + 1);
				spot.mRun = nullptr;
			}
			if (spot.mRun || (spot.mLastStart && now - spot.mLastStart < 5000)) {
				continue;
			}
			spot.mLastStart = now;
			const float radians = spot.mSpot.mHeading / 57.2957795f;
			std::string script = kSpot;
			text::Replace(script, "{X}", Number(spot.mSpot.mX));
			text::Replace(script, "{Y}", Number(spot.mSpot.mY));
			text::Replace(script, "{Z}", Number(spot.mSpot.mZ));
			text::Replace(script, "{FX}", Number(spot.mSpot.mX + std::sin(radians)));
			text::Replace(script, "{FY}", Number(spot.mSpot.mY + std::cos(radians)));
			char tag[32];
			snprintf(tag, sizeof(tag), "vendor %zu", i + 1);
			skookum::Run* run = skookum::Start(tag, Fill(script, *spot.mGun, tag));
			if (!run->mFinished) {
				spot.mRun = run;
			}
		}

		if (gHereRun && gHereRun->mFinished) {
			gHereRun = nullptr;
		}
		if (!gConfig.mVendorHereKey) {
			return;
		}
		const bool down = (GetAsyncKeyState(gConfig.mVendorHereKey) & 0x8000) != 0;
		const bool pressed = down && !gKeyWasDown;
		gKeyWasDown = down;
		if (!pressed || !GameHasFocus()) {
			return;
		}
		if (gHereRun) {
			LOG("vendor: a test vendor is still out; leave him (%.0f m) first", kFar);
			return;
		}
		LOG("vendor: test vendor in front of the player");
		skookum::Run* run = skookum::Start("test vendor", Fill(kHere, *items::Find(kTestItem), "test vendor"));
		if (!run->mFinished) {
			gHereRun = run;
		}
	}

	void Install()
	{
		items::Install();
		if (!gConfig.mVendor) {
			LOG("vendor: off");
			return;
		}
		if (!skookum::Ready()) {
			LOG("vendor: no scripts, no vendor");
			return;
		}
		for (const VendorSpot& s : gConfig.mVendorSpots) {
			const items::Gun* gun = items::Find(s.mItem);
			if (!gun) {
				LOG("vendor: unknown gun \"%s\" at (%.1f, %.1f, %.1f), spot left out", s.mItem.c_str(), s.mX, s.mY, s.mZ);
				continue;
			}
			gSpots.push_back({ s, gun });
			LOG("vendor: spot %zu at (%.1f, %.1f, %.1f) facing %.0f: %s for HK$%d", gSpots.size(), s.mX, s.mY, s.mZ, s.mHeading, gun->mItem,
				Price(*gun));
		}
		skookum::OnPrintTag("[SDEncore:spot]", &OnSpot);
		skookum::OnPrintTag("[SDEncore:facing]", &OnFacing);
		skookum::OnTick(&Tick);
		LOG("vendor: %zu spot(s)%s", gSpots.size(), gConfig.mVendorHereKey ? "; the test key puts one in front of the player" : "");
	}
}
