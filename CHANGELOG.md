# Dragon Rage 0.8.0

**Dragon Net**: online with the Nintendo Switch port (DBT-NX) on a service of our own, no longer stopped by each new
release of the PC port. Online against PC stays as an option.

## Dragon Net Battle, a new online screen
- The game's **Dragon Net Battle** (main menu) opens a new screen in DBT-NX's look, for touch and controller, in
  English, Portuguese and Spanish: **Create room**, **Join room**, **Quick match**, **Ranked** and **General chat**,
  with the count of players online.
- **Play against**: **Android + Switch** (Dragon Net, the default) or **PC** (the PC port's service, which asks for
  its newest release). Also in Settings → Online. Ranked and the chat are Dragon Net's.
- **Ranked**: a profile with rank, Elo, wins, losses and position; single or team search by level; the ranking's top
  players. The points change when both games send the same result; leaving a ranked fight before its end is a loss.
- Rooms show a big code to copy; Join room takes a pasted code. Ranked and quick fights: no time limit, the standard
  speed, rollback.
- Controller: D-pad to choose, A accept, B back, Y switches Android + Switch / PC, Start edits your name.

## Under the hood
- Dragon Net speaks the PC port's 0.1.37 netcode with version **5001**; the service keeps every version apart, so
  games that cannot play together never meet (`docs/DRAGON_NET.md`).
- `port/dragonnet`: the service (DBT-NX's Worker, taking several versions at once) and its tests.
- The launcher's online card follows the chosen mode, and says when the Dragon Net service does not take this
  version yet.
