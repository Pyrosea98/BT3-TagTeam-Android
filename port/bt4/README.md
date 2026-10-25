# Budokai Tenkaichi 4 in Dragon Rage (`port/bt4`)

Team BT4's mod of Budokai Tenkaichi 3 (SLUS-219.78, Beta 14 Rev2) on Android, next to Budokai Tenkaichi 3, in the same
app: the launcher's welcome screen picks the game; each has its own folder (`bt4/`), image, saves and settings.

The mod changes the game's program, so the BT3 engine (the decompilation) cannot run it. This engine is the mod
**recompiled**: [BT3-Recomp](https://github.com/z3xox/BT3-Recomp) (PS2Recomp's runtime for BT3) with
[dbz-bt4-recomp](https://github.com/recomp-code/dbz-bt4-recomp)'s patches, built for Android, and drawn by **Dragon
Rage's own GS** (`port/src/gs`, unchanged: Vulkan / OpenGL ES, its settings, texture packs, gamepads and the on-screen
controller).

```
Bt4Activity (SDL3) ── libbt4.so
                       ├─ the game: BT3-Recomp's runtime + the mod's generated code, on a thread of its own,
                       │  with no window (host/bt3gl_android.cpp: the headless raylib of its browser build, SDL3 audio)
                       │        │ live GS stream (ps2_gs_live.cpp: GIF packets, registers, VSync; a ring in memory)
                       │        ▼                              ▲ pad (gs_input.c: controller, keyboard, touch)
                       └─ host/bt4_main.cpp on SDL's thread ───┘ → Dragon Rage's GS (Gs_Gif, GsGpu_FrameEnd)
```

It is the browser build's design (`ps2xRuntime/web`), with Dragon Rage's GS in place of Play!'s.

## Files

| | |
|---|---|
| `host/bt4_main.cpp` | `SDL_main`: the log, the runtime's start, the ring reader feeding Dragon Rage's GS, the pad, leaving |
| `host/bt4_shim.c` | what the GS asks of the rest of the BT3 port, answered for this engine (no menus text, online, pacing) |
| `host/bt3gl_android.cpp`, `host/glad_null.cpp` | BT3-Recomp's headless raylib and null GL loader (GPLv3), SDL3 audio |
| `host/android_*` | what bionic lacks: getcontext / makecontext / swapcontext (aarch64), affinity, backtrace, shm |
| `patches/bt3recomp-android.patch` | the runtime's headless path (`PS2X_HEADLESS_HOST`, as `__EMSCRIPTEN__`) and the fibers on Android |
| `CMakeLists.txt` | the runtime, the GS and libbt4.so for Android arm64 |
| `build_bt4.py` | fetches and patches the sources, builds FFmpeg, generates the game's code, builds and installs libbt4.so |

## Building

```
python3 port/tools/android.py                       # the BT3 engine; also the toolchain, SDL3 and the GS shaders
python3 port/bt4/build_bt4.py --data <folder>       # the first time: generates the game's code from your files
python3 port/bt4/build_bt4.py                       # later
cd android && ./gradlew assembleRelease
```

`<folder>` holds the mod's `SLUS_219.78` and `BIN/DBZP.BIN`. After the image is set up in the app, they are in
`Android/data/com.dfdx047.dragonrage/files/bt4/runtime/data` (copy that folder to the PC). The ELF is checked (SHA-256):
the code is generated for Beta 14 Rev2 only. Everything goes to `port/build/bt4/`; nothing of the game is in git.

On the PC: git, cmake, ninja, make, a C/C++ compiler (the recompiler runs there) and Python 3.

## Picture and speed

- The mod draws 16:9 squeezed into the PlayStation 2's 4:3 frame (the community widescreen values are built into its
  program), so the frame is shown stretched to 16:9 (`bt4_aspect`: 0 16:9, 1 4:3, 2 the whole screen; bt4_shim.c).
- 60 fps fights (`bt4_fps60`): BT3-Recomp's mode, the fight at step 1 with its pacing table
  (`games/bt3/fps60_sites.txt`, built into libbt4.so by CMakeLists.txt and written to `runtime/` at start), as the
  mod's Windows and Mac builds offer it. Live from the touch menu.

## The renderer "ARMSX2" (the default since Dragon Rage 0.9.0, `port/bt4/gsx`)

`bt4_renderer` = 1 (the default; 0 is Dragon Rage's renderer, experimental for BT4) draws the game with the ARMSX2 emulator's GS instead of Dragon Rage's: PCSX2's hardware renderer
with ARMSX2's Vulkan / OpenGL ES fixes for Mali and Adreno, reading exactly the live stream Dragon Rage's GS reads
(it is a PCSX2 GS dump's format). It is what dbz-bt4-recomp does on Windows and macOS with `pcsx2-gsrunner` in a
process of its own; here it is a library in the game's process, `libbt4gsx.so`:

- `patches/armsx2-bt4.patch` (on ARMSX2 `90126bdfea`): GSDumpReplayer's live mode (the packets from a callback, no
  frame limiter, ReadFIFO2 answered), gsrunner given the app's window and folders, and a hook for extra CMake targets
  at the end of ARMSX2's Android build;
- `gsx/bt4_gsx.cpp`: reads the ring for GSDumpReplayer, hands the window over (and again after the app comes back
  from the background), runs gsrunner's `main` with `-renderer vulkan|gl -upscale <n> -ini gsx/dbz.ini live.gs`;
- `gsx/bt4gsx.cmake`: the library, built inside ARMSX2's own Android build (all its libraries from its tree);
- `host/bt4_main.cpp`: with `bt4_renderer` = 1 it loads the library, writes `gsx/dbz.ini` (Team BT4's PCSX2 profile:
  CPU CLUT, merged sprites, the texture offset of the resolution, the shape) and keeps the window's events, the pad
  and the menu; if the library is missing it falls back to Dragon Rage's renderer.

`python3 port/bt4/build_bt4.py --armsx2` builds it (the first time: ARMSX2's core and shaderc, long) and puts its
shaders and fonts in the app's assets (`bt4gsx/`), which Bt4Activity copies to `bt4/gsx/resources`.

## Not yet

- Online, mods, cheats and custom drivers (the BT3 engine's).
- GS -> EE downloads are not answered (as in the browser build): the game keeps its own data there.
