# BT2 online play (Dragon Net 2)

Written 2026-10-25. Android only. Names stay neutral; addresses are BT2's (USA disc, SLUS-21441).

## What is decided

- **The app does everything that is not the game's memory.** The player's name, the room code, the search, the requests to the
  matchmaking service and their errors are Kotlin (`android/.../data/Bt2Online.kt`, `engine/Bt2OnlineOverlay.kt`). The engine
  (`libbt2.so`, a signed package outside GitHub) keeps what touches the game: the menu pages, the pictures drawn over the frame, and
  later the exchange of the pads and the stepping of the game. Anything that can live in the app goes there: an app update reaches
  the players with the APK, an engine update needs a new motor package.
- **The same matchmaking service as BT3** (`port/dragonnet/worker.js`, run by its owner), with a netcode version of its own:
  `Bt2Online.VERSION = 6001` (BT3: 5001). The service pairs only equal versions, so a BT2 player never meets a BT3 one, and BT3 is not
  touched. The service's `ALLOWED_VERSIONS` must list 6001 (its owner's change) before the official service takes BT2.
- **No BT2-against-BT3 play, and none against the Switch or the PC port**: different games, different fights.
- **Development without the official service:** `node bt2/tools/dev_server/serve.mjs` runs `worker.js` unchanged on the developer's
  computer (in-memory database, `adb reverse tcp:8787 tcp:8787`, `dragonnet_url.txt` in `<root>/bt2/user`). Debug builds accept
  cleartext to 127.0.0.1 only (`android/app/src/debug`).

## The stages

1. **Menus** (done, tested on a phone): Online Mode item in the main menu, its pages (Online / Ranked; Create room / Join room / Quick
   match) and the name box.
2. **Pairing** (done, tested against the development service): create a room (a code), join with a code, quick match; the opponent's
   name is shown. The app talks to the service; the game's library draws the screens and goes by the app's word, files in
   `<root>/bt2/user`: `online_peer` (the opponent: name, room code, his addresses, a line each), `online_back`; and the library
   tells the app through `online_local` (the addresses of its socket) and `online_link` (`rtt=<ms>`). The state is the fourth count
   of `Bt2Bridge.nativeStats()`: 0 closed, 1 Online/Ranked, 2 the three ways in, 11/12/13 create/join/quick in progress, 15 saying
   hello to the other game, 14 the line is up, 16 it could not be made.
2b. **The library** (done): everything of the menu and of the online play that does not need the game's code is `libbt2online.so`
   (`bt2/plugin/`, API in `bt2_plugin.h`), built by the public CI (`build.py plugin`, non-blocking step in `android.yml`) and carried
   by the APK; the engine loads it (`runtime/plugin/plugin_host.cpp`). Quick loop: `build.py plugin` takes 15 s.
2c. **The direct line** (done, tested 2026-10-26 on the same Wi-Fi: phone shows "Conectado! Atraso: 38 ms", PC measures 22-32 ms): `bt2/plugin/online/link.{h,cpp}`, UDP "hello" to every address the service
   gave, "ping" for the round trip, 15 s to give up. Test with one phone: `bt2/tools/dev_server/peer.py` plays the other player on the
   computer (same Wi-Fi).
3. **The fight** (not started, the hard part).

## Next, in this order

1. (done) the line over the same Wi-Fi works; not tested across networks or on mobile data.
2. **The relay (TURN)**, so that any two phones can play. BT3 does it in `port/src/gs/net_relay.c` (RFC 5766 over UDP: Allocate with
   long-term credentials, CreatePermission, Refresh, Send/Data indications, ChannelBind). The service gives the host `turn`
   {server, username, credential} in the answer of `/v1/host` (only when it has `TURN_KEY_ID`/`TURN_KEY_TOKEN`; the development
   copy has none, so test the relay with a local TURN server such as coturn, or with the owner's keys); the host publishes
   `relay=ip:port` with `/v1/addrs`; the other player's addresses carry `seen=ip`. In the library: a small TURN client next to `link.cpp`;
   `Link::start` already skips `relay=` / `seen=` entries.
3. The repeatability check (stage 3, step 1 below), the second pad, lockstep.
## Stage 3: what BT3 does, and what BT2 lacks

BT3 (`port/src/gs/net.c`, `state.c`, `net_relay.c`; notes in `docs/netplay_notes.md`): both copies run the same game from the first
vertical blank, each with one local player (the host is player 1); every blank a copy sends its player's input for that blank over UDP
(the last 16 blanks in each packet, so a lost one is covered) and does not go on without the other's input; an input delay of a few
blanks gives the packet time to arrive; on top of that, rewinding (the state of the game as one number, saved and restored) lets a copy
guess the other's input and go back when wrong. When two players cannot reach each other, the host asks the service for a TURN relay
(Cloudflare) and publishes its address.

What BT2 has: the pad service is one place (`services/pad_service.cpp`), so the other player's input can enter there; `ee/state_hash.cpp`
hashes EE RAM, the scratchpad and the VU memories at the end of every field; fields are counted (one per vertical blank).

What BT2 lacks, in the order it must be settled:

1. **Is a run repeatable?** The same pad input from the same start must give the same states, field by field. This is the question
   nothing else can be built without. Test: two runs with the same input (the state hash tool, `tools/ps2recomp/compare_state_hash.py`).
   Sources to look at first: the game's random generators seeded from a timer at boot, the disc's read timing, the host-side threads of
   the recompiled EE (`ee/retained_ee_stack.cpp`), the sound and video services' timing. A first attempt (2026-10-25) recorded the
   player's input and replayed it, hashing every field; the hash slowed the game from 60 to about 48 fields a second, so a check must be
   made cheaper (hash every N fields, or hash only the fight's own values) before it can run in a normal session.
2. **A second pad.** The pad service answers "no device" for port 1; a two-player fight needs both. The other player's input is written
   as port 1 (the host: port 0 local, port 1 remote; the one who joins: reversed or mirrored).
3. **Starting both from the same state.** BT3 keeps the state at the first blank and puts the game back to it at a session's start.
   BT2 would need the whole machine's state saved and restored: EE RAM, scratchpad, VU memories, registers, the recompiled threads'
   stacks, the services' own state (cards, sound, GS). Alternative: both games reach the same point by running the same input from a
   fresh start, if (1) holds.
4. **Lockstep** over UDP with the redundancy and delay above, then rewinding only if lockstep alone is too slow.
5. **The connection**: addresses from the service (`addrs`, "ip:port"), a hole punch, and the host's TURN relay when needed. The app
   now reports this phone's addresses with a placeholder port; the engine's socket will report the real one.

## Release

Stage 2 needs the app and the engine updated together (the app's overlay reads the engine's fourth count; the engine's pages need the
app's pictures). Stage 3 will also raise `BT2_ENGINE_ABI` if the app and engine start to exchange more than counts and files.
