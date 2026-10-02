# Sleeping Dogs: Definitive Edition — cut content restored (SDEncore)

[中文](#中文) | [English](#english)

## 中文

把《热血无赖：终极版》里**做了一半就被砍掉的内容**加回游戏：

- 手机通讯录里多出五个联系人（游戏脚本里本来就有，只是从来没开放）：
  - **武器联络人**：派人开车给你送一把枪；
  - **武装支援**、**近战支援**：一个水街兄弟带着枪或菜刀赶来，跟着你打，直到被打倒；
  - **快艇联络人**：在离你最近的水边放一艘快艇；
  - **特勤联络人**：派人给你开来一辆特警车。
- **枪贩**：像在路边买小吃一样买枪。

原版只留下了占位的英文字幕，这个 mod 换成了正式的台词；装了中文汉化的话，字幕和物品名也是中文。

状态：**开发中**，还在游戏里测试。枪贩的摆摊位置还没定好，暂时不会出现。

> 适用于**任何版本**的《热血无赖：终极版》，Windows 10/11 64 位。想了解这些内容原来是什么样、自己编译或调
> 参数，请看 [ADVANCED.md](ADVANCED.md)。

### 安装（大约三分钟）

**第 1 步：下载**

点这里下载 **[SDEncore.zip](https://github.com/aUsernameWoW/sleeping-dogs-encore/releases/latest/download/SDEncore.zip)**。

压缩包里只有这些：

```text
dinput8.dll                  ← Ultimate ASI Loader：让游戏加载 mod 的“加载器”
plugins\
    SDEncore.asi              ← mod 本体
    SDEncore-THIRD-PARTY-NOTICES.md
```

**第 2 步：打开游戏文件夹**

1. 打开 Steam，进入「库」。
2. 在左侧列表里右键点「Sleeping Dogs: Definitive Edition」→「管理」→「浏览本地文件」。
3. 弹出来的就是游戏文件夹，里面有 `sdhdship.exe`（如果电脑不显示扩展名，就是一个叫 `sdhdship` 的程序）。

**第 3 步：把文件放进去**

1. 双击打开下载的 `SDEncore.zip`。
2. 选中里面的 `dinput8.dll` 和 `plugins` 文件夹，一起拖进游戏文件夹。
3. 如果 Windows 弹出「替换或跳过文件」，说明游戏文件夹里已经有 `dinput8.dll` 了（你以前装过别的 mod，
   加载器已经在了），选「跳过该文件」。已有的 `plugins` 文件夹会自动合并，不用管。

放好后，游戏文件夹里应该是这样（只列出相关的部分）：

```text
SleepingDogsDefinitiveEdition\
    sdhdship.exe
    dinput8.dll
    plugins\
        SDEncore.asi
```

注意 `dinput8.dll` 要和 `sdhdship.exe` 在同一层，不要多套一层文件夹。

**第 4 步：启动游戏**

照常从 Steam 启动游戏。`plugins` 里多出 `SDEncore.ini` 和 `SDEncore.log` 两个文件，就说明 mod 已经加载。

### 怎么用

在游戏里打开手机 →「通讯录」，往下翻就能看到新的联系人，选中拨打即可。叫来的人在路上时，这个联系人会暂时从
通讯录里消失，事情办完后再出现。

### 常见问题

**不想要某个联系人，或者想改送的枪、枪贩的价格**

用记事本打开 `plugins\SDEncore.ini`，每一项都有中文说明。例如把 `[BoatContact]` 下的 `Enabled` 改成 0 就去掉
快艇联络人。保存后重启游戏。

**`plugins` 里没有 `SDEncore.log`**

说明 mod 没被加载：检查 `dinput8.dll` 是否和 `sdhdship.exe` 在同一层，杀毒软件有没有删掉它（ASI 加载器偶尔
会被误报，可以从隔离区还原并把游戏文件夹加入排除项）。如果第 3 步跳过了原有的 `dinput8.dll`，那个文件可能
不是 ASI 加载器，备份后换成压缩包里的。

**更新**

下载新的 `SDEncore.zip`，只把里面的 `plugins` 文件夹拖进游戏文件夹，Windows 询问时选「替换目标中的文件」。
`SDEncore.ini` 不在压缩包里，你的设置会保留。

**卸载**

删掉 `plugins` 里的 `SDEncore.asi`、`SDEncore.ini` 和 `SDEncore.log`。如果 `plugins` 里已经没有其他 `.asi` 文件了，
`dinput8.dll` 也可以删掉。

**遇到问题怎么反馈**

在 [GitHub Issues](https://github.com/aUsernameWoW/sleeping-dogs-encore/issues) 里说明情况，并附上
`plugins\SDEncore.log`。

### 致谢

这个 mod 用到或参考了下面这些人和项目的成果，在此致谢。

**研究资料**

- Sleeping Dogs Wiki 的 [Cut Content](https://sleepingdogs.fandom.com/wiki/Cut_Content) 页面：早已记录了 2011 年 11 月原型里的「SWAT 联系人」，以及它留在游戏文件里的头像。
- [SDmodding](https://github.com/SDmodding)，几乎全部出自 [sneakyevil](https://github.com/sneakyevil) 一人之手。这个 mod 用到了：
  - SDmodding 分享的游戏 v1.0 版 exe 和调试符号（PDB，Steam 首发版自带）：游戏的脚本系统、手机联系人和商店的结构都是从这里查到的；
  - [SDK](https://github.com/SDmodding/SDK)：游戏里的类名和数据结构；
  - [Files](https://github.com/SDmodding/Files) 里导出的属性集、本地化文本、动作树和符号表（QSymbolsDictionary）；
  - [BigFileSystem](https://github.com/SDmodding/BigFileSystem)、[TheoryEngine](https://github.com/SDmodding/TheoryEngine)，以及 sneakyevil 的 [SD-BigFileExplorer](https://github.com/sneakyevil/SD-BigFileExplorer) 和 [Ekey](https://github.com/Ekey) 的 SDDEUnpacker 里的文件名列表：
    我们照着它们写了读取游戏资源包（`.big`）的工具，游戏脚本和任务数据都是用它从资源包里取出的。
- Keylol 上的 [PS4 官方中文移植 + 粤语修正补丁](https://keylol.com/t987308-1-1)（SneakyEvil、MuYou 等）：中文台词里的人名和叫法沿用
  其中 PS4 版官方中文的译法，中文字幕用它的字体。

**游戏原有的内容**

- 恢复的内容照游戏里被砍掉的原版脚本移植，部分台词改写自原版的占位台词；它们由 United Front Games 编写，
  版权归 Square Enix 所有。
- SkookumScript（Agog Labs）：游戏的脚本语言，mod 用游戏自带的编译器运行这些脚本。

**mod 里包含的代码**（许可证全文见 [THIRD-PARTY-NOTICES.md](THIRD-PARTY-NOTICES.md)）

- [Ultimate ASI Loader](https://github.com/ThirteenAG/Ultimate-ASI-Loader)（ThirteenAG）：压缩包里的 `dinput8.dll`，让游戏加载 mod。它本身还包含 MinHook、
  [miniz](https://github.com/richgel999/miniz)（Rich Geldreich 等）和 [praydog](https://github.com/praydog) 的 FunctionHookMinHook。
- [MinHook](https://github.com/TsudaKageyu/minhook)（Tsuda Kageyu，内含 Vyacheslav Patkov 的 Hacker Disassembler Engine）：mod 靠它接入游戏。

**工具**

- [IDA Pro](https://hex-rays.com/ida-pro)（Hex-Rays）和 [ida-pro-mcp](https://github.com/mrexodia/ida-pro-mcp)（mrexodia）：分析游戏程序。
- [Claude Code](https://claude.com/claude-code)（Anthropic）：这个 mod 的代码、文档和逆向分析几乎全部由 Claude 完成；作者负责提出需求、把握方向和在游戏里测试，代码审查得很少。

**游戏与商标**

《热血无赖：终极版》（Sleeping Dogs: Definitive Edition）由 United Front Games 开发、Square Enix 发行，
游戏及其内容的版权归 Square Enix 所有。

与 Square Enix、United Front Games 均无关联。

## English

Puts **content that was half-made and then cut** back into Sleeping Dogs: Definitive Edition:

- five new contacts in the phone (they're in the game's scripts but were never available):
  - **Weapons contact**: has a gun driven over to you;
  - **Armed backup**, **Hand to hand backup**: a Water Street man with a gun or a cleaver comes and fights at
    your side until he's down;
  - **Boat contact**: leaves a boat at the water nearest to you;
  - **SWAT contact**: has a SWAT truck driven over for you.
- **A gun vendor**: buy guns the way you buy street food.

The originals only had placeholder subtitles; the mod gives them real lines, in Chinese too when a Chinese
translation pack is installed.

Status: **in development**, being tested in game. The gun vendor's spots aren't chosen yet, so he doesn't appear
for now.

> Works with **any version** of Sleeping Dogs: Definitive Edition, Windows 10/11 64-bit. What the cut content
> originally was, building the mod and all settings: [ADVANCED.md](ADVANCED.md).

### Install (about three minutes)

**Step 1: download**

Download **[SDEncore.zip](https://github.com/aUsernameWoW/sleeping-dogs-encore/releases/latest/download/SDEncore.zip)**.

It only contains:

```text
dinput8.dll                  ← Ultimate ASI Loader: what makes the game load mods
plugins\
    SDEncore.asi              ← the mod
    SDEncore-THIRD-PARTY-NOTICES.md
```

**Step 2: open the game folder**

1. Open Steam and go to your Library.
2. Right-click "Sleeping Dogs: Definitive Edition" → Manage → Browse local files.
3. That's the game folder; it contains `sdhdship.exe` (or `sdhdship`, if file extensions are hidden).

**Step 3: put the files in**

1. Open the downloaded `SDEncore.zip`.
2. Select `dinput8.dll` and the `plugins` folder and drag both into the game folder.
3. If Windows asks whether to replace or skip a file, the game folder already has a `dinput8.dll` (you've
   installed a mod before and the loader is there): choose "Skip this file". An existing `plugins` folder is
   merged automatically.

Afterwards the game folder should look like this (only the relevant part):

```text
SleepingDogsDefinitiveEdition\
    sdhdship.exe
    dinput8.dll
    plugins\
        SDEncore.asi
```

`dinput8.dll` has to be next to `sdhdship.exe`, not in a subfolder.

**Step 4: start the game**

Start the game from Steam as usual. When `SDEncore.ini` and `SDEncore.log` appear in `plugins`, the mod is loaded.

### How to use

In the game, open the phone → Contacts and scroll down to the new contacts; select one to call. While the man
you called is on his way, that contact is hidden; it comes back once he's done.

### FAQ

**Don't want one of the contacts, or want a different gun or the vendor's prices changed**

Open `plugins\SDEncore.ini` in Notepad; every setting is explained in the file. For example `Enabled = 0` under
`[BoatContact]` removes the boat contact. Save and restart the game.

**There's no `SDEncore.log` in `plugins`**

The mod wasn't loaded: check that `dinput8.dll` is next to `sdhdship.exe` and that your antivirus didn't remove
it (ASI loaders are sometimes flagged; restore it from quarantine and exclude the game folder). If you skipped
an existing `dinput8.dll` in step 3, that file may not be an ASI loader: back it up and use the one from the zip.

**Updating**

Download the new `SDEncore.zip` and drag only its `plugins` folder into the game folder; choose "Replace the files
in the destination". `SDEncore.ini` isn't in the zip, so your settings stay.

**Uninstalling**

Delete `SDEncore.asi`, `SDEncore.ini` and `SDEncore.log` from `plugins`. If there are no other `.asi` files left in
`plugins`, you can delete `dinput8.dll` too.

**Reporting a problem**

Describe it in [GitHub Issues](https://github.com/aUsernameWoW/sleeping-dogs-encore/issues) and attach
`plugins\SDEncore.log`.

### Credits

This mod uses or builds on the work of these people and projects. Thank you.

**Research**

- The Sleeping Dogs Wiki's [Cut Content](https://sleepingdogs.fandom.com/wiki/Cut_Content) page: it documented the "SWAT contact" of the November 2011 prototype, and
  its portrait left in the game files, long before this mod.
- [SDmodding](https://github.com/SDmodding), almost all of it the work of one person, [sneakyevil](https://github.com/sneakyevil). This mod used:
  - the game's v1.0 exe and its debug symbols (PDB, shipped with the original Steam release), shared by
    SDmodding: the game's script system, phone contacts and shops were worked out from them;
  - the [SDK](https://github.com/SDmodding/SDK): the game's class names and data structures;
  - the property sets, localization text, action trees and symbol names (QSymbolsDictionary) exported in [Files](https://github.com/SDmodding/Files);
  - [BigFileSystem](https://github.com/SDmodding/BigFileSystem), [TheoryEngine](https://github.com/SDmodding/TheoryEngine), and the file name lists in sneakyevil's [SD-BigFileExplorer](https://github.com/sneakyevil/SD-BigFileExplorer) and in [Ekey](https://github.com/Ekey)'s
    SDDEUnpacker: our tool for reading the game's `.big` archives follows them; the game's scripts and mission data were taken out of the archives with it.
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
- [Claude Code](https://claude.com/claude-code) (Anthropic): almost all of this mod's code, documentation and reverse
  engineering was done by Claude; the author set the goals, steered and tested in game, and reviewed little of the
  code.

**The game and trademarks**

Sleeping Dogs: Definitive Edition was developed by United Front Games and published by Square Enix; the game
and its content are © Square Enix.

Not affiliated with Square Enix or United Front Games.
