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

Not affiliated with Square Enix or United Front Games.
