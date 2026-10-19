# Dragon Rage 0.8.1

**Dragon Net on three servers**, so online keeps working when one of them is down or out of its free requests for
the day. By Ariel R Rodrigues (pull request #3).

## Dragon Net
- **Server A** (the main one, `mitigo-rooms`): rooms, quick match and the players online.
- **Server B** (the backup, DBT-NX's): when server A fails three times in a row, the game moves to B by itself until
  00:00 UTC, then goes back to A. Two players must be on the same server to meet: if a room is not found, try again a
  little later.
- **Server C** (`bt-ranked`): **Ranked**, its searches and its rooms.
- Other servers can be set with one-line files in the game folder: `dragonnet_url.txt` (A), `dragonnet_url_b.txt` (B),
  `dragonnet_ranked_url.txt` (C).
- **General chat switched off for now**: it is greyed out in the Dragon Net Battle menu and comes back in a later
  version.

## Fewer requests to the servers
The free servers have a daily limit of requests, so the game asks them less often:
- the players online every 5 minutes (was 90 seconds);
- the Ranked profile every minute (was 15 seconds), and the Ranked search every 4 seconds (was 2);
- a room waiting for its rival every 3 seconds in the first minute and every 6 after it (was 2 and 4); a room code
  still lasts about ten minutes;
- the quick match queues (PC mode) every 90 seconds (was 30).

## Other
- The app's online guide and settings no longer mention the chat.
- A test APK that had been added to the code by mistake was removed.

## Update notes
- Installs over 0.8.0 (and 0.7.x) and keeps everything.
- Everything of 0.8.0 is still here: the Dragon Net Battle screen, Ranked, online against PC, the logs for everything
  and the online guide in three languages.
