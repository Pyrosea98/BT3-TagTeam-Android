# Budokai Tenkaichi 4 for Dragon Rage (Android arm64): libbt4.so. Configured by port/bt4/build_bt4.py, which also
# fetches and patches its inputs; see port/bt4/README.md.
#
#   BT3R      BT3-Recomp (z3xox, e22a34b) with dbz-bt4-recomp's patches (bt3recomp-bt4mod-full) and ours
#             (port/bt4/patches/bt3recomp-android.patch), and the game's code generated in ps2xRuntime/src/runner*
#   DR        this repository (Dragon Rage's GS: port/src/gs)
#   DRGEN     port/build/gen/gs (the GS shaders, made by port/tools/android.py)
#   SDL3      the SDL3 of port/tools/android.py (include/, lib/libSDL3.so)
#   FFMPEG    FFmpeg built for Android (include/, lib/*.a)
#
# Like BT3-Recomp's browser build (ps2xRuntime/web/web.cmake): the runtime with no window of its own (the headless
# raylib) sends its GS stream to the host in the same process, here Dragon Rage's GS (host/bt4_main.cpp).
cmake_minimum_required(VERSION 3.21)
project(bt4 C CXX ASM)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(RT "${BT3R}/ps2xRuntime")
set(HOST "${CMAKE_CURRENT_SOURCE_DIR}/host")

# Same numeric contract as BT3-Recomp's builds: no FMA contraction (ARM fuses by default), type punning, wraparound.
set(PS2X_NUMERIC -fno-strict-aliasing -fwrapv -ffp-contract=off)
add_compile_definitions(PS2X_HEADLESS_HOST=1 USE_SSE2NEON)

include(FetchContent)
set(FETCHCONTENT_QUIET TRUE)

# SSE intrinsics of the runtime and of the generated code, on NEON (as BT3-Recomp's ARM builds: sse2neon v1.9.1)
FetchContent_Declare(sse2neon GIT_REPOSITORY https://github.com/DLTcollab/sse2neon.git GIT_TAG v1.9.1 GIT_SHALLOW TRUE)
FetchContent_MakeAvailable(sse2neon)
include_directories(${sse2neon_SOURCE_DIR})

# ---- the headless raylib (no window) and the null GL loader of the runtime's (never run) GL renderer
add_library(bt3gl STATIC "${HOST}/bt3gl_android.cpp" "${HOST}/glad_null.cpp")
target_include_directories(bt3gl PUBLIC "${RT}/third_party/bt3gl/src" "${SDL3}/include")
target_compile_definitions(bt3gl PUBLIC PLATFORM_WEB_HEADLESS)

# ---- ImGui (the runtime's settings overlay keeps its settings; it never draws here)
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG "docking" GIT_SHALLOW TRUE)
FetchContent_GetProperties(imgui)
if(NOT imgui_POPULATED)
    FetchContent_Populate(imgui)
endif()
add_library(imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp" "${imgui_SOURCE_DIR}/imgui_draw.cpp" "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp" "${imgui_SOURCE_DIR}/imgui_demo.cpp" "${imgui_SOURCE_DIR}/backends/imgui_impl_opengl3.cpp")
target_include_directories(imgui PUBLIC "${imgui_SOURCE_DIR}" "${imgui_SOURCE_DIR}/backends")
target_compile_definitions(imgui PUBLIC IMGUI_ENABLE_DOCKING)
# (its OpenGL backend loads GL itself: on Android it is never initialized)
target_compile_definitions(imgui PRIVATE IMGUI_IMPL_OPENGL_ES3)

add_library(ffmpeg INTERFACE)
target_include_directories(ffmpeg INTERFACE "${FFMPEG}/include")
target_link_libraries(ffmpeg INTERFACE "${FFMPEG}/lib/libavformat.a" "${FFMPEG}/lib/libavcodec.a" "${FFMPEG}/lib/libswresample.a"
    "${FFMPEG}/lib/libswscale.a" "${FFMPEG}/lib/libavutil.a")

add_library(ps2x_settings STATIC "${RT}/src/lib/ps2x_settings.cpp")
target_include_directories(ps2x_settings PUBLIC "${RT}/include")
add_subdirectory("${RT}/third_party/rcheevos" rcheevos EXCLUDE_FROM_ALL)

# ---- the runtime: the browser build's list of sources (web/web.cmake), with bionic's missing pieces
set(L "${RT}/src/lib")
add_library(ps2_runtime STATIC
    ${L}/game_overrides.cpp ${L}/bt4mod_overrides.cpp ${L}/ps2x_mods.cpp ${L}/ps2x_dueldump.cpp ${L}/ps2x_net_texdump.cpp
    ${L}/ps2x_duel_fields.cpp ${L}/ps2x_mainmenu.cpp ${L}/ps2x_injected.cpp ${L}/ps2_eeprof.cpp ${L}/ps2_stepcensus.cpp
    ${L}/ps2_seamprobe.cpp ${L}/ps2_seammesh.cpp ${L}/ps2_fiber.cpp ${L}/ps2_gif_arbiter.cpp ${L}/ps2_gs_live.cpp
    ${L}/ps2_audio.cpp ${L}/ps2_audio_vag.cpp ${L}/ps2_host_audio.cpp ${L}/ps2_host_window.cpp ${L}/ui_sdl.cpp
    ${L}/ps2_host_pad.cpp ${L}/ps2_gs_gpu.cpp ${L}/ps2_gs_gpu_renderer.cpp ${L}/ps2_win_timer.cpp ${L}/ps2_gs_memory.cpp
    ${L}/ps2_gs_rasterizer.cpp ${L}/ps2_texreplace.cpp ${L}/ps2_coverage.cpp ${L}/ps2_iop_module.cpp ${L}/ps2_fmv_override.cpp
    ${L}/ps2_netplay.cpp ${L}/ps2x_achieve.cpp ${L}/ps2x_notify.cpp ${L}/ps2x_notify_sound.cpp ${L}/ps2x_perf.cpp
    ${L}/ps2x_memstat.cpp ${L}/ps2_iop.cpp ${L}/ps2_iop_audio.cpp ${L}/ps2_iop_dbcman.cpp ${L}/ps2_iop_sdrdrv.cpp
    ${L}/ps2_iop_cl.cpp ${L}/ps2_memory.cpp ${L}/ps2_pad.cpp ${L}/pad_config.cpp ${L}/ps2_runtime.cpp ${L}/ps2_interp.cpp
    ${L}/ps2_vif1_interpreter.cpp ${L}/ps2_vu1.cpp ${L}/ps2_vu1_native.cpp ${L}/games_database.cpp ${L}/pad_evdev_linux.cpp
    ${RT}/src/gfx/gl/GlApi.cpp ${RT}/src/gfx/gl/GlDevice.cpp ${RT}/src/gfx/gl/GlGfx.cpp ${RT}/src/gfx/gl/gsrl_vendor.cpp
    ${RT}/src/gfx/gl/gsrl.cpp ${RT}/src/gfx/gl_context.cpp ${RT}/src/gfx/gs_resources.cpp ${RT}/src/gfx/gs_rt.cpp
    ${RT}/src/gfx/gs_gl.cpp ${RT}/src/gfx/image_io.cpp ${RT}/src/gfx/video_overlay.cpp
    ${HOST}/android_ucontext.S ${HOST}/android_makecontext.cpp)
file(GLOB KERNEL_SRC_FILES CONFIGURE_DEPENDS "${RT}/src/lib/Kernel/*.cpp" "${RT}/src/lib/Kernel/*/*.cpp")
target_sources(ps2_runtime PRIVATE ${KERNEL_SRC_FILES})
target_include_directories(ps2_runtime PUBLIC "${RT}/include" "${RT}/src/lib/Kernel" "${RT}/src" "${HOST}")
target_link_libraries(ps2_runtime PUBLIC bt3gl ffmpeg ps2x_settings rcheevos)
target_link_libraries(ps2_runtime PRIVATE imgui)
target_compile_options(ps2_runtime PRIVATE -O3 ${PS2X_NUMERIC} $<$<COMPILE_LANGUAGE:C,CXX>:-include${HOST}/android_compat.h>)

# ---- Dragon Rage's GS, the renderer of Budokai Tenkaichi 3 (port/src/gs), compiled as android.py compiles it
set(GS "${DR}/port/src/gs")
file(GLOB ASTCENC_SRC "${DR}/port/third_party/astcenc/*.cpp")
add_library(drgs STATIC
    ${GS}/gs_core.c ${GS}/gs_draw.c ${GS}/gs_gpu.c ${GS}/gs_gl.c ${GS}/gs_vu1.c ${GS}/gs_texpack.c ${GS}/gs_input.c
    ${DR}/port/src/plat_settings.c ${HOST}/bt4_shim.c ${GS}/gs_astc.cpp ${ASTCENC_SRC})
target_include_directories(drgs PRIVATE "${GS}" "${DRGEN}" "${DR}/port/src" "${SDL3}/include")
target_compile_definitions(drgs PRIVATE BT3_ANDROID=1)
set_source_files_properties(${GS}/gs_core.c ${GS}/gs_draw.c ${GS}/gs_gpu.c ${GS}/gs_gl.c ${GS}/gs_vu1.c ${GS}/gs_texpack.c
    ${GS}/gs_input.c ${DR}/port/src/plat_settings.c ${HOST}/bt4_shim.c PROPERTIES COMPILE_OPTIONS "-std=gnu99;-O2;-fno-strict-aliasing;-w")
set_source_files_properties(${GS}/gs_astc.cpp PROPERTIES COMPILE_OPTIONS "-std=c++17;-O2;-fno-exceptions;-fno-rtti;-w")
set_source_files_properties(${ASTCENC_SRC} PROPERTIES COMPILE_OPTIONS
    "-std=c++17;-O2;-w;-DASTCENC_NEON=1;-DASTCENC_SSE=0;-DASTCENC_AVX=0;-DASTCENC_POPCNT=0;-DASTCENC_F16C=0;-DASTCENC_SVE=0")

# ---- libbt4.so: the game's generated code, the IOP modules, the runtime's main() and the host
file(GLOB RUNNER_SRC_FILES CONFIGURE_DEPENDS "${RT}/src/runner/*.cpp")
file(GLOB RUNNER_OVERLAY_SRC_FILES CONFIGURE_DEPENDS "${RT}/src/runner_overlay/*.cpp")
list(REMOVE_ITEM RUNNER_SRC_FILES "${RT}/src/runner/main.cpp")
file(GLOB IOP_NATIVE_SRC_FILES CONFIGURE_DEPENDS "${RT}/src/iop_native/*/*.cpp")
if(NOT RUNNER_SRC_FILES)
    message(WARNING "No generated game code in ${RT}/src/runner: build_bt4.py generates it from the mod's ELF first.")
endif()
add_library(bt4 SHARED
    ${RUNNER_SRC_FILES} ${RUNNER_OVERLAY_SRC_FILES} ${IOP_NATIVE_SRC_FILES}
    ${RT}/src/main.cpp ${RT}/src/lib/ps2_settings_overlay.cpp ${RT}/src/gfx/ps2x_ui.cpp
    ${HOST}/bt4_main.cpp)
target_include_directories(bt4 PRIVATE "${RT}/src/lib" "${SDL3}/include")
set_source_files_properties(${RT}/src/main.cpp PROPERTIES COMPILE_DEFINITIONS "main=ps2x_runtime_main")
set(PS2X_RUNNER_OPT "-O2" CACHE STRING "Optimisation of the generated game code")
target_compile_options(bt4 PRIVATE ${PS2X_RUNNER_OPT} ${PS2X_NUMERIC} $<$<COMPILE_LANGUAGE:C,CXX>:-include${HOST}/android_compat.h>)
set_target_properties(bt4 PROPERTIES UNITY_BUILD ON UNITY_BUILD_BATCH_SIZE 16)
set_source_files_properties(${RT}/src/main.cpp ${RT}/src/lib/ps2_settings_overlay.cpp ${RT}/src/gfx/ps2x_ui.cpp ${HOST}/bt4_main.cpp
    PROPERTIES SKIP_UNITY_BUILD_INCLUSION ON)
file(GLOB _ps2x_register_srcs CONFIGURE_DEPENDS "${RT}/src/runner/register_functions.cpp" "${RT}/src/runner_overlay/overlay_register.cpp"
    "${RT}/src/iop_native/*/register_functions.cpp")
if(_ps2x_register_srcs)
    set_source_files_properties(${_ps2x_register_srcs} PROPERTIES COMPILE_OPTIONS "-O0" SKIP_UNITY_BUILD_INCLUSION ON)
endif()
target_link_libraries(bt4 PRIVATE
    -Wl,--whole-archive drgs -Wl,--no-whole-archive
    ps2_runtime imgui
    "${SDL3}/lib/libSDL3.so" GLESv3 EGL log android z m dl)
target_link_options(bt4 PRIVATE -Wl,--gc-sections -Wl,-z,max-page-size=16384 -static-libstdc++)
