# Dragon Rage 0.9.0

**La versión 0.1.38.7 del port de PC, Budokai Tenkaichi 4 dibujado por el renderizador de ARMSX2 y peleas más ligeras
en GPUs Mali.** Todo medido en un RG Cube (Mali-G57) y un Retroid Pocket 5 (Adreno 650), con una pelea grabada y
comparación de imagen píxel a píxel.

## Más rápido
- **Resolución interna 2x o más**: las pasadas de dibujo que no usan profundidad corren sin ella, las sombras planas
  comparten una sola limpieza, y el brillo de los personajes se dibuja a la resolución de PlayStation 2 (del PC). En el
  RG Cube, la pelea en Vulkan 2x pasó de cerca del 76% al **95%** de la velocidad (OpenGL 2x: del 92% al 98%); en el
  Retroid Pocket 5, Vulkan 4x pasó del 71% al 84%.
- **OpenGL en Adreno**: buffers persistentes también en Adreno. En el Retroid Pocket 5, OpenGL 2x tardaba unos 4
  segundos por cuadro y ahora va a 30 estables.
- **Adreno**: el stencil de las sombras planas queda apagado por defecto, más rápido en estos chips.
- **Perfiles de GPU** medidos para Mali-G57 MC4 y Adreno 650: el launcher sugiere la resolución que mantiene la pelea a
  30.
- **Modo Lite** (Ajustes > Rendimiento; lo ofrece el perfil de GPU en chips del nivel del Mali-G52): el renderizador en
  su propio hilo, y el brillo y el reflejo del cielo dejados fuera. En un Mali-G52 MC1 simulado, la pelea a 1x pasó del
  53% al 77% de la velocidad.
- **Del PC**: menos tirones la primera vez que aparece un especial o una transformación (las texturas nuevas se
  preparan mucho más rápido, en varios núcleos), buffers de dibujo preparados antes del primer gran efecto de la
  pelea, y el contorno de los personajes dibujado solo donde hay línea.
- **Packs de texturas**: los archivos se leen en segundo plano y quedan en memoria, y un cuadro recibe como máximo
  4 MB de ellos. Menos tirones con packs grandes (del PC).

## Imagen (del PC)
- **Brillo de los Super Saiyajin** correcto en cualquier resolución interna: antes salía como una copia nítida y
  desplazada del personaje.
- **Desenfoque de la distancia** como en la consola a resolución alta (las hojas ya no aparecen como cuadrados).
- **Contornos** con el mismo grosor en cualquier proporción de pantalla.
- **La espalda de la camisa de Videl** sin la línea discontinua.
- **Pantalla blanca** con el tercer traje de Gohan adolescente en Namek corregida.
- Una textura escrita en formato de 24 bits o de 4/8 bits por píxel ya no conserva contenido viejo en sus últimas
  filas.

## Nuevo: Budokai Tenkaichi 4
- **El mod de Team BT4 en la misma app**, junto al BT3: personajes, formas y escenarios nuevos. En la pantalla de
  inicio del launcher eliges el juego; cada uno tiene su imagen, partidas guardadas y ajustes. Necesitas tu imagen del
  mod **Beta 14 Rev2 (SLUS-219.78)**: el juego está recompilado para esa versión (el motor es el mod recompilado, de
  BT3-Recomp).
- **Renderizador de ARMSX2 por defecto**: el del emulador de PlayStation 2 (el de PCSX2, con las correcciones de ARMSX2
  para Mali y Adreno), la imagen más fiel del mod. El de Dragon Rage queda como opción **experimental** (Ajustes y menú
  del juego, con una línea que explica la diferencia). Con él, el escenario muy lejano todavía puede salir con
  bloques de ruido de colores; el de ARMSX2 lo dibuja bien.
- **Shaders compilados de antemano**: el APK ya trae los shaders de ARMSX2 compilados en un Mali y en un Adreno. La
  primera imagen aparece en ~2,5 s en lugar de ~4,5 s, sin los pequeños tirones de compilación en menús y peleas.
- **16:9** (como lo hizo Team BT4), 4:3 o pantalla completa; **drivers personalizados** también en el BT4; **medidor
  de FPS**.
- **Peleas a 60 fps (experimental)**: cuando el dispositivo no las aguanta (menos de 57 actualizaciones por segundo
  durante 3 s en una pelea), el juego vuelve solo a 30 y avisa: "Este equipo no aguanta 60 fps en las peleas de BT4;
  volviendo a 30". En los dispositivos medidos todavía no las aguanta: el RG Cube hace 30 y el Retroid Pocket 5
  unas 36 a 41.
- La imagen se puede elegir desde Descargas y sigue abriendo después de reiniciar el dispositivo.

## Packs de texturas
- **Ahora funcionan en Android**: el juego buscaba los packs en una carpeta interna de la app, así que un pack
  importado nunca se usaba ("se instaló y nada cambió"). Ahora se usan en **BT3** y **BT4** (con los dos
  renderizadores). Medido con el pack HD de SLUS-21678 (18.704 texturas, 10,3 GB): unas 3.000 texturas cambiadas en
  la pelea grabada, a 30 fps estables en el RG Cube y el Retroid Pocket 5.
- **Abren más zips**: los hechos en 7-Zip con Deflate64 o BZip2 y los escritos como flujo (sin compresión, con
  descriptor) daban "una compresión que la app no puede leer".
- **Espacio**: un pack que no cabe se rechaza antes de extraerlo, con su tamaño y el espacio libre ("No hay espacio
  suficiente: este pack ocupa 10,3 GB descomprimido y el dispositivo tiene 7,8 GB libres"), en vez de extraer durante
  más de un minuto y fallar con un error del sistema en inglés. Se borra lo que dejaba una importación interrumpida.
- El selector de archivos también muestra .rar y .7z, y la app explica cómo convertirlos a .zip.

## Online
- **Servidor ocupado**: cuando el servidor gratuito llega a su límite diario (HTTP 429, error 1027 de Cloudflare), el
  juego deja de preguntar un rato (30 s, duplicando hasta 10 min) en lugar de insistir, pasa al servidor de reserva, y
  el launcher muestra **"Servidor ocupado (límite diario), inténtalo más tarde"**. Jugar sin conexión no cambia.
- **Menos peticiones**: la cola de la Clasificatoria solo se consulta con su pantalla abierta; la búsqueda pregunta
  cada 4 s en el primer medio minuto, luego cada 8 s y, pasados dos minutos, cada 15 s; un resultado que el rival aún
  no confirmó se reenvía en 5 s, duplicando hasta un minuto (antes: en cada ronda).
- **Del PC** (vale para Dragon Net y contra PC): sonidos del rival que se perdían con el rollback (Z-counter, vanish,
  teletransporte), el sonido de cargar ki que quedaba toda la pelea, la Solar Flare cegando como sin conexión, voces
  japonesas cambiadas por frases en inglés, y una desincronización es un aviso en lugar de terminar la partida. Dos
  juegos con datos distintos (una ISO modificada contra la original) ya no se emparejan.
- El log de la app registra el motivo cuando el launcher no puede hablar con el servicio online.

## Controles y launcher
- **Pantalla completa**: en el Retroid Pocket 5 (y en dispositivos con barra de navegación) el juego a veces empezaba
  en un área más pequeña (1920x972) que el sistema estiraba a la pantalla, todo un 11% más alto y franjas negras a los
  lados en 16:9. Ahora la imagen ocupa toda la pantalla desde el inicio, en BT3 y BT4 (10 de 10 inicios bien; antes,
  3 de 8 mal).
- **Control clásico en pantalla cuadrada** (RG Cube 720x720): L1/L2 y R1/R2 ya no se superponen, y el rombo de botones
  y la cruceta quedan dentro de la pantalla. En 16:9 no cambia nada.
- **START + SELECT** también cierra el menú del juego, no solo lo abre.
- El control táctil queda oculto al abrir cuando hay un mando con stick conectado.
- En la pestaña Drivers, toda la fila elige el driver.
- La barra lateral del launcher se desplaza (las nueve páginas del BT3 en pantallas 1080p).
- Cuando Vulkan no arranca (por ejemplo con el driver MaliSX2 en la Mali-G57), el juego pasa a OpenGL en lugar de
  quedarse con la pantalla negra.
- ADPF y modo de rendimiento sostenido en los dispositivos que lo tienen, para que el sistema no baje el reloj en
  medio de la pelea.

## Notas de actualización
- Se instala sobre la 0.8.x y conserva partidas guardadas, ajustes y las ISO importadas.
- Online: el modo PC habla con el servidor oficial de la versión 0.1.38; Dragon Net sigue jugando con Switch (DBT-NX).
