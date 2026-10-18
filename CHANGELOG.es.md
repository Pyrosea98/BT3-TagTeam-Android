# Dragon Rage 0.8.0

**Dragon Net**: online con el port de Nintendo Switch (DBT-NX) en un servicio propio, que ya no se detiene con cada
nueva versión del port de PC. El online contra PC sigue como opción.

## Dragon Net Battle, una nueva pantalla de online
- El **Dragon Net Battle** del juego (menú principal) abre una pantalla nueva con el estilo de DBT-NX, para táctil y
  mando, en inglés, portugués y español: **Crear sala**, **Entrar a sala**, **Partida rápida**, **Clasificatoria** y
  **Chat general**, con el número de jugadores en línea.
- **Jugar contra**: **Android + Switch** (Dragon Net, por defecto) o **PC** (el servicio del port de PC, que pide su
  versión más nueva). También en Ajustes → Online. Clasificatoria y chat son de Dragon Net.
- **Clasificatoria**: perfil con rango, Elo, victorias, derrotas y puesto; búsqueda individual o por equipos según tu
  nivel; los mejores de la clasificación. Los puntos cambian cuando los dos juegos envían el mismo resultado; salir de
  una pelea Ranked antes del final cuenta como derrota.
- Las salas muestran un código grande para copiar; Entrar a sala acepta un código pegado. Peleas Ranked y rápidas: sin
  límite de tiempo, velocidad estándar, rollback.
- Mando: cruceta para elegir, A acepta, B vuelve, Y cambia Android + Switch / PC, Start edita tu nombre.

## Por dentro
- Dragon Net usa el netcode de la 0.1.37 del port de PC con la versión **5001**; el servicio separa cada versión, así
  que juegos que no pueden jugar juntos nunca se cruzan (`docs/DRAGON_NET.md`).
- `port/dragonnet`: el servicio (el Worker de DBT-NX, aceptando varias versiones a la vez) y sus pruebas.
- La tarjeta de online de la app sigue el modo elegido y avisa cuando el servicio Dragon Net aún no acepta esta versión.
