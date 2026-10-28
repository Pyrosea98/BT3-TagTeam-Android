# Android v0.4.1 — release record

**Published APK:** `BT3-TagTeam-v0.4.1-Android.apk`, SHA-256 `2763d1f9b0ac6ae30ceab37a18b1c45a7b6aa54ae28283b778aa7b3145f00ac8`, app ID `com.pyrosea98.bt3tagteam`, version 0.4.1 (401).
Signed with the project's own release key (certificate SHA-256 `d168d1bc90865404a97ef6022cfcd03d0516cb7ae7afeb34af851a2bfdc524fc`), kept outside the repository. Native engine `lib/arm64-v8a/libbt3.so` SHA-256 `93db6b5ed01e523b325b87cb548a9177c34b55b97b6513baf0c82cd29973d247`.

## What it is

The Dragon Rage 0.9.0 Android port (renderer speed-ups for Mali/Adreno, online protocol 5, quick saving) with the PC v0.4.1 native Tag Team / Real Power Scale code (identical to the published PC source apart from Android-only changes below) and the PC's menu-text system.

Android-only changes:
- BT3 only (no game picker / BT4), Settings as the second tab.
- The import (`android/app/src/main/java/com/dfdx047/dragonrage/data/TagTeamImport.kt`): the PC v0.4.1 recipe (`BT3-TagTeam-v0.4.1-ImportRecipes.zip`, SHA-256 `1189068004347ab03a08ebd4454587df2923ad7452fc2a42096c216a4b2fd318`, content list `63802e4d…`) applied to the player's original / R4 normal / R4 2x image, normal or 2x maps. The second archive (DATA/PAK3US2.AFS, 65,000 entries) is copied whole and its unchanged entries read from inside it; every other file is written loose and SHA-256 checked. The mod's readers of that archive go through `Port_OpenSpan` (plat_fighters.c, snd_se.c, fight_panels_ui.cpp).
- Recommended first-install settings (the PC's accepted profile without device keys) and the completed-progression save, never over a player's own.
- Touch: target left/right buttons; overhead HUD option for the player and target only (on in the phone's recommended settings).
- Its own application ID and signing key, so it installs beside Dragon Rage; Dragon Rage's update check is off.

## Verified on a Redmi (Mali-G76, Android 13)

Fresh install of this APK: recipe download 38 s, image check 19 s, import 3 min 38 s total; the game starts. With the same import: main menu (including the Tenkaichi Online item), 1v1 Duel and Team Battles with extra fighters (2v1, 2v3, 7 fighters) played by the owner; music and voices; no missing files in the log. Fights 90-100% speed; 11-28 fps depending on fighters.

No disc image, game archives, BIOS, extracted game data or private key is published.
