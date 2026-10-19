# Dragon Rage 0.8.1

**Dragon Net en tres servidores**, para que el online siga funcionando cuando uno de ellos cae o gasta las
solicitudes gratuitas del día. Por Ariel R Rodrigues (pull request #3).

## Dragon Net
- **Servidor A** (el principal, `mitigo-rooms`): salas, partida rápida y jugadores en línea.
- **Servidor B** (el de reserva, el de DBT-NX): cuando el servidor A falla tres veces seguidas, el juego pasa solo al B
  hasta las 00:00 UTC y luego vuelve al A. Dos jugadores tienen que estar en el mismo servidor para encontrarse: si la
  sala no aparece, inténtalo de nuevo un poco más tarde.
- **Servidor C** (`bt-ranked`): la **Clasificatoria**, sus búsquedas y sus salas.
- Se pueden poner otros servidores con archivos de una línea en la carpeta del juego: `dragonnet_url.txt` (A),
  `dragonnet_url_b.txt` (B), `dragonnet_ranked_url.txt` (C).
- **Chat general desactivado por ahora**: aparece apagado en el menú de Dragon Net Battle y vuelve en una próxima
  versión.

## Menos solicitudes a los servidores
Los servidores gratuitos tienen un límite de solicitudes por día, así que el juego pregunta menos veces:
- los jugadores en línea cada 5 minutos (antes 90 segundos);
- el perfil de la Clasificatoria cada minuto (antes 15 segundos), y la búsqueda Ranked cada 4 segundos (antes 2);
- una sala esperando al rival cada 3 segundos el primer minuto y cada 6 después (antes 2 y 4); el código de sala sigue
  durando unos diez minutos;
- las colas de partida rápida (modo PC) cada 90 segundos (antes 30).

## Otros
- La guía de online y los ajustes de la app ya no mencionan el chat.
- Se quitó un APK de prueba que había entrado en el código por error.

## Notas de actualización
- Se instala encima de la 0.8.0 (y la 0.7.x) y conserva todo.
- Todo lo de la 0.8.0 sigue: la pantalla Dragon Net Battle, la Clasificatoria, el online contra PC, los logs para todo
  y la guía de online en los tres idiomas.
