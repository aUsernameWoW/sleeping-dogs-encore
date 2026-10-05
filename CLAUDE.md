# SDEncore — cut content put back into Sleeping Dogs: Definitive Edition

The user's request (2026-10-02): after the research below on the rumoured hidden gun vendor, "a mod that adds the
cut features back". Scope chosen by the user: the five cut phone contacts and a gun vendor NPC, **faithful to the
originals with the rough edges fixed** (real lines instead of `[ Placeholder ]` subtitles, Chinese included,
obvious bugs fixed). Not in scope yet: rebuilding the favour "Piece of Work" (its world objects weren't found),
other cut missions. Public repo https://github.com/aUsernameWoW/sleeping-dogs-encore (created 2026-10-02 at the
user's request, CI from SDWet's; no Nexus page yet, so the `nexus` jobs skip). Named SDUncut at first (repo
`sleeping-dogs-uncut`, prerelease build-1); renamed the same day to the user's SDEncore.

Built from SDTaxi's core (`skookum.cc`, `phone.cc`, `console.cc`, `crash.cc`, `scan.cc`, `mem.hh`): see
`mods\SDTaxi\CLAUDE.md` for how running SkookumScript from the .asi and adding phone contacts work.

## Research (2026-10-02, installed build)

- **Engine support for buying guns**: `FaceActionComponent::SetIcon` gives seven guns (PISTOL_SERVICE, PISTOL_45CAL,
  RIFLE_ASSAUL_TAC, RIFLE_ASSAULT, RIFLE_ASSAULT_DLC, SHOTGUN_ANTI_TAC, SMG_45CAL_TACLIG) the "gun" purchase icon
  (`UIHKSocialActionWidget::Flash_ShowActionIcon` → `qSymbol_Weapon` → `Icon_SocialPurchase_Weapon`; no
  `Icon_SocialPurchase_*` art exists in any UI movie, for any category). `GameStatAction::Money::PurchaseItem` adds
  gun purchases to `CashSpentOnWeapons` (Int32Stat 70) and `WeaponsPurchased` (Int32RangedStat 0). `StoreFront`
  (menu shops) only has Clothing/Vehicle/Boat inventory types: buying a gun was meant to be a street-vendor style
  purchase (walk up, hold E).
- **Item profiles** (`GlobalProperties.bin`, `propprofiles-eINVENTORY_ITEM_*`, also in
  `reference\SDmodding\Files\GlobalProperties`): every gun costs a placeholder HK$10 and has English written in
  (".45 CAL PISTOL", "A powerful pistol") where shipped goods have `$ITEMS_TITLE_*` keys. Several guns have no
  profile (PISTOL_9MM, SMG_MACHINE_PISTOL, SHOTGUN_PUMP, RIFLE_ASSAULT...). `UFG::ItemProfiles::mpInstance` →
  0x38-byte entries per eInventoryItemEnum: +0x4 cost, +0x10 captioning, +0x18 name, +0x20 descriptions[3].
- `object-physical-character-Vendors-WeaponVendor-*` exists but holds the three fish sellers (Mrs. Ko, Old Man
  Chao, Samson), who sell FISHWRAPPED (the fish is a melee weapon).
- **Cut content in the scripts** (`tools\extract\build\skoo`): favour `F_PieceOfWork` (F_PW: Granny Annie, a shady
  gun vendor in a construction site, raise and lose heat in front of him, buy a .45 with
  `_wait_for_scripted_social_dialogue('eFACEACTION_PURCHASE', 0, false, 'eINVENTORY_ITEM_PISTOL_45CAL')`, hand it to
  her; `[PLACEHOLDER]` dialogue); ambient gameslices `WeaponContact` (WCAmbient), `GunBackup` (GBAmbient),
  `MeleeBackup` (MBAmbient), `BoatContact` (BCAmbient), `E_SwatContact` (SCEvent).
- **Metagame**: mission/favour definitions, objectives and the progression graph are XML in `Global.big`
  `data\global\xmlcache\XML_CacheList_MetaData.bin` (403 entries, same layout as `XML_CacheList.bin`; paths
  truncated with `~` and colliding, so index them; `extract.ps1 xml` doesn't read this cache).
  `ProgressionTracker::LoadGraph` reads `Data\World\Game\MetaGame\...`. F_PW is defined (Definitions_Bryant) with
  objectives localized into 7 languages but has no `<graphitem>` in `ProgressionDependencyGraph_Main`: it can never
  unlock. The five contacts are in `Definitions_Taylor.xml` (a developer's test file), `repeatableinterval="5"`
  (×1000 ms in `XMLWrapperGameSlice`: 5 s, no real cooldown), not in any graph. 139 definitions are in no graph;
  many are older versions or demos (fight clubs E_FC_02-05, safehouse events, demo missions), real cut candidates:
  smuggling ring (F_SRI, E_SR1-4; five `*_SmugglersDen01` world sections exist), E_BA, E_INT, E_TIP, F_BPI, M_CO,
  E_WR, unused races.
- F_PW's world objects (`F_PW-Spawn_GunVendor`, `ProgressionTriggers_Favours-F_PW`, ...) weren't found by name hash
  in the HD zone/section files, but neither were a shipped favour's (F_TTS), so mission scene layers live somewhere
  not yet understood: whether F_PW's layer still exists is open.
- The contacts in `default-unlockables-contactList-list`: names `$PDA_CONTACT_WEAPON` "Weapons contact",
  `$PDA_CONTACT_GUN` "Armed backup", `$PDA_CONTACT_MELEE` "Hand to hand backup", `$PDA_CONTACT_BOAT` "Boat contact",
  `$PDA_CONTACT_SWAT` "SWAT contact" (the Chinese pack translates them, e.g. 武器聯絡人 / 武裝支援 / 特勤聯絡人); the
  `*_INFO` keys don't exist; portraits Unknown, except SWAT's own `Portrait_Smartphone_SWAT` (in the game).
- Script facts used: `GameSlice.faction_set_standing` is an empty method in the shipped scripts (the originals'
  calls did nothing; `World.faction_set_standing` calls it through `c_gameslice`, so it's left out).
  `_wait_until_fight_over(list)` (TSActor) returns when at most `num_fighters_min` (0) of the list are up (not
  knocked out, sim object valid): the backups' "until he's down". `Character.follow` shows FOLLOW_TARGET only
  follows and BE_ALLY fights. `World._find_boat_spawn_xform` exists (C++, no script uses it). Firearm types:
  `Firearm.type_to_propset_name`. `Character.face_set_purchase_item` adds the FaceAction parent, SellableItem and an
  Inventory entry and reloads the components. Purchases: the action tree's `GesturePlayerPurchaseItem` pays
  (`TargetPurchaseItemTrack`: `ItemProfiles::GetCost` adjusted by clothing buffs, `Money::PurchaseItem`, "cha-ching")
  and exchanges the vendor's `SellableItemProp` object (none set: `NoExchange`). `money_get()` reads the player's money.

## What the mod does

- `core/contacts.cc`: the five contacts in the phone (`phone.cc`, generalized from SDTaxi's to several contacts),
  each a script built from templates (`kCall` + `kCourier`/`kBoat` + a service), placeholders filled per
  service; `SDEncore-<Key>.sk` next to the .asi replaces a built script (development; gets the leaf placeholders).
  A contact is hidden while its service runs. Changes from the originals, all deliberate:
  - real lines (Wei / the contact, the contact under the game's name for him via `UI.localize_string`);
  - the spot search falls back to 40-120 m out of sight; failures say why;
  - backups: BE_ALLY instead of FOLLOW_TARGET (so they fight), `NoSuspend` for both, leave when the player has been
    > Leash m away for 30 s, optional time limit;
  - boat: an empty boat at `_find_boat_spawn_xform` (15-150 m, then 300 m), unlocked and blipped until boarded
    (the original sent a thug driving a boat on the road search: "Once I can get a random boat spawn position,
    this will work.");
  - SWAT: a SWAT officer (`CJ_SWAT01_Character`) instead of the placeholder thug, on the thug AI with faction
    `TRIAD_WINSTON` like every courier. He first had his archetype's cop AI and faction LAW: the cop AI's own police
    logic (`Cop_behaviour.act` `PRIVATE_BANK\InfractionResponse`, `Objectives\Investigate`) replaces the
    objective with a pursuit or an investigation, after which a cop at the wheel patrols
    (`Actions\DrivingDummy\PatrolFallback`): on 2026-10-05 he joined police patrols he met and drove off; the cop
    AI's Follow also kept him at the wheel, so the player had to stand at the driver's door. With the thug AI's
    Follow (build-14) the SWAT archetype never drove: the truck stayed where it spawned (cause unknown). So the
    officer has no objective, and the script drives the truck like SDTaxi's taxi (`_path_to_xform(player
    xform, true)` re-aimed every 5 s until within 12 m, then `stop()` repeated while he gets out), and walks him
    over (`_path_to_actor`); the speed is logged every 2 s on the way. The player at the stopped truck (3.5 m)
    also counts as meeting him; he walks off afterwards; the truck unlocked and blipped until the player drives
    it. The archetype and the truck (`CopSUV`) still have
    `default-component-CopUnit` (`UsableByCopSystem` true, read once in `CopUnitComponent::OnAttach`), so during a
    police chase `CopSystem::Reacquire` could still take them over (untested);
  - a courier who's done walks or drives off and is despawned out of sight only once 40 m from the player or after
    90 s (`despawn(true)` waits for the ped's suspension, which comes as soon as he's off screen: the SWAT officer
    vanished the moment the camera turned); his car goes with him if he drives it.
  - every courier gets out of a car the casual way (`UseCasualGetInGetOutAnims`), so the door gets shut. The thug
    AI's Follow behaviour (`Thug_behaviour.act` `Objectives\Follow\VehicleBehaviour\TargetOnFoot`) parks within
    20 m, exits and jogs off once the get-out stops being uninterruptible (Jog → `Vehicle\Queries\GetOutFast`, 1.2 s
    into `Car_Drive_Out`); the car's `DoorControllers\Car\Driver\GetOut\Open\Regular` leaves the door swinging
    (`ATT_SIMULATED_NO_MOTOR`) when the driver is gone before 1.6 s, and the man then ran on the spot behind it
    until the player walked over (reported 2026-10-05, build-8 on the second machine; the user thought the SWAT
    officer, who gets out by script after the meeting, didn't). `Car_Casual` is uninterruptible until 2.25 s and its
    door controller has no interrupted branch. Vans and trucks have no casual get-out. `kWatch` logs where he got out
    and a 5 s standstill on his way to the player.
- `core/vendor.cc` + `core/items.cc`: gun vendors at the ini's spots (none by default yet: the spots are to be picked
  with the user in game). Each spot is a looping script: within 60 m the vendor (a Water Street thug) spawns,
  `face_set_requires_greet(false)`, then `_wait_for_scripted_social_dialogue('eFACEACTION_PURCHASE', ..., item)` over
  and over; money gone = sold → `player.equip_firearm(type)`; he goes when the player is 90 m away (or he's down).
  `items::SetGun` writes price/name/description into the item profile (language-dependent text, redone when the
  language is known). `[Debug] VendorHereKey` puts a test vendor (PISTOL_45CAL) 2 m in front of the player and logs
  the spot in the ini's format (tags `[SDEncore:spot]`/`[SDEncore:facing]`).
- `core/text.cc`: `Language = auto` asks the game (`UI.localize_string` of three contact names, tag
  `[SDEncore:lang]`): English text → English, CJK → Traditional or Simplified by counting characters that differ.
- `core/skookum.cc` (from SDTaxi) changes: the Debug print patch always happens (tags carry values back); when the
  methods are already replaced (another mod, e.g. SDTaxi), ours is chained in front and hands on everything but our
  tags; `Loaded()`.

## Test round 1 (deployed 2026-10-02; plugins\SDEncore.ini created with Console = 1, VendorHereKey = 0x79 = F10)

To check, from `plugins\SDEncore.log`: the language check; each contact in the phone (names localized?), each call
(subtitles in Chinese? the spawn, `active master` in free roam vs a mission), the boat search, the SWAT officer's
behaviour; F10: the test vendor, the purchase prompt (price/name), whether money goes and the gun works. Open
questions: does the purchase prompt appear on a script-controlled spawned ped; does the game itself also hand
something over; Chinese in Skookum string literals.

## Testing

`tools\build.ps1 -Mod SDEncore -Test -Deploy`. `load_test` loads the .asi outside the game (default ini, every
function reported missing). `script_test` includes `contacts.cc`/`vendor.cc` (Skookum, phone and items stubbed),
builds every script in every language and rejects grouping parentheses (Skookum reads a `(` that starts an expression
as a closure's parameters: `SDTaxi\CLAUDE.md`), unbalanced brackets and unfilled `{PLACEHOLDERS}`.

## Test round 1 result (2026-10-04, the GitHub build on another machine)

The language check worked (English). Calling GunBackup: `Whitespace required` at `(UI.localize_string(...) + ...)`:
`Say` wrapped the contact's lines in parentheses (every contact's script failed the same way; the courier's
`create_at_pos((pos + (left *= 2.0)), ...)` and the test vendor's `pos + (dir *= 2.0)` would have too, copied from the
`skoo` dump, which printed operators that way). The call screen stayed on "Connected", since the script hangs up
itself: a script that doesn't compile now gets `PDA.end_phone_call()` run on its own. SDTaxi's contact was missing in
the same run (its `LaunchSubOption` scan, see `SDTaxi\CLAUDE.md`).

## NPCs, cars and car doors (2026-10-05, from the action trees, the scripts and the installed-build IDA database)

Worth knowing for any mod that sends an NPC by car (SDTaxi too):

- **Which get-out plays**: `Vehicle.act` `Interactions\Action\GetOut\AsDriver` picks, in this order: NotOnWheels,
  ActionHijackStart, Blocked, DismountShoot, VehicleMoving (speed >= 40/60: aggressive bail-outs), boats, vans,
  trucks, minibus, DDBus, `GetOutCar`. `GetOutCar\Anims`: `Car_Casual` (`Vehicle\Queries\OccupantShouldUseCasualAnims`
  = the character property `UseCasualGetInGetOutAnims`, which scripts give Wei in Bride to Be and the valet
  `M_IMP_Valet` has in its archetype), `IntoCombat` (property `SkipCombatEntry`), `Default` (`Car_Drive_Out`). Vans
  and trucks have one anim each, no casual one. `RidgeSportSWAT01` (parent `CopSUV`) is a car.
- **Why doors stay open**: the get-out spawns a door controller on the vehicle
  (`ObjectSpawns\DoorControllers\<Car|Van|Truck|Minibus>\Driver\GetOut\Open\<variant>`). `Regular` watches the
  vehicle's driver for 1.6 s (car; van 1.86 s): if the driver target is gone by then, the door freezes open and is
  left `ATT_SIMULATED_NO_MOTOR` (free to swing, never shut). The character may cut `Car_Drive_Out` from 1.2 s
  (`Vehicle\Queries\GetOutFast`: a Walk/Jog one-shot, `ExitVehicleModifierFast`, a grapple lock, Focus, or speed
  >= 7.5), and the thug AI's Follow (`TargetOnFoot\ExitVehicle`, `EndExit` at 1.2 s once not uninterruptible)
  jogs off at once: the door stays open in his path. `Car_Casual` is uninterruptible for 2.25 s, `GetOutFast` only
  from 3.15 s, and its door controller has no interrupted branch: the door always shuts. The van/truck
  `AutoCloseDoor` banks are switched off (`False`). Scripts can only animate the minibus passenger door and truck
  cargo doors (`Vehicle.open/close_passenger_door`, `_set_open_truck_doors`); playing
  `DoorControllers\Car\Driver\GetIn\Close\Regular` on a car would shut its driver door but also starts the engine.
- **A calm scripted exit**: `Character._exit_vehicle()` sets `eAI_OBJECTIVE_GET_OUT_OF_VEHICLE` (ScriptObjectives,
  which end with objective none). With no follow objective nothing cuts the anim short; give the next objective
  after it (the ambulance response and the Car Valet do the same).
- **The AI driving to the player**: the thug AI's Follow (`FollowBehaviour.act` → `VehicleBehaviour.act`
  `TargetOnFoot`: `DriveNear` with a 20 m tolerance, 0.75 s, then the exit at speed <= 5) drives Water Street thug
  archetypes fine (WeaponContact, build-8). `CJ_SWAT01_Character` with `Thug_behaviour.act` never moved the truck
  (build-14; cause not found; the game's scripts only ever pair police archetypes with `Cop_behaviour.act`).
  Scripted driving works with any driver: SDTaxi's loop (`_path_to_xform(player.get_xform(), true)` re-aimed every
  5 s, `stop()` repeated, here in `kSwatService`) had the truck at speed ~40 within 2 s and within 12 m of the
  player 3-4 s later.
- **A cop doesn't keep a script's objective**: `Cop_behaviour.act` `MasterSpawn` always runs
  `PRIVATE_BANK\InfractionResponse` (player heat → `eAI_OBJECTIVE_PURSUIT_TARGET`) and `CopStimulus`; Investigate
  and Pursuit end with objective none, and a cop at the wheel with no objective patrols
  (`Actions\DrivingDummy\PatrolFallback`). Separately, in heat chases (`IsInFight`), `CopSystem::Reacquire` adopts
  units whose `CopUnitComponent` (+0x1E0) is usable (bit 1), alive (bit 3) and not managed (bit 0): usable comes
  from the property `UsableByCopSystem` (`default-component-CopUnit`: true; the event cop
  `F_CNY_HelpRookie1_Character` sets it false), read only in `CopUnitComponent::OnAttach`, so it can't be changed
  on a spawned ped from script.
- **Despawning**: `Actor.despawn(true)` waits for the suspension, and a ped with `PedSuspendOption_SuspendAllowed`
  is suspended as soon as he's off screen, even 3 m away: let him get away first.

## Test results (2026-10-05)

- build-8 on the second machine: WeaponContact (the machine pistol handed over) and SwatContact work. Reported by
  the user: the couriers' open car door blocks their way (build-13: casual get-out); the SWAT officer joined police
  patrols, had to be met at the driver's door and vanished when the camera turned (build-14: thug AI, meeting at
  the truck, leaving 40 m first).
- build-14 on this machine: the truck never left its spawn point (build-15: driven by script). Two SWAT calls with
  build-15: on the way at speed ~40, within 12 m in 3-4 s, the officer out ~3.5 s later 7-10 m from the player, the
  handover (once at 2 m, once at the truck), the player drove off; no standstill logged. The user: much better.
- Not tried yet: the WeaponContact and backups' casual get-out (does the door shut, do they reach the player), the
  backups at all, leaving 40 m before despawning.
