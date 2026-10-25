# Dragon Rage 0.9.0

**The PC port's 0.1.38.7, Budokai Tenkaichi 4 drawn by ARMSX2's renderer, and lighter fights on Mali GPUs.** Everything
measured on an RG Cube (Mali-G57) and a Retroid Pocket 5 (Adreno 650), with a recorded fight and pixel-by-pixel picture
comparison.

## Faster
- **Internal resolution 2x and above**: drawing passes that never use depth run without it, flat shadows share a single
  clear, and the characters' glow is drawn at the PlayStation 2's resolution (from the PC). On the RG Cube a Vulkan 2x
  fight went from about 76% to **95%** of full speed (OpenGL 2x: 92% to 98%); on the Retroid Pocket 5, Vulkan 4x went
  from 71% to 84%.
- **OpenGL on Adreno**: persistent buffers on Adreno too. On the Retroid Pocket 5, OpenGL 2x took about 4 seconds a
  frame and now runs at a steady 30.
- **Adreno**: the flat shadows' stencil is off by default, faster on these chips.
- **GPU profiles** measured for the Mali-G57 MC4 and the Adreno 650: the launcher suggests the resolution that keeps
  fights at 30.
- **From the PC**: fewer hitches the first time a special or a transformation appears (new textures are prepared much
  faster, on several cores), drawing buffers made ready before a fight's first big effect, and the characters' outline
  drawn only where there is a line.
- **Texture packs**: files are read in the background and kept in memory, and a frame takes in at most 4 MB of them.
  Fewer hitches with large packs (from the PC).

## Picture (from the PC)
- **Super Saiyan glow** right at every internal resolution: it used to come out as a sharp, shifted copy of the
  character.
- **Distance blur** as on the console at high resolutions (leaves no longer show up as squares).
- **Outlines** the same thickness at every aspect ratio.
- **The back of Videl's shirt** without the dashed line.
- **White screen** with teen Gohan's third costume on Namek fixed.
- A texture written in a 24-bit or 4/8-bit format no longer keeps old content in its last rows.

## New: Budokai Tenkaichi 4
- **Team BT4's mod in the same app**, next to BT3: new characters, forms and stages. The launcher's opening screen
  picks the game; each has its own image, saves and settings. It needs your image of the mod **Beta 14 Rev2
  (SLUS-219.78)**: the game is recompiled for that version (the engine is the mod recompiled, from BT3-Recomp).
- **ARMSX2's renderer by default**: the PlayStation 2 emulator's (PCSX2's, with ARMSX2's fixes for Mali and Adreno),
  the most faithful picture of the mod. Dragon Rage's stays as an **experimental** option (Settings and the in-game
  menu, with one line explaining the difference). With it, far-off scenery can still come out as blocks of
  coloured noise; ARMSX2's draws it right.
- **Shaders compiled in advance**: the APK carries ARMSX2's shaders compiled on a Mali and an Adreno. The first picture
  appears in ~2.5 s instead of ~4.5 s, without the small compile hitches in menus and fights.
- **16:9** (as Team BT4 made it), 4:3 or full screen; **custom drivers** for BT4 too; an **FPS meter**.
- **60 fps fights (experimental)**: where the device cannot hold them (under 57 updates a second for 3 s in a fight),
  the game goes back to 30 by itself and says so: "This device could not hold 60 fps in BT4's fights; back to 30". On
  the devices measured it cannot yet: the RG Cube does 30 and the Retroid Pocket 5 about 36 to 41.
- The image can be picked from Downloads and still opens after the device restarts.

## Online
- **Server busy**: when the free server reaches its daily limit (HTTP 429, Cloudflare's error 1027), the game stops
  asking for a while (30 s, doubling up to 10 min) instead of insisting, moves to the backup server, and the launcher
  says **"Server busy (daily limit), try again later"**. Offline play is not affected.
- **Fewer requests**: the Ranked queue is only asked while its screen is open; a search asks every 4 s for its first
  half minute, then every 8 s, and after two minutes every 15 s; a result the rival has not confirmed yet is sent again
  in 5 s, doubling up to a minute (before: every round).
- **From the PC** (for Dragon Net and against PC): the rival's sounds that went missing with rollback (Z-counter,
  vanish, teleport), the ki charge sound that stayed the whole fight, Solar Flare blinding as offline, Japanese voices
  replaced by English lines, and a desync is a warning instead of the end of the match. Two games with different data
  (a modified ISO against the original) are no longer matched.
- The app's log says why when the launcher cannot reach the online service.

## Controls and launcher
- **Classic pad on a square screen** (RG Cube 720x720): L1/L2 and R1/R2 no longer overlap, and the face buttons and the
  d-pad stay on screen. Nothing changes on 16:9.
- **START + SELECT** also closes the in-game menu, not only opens it.
- The touch pad is hidden at start when a controller with a stick is connected.
- In the Drivers tab, the whole row picks the driver.
- The launcher's side rail scrolls (BT3's nine pages on 1080p screens).
- When Vulkan cannot start (for example with the MaliSX2 driver on the Mali-G57), the game falls back to OpenGL
  instead of a black screen.
- ADPF and sustained performance mode on the devices that have them, so the system does not lower the clock in the
  middle of a fight.

## Update notes
- Installs over 0.8.x and keeps saves, settings and the imported ISOs.
- Online: PC mode talks to the official server of release 0.1.38; Dragon Net still plays with the Switch (DBT-NX).
