# BT2 online: the fight, from what BT3 does (2026-10-27)

Read together with [ONLINE_PLAN.md](ONLINE_PLAN.md) (stage 3). This is what BT3's netplay does, what of it BT2 can have and how, and where the work stands.

## What the games have

| | Online play |
|---|---|
| BT3 | Complete: `port/src/gs/net.c`, `net_relay.c`, `net_match.c`, `state.c`; the design is in `docs/netplay_notes.md`. |
| BT4 | None. Its engine answers "no" to every network question (`port/bt4/host/bt4_shim.c`: `Port_NetActive`, `Port_NetSession` return 0; `port/bt4/README.md`: online is the BT3 engine's). |
| BT2 | The line between two phones (pairing through the service, STUN, hole punch, UDP, optional relay) and the screens. The fight is what is left. |

BT3 and BT4 are not touched by anything below.

## How BT3 keeps two copies in step

1. **Same start.** Two copies only agree if they start from the same state. A session is a fresh start of the program on both sides
   (first by relaunching with `BT3_NET_SESSION=1`, later by putting the game back to a snapshot kept at the first blank), with a save folder
   of the session's own and everything unlocked, so the roster is the same. The start-up runs with nothing pressed, without picture or sound and
   not held to real time, to the character select; the two meet there (`T_ARRIVED`).
2. **Lockstep.** One player per copy (the host is player 1). Every vertical blank a copy sends its player's input for that blank, the last 16
   in every packet, and does not go on without the other's input for it. An input takes effect `delay` blanks after it was made (default 2;
   automatic from the measured round trip). A waiting copy sends its latest packet again every few milliseconds.
3. **Randomness.** The simulation's own generators reset with the fighters; the ones fed from a timer at boot are the problem (notes,
   "Sources of randomness"). Anything the game waits for that depends on real time (voice playback, model loading) must be made the same.
4. **Checks.** Every 32 blanks each side notes a number made of what must be equal (the fighters' health, position and the clock); the newest
   travels in every packet and a difference ends the match with a message.
5. **Rollback** (optional, off by default) needs the whole state saved per blank. **Clock balance**: each packet says how far the sender is
   beyond the other's input; the side that is ahead lets its blanks come a little later.
6. **Round trip and delay**: 12 pings at the start, the host chooses the delay (1 to 6) and tells the other side.

## What BT2 can have, and what is different

- BT2 is recompiled, not decompiled: there is no state to save, so **no rollback**; lockstep only, with the delay from the round trip. At 45 ms
  (the test across two networks) the delay is 3 fields and the game does not wait; a worse line makes both games wait, as BT3's first version did.
- The time base is the field (a vertical blank, 60 a second; a fight draws every second field). The input is the pad service's six bytes per field
  (buttons, four sticks), the format of `BT2_INPUT_RECORD`.
- Where input enters: the game's own pad routines (`frame_pad` 0x28B090, `frame_pad2` 0x28BDA0) work on one structure at 0x3B0B00 that holds both
  pads; the plugin already hooks `frame_pad` and writes `pad+0x144` (held) and `+0x134` (pressed). The second player's fields in that structure are not
  mapped yet (see "Next").

## Done: `plugin/online/lockstep.{h,cpp}`, tested

The exchange itself, with nothing of the network or the game in it: `pushLocal`, `ready`, `pair`, `done`, `makePacket`, `receive`, `noteDigest`, `delayFor`.
A packet holds every sample of its side the other has not confirmed (up to 64), the position up to which it holds the other's, `ahead`, the newest
check value and a sum over the whole packet.

`plugin/tests/lockstep_tests.cpp` (AddressSanitizer and UBSan on) simulates two sides on a line and checks that the game on both sees the same two
controllers at every field, the ones the players made `delay` fields before: clean lines at delay 0 to 16; 180 runs with 10 to 40% of packets lost,
15% repeated, reordered and 1 to 6 fields late; a 2.5 s and a 5 s tunnel; one clock slower than the other; damaged packets; a check value changed on
one side (found on both, at the right field); random bytes into `receive` (never taken). 892 checks, 0 failures.

The first run of the tests found a real defect: a side dropped its own samples as soon as its game was past them, while the other could still be
waiting for them (every lossy run hung). Samples are now kept until the other side confirms them.

## Next, in this order

1. **Is a run repeatable across two phones?** `sync_test = 1` then `2` on one phone (written, never run), and the same scripted run on two phones with the
   state hashes compared (`tools/ps2recomp/compare_state_hash.py`). Everything below depends on it.
2. **The second pad.** Dump 0x3B0B00.. in a fight and find where player 2's held / pressed / sticks are; the pad service answers "no device" for port 1.
3. **The per-field loop in the plugin**: read the local controller (as the game has it after `frame_pad`), `pushLocal`, send, `link.poll`, wait for `ready`
   (the game thread sleeps in short steps, keeps polling the line and gives up with the existing silence rule), write both players' fields, `done`.
4. **A session start both sides share**: the app starts the engine with a session flag, a card of its own with everything unlocked (the app already carries
   one), the start-up walked by script to the character select, the two meet (an "arrived" message), the host is player 1 (drawn at random), lockstep begins.
   The line must survive that restart, or be made again quickly (the socket is new, so its address changes): decide after step 1.
5. **Checks**: pick the digest (both fighters' health, position, the clock) and note it every 32 fields; show a message on a difference.
6. **Clock balance and the meter** (delay, waits, ping), then the names under the health bars and the stage and music chosen by player 1 only.
