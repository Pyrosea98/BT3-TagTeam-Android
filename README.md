# BT3 Tag Team + Real Power Scale â€” Android

Our Android fork of [Dragon Rage](https://github.com/dfdx047/Tenkaichi3DecompAndroid), with the current native Tag Team / Power Scale BETA 8 code merged from the PC port.

- **APK releases:** https://github.com/Pyrosea98/BT3-TagTeam-Android/releases
- **Current Android source:** https://github.com/Pyrosea98/BT3-TagTeam-Android/tree/powerscale-port
- **PC installer:** https://github.com/Pyrosea98/BT3-TagTeam-Native/releases/tag/v0.4.1
- **PC engine source:** https://github.com/Pyrosea98/BT3-TagTeam-Runtime/tree/decomp-native

## Install (v0.4.1)

1. Download `BT3-TagTeam-v0.4.1-Android.apk` from the [v0.4.1 release](https://github.com/Pyrosea98/BT3-TagTeam-Android/releases/tag/v0.4.1) and install it. It is its own app, **BT3 Tag Team** (`com.pyrosea98.bt3tagteam`), so it installs next to Dragon Rage without touching it. Android 10 or newer, 64-bit (arm64).
2. Put your **Power Scale BETA 8** ISO on the phone: the original (SHA-256 `f4afdfd3…`) or the R4 normal / 2x image (`3039ac35…` / `258d8b94…`). Any other image is refused.
3. Open the app, Home > Game data: choose **2x maps** (default) or **Normal maps**, keep "Use recommended settings" and "Start with a completed save" if you want them, then **Choose ISO**.
4. The app downloads the v0.4.1 import recipe (429 MB, the same `BT3-TagTeam-v0.4.1-ImportRecipes.zip` as the PC release, checked by its SHA-256), checks your ISO and builds the game data, every file checked by its SHA-256. About 3-4 minutes. Keep the screen on the app; existing saves and settings are kept.
5. Press **PLAY**.

Recommended settings are the PC's tested setup (fusion control alternating every 10 s, assists, CPU transformations, revival, HUD...); Settings > "Tag Team recommended settings" applies them again at any time. The pause menu's Tag Team page has every mod setting. In fights, the two crosshair buttons next to R3 move the lock-on target left/right (a right-stick flick).

See `RELEASE-ANDROID.md` for the build, signing and verification record.

Supply your own compatible Power Scale BETA 8 ISO. No disc image, game archives, BIOS, extracted game data, personal save, device settings or private signing key is included in our changes. Normal and optional 2x maps must remain supported through import; the PC Windows importer is not an Android installer.

## Build

See `README.dragon-rage.md`, `android/` and `port/tools/android_toolchain.sh`. Build the ARM64 engine with `BT3_SKELETON=1 python3 port/tools/android.py`, then build the Android app with `android/gradlew assembleRelease` and the documented JDK/SDK. Production signing is supplied externally through `DR_KEYSTORE`, `DR_KEYSTORE_PASSWORD`, `DR_KEY_ALIAS`, `DR_KEY_PASSWORD`; never commit a private keystore. No signing keystore is included in this publication branch or source archive. Debug builds use a locally generated key; assembleRelease requires external signing before distribution. Keep package/signing compatibility decisions with Claude.

## Credits and licenses

Dragon Rage by dfdx047 and contributors; native decompilation/port by z3xox and contributors; original Tag Team mod by [The Mufti](https://github.com/tehmufti/Budokai-Tenkaichi-3-Tag-Team-Mod-PCSX2-); Power Scale by LetsPlayBt3. Special thanks to [RidJuampa](https://www.youtube.com/@RidJuampa) for helping me understand the code. Preserve original startup credits/cards and the shared credits/thank-you pages.

Preserve `LICENSE` (AGPL-3.0), `LICENSE-MIT`, all upstream attribution and component notices. This fork retains Dragon Rage ancestry; upstream engine and old PC runtime remain separate repositories.

The publication branch is a clean source snapshot of the Dragon Rage merge identified in `ANDROID_RELEASE_PROVENANCE.json`. Its earlier local commits contained the inherited test keystore, so that history is deliberately not pushed. The GitHub repository retains its Dragon Rage fork relationship.
