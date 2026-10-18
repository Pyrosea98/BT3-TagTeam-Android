# Dragon Rage 0.8.0

**Dragon Net is here**: online with the Nintendo Switch port (DBT-NX) on a service of our own, with Ranked, a general
chat and the players online, and no longer stopped by each new release of the PC port. Online against PC stays as an
option. This version also brings everything of 0.7.3 (the PC port's 0.1.37, the heap fix for some Snapdragon phones,
logs saved by themselves) and a log for everything the app does.

## Dragon Net: Android + Switch
- **Two kinds of online**, chosen in **Play against**:
  - **Android + Switch (Dragon Net)**, the default: Dragon Rage and DBT-NX on their own official service (the one of
    the Tenkaichi3DecompAndroidOnline fork), for Android × Android and Android × Switch. It does not depend on the PC
    port's releases, so a new PC version no longer turns online off.
  - **PC**: rooms and quick match against the PC port, on its service (which asks for the PC port's newest release).
- Choose it in the game (Dragon Net Battle → Play against, or **Y** on a controller) or in the app (Settings →
  **Online**). Both players must choose the same one.
- Games of versions that cannot fight together never meet: the service keeps each version apart.

## A new Dragon Net Battle screen
- The game's **Dragon Net Battle** (main menu) opens a new screen in DBT-NX's look, made for touch and controller, in
  **English, Portuguese and Spanish**.
- **Create room**: choose the battle (single, team, DP 10/15/20) and open it; a big 6-character code appears, a tap
  copies it.
- **Join room**: type or **paste** the code.
- **Quick match**: choose the battle; how many wait in each queue; the search ends by itself after three minutes.
- **Ranked** and **General chat** (Dragon Net).
- Your **name** in a badge at the top: tap it (or Start) to change it.
- The count of **players online** (Android and Switch) at the bottom.
- Clear messages for every problem: the other game on another version, nobody searching, the two games unable to
  reach each other, the service not taking this version yet.
- Controller: D-pad to choose, A accept, B back, Y switches Android + Switch / PC, Start edits your name.

## Ranked (Elo)
- A **profile** made the first time you open Ranked: rank (Bronze to Master), **Elo**, wins, losses and position.
- **Search for a rival**, single or team: you are paired with a player of your level.
- Ranked fights: no time limit, the standard speed, rollback.
- At the end both games send the result; **points change only when both agree**. Leaving a ranked fight before its end
  counts as a loss.
- **Ranking**: the best players; **Update profile**.

## General chat
- One chat for everyone on Dragon Net, Android and Switch, in the online screen.

## The app (launcher)
- Settings → **Online**: Play against (Android + Switch / PC).
- The **online card** on Home follows the chosen mode: Dragon Net working, Dragon Net being updated (the service does
  not take this version yet), PC needing an update, no answer.
- **How online works** is a full guide now, in the three languages: the two kinds of online, how to play, Ranked, the
  chat and players online, what each message means and what to do, and a **Send log** button.

## Logs for everything
- The app keeps a **diary of what it does** (`app_activity.txt` in the game folder): installing the game from an ISO
  (the file, the free space, each step, how long it took), texture packs, mods, drivers, updates, the online check,
  the renderer, every start of the game, and the **whole error** when something fails.
- **Any failure saves a log in Downloads by itself** and shows a dialog with **Send log**: the ISO that could not be
  installed, a driver, a mod, an update, a texture pack, the game closing badly.
- The game keeps its **last three runs** (`bt3_log.txt`, `.1`, `.2`): a problem seen one run late is still there.
- The exported log has all of it: the device, the app's diary, the game folder (sizes, packs, mods), the settings, the
  game's last three runs and the system's log.

## From the PC port's 0.1.37 (in 0.7.3)
- Online: a line on the screen while a match starts (*Connecting to your rival…*, then *Your rival "NAME" is here!*);
  no more pause before the match that grew with the ping; the profile name no longer erased by Quick Match.
- Blending: a source alpha above 0x80 counts as on the PS2 (the Solar Flare's whiteout).
- Clashes, the Square option: a press can count 2.25, the default unless another is chosen.
- Game text translations from catalogs in `translations/`, copied into the game folder by the app (English so far).

## Fixes and help (in 0.7.3)
- **Phones where the game closed at once** (a Redmi with a Snapdragon 8s Gen 3, HyperOS 2: *"cannot map the game
  heap"*): the game heap now goes elsewhere when its PS2 address is taken.
- **Textures page**: a card explaining how packs work (USA release, the .zip as it came, what to expect) with a log
  button; after a fight in which a pack was on but none of its textures was used, the app says so.
- The *"game closed"* dialog names the real cause (memory, data, engine, textures).
- Changelogs: one file per language, shown by the app's update dialog.

## For developers
- `port/dragonnet`: the Dragon Net service's code (rooms, quick match, Ranked, chat, players online; it takes several
  versions at once and keeps them apart) with its tests, for the official server.
- `docs/DRAGON_NET.md` (English and Spanish): what Android and Switch must agree on (version 5001), Ranked's flow, the
  result's digest, the chat and presence, and how to update the service.

## Update notes
- Installs over 0.7.x and 0.6.0 and keeps everything.
- Dragon Net needs its server running this version's `port/dragonnet` (done with this release). If the online card says *"Dragon Net
  is being updated"*, choose **Play against: PC** meanwhile.
- If the update says *"App not installed: the package conflicts with an existing package"*, the app on the phone is
  another build of the code (another signature): save `saves/` from
  `Android/data/com.dfdx047.dragonrage/files/`, uninstall, install this one and put the saves back.
