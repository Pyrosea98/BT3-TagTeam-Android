# Dragon Net: online Android + Switch

*(Español abajo.)*

Dragon Net is the online play between **Dragon Rage** (Android) and **DBT-NX** (Nintendo Switch) on a service of
their own: DBT-NX's Cloudflare Worker, updated to `port/dragonnet`. It does not depend on the PC port's releases.
Online against PC stays as a second option (the PC port's service, which only lets in its newest release).

## What both games must agree on

| | |
|---|---|
| Netcode | the PC port's 0.1.37: `NET_VERSION` 5 (`port/src/gs/net.c`), rollback, the same packets |
| Simulation | `DRNET_SIM` 1 (`port/src/gs/net_dragon.h`): raised together, only when something that decides a fight changes |
| `v` sent to the service | `NET_VERSION * 1000 + DRNET_SIM` = **5001** in every request (rooms, quick match, Ranked, chat, presence) |
| Game | the USA disc (SLUS-21678) |
| Host | side 0 (player 1); the one who joins is side 1 |

Against PC (mode 1) the game sends the plain `NET_VERSION` (5) to the PC port's service, as the PC does.

## Rooms and quick match

The same requests as the PC port (`/v1/host`, `/v1/join`, `/v1/quick`, `/v1/queues`, ...: see `worker.js`), with
`v` = 5001. The service never pairs two different `v`.

## Ranked

1. `ranked/register {v, name}` → `{id, token}` (kept by the game; Dragon Rage: `dragonnet-device.txt`).
2. `ranked/profile`, `ranked/leaderboard`: the profile (rank, rating, wins, losses, position) and the top players.
3. `ranked/queue {v, id, token, rules: "solo-inf-v1" | "team3-inf-v1", rematches: 0}` until it answers `paired` with
   `match`, `role` (1 host, 2 joins) and the rival's name.
4. The host opens a room with a code (as Create room, `v` 5001) and publishes it: `ranked/room {match, code}`. The
   other one asks `ranked/queue` until the code is there and joins it.
5. Rules of a ranked fight: no time limit, the standard speed (`Port_NetSpeedStandard`), rollback 8 and the delay from
   the connection; single (`solo-inf-v1`) or team of three (`team3-inf-v1`).
6. At the end of the fight **both** send `ranked/result {match, score, digest, round: 1}`:
   - `score` from the **host's** view: 2 the host won, 0 the host lost, 1 a draw;
   - `digest`: FNV-1a 32 bits (offset 2166136261, prime 16777619) over the 12 bytes of three little-endian `uint32`:
     the battle's result flags (`BattleResult.winner`), its reason and `clock.ticks`; 8 lowercase hex digits.
   `ranked/match {match}` tells its state. The points change when both results agree (`confirmed`); different
   results: `disputed`, no points.
7. Leaving before the end of the fight: `ranked/forfeit` (a loss). `ranked/cancel` leaves the search.

## General chat and players online

- `chat/sync {v, id, token, after, text_hex?, nonce?}` → `rows` of `{seq, name_hex, text_hex, mine}`; every 3 s while
  the chat is open.
- `presence {v, id, action: "beat" | "leave"}` → `{online}`; a beat every 90 s while the online screen is open, an
  anonymous id of its own (not the Ranked one).

## The service

`port/dragonnet/README.md`: one SQL migration (`ranked-migrate-version.sql`), the five `.js` files and
`ALLOWED_VERSIONS = 40001,5001` (40001: DBT-NX before its merge with 0.1.37, until its players have updated). Profiles,
Elo, chat and the count are shared between versions; fights never are.

**Order of a release:** the service is updated first (with 5001 allowed), then Dragon Rage 0.8.0 and the merged
DBT-NX are released. Until then Dragon Rage says *"The Dragon Net service does not take this version yet"* and online
against PC keeps working.

---

# Dragon Net: online Android + Switch (ES)

Dragon Net es el online entre **Dragon Rage** (Android) y **DBT-NX** (Nintendo Switch) en un servicio propio: el
Worker de Cloudflare de DBT-NX, actualizado a `port/dragonnet`. No depende de las versiones del port de PC. El online
contra PC queda como segunda opción (el servicio del port de PC, que solo acepta su versión más nueva).

## Lo que los dos juegos deben tener igual

| | |
|---|---|
| Netcode | el del port de PC 0.1.37: `NET_VERSION` 5 (`port/src/gs/net.c`), rollback, los mismos paquetes |
| Simulación | `DRNET_SIM` 1 (`port/src/gs/net_dragon.h`): se sube a la vez y solo cuando cambia algo que decide la pelea |
| `v` enviado al servicio | `NET_VERSION * 1000 + DRNET_SIM` = **5001** en todas las peticiones (salas, partida rápida, Ranked, chat, presencia) |
| Juego | el disco americano (SLUS-21678) |
| Anfitrión | lado 0 (jugador 1); el que se une es el lado 1 |

Contra PC (modo 1) el juego envía `NET_VERSION` tal cual (5) al servicio del port de PC, como el PC.

## Salas y partida rápida

Las mismas peticiones que el port de PC (`/v1/host`, `/v1/join`, `/v1/quick`, `/v1/queues`, ...: ver `worker.js`), con
`v` = 5001. El servicio nunca empareja dos `v` distintas.

## Clasificatoria (Ranked)

1. `ranked/register {v, name}` → `{id, token}` (el juego lo guarda; Dragon Rage: `dragonnet-device.txt`).
2. `ranked/profile`, `ranked/leaderboard`: el perfil (rango, Elo, victorias, derrotas, puesto) y los mejores.
3. `ranked/queue {v, id, token, rules: "solo-inf-v1" | "team3-inf-v1", rematches: 0}` hasta que responde `paired` con
   `match`, `role` (1 anfitrión, 2 se une) y el nombre del rival.
4. El anfitrión abre una sala con código (como Crear sala, `v` 5001) y la publica: `ranked/room {match, code}`. El otro
   pregunta `ranked/queue` hasta que llega el código y se une.
5. Reglas de una pelea Ranked: sin límite de tiempo, la velocidad estándar (`Port_NetSpeedStandard`), rollback 8 y el
   retardo según la conexión; individual (`solo-inf-v1`) o equipo de tres (`team3-inf-v1`).
6. Al terminar la pelea **los dos** envían `ranked/result {match, score, digest, round: 1}`:
   - `score` desde el punto de vista del **anfitrión**: 2 ganó el anfitrión, 0 perdió, 1 empate;
   - `digest`: FNV-1a de 32 bits (offset 2166136261, primo 16777619) sobre los 12 bytes de tres `uint32` little-endian:
     las flags del resultado (`BattleResult.winner`), el motivo y `clock.ticks`; 8 dígitos hex en minúscula.
   `ranked/match {match}` dice su estado. Los puntos cambian cuando los dos resultados coinciden (`confirmed`); si no
   coinciden: `disputed`, sin puntos.
7. Salir antes del final de la pelea: `ranked/forfeit` (derrota). `ranked/cancel` deja la búsqueda.

## Chat general y jugadores en línea

- `chat/sync {v, id, token, after, text_hex?, nonce?}` → `rows` de `{seq, name_hex, text_hex, mine}`; cada 3 s con el
  chat abierto.
- `presence {v, id, action: "beat" | "leave"}` → `{online}`; un latido cada 90 s con la pantalla online abierta, con un
  id anónimo propio (no el de Ranked).

## El servicio

`port/dragonnet/README.md`: una migración SQL (`ranked-migrate-version.sql`), los cinco `.js` y
`ALLOWED_VERSIONS = 40001,5001` (40001: DBT-NX antes del merge con 0.1.37, hasta que sus jugadores actualicen). Perfiles,
Elo, chat y contador se comparten entre versiones; las peleas nunca.

**Orden de un lanzamiento:** primero se actualiza el servicio (con 5001 permitido), después salen Dragon Rage 0.8.0 y
el DBT-NX con el merge. Hasta entonces Dragon Rage dice *"El servicio Dragon Net aún no acepta esta versión"* y el
online contra PC sigue funcionando.
