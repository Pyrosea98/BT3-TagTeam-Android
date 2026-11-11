# Android release record

## v0.5.alpha.2.1

Version 0.5.alpha.2.1 (515), the Android build of the PC 0.5.alpha.2 Nightly (revision battle-presets-roster-20261111:
power-scale presets, roster and Great Ape fixes, Spanish power units, credits) with the Android changes of 0.5.alpha.2.
The game data update takes the PC v0.5.alpha.2 Windows-Update.zip (250,647,515 bytes, SHA-256
99788460380709e33aff7d6c251a444653d8e75d6ef5ee421934ae0345077408) over a v0.4.1 import or 0.5.alpha / 0.5.alpha.1 data.

- The data update runs by itself: when the app opens on older game data (once per run of the app; a failure shows why
  and is not retried in a loop), and before Play / the Play shortcut (a failure offers "Play anyway"). Home's button
  stays. The earlier update files are deleted once the data is current.

## v0.5.alpha.2

Version 0.5.alpha.2 (514), Android-only fixes over 0.5.alpha.1 (same game data: the PC v0.5.alpha.1 update).

- Language: Settings offers System / English / Español on every Android version; at each start the choice (else the
  system language) writes `mod_language` (the mod's menus and features, as on PC) and `dr_lang` (the in-game menu).
- A fight ending during a technique's camera cut with an extra fighter: `DemoCam_Stop` after `BtlCam_Term` freed the
  camera (btl_demo_cam.c).
- A model without one of its attack's nodes: `BtlObjHit_BuildVolumes` skips it (bobj_a.c).
- `bt3_dlc.log` / `bt3_fusion.log` only with `BT3_DIAG_LOGS` on Android: they reopened the file per line on shared
  storage (60-90 s at 2-6 fps at the select, Lenovo TB321FU).

## v0.5.alpha.1

Version 0.5.alpha.1 (511), the Android build of the PC 0.5.alpha.1 Nightly (Runtime tag v0.5.alpha.1, 0083b12d) merged
with the Android fixes since 0.5.alpha. The app's game data update now takes the PC v0.5.alpha.1 Windows-Update.zip
(243,185,421 bytes, SHA-256 55a6dc069b80415b3303793d296580b330c012f96c3e3345000652b0848f121e) over a v0.4.1 import
or 0.5.alpha data.

- Team Battle crashes after CPU transformations: effect sets whose groups have no data (eft_i/eft_j/eft_k/eft_o/eft_p),
  extra fighters' shot slots after their effect task is rebuilt (eft_h).
- Leaving the modern select while a preview loads: the buffer was freed twice (view_a_d.c, the PC's fix).
- PowerVR (Moto g56 5G, BXM-8-256): its Vulkan driver refuses pipelines with alpha blending and a depth target, so the
  launcher picks OpenGL ES by default there, with the persistent uniform ring. Confirmed by the owner's tester.
- Log: the GPU's Vulkan features, and OpenGL ES per-frame timings.

## v0.5.alpha (pre-release)

**Published APK:** `BT3-TagTeam-v0.5.alpha-Android.apk`, SHA-256 `eb034d16e294ba8cf7555c891303d667805b91bab747de9e26143caff800d8cd`, version 0.5.alpha (507), signed with the project's release key (certificate SHA-256 `d168d1bc90865404a97ef6022cfcd03d0516cb7ae7afeb34af851a2bfdc524fc`). Native engine `lib/arm64-v8a/libbt3.so` SHA-256 `a3cd424c97779148480bed98fc0215fbab10c2d5c8970d11c0e75a4c39d86668`.

The PC v0.5.alpha source changes over v0.4.1 on top of v0.4.4, plus:
- Story health on Android: `port/src/softfloat_ps2.c` defines `__floatundisf`, `__floatdisf`, `__fixsfdi` and `__fixunssfdi` with the soft-float ABI. The link had taken `__floatundisf` from libclang_rt (hard-float, result in s0) while the game's code reads w0, so `(int)(long long max * ratio + .5f)` in `btl_char_mgr.c` set Story fighters' health to garbage (one-Kamehameha wins in chapters 2-3, Gohan at 0 HP at the start of chapter 4). No float routine of libclang_rt is linked any more (`libbt3.so.map`).
- `port/tools/android.py`: non-ASCII text in the preprocessed files written as octal escapes (ptr32 read them as latin-1, libclang counts UTF-8 offsets).
- Stable / Nightly update channels (`AppUpdates.kt`, Settings, first-start prompt).
- `Port_ModClashButtons`: on Android the clash buttons follow the `clash_square` toggle.
- The Scenario, Story, Achievement and modded select pages are drawn inside the 4:3 picture (`UiPictureScope`).

Verified on a Redmi (Mali-G76, Android 13) by the owner: Dragon History chapters 1-4 (test2/test3 builds of the same engine and code).

## v0.4.4

The v0.4.3 fixes published under the next number (version 0.4.4, 408): the 0.4.3 name had been used by test builds for another phone. Same engine and app code as v0.4.3.

## v0.4.3 (hotfix)

Version 0.4.3 (407): the version code is 4 above the old scheme because test builds of these fixes were 403-406.

- Team Battle crash after a CPU fighter transforms (seen with Goku (Early) + Master Roshi + Kid Gohan against Wheelo, ~10 s in, when Master Roshi goes to full power; also crashes in effects and models after fusions/defusions): `Quat_Unpack` (src/sys/math3d.c) used the packed key's 4-bit index (0-15) to write into a 4-float array, so a key that is not a packed rotation overwrote the function's saved frame pointer / return address. An index above 3 is now read as the identity and logged (`bt3: anim: rotation key ... is not a packed rotation`). Reproduced and verified on a Redmi (Mali) and reported on a Samsung Galaxy M52 (Adreno, with every driver), so not GPU-specific.
- The SELECT mod menu is drawn inside the game picture (4:3 etc.) instead of across the whole window over the side bars and touch controls (port/src/gs/mod_menu_ui.inc). Verified by the owner.
- Display-list blocks outside the game's memory are left out and logged instead of crashing the render thread's copy (gs_core.c / plat_mem.c). Reported on a Moto g42 at the character select; the cause of those blocks there is still open.

## v0.4.2

- 4:3 aspect ratio by default (the engine's Android default was the screen's own shape, which widens fights and stretches menus); a player's choice is kept.
- Internal resolution 1x by default and in every GPU profile (the engine's own default was 2x; a user's phone only worked after switching to 1x).
- The launcher no longer drops settings with names of 24+ characters when it saves one (most of the Tag Team mod's settings fell back to defaults).
- Wide aspect ratios: the fight's 2D (damage, timer, combo counter, READY/FIGHT, technique names) is narrowed to 4:3 in either of the game's two frame buffers (gs_sony.c / gs_draw.c); before, much of it was drawn into the buffer that was not this frame's and showed a third too wide, flickering. HUD markers are also taken on skipped frames. Verified by the owner at 16:9.
- The "texture pack was not used" warning only appears when a texture pack is installed.

## v0.4.1

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
