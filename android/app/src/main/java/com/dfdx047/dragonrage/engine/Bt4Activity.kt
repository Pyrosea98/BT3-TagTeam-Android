package com.dfdx047.dragonrage.engine

import android.os.Bundle
import android.os.ParcelFileDescriptor
import com.dfdx047.dragonrage.data.Bt4Installer
import com.dfdx047.dragonrage.data.Game
import com.dfdx047.dragonrage.data.GamePaths

/**
 * Budokai Tenkaichi 4 (Team BT4's mod): the same game screen as Budokai Tenkaichi 3's (GameActivity: renderer
 * choice, language, orientation, on-screen controller, back key), with its own folder (bt4/) and its own engine,
 * libbt4.so: the mod's code recompiled with PS2Recomp, drawn by Dragon Rage's GS. Its own process (":bt4").
 *
 * The disc image is read where it is: opened here and handed to the engine as /proc/self/fd/<n>.
 */
class Bt4Activity : GameActivity() {
    private var image: ParcelFileDescriptor? = null

    override fun gamePaths(): GamePaths = GamePaths(this, Game.BT4)

    override val game: Game get() = Game.BT4

    override fun installDataList() {} // (the BT3 engine's data lists: not this engine's)

    override fun onCreate(savedInstanceState: Bundle?) {
        image = runCatching { Bt4Installer(this, GamePaths(this, Game.BT4), contentResolver).openImage() }.getOrNull()
        super.onCreate(savedInstanceState)
    }

    override fun onDestroy() {
        super.onDestroy()
        runCatching { image?.close() }
        image = null
    }

    override fun getLibraries(): Array<String> = arrayOf("SDL3", "bt4")

    /** The game folder, the app's native libraries and the disc image ("" when it could not be opened). */
    override fun getArguments(): Array<String> =
        arrayOf(paths.root.path, applicationInfo.nativeLibraryDir, image?.let { "/proc/self/fd/${it.fd}" } ?: "")
}
