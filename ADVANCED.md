# SDEncore — advanced users and developers

> [!IMPORTANT]
> **关于这个 mod**：它完全是用 Claude Code 里的 Claude Fable 和 Opus vibe coding 写出来的，几乎没有经过审查，请当作实验性质的 mod 使用，发现异常请反馈。作为一个长期缺少 mod 而自己下场做 mod 的普通玩家，我在 vibe 的过程中收获了许多快乐；如果你也有想实现的灵感，不妨也试着 vibe 一下。由于这些代码都是 vibe 出来的，所以我不会以我的 mod 盈利，也不接受捐助。如果你喜欢我的作品，请考虑向[致谢](#致谢)中提到的开源项目和作者捐赠，祝你游玩愉快！
>
> **About this mod**: it was fully vibe-coded with Claude Fable and Opus in Claude Code, with little review, so treat
> it as experimental and please report anything unusual. I'm just an ordinary player who went a long time without
> mods for this game and finally started making them myself. Vibe coding them has been a lot of fun; if you have an
> idea of your own, it might be for you too. Since all this code is vibe-coded, I won't make money from my mods and
> don't accept donations. If you like my work, please consider donating to the open-source projects and authors
> listed in the [Credits](#credits) instead. Have fun!

[中文](#中文) | [English](#english)

新手安装说明见 [README.md](README.md)。 · Step-by-step install for players: [README.md](README.md).

## 中文

### 被砍掉的是什么

游戏的脚本（`Global.big` 里编译好的 SkookumScript）和任务定义里留着几样从没开放的东西：

- **五个手机联系人**：`WeaponContact`、`GunBackup`、`MeleeBackup`、`BoatContact` 和 `E_SC`（SWAT Contact），定义在一个开发者的测试文件 `Definitions_Taylor.xml` 里，不在任何进度图中，所以永远不会解锁。脚本是完整的，对白全是 `[ Placeholder ]` 字幕。通讯录数据（`default-unlockables-contactList-list`）里有它们的名字（`$PDA_CONTACT_WEAPON` 等，中文汉化也翻译了），SWAT 联系人还有自己的头像 `Portrait_Smartphone_SWAT`（Sleeping Dogs Wiki 的 [Cut Content](https://sleepingdogs.fandom.com/wiki/Cut_Content) 页面早已记录：2011 年 11 月的原型里就有这个联系人，头像留在了游戏文件里）。
- **枪贩**：被砍掉的支线 “Piece of Work”（F_PW）里，Granny Annie 让 Wei 去工地找一个枪贩，买一把 .45 手枪给她。引擎里买枪的流程是完整的：七把枪有“枪械”购买图标，购买会记进 `CashSpentOnWeapons` / `WeaponsPurchased`
  统计；但物品表里所有枪的价格都是占位的 HK$10，名字是直接写的英文。正式版里没有任何人卖枪（属性集里
  `Vendors-WeaponVendor` 这个分类下是三个卖鱼的）。F_PW 本身没有接进进度图，它用到的地图对象也没找到，所以这个支线没有恢复。

### 这个 mod 做了什么

联系人照原脚本移植，补了原版的粗糙处：

| 联系人 | 原版 | 本 mod |
|---|---|---|
| 武器联络人 | 小弟开车过来，走到你身边给你一把冲锋手枪 | 同左；刷车点找不到时扩大到视野外 40-120 m |
| 武装支援 / 近战支援 | 小弟跟着你直到被打倒；行为设成 FOLLOW_TARGET，只跟随不出手 | 改成 BE_ALLY（会帮你打）；你离开 `Leash` 米超过 30 秒就收工 |
| 快艇联络人 | 在马路刷车点刷快艇（脚本自己写着“等有了随机的刷船点就能用”） | 用游戏后来加的 `_find_boat_spawn_xform`，在最近的水边放一艘空艇并标在地图上 |
| 特勤联络人 | 占位的混混开 SWAT 车来，交钥匙后还坐在驾驶座上 | SWAT 警员送车，交钥匙后下车走开，车解锁并标在地图上 |

台词换成正式文本（说话人用游戏自己的联系人名字）。原版的冷却时间（`repeatableinterval="5"`）只有 5 秒，所以没有冷却：联系人在服务进行中隐藏，结束后马上能再叫。

枪贩：`SDEncore.ini` 的每个 `Spot` 是一个摊位。你走近 60 m 时，摊位上出现一个水街的人，像 F_PW 的枪贩那样用脚本发起购买对话；扣了钱就把枪发给你（`equip_firearm`，满弹药），你离开 90 m 后他离开。物品表里枪的价格、名字和描述在内存里改成 ini 的价格和对应语言的文本。

### 原理

- 运行时编译执行 SkookumScript：游戏的 exe 里还带着 Skookum 编译器（`UFG::ScriptCache::GetScript` +
  `SkookumMgr::RunExternalCodeBlock`），在脚本自己的 tick（`SkookumScript::update_delta`）里调用。
- 通讯录：hook `UIHK_PDAPhoneContactsWidget::PopulateList` 加联系人，`LaunchSubOption` 处理拨打（拨号界面照
  `LaunchCallMission` 的做法）。不往游戏的进度里加东西，存档里不会留下这些联系人。
- 物品表：`UFG::ItemProfiles::mpInstance`（经 `ItemProfiles::GetCost` 定位）。
- 语言：`Language = auto` 时问游戏这几个联系人名字的翻译（`UI.localize_string`），是中文就按繁简用字判断。
- 函数都用在旧版 v1.0 和当前 Steam 版里都唯一的字节特征码定位，找不到时对应功能关闭，日志里写 `MISSING`。

调查过程和全部细节见 [CLAUDE.md](CLAUDE.md)（英文）。

### 日志

- `contacts: N of 5 cut contacts in the phone`、`phone: N contact(s) in the phone's contacts`：启动时。
- `text: the game says "...": Traditional Chinese`：语言判断。
- `phone: the player called ...`，然后 `[SDEncore] <联系人>: ...` 开头的行：每次拨打的经过（刷在哪、人到没到、交付）。
- `vendor:`、`items:` 开头的行：枪贩摊位、价格；`[SDEncore] vendor N: ...`：枪贩出现、每次购买对话的结果。
- `skookum:` 开头的行里有 `error`：脚本编译或运行出错（附带出错位置）。
- `crash:` 开头的行：崩溃时的位置和调用栈。游戏每次退出都会崩一次（原版问题），这一条可以忽略。

### 设置（`plugins\SDEncore.ini`）

| 项 | 默认 | 说明 |
|---|---|---|
| `[General] Language` | auto | 字幕和物品名的语言：auto、en、zh-Hant、zh-Hans。 |
| `[WeaponContact]` 等 `Enabled` | 1 | 每个联系人单独开关。 |
| `Vehicle` / `Character` / `Weapon` | 原版的 | 派来的车、人（属性集名）和武器（枪械类型或近战武器名）。 |
| `[Backup] Leash` | 250 | 离开支援多远（米）超过 30 秒他就回去。 |
| `[Backup] Minutes` | 0 | 支援最多跟几分钟，0 = 直到被打倒（原版）。 |
| `[GunVendor] Enabled` | 1 | 枪贩。 |
| `[GunVendor] SpotN` | 无 | 摊位：`X, Y, Z, 朝向（度，0 = +Y）, 枪`。 |
| `[GunVendor] Price*` | 1500/3000/4000/6000 | 手枪、冲锋枪、霰弹枪、步枪的价格（港币）。 |
| `[Debug] Logging` | 1 | 写 `SDEncore.log`；崩溃时另写 `SDEncore-crash-<n>.dmp`。 |
| `[Debug] ScriptPrints` | 1 | 把游戏脚本的 `Debug.println`（发行版里是空函数）也写进日志。 |
| `[Debug] Console` / `ConsoleKey` | 0 / F11 | 开发用：按键执行 `SDEncore-console.sk`。 |
| `[Debug] VendorHereKey` | 0 | 开发用：按键在你面前放一个测试枪贩，并把位置按 `Spot` 格式写进日志。 |

改完重启游戏生效。开发时，`plugins` 里放一个 `SDEncore-<联系人>.sk`（如 `SDEncore-WeaponContact.sk`）可以替换该联系人的脚本，每次拨打时重新读取。

### 需求与兼容性

- 《热血无赖：终极版》的两个发行版本（当前 Steam 版和旧版 v1.0，特征码在两者上都唯一匹配），Windows 10/11 x64。不依赖 Windows 专有服务，应当能在 Wine/Proton/CrossOver 下运行（未测试）。
- 任意 ASI 加载器，例如 [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)（`SDEncore.zip`
  里自带一份，作为 `dinput8.dll`）。
- 不修改任何游戏文件。中文字幕需要中文汉化提供的字体。

### 下载

[Releases](https://github.com/aUsernameWoW/sleeping-dogs-encore/releases) 里每个版本都有 `SDEncore.zip`（加载器 +
mod）、`SDEncore.asi`（只有 mod）、`SDEncore.pdb`（调试符号）和 `THIRD-PARTY-NOTICES.md`。`main` 上每次提交都会自动编译、测试并发布为预发布版 `build-<N>`（没有在游戏里测过）；在游戏里验证过的构建会转为正式版，README 里的下载链接指向最新的正式版。

### 编译与测试

Visual Studio 2022（v143），Windows SDK 10.0.26100。项目需要放在工作区的 `mods\SDEncore`，工作区里还要有
`reference\minhook`（[MinHook](https://github.com/TsudaKageyu/minhook) v1.3.4 源码，随项目一起编译）。在工作区根目录运行 `.\tools\build.ps1 -Mod SDEncore -Test`：`load_test` 在游戏之外加载 .asi，不能崩溃，写出默认 ini，并报告找不到游戏函数；`script_test` 生成每个联系人（每种语言）和枪贩的脚本，检查游戏的 SkookumScript 编译器会拒绝的写法（用圆括号分组、括号不配对、没填的占位符）。GitHub Actions 用同样的布局编译（`-warnAsError`）、测试、打包并发布预发布版，依赖版本固定在
`.github/reference.env` 和 `.github/asi-loader.env`。

### 致谢

这个 mod 用到或参考了下面这些人和项目的成果，在此致谢。

**研究资料**

- Sleeping Dogs Wiki 的 [Cut Content](https://sleepingdogs.fandom.com/wiki/Cut_Content) 页面：早已记录了 2011 年 11 月原型里的「SWAT 联系人」，以及它留在游戏文件里的头像。
- [SDmodding](https://github.com/SDmodding)，几乎全部出自 [sneakyevil](https://github.com/sneakyevil) 一人之手。这个 mod 用到了：
  - SDmodding 随 [SDK](https://github.com/SDmodding/SDK) 发布的 [Visual Studio 2022 项目模板](https://github.com/SDmodding/SDK/releases/tag/vs2022)：这个 mod 的 Visual Studio 工程源自这个模板，编译设置和以 `dllmain.cc` 为起点的源文件结构都来自它；
  - SDmodding 分享的游戏 v1.0 版 exe 和调试符号（PDB，Steam 首发版自带）：游戏的脚本系统、手机联系人和商店的结构都是从这里查到的；
  - [SDK](https://github.com/SDmodding/SDK)：游戏里的类名和数据结构；
  - [Files](https://github.com/SDmodding/Files) 里导出的属性集、本地化文本、动作树和符号表（QSymbolsDictionary）；
  - [BigFileSystem](https://github.com/SDmodding/BigFileSystem)、[TheoryEngine](https://github.com/SDmodding/TheoryEngine)，以及 sneakyevil 的 [SD-BigFileExplorer](https://github.com/sneakyevil/SD-BigFileExplorer) 和 [Ekey](https://github.com/Ekey) 的 SDDEUnpacker 里的文件名列表：读取游戏资源包（`.big`）的工具是照着它们写的，游戏脚本和任务数据都是用它从资源包里取出的。
- Keylol 上的 [PS4 官方中文移植 + 粤语修正补丁](https://keylol.com/t987308-1-1)（SneakyEvil、MuYou 等）：中文台词里的人名和叫法沿用其中 PS4 版官方中文的译法，中文字幕用它的字体。

**游戏原有的内容**

- 恢复的内容照游戏里被砍掉的原版脚本移植，部分台词改写自原版的占位台词；它们由 United Front Games 编写，版权归 Square Enix 所有。
- SkookumScript（Agog Labs）：游戏的脚本语言，mod 用游戏自带的编译器运行这些脚本。

**mod 里包含的代码**（许可证全文见 [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)）

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)（ThirteenAG）：压缩包里的 `dinput8.dll`，让游戏加载 mod。它本身还包含 MinHook、[miniz](https://github.com/richgel999/miniz)（Rich Geldreich 等）和 [praydog](https://github.com/praydog) 的 FunctionHookMinHook。
- [MinHook](https://github.com/TsudaKageyu/minhook)（Tsuda Kageyu，内含 Vyacheslav Patkov 的 Hacker Disassembler Engine）：mod 靠它接入游戏。

**工具**

- [IDA Pro](https://hex-rays.com/ida-pro)（Hex-Rays）和 [ida-pro-mcp](https://github.com/mrexodia/ida-pro-mcp)（mrexodia）：分析游戏程序。
- [Claude Code](https://claude.com/claude-code)（Anthropic）：这个 mod 完全是用 Claude Fable 和 Opus vibe coding 写出来的，代码、文档和逆向分析都出自 Claude，几乎没有经过人工审查。

**游戏与商标**

《热血无赖：终极版》（Sleeping Dogs: Definitive Edition）由 United Front Games 开发、Square Enix 发行，游戏及其内容的版权归 Square Enix 所有。

与 Square Enix、United Front Games 均无关联。

## English

### What was cut

The game's scripts (compiled SkookumScript in `Global.big`) and mission definitions still hold things that were
never made available:

- **Five phone contacts**: `WeaponContact`, `GunBackup`, `MeleeBackup`, `BoatContact` and `E_SC` (SWAT Contact),
  defined in a developer's test file, `Definitions_Taylor.xml`, and in no progression graph, so they never unlock.
  The scripts are complete; all their lines are `[ Placeholder ]` subtitles. The contact list data
  (`default-unlockables-contactList-list`) has their names (`$PDA_CONTACT_WEAPON` etc., translated by the Chinese
  pack too), and the SWAT contact has its own portrait, `Portrait_Smartphone_SWAT` (the
  Sleeping Dogs Wiki's [Cut Content](https://sleepingdogs.fandom.com/wiki/Cut_Content) page documented this contact in the November 2011 prototype, and its
  portrait left in the game files, long before this mod).
- **A gun vendor**: in the cut favour "Piece of Work" (F_PW), Granny Annie sends Wei to a gun vendor at a
  construction site to buy her a .45. The engine's purchase path handles guns: seven of them have the "weapon"
  purchase icon, and purchases go into the `CashSpentOnWeapons` / `WeaponsPurchased` stats; but every gun in the
  item profiles costs a placeholder HK$10 and has English written in as its name. Nobody in the shipped game sells
  guns (the property sets' `Vendors-WeaponVendor` category holds three fish sellers). F_PW itself isn't in the
  progression graph and its world objects weren't found, so the favour isn't restored.

### What the mod does

The contacts are ported from the original scripts, with their rough edges fixed:

| Contact | Original | This mod |
|---|---|---|
| Weapons contact | a man drives over, walks up and hands you a machine pistol | the same; if no spawn spot is found, also out of sight at 40-120 m |
| Armed / hand to hand backup | a man follows you until he's down; set to FOLLOW_TARGET, which only follows | BE_ALLY (he fights); he leaves when you've been `Leash` m away for 30 s |
| Boat contact | spawns a boat at a road spawn spot (its script says "Once I can get a random boat spawn position, this will work.") | uses `_find_boat_spawn_xform`, added to the game later: an empty boat at the nearest water, marked on the map |
| SWAT contact | the placeholder thug drives a SWAT truck over and stays in the driver's seat after handing over the keys | a SWAT officer brings it, gets out and walks off; the truck is unlocked and marked on the map |

The lines are real ones (the contact speaks under the game's own name for him). The originals' cooldown
(`repeatableinterval="5"`) is 5 seconds, so there is none: a contact is hidden while it's busy and back right after.

The gun vendor: each `Spot` in `SDEncore.ini` is a stall. When you come within 60 m, a Water Street man appears there
and, like F_PW's vendor, starts a scripted purchase conversation; when money goes, you get the gun (`equip_firearm`,
fully loaded); he leaves when you're 90 m away. The guns' prices, names and descriptions in the item profiles are
set in memory to the ini's prices and text in your language.

### How it works

- SkookumScript compiled and run at run time: the exe still contains the Skookum compiler
  (`UFG::ScriptCache::GetScript` + `SkookumMgr::RunExternalCodeBlock`), called from the scripts' own tick
  (`SkookumScript::update_delta`).
- Contacts: a hook on `UIHK_PDAPhoneContactsWidget::PopulateList` adds them, `LaunchSubOption` handles the call (the
  call screen as `LaunchCallMission` shows it). Nothing is added to the game's progression; saves don't keep them.
- Item profiles: `UFG::ItemProfiles::mpInstance` (found through `ItemProfiles::GetCost`).
- Language: with `Language = auto` the mod asks the game how it translates these contacts' names
  (`UI.localize_string`); Chinese text is told Traditional or Simplified by the characters used.
- Every function is found by a byte signature unique in both the legacy v1.0 and the current Steam build; a missing
  one turns its feature off and logs `MISSING`.

The investigation and all details: [CLAUDE.md](CLAUDE.md).

### Log

- `contacts: N of 5 cut contacts in the phone`, `phone: N contact(s) in the phone's contacts`: at start.
- `text: the game says "...": Traditional Chinese`: the language.
- `phone: the player called ...`, then lines starting with `[SDEncore] <contact>: ...`: each call (where the vehicle
  spawned, whether the man arrived, the handover).
- `vendor:`, `items:`: the vendor's spots and prices; `[SDEncore] vendor N: ...`: the vendor appearing, each
  purchase conversation's result.
- `skookum:` lines with `error`: a script failed to compile or run (with the position).
- `crash:`: where a crash happened, with the stack. The game crashes on every exit (an original bug): ignore that one.

### Settings (`plugins\SDEncore.ini`)

| Setting | Default | |
|---|---|---|
| `[General] Language` | auto | Subtitles and item names: auto, en, zh-Hant, zh-Hans. |
| `[WeaponContact]` etc. `Enabled` | 1 | Each contact on or off. |
| `Vehicle` / `Character` / `Weapon` | the originals' | The vehicle, the man (property sets) and the weapon (firearm type or melee weapon). |
| `[Backup] Leash` | 250 | Backup goes home after you've been this far (m) away for 30 s. |
| `[Backup] Minutes` | 0 | Minutes backup stays at most; 0 = until he's down (the original). |
| `[GunVendor] Enabled` | 1 | The gun vendor. |
| `[GunVendor] SpotN` | none | A stall: `X, Y, Z, heading (degrees, 0 = +Y), gun`. |
| `[GunVendor] Price*` | 1500/3000/4000/6000 | Pistol, SMG, shotgun, rifle prices (HK$). |
| `[Debug] Logging` | 1 | Write `SDEncore.log`; on a crash also `SDEncore-crash-<n>.dmp`. |
| `[Debug] ScriptPrints` | 1 | Log the game scripts' `Debug.println` too (an empty function in this build). |
| `[Debug] Console` / `ConsoleKey` | 0 / F11 | Development: a key runs `SDEncore-console.sk`. |
| `[Debug] VendorHereKey` | 0 | Development: a key puts a test vendor in front of you and logs the spot in `Spot` format. |

Restart the game after changing it. For development, an `SDEncore-<contact>.sk` in `plugins` (e.g.
`SDEncore-WeaponContact.sk`) replaces that contact's script and is read at every call.

### Requirements and compatibility

- Both releases of Sleeping Dogs: Definitive Edition (the current Steam build and the legacy v1.0; the signatures
  match uniquely in both), Windows 10/11 x64. No Windows-only services, so it should run under
  Wine/Proton/CrossOver (untested).
- Any ASI loader, e.g. [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) (`SDEncore.zip`
  ships one as `dinput8.dll`).
- Changes no game files. Chinese subtitles need the fonts of a Chinese translation pack.

### Downloads

Each release in [Releases](https://github.com/aUsernameWoW/sleeping-dogs-encore/releases) has `SDEncore.zip` (loader +
mod), `SDEncore.asi` (the mod alone), `SDEncore.pdb` (debug symbols) and `THIRD-PARTY-NOTICES.md`. Every commit on
`main` is built, tested and published as a prerelease `build-<N>` (not tested in game); builds verified in game
become full releases, which the README's download link points to.

### Building and testing

Visual Studio 2022 (v143), Windows SDK 10.0.26100. The project has to sit in the workspace's `mods\SDEncore`, with
`reference\minhook` ([MinHook](https://github.com/TsudaKageyu/minhook) v1.3.4 sources, compiled in) next to it. From
the workspace root, `.\tools\build.ps1 -Mod SDEncore -Test` builds and runs the tests: `load_test` loads the .asi
outside the game (it must not crash, must write its default ini and must report the game functions missing), and
`script_test` builds every contact's script (in each language) and the vendors' and checks them for what the game's
SkookumScript compiler rejects (parentheses used for grouping, unbalanced brackets, unfilled placeholders). GitHub Actions
builds the same layout (`-warnAsError`), tests, packages and publishes prereleases; the dependencies are pinned in
`.github/reference.env` and `.github/asi-loader.env`.

### Credits

This mod uses or builds on the work of these people and projects. Thank you.

**Research**

- The Sleeping Dogs Wiki's [Cut Content](https://sleepingdogs.fandom.com/wiki/Cut_Content) page: it documented the "SWAT contact" of the November 2011 prototype, and
  its portrait left in the game files, long before this mod.
- [SDmodding](https://github.com/SDmodding), almost all of it the work of one person, [sneakyevil](https://github.com/sneakyevil). This mod used:
  - the [Visual Studio 2022 project template](https://github.com/SDmodding/SDK/releases/tag/vs2022) released with SDmodding's [SDK](https://github.com/SDmodding/SDK): the mod's Visual Studio project derives from it, including its build settings and the source layout that starts at `dllmain.cc`;
  - the game's v1.0 exe and its debug symbols (PDB, shipped with the original Steam release), shared by
    SDmodding: the game's script system, phone contacts and shops were worked out from them;
  - the [SDK](https://github.com/SDmodding/SDK): the game's class names and data structures;
  - the property sets, localization text, action trees and symbol names (QSymbolsDictionary) exported in [Files](https://github.com/SDmodding/Files);
  - [BigFileSystem](https://github.com/SDmodding/BigFileSystem), [TheoryEngine](https://github.com/SDmodding/TheoryEngine), and the file name lists in sneakyevil's [SD-BigFileExplorer](https://github.com/sneakyevil/SD-BigFileExplorer) and in [Ekey](https://github.com/Ekey)'s
    SDDEUnpacker: the tool that reads the game's `.big` archives follows them; the game's scripts and mission data were taken out of the archives with it.
- The [PS4 official Chinese port + Cantonese fix](https://keylol.com/t987308-1-1) on Keylol (SneakyEvil, MuYou and others): the
  Chinese lines use the names of its official PS4 Chinese text, and Chinese subtitles use its fonts.

**The game's own content**

- What the mod restores is ported from the game's own cut scripts, and some lines are reworked from their
  placeholder dialogue; they were written by United Front Games and are © Square Enix.
- SkookumScript (Agog Labs): the game's scripting language; the mod runs its scripts through the game's own
  compiler.

**Code in the mod** (full license texts in [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md))

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader) (ThirteenAG): the `dinput8.dll` in the zip, which makes the game load mods.
  It contains MinHook, [miniz](https://github.com/richgel999/miniz) (Rich Geldreich and others) and [praydog](https://github.com/praydog)'s FunctionHookMinHook.
- [MinHook](https://github.com/TsudaKageyu/minhook) (Tsuda Kageyu, with Vyacheslav Patkov's Hacker Disassembler Engine): how the mod hooks into the game.

**Tools**

- [IDA Pro](https://hex-rays.com/ida-pro) (Hex-Rays) and [ida-pro-mcp](https://github.com/mrexodia/ida-pro-mcp) (mrexodia): analyzing the game's code.
- [Claude Code](https://claude.com/claude-code) (Anthropic): this mod was fully vibe-coded with Claude Fable and Opus; its code,
  documentation and reverse engineering are all Claude's, with little human review.

**The game and trademarks**

Sleeping Dogs: Definitive Edition was developed by United Front Games and published by Square Enix; the game
and its content are © Square Enix.

Not affiliated with Square Enix or United Front Games.
