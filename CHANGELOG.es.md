# Dragon Rage 0.8.0

**Llegó Dragon Net**: online con el port de Nintendo Switch (DBT-NX) en un servicio propio, con Clasificatoria, chat
general y jugadores en línea, que ya no se detiene con cada versión nueva del port de PC. El online contra PC sigue
como opción. Esta versión también trae todo lo de la 0.7.3 (la 0.1.37 del port de PC, la corrección del heap en
algunos Snapdragon, logs guardados solos) y un log para todo lo que hace la app.

## Dragon Net: Android + Switch
- **Dos tipos de online**, elegidos en **Jugar contra**:
  - **Android + Switch (Dragon Net)**, por defecto: Dragon Rage y DBT-NX en su servidor oficial (el del fork
    Tenkaichi3DecompAndroidOnline), para Android × Android y Android × Switch. No depende de las versiones del port de
    PC, así que una versión nueva del PC ya no apaga el online.
  - **PC**: salas y partida rápida contra el port de PC, en su servicio (que pide la versión más nueva del port de PC).
- Elígelo en el juego (Dragon Net Battle → Jugar contra, o **Y** en el mando) o en la app (Ajustes → **Online**). Los
  dos jugadores deben elegir el mismo.
- Juegos de versiones que no pueden pelear juntas nunca se cruzan: el servicio separa cada versión.

## Una nueva pantalla Dragon Net Battle
- El **Dragon Net Battle** del juego (menú principal) abre una pantalla nueva con el estilo de DBT-NX, hecha para
  táctil y mando, en **inglés, portugués y español**.
- **Crear sala**: elige el combate (individual, equipos, DP 10/15/20) y ábrela; aparece un código grande de 6
  caracteres, un toque lo copia.
- **Entrar a sala**: escribe o **pega** el código.
- **Partida rápida**: elige el combate; cuántos esperan en cada cola; la búsqueda termina sola a los tres minutos.
- **Clasificatoria** y **Chat general** (Dragon Net).
- Tu **nombre** en una insignia arriba: tócalo (o Start) para cambiarlo.
- El número de **jugadores en línea** (Android y Switch) abajo.
- Mensajes claros para cada problema: el otro juego en otra versión, nadie buscando, los dos juegos sin poder
  conectarse, el servicio que aún no acepta esta versión.
- Mando: cruceta para elegir, A acepta, B vuelve, Y cambia Android + Switch / PC, Start edita tu nombre.

## Clasificatoria (Elo)
- Un **perfil** creado la primera vez que abres la Clasificatoria: rango (Bronce a Maestro), **Elo**, victorias,
  derrotas y puesto.
- **Buscar rival**, individual o equipos: te empareja con un jugador de tu nivel.
- Peleas Ranked: sin límite de tiempo, velocidad estándar, rollback.
- Al final los dos juegos envían el resultado; **los puntos solo cambian cuando coinciden**. Salir de una pelea Ranked
  antes del final cuenta como derrota.
- **Clasificación**: los mejores; **Actualizar perfil**.

## Chat general
- Un chat para todos en Dragon Net, Android y Switch, en la pantalla online.

## La app (launcher)
- Ajustes → **Online**: Jugar contra (Android + Switch / PC).
- La **tarjeta de online** de Inicio sigue el modo elegido: Dragon Net funcionando, Dragon Net actualizándose (el
  servicio aún no acepta esta versión), PC que necesita actualización, sin respuesta.
- **Cómo funciona el online** ahora es una guía completa, en los tres idiomas: los dos tipos de online, cómo jugar, la
  Clasificatoria, el chat y los jugadores en línea, qué significa cada mensaje y qué hacer, y un botón **Enviar log**.

## Logs para todo
- La app lleva un **diario de lo que hace** (`app_activity.txt` en la carpeta del juego): instalar el juego desde la ISO
  (el archivo, el espacio libre, cada paso, cuánto tardó), packs de texturas, mods, drivers, actualizaciones, la
  comprobación del online, el renderizador, cada inicio del juego, y el **error completo** cuando algo falla.
- **Cualquier fallo guarda un log en Descargas solo** y muestra una ventana con **Enviar log**: la ISO que no se
  instaló, un driver, un mod, una actualización, un pack de texturas, el juego cerrándose mal.
- El juego guarda sus **tres últimas ejecuciones** (`bt3_log.txt`, `.1`, `.2`): un problema visto una ejecución después
  sigue ahí.
- El log exportado lo tiene todo: el dispositivo, el diario de la app, la carpeta del juego (tamaños, packs, mods), los
  ajustes, las tres últimas ejecuciones del juego y el log del sistema.

## De la 0.1.37 del port de PC (en la 0.7.3)
- Online: una línea en pantalla mientras empieza la partida (*Connecting to your rival…*, luego *Your rival "NOMBRE"
  is here!*); se acabó la pausa antes de la partida que crecía con el ping; el nombre del perfil ya no lo borra la
  Partida rápida.
- Blending: un alfa de origen por encima de 0x80 cuenta como en PS2 (el destello de la Solar Flare).
- Choques, la opción del Cuadrado: una pulsación puede valer 2,25, por defecto salvo que se elija otra.
- Traducciones de los textos del juego desde catálogos en `translations/`, copiados por la app a la carpeta del juego
  (por ahora solo inglés).

## Correcciones y ayuda (en la 0.7.3)
- **Teléfonos donde el juego se cerraba al instante** (un Redmi con Snapdragon 8s Gen 3, HyperOS 2: *"cannot map the
  game heap"*): el heap del juego ahora va a otro sitio cuando la dirección del PS2 ya está ocupada.
- **Página de Texturas**: una tarjeta que explica cómo funcionan los packs (versión americana, el .zip tal como vino,
  qué esperar) con botón de log; tras una pelea con un pack activo del que no se usó ninguna textura, la app avisa.
- El aviso *"el juego se cerró"* muestra la causa real (memoria, datos, motor, texturas).
- Changelogs: un archivo por idioma, mostrado por la ventana de actualización de la app.

## Para desarrolladores
- `port/dragonnet`: el código del servicio de Dragon Net (salas, partida rápida, Clasificatoria, chat, jugadores en
  línea; acepta varias versiones a la vez y las separa) con sus pruebas, para el servidor oficial.
- `docs/DRAGON_NET.md` (inglés y español): lo que Android y Switch deben tener igual (versión 5001), el flujo de la
  Clasificatoria, el digest del resultado, el chat y la presencia, y cómo actualizar el servicio.

## Notas de actualización
- Se instala encima de la 0.7.x y la 0.6.0 y conserva todo.
- Dragon Net necesita el servidor ejecutando el `port/dragonnet` de esta versión (hecho junto con este lanzamiento). Si la tarjeta de
  online dice *"Dragon Net se está actualizando"*, elige **Jugar contra: PC** mientras tanto.
- Si la actualización dice *"App no instalada: el paquete entra en conflicto con un paquete existente"*, la app del
  teléfono es otra build del código (otra firma): guarda la carpeta `saves/` de
  `Android/data/com.dfdx047.dragonrage/files/`, desinstala, instala esta y devuelve los saves.
