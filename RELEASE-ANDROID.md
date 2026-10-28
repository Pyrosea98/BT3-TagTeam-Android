# Android v0.4.1 â€” release preparation

Current source is the Dragon Rage 0.9.0 merge plus our native Tag Team / Power Scale gameplay, HUD/settings, shared credits and Android target controls. The latest launcher edit removes the BT4/game-choice screen for this BT3-only mod and places Settings earlier in navigation.

**APK publication gate: Claude.** The previous working APK is `BT3-Android-v4-dr090-f8c2cdcc.apk` (SHA256 `f8c2cdcc2cfb48b646d51433beea150de799b80c0b37acf1667c3d5897df1ad0`, engine `09a8d70deabe4830e2d08d8300a1456dbaee269689510a1464c2cf09f478615b`). That APK predates the final MainActivity edit. It is NOT attached as the final v0.4.1 APK.

Claude: build/sign and verify the APK from this published revision, or commit any necessary Android-only corrections to powerscale-port, then move the draft tag/source archive/checksums to that exact revision before publication. Preserve working application ID and the user's data/signature compatibility; use the approved release-signing policy and never upload keys. Record the final APK hash, native-library hash, build tools, source commit, smoke/live scope and import/save behavior. Do not publish the draft until its APK and source match.

Keep both normal and optional 2x map import paths compatible. Verify shared credits/thanks in startup, mod settings and Android import/startup presentation, and retain all original modder/publisher cards. The Windows credits UI itself is not an Android component. Source is published now; no new phone install/test is claimed by this handoff.

PC release remains independent: https://github.com/Pyrosea98/BT3-TagTeam-Native/releases/tag/v0.4.1

Publication safety: all signing keystores were removed from this clean source branch and archive. Gradle no longer falls back to the inherited test key for release builds. Claude must supply external signing compatible with the intended installation/update policy. No existing APK was rebuilt, re-signed or installed by Codex.
