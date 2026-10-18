# Dragon Rage 0.7.3

**El online vuelve a funcionar**: el port de PC publicó la 0.1.37 y el servicio de partidas ahora la exige. Esta
versión es la 0.1.37.

## De la 0.1.37 del port de PC
- Online: una línea en pantalla mientras empieza la partida (*Connecting to your rival…*, luego *Your rival "NOMBRE"
  is here!*); se acabó la pausa antes de la partida que crecía con el ping; el nombre del perfil ya no se borra con la
  Partida rápida.
- Blending: un alfa de origen por encima de 0x80 cuenta como en la PS2 (el destello de la Solar Flare).
- Choques, la opción del Cuadrado: una pulsación puede valer 2,25, el valor por defecto salvo que se elija otro.
- Traducciones de los textos del juego (menús, HUD de combate, diálogos, textos del propio port) desde catálogos en
  `translations/`, que la app copia a la carpeta del juego. Por ahora solo existe el catálogo en inglés; la opción del
  disco europeo del instalador de PC no forma parte de Dragon Rage.

## Correcciones y ayuda
- **Móviles en los que el juego se cerraba al instante** (un Redmi con Snapdragon 8s Gen 3, HyperOS 2: *"cannot map
  the game heap"*): cuando la dirección del heap del juego (la de la PS2) ya está ocupada en el proceso, el heap ahora
  va a otro lugar por debajo de 2 GB, como en Windows.
- **Logs guardados solos**: cuando el juego se cierra con un problema, o un pack de texturas no se instala, la app
  guarda el log en Descargas al momento, dice su nombre, y un botón **Enviar log** abre el menú de compartir. *"Envía
  este archivo al desarrollador."*
- **Página de Texturas**: una tarjeta que explica cómo funcionan los packs (versión estadounidense, el .zip tal como
  vino, qué esperar) con botón de log; tras una pelea con un pack activado en la que no se usó ninguna de sus
  texturas, la app lo avisa (normalmente un pack de otra versión del juego) y guarda el log.
- El aviso *"el juego se cerró"* muestra la causa real (memoria, datos, motor, texturas) antes de un cierre que viene
  después.

## Servidor online
- Ajustes → **Servidor de partidas**: **Oficial** (el del port de PC, PC y Android juntos; el predeterminado) o
  **Comunidad** (un servicio mantenido por jugadores, del fork Tenkaichi3DecompAndroidOnline; allí solo se encuentran
  quienes lo eligieron). La conexión directa (dirección y puerto) no usa servidor.

## Notas de actualización
- Se instala sobre la 0.7.x / 0.6.0 y conserva todo.
- Si la actualización dice *"App no instalada: el paquete entra en conflicto con un paquete existente"*, la app del
  móvil no es una de estas versiones (otra build del código, con otra firma): guarda la carpeta `saves/` de
  `Android/data/com.dfdx047.dragonrage/files/`, desinstala, instala esta y devuelve las partidas.
