package com.dfdx047.dragonrage.engine

import com.dfdx047.dragonrage.data.Bt2Drivers
import com.dfdx047.dragonrage.data.Game
import com.dfdx047.dragonrage.data.GpuDriver
import java.io.File

/**
 * Budokai Tenkaichi 2: BT2-Recomp's runtime built for arm64 as libbt2.so, started by SDL's activity like BT3 is, with
 * the same on-screen controller, in-game menu key and "quit to launcher" ([GameActivity]).
 *
 * It runs in a process of its own (":game_bt2" in the manifest) and never loads libmain.so: BT3's loader reserves
 * the low addresses BT3's engine needs as soon as it is loaded, and SDL keeps its state in statics. BT2's runtime
 * keeps the PS2's memory in an array of its own, so libbt2.so is an ordinary shared library that SDL loads itself
 * and whose SDL_main it calls.
 *
 * What libbt2.so provides (bt2/runtime/android/android_main.cpp, built by bt2/android/build.py):
 *   SDL_main(argc, argv)    argv[1] the game's folder (<root>/bt2: data/BT2.iso, saves/, startup/, user/), argv[2]
 *                           the app's native library folder, argv[3] and argv[4] the chosen GPU driver's folder and
 *                           file ("" for none)
 *   Java_com_dfdx047_dragonrage_engine_TouchBridge_nativeSetPad(buttons, lx, ly, rx, ry)   player 1's pad (PS2 bits)
 *   Java_com_dfdx047_dragonrage_engine_TouchBridge_nativeMenuOpen()                        its own menu is open
 *   Java_com_dfdx047_dragonrage_engine_TouchBridge_nativePlayerTeamSize()                  (-1: unknown)
 * and it calls [quitToApp] when the player leaves the game (back twice).
 */
class Bt2GameActivity : GameActivity() {
    /** BT2's runtime reads its disc image and settings itself and makes its startup inputs from the image
     *  (bt2/runtime/game/startup_inputs.h): nothing of BT3's to put in place. */
    override fun prepareEngine() {}

    override fun getLibraries(): Array<String> = arrayOf("SDL3", Game.BT2.library)

    /** BT2's own controller layout (<root>/bt2/touch_controls.json): what the BT2 tab sets, apart from BT3's. */
    override fun touchFolder(): File = paths.bt2Root

    /** BT2's own GPU driver choice (Bt2ViewModel: <root>/bt2/user/driver), apart from BT3's. */
    override fun gpuDriver(): GpuDriver? = Bt2Drivers.selected(this, paths)

    override fun getArguments(): Array<String> {
        val driver = gpuDriver()
        return arrayOf(paths.bt2Root.path, applicationInfo.nativeLibraryDir, driver?.dir?.path ?: "", driver?.library ?: "")
    }
}
