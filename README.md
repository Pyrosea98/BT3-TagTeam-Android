# BT3 Tag Team + Real Power Scale â€” Android

Our Android fork of [Dragon Rage](https://github.com/dfdx047/Tenkaichi3DecompAndroid), with the current native Tag Team / Power Scale BETA 8 code merged from the PC port.

- **APK releases:** https://github.com/Pyrosea98/BT3-TagTeam-Android/releases
- **Current Android source:** https://github.com/Pyrosea98/BT3-TagTeam-Android/tree/powerscale-port
- **PC installer:** https://github.com/Pyrosea98/BT3-TagTeam-Native/releases/tag/v0.4.1
- **PC engine source:** https://github.com/Pyrosea98/BT3-TagTeam-Runtime/tree/decomp-native

Claude owns Android builds, signing, phone validation and the final APK release gate. v0.4.1 is the mod version; the inherited launcher currently reports the Dragon Rage 0.9.0 base. See `RELEASE-ANDROID.md` for the exact source/build handoff and release status. A draft release is preparation, not an APK download announcement.

Supply your own compatible Power Scale BETA 8 ISO. No disc image, game archives, BIOS, extracted game data, personal save, device settings or private signing key is included in our changes. Normal and optional 2x maps must remain supported through import; the PC Windows importer is not an Android installer.

## Build

See `README.dragon-rage.md`, `android/` and `port/tools/android_toolchain.sh`. Build the ARM64 engine with `BT3_SKELETON=1 python3 port/tools/android.py`, then build the Android app with `android/gradlew assembleRelease` and the documented JDK/SDK. Production signing is supplied externally through `DR_KEYSTORE`, `DR_KEYSTORE_PASSWORD`, `DR_KEY_ALIAS`, `DR_KEY_PASSWORD`; never commit a private keystore. No signing keystore is included in this publication branch or source archive. Debug builds use a locally generated key; assembleRelease requires external signing before distribution. Keep package/signing compatibility decisions with Claude.

## Credits and licenses

Dragon Rage by dfdx047 and contributors; native decompilation/port by z3xox and contributors; original Tag Team mod by [The Mufti](https://github.com/tehmufti/Budokai-Tenkaichi-3-Tag-Team-Mod-PCSX2-); Power Scale by LetsPlayBt3. Special thanks to [RidJuampa](https://www.youtube.com/@RidJuampa) for helping me understand the code. Preserve original startup credits/cards and the shared credits/thank-you pages.

Preserve `LICENSE` (AGPL-3.0), `LICENSE-MIT`, all upstream attribution and component notices. This fork retains Dragon Rage ancestry; upstream engine and old PC runtime remain separate repositories.

The publication branch is a clean source snapshot of the Dragon Rage merge identified in `ANDROID_RELEASE_PROVENANCE.json`. Its earlier local commits contained the inherited test keystore, so that history is deliberately not pushed. The GitHub repository retains its Dragon Rage fork relationship.
