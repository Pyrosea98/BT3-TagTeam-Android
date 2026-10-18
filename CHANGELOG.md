# Dragon Rage 0.7.3

**Online works again**: the PC port released 0.1.37 and the matchmaking service now asks for it. This version is
0.1.37.

## From the PC port's 0.1.37
- Online: a line on the screen while a match starts (*Connecting to your rival…*, then *Your rival "NAME" is here!*);
  no more pause before the match that grew with the ping; the profile name no longer erased by Quick Match.
- Blending: a source alpha above 0x80 counts as on the PS2 (the Solar Flare's whiteout).
- Clashes, the Square option: a press can count 2.25, the default unless another is chosen.
- Game text translations (menus, battle HUD, dialogue, the port's own text) from catalogs in
  `translations/`, copied into the game folder by the app. English is the only catalog so far; the European disc
  option of the PC setup is not part of Dragon Rage.

## Fixes and help
- **Phones where the game closed at once** (a Redmi with a Snapdragon 8s Gen 3, HyperOS 2: *"cannot map the game heap"*):
  when the game heap's PS2 address is taken in the process, the heap now goes elsewhere below 2 GB, as on Windows.
- **Logs saved by themselves**: when the game closed badly, or a texture pack cannot be installed, the app saves the
  log in Downloads at once, says its name, and a **Send log** button opens the share sheet. *"Send this file to the
  developer."*
- **Textures page**: a card explaining how packs work (USA release, the .zip as it came, what to expect) with a log
  button; after a fight in which a pack was on but none of its textures was used, the app says so (usually a pack for
  another release of the game) and saves the log.
- The *"game closed"* dialog names the real cause (memory, data, engine, textures) before a crash that follows it.

## Online server
- Settings → **Matchmaking server**: **Official** (the PC port's, PC and Android together; the default) or
  **Community** (a service run by players, from the Tenkaichi3DecompAndroidOnline fork; only the players who chose it
  meet there). Direct connections (address and port) use no server.

## Update notes
- Installs over 0.7.x / 0.6.0 and keeps everything.
- If an update says *"App not installed: the package conflicts with an existing package"*, the app on the phone is
  not one of these releases (another build of the code, with another signature): save `saves/` from
  `Android/data/com.dfdx047.dragonrage/files/`, uninstall it, install this one and put the saves back.
