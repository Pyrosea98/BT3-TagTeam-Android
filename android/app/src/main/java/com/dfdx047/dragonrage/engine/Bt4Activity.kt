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
        runCatching { copyGsxResources() }.onFailure { android.util.Log.w("bt4", "ARMSX2 renderer resources", it) }
        super.onCreate(savedInstanceState)
    }

    /** ARMSX2's renderer (libbt4gsx.so, when this build has it) reads its shaders and fonts from files: the APK's
     *  assets/bt4gsx/resources go to bt4/gsx/resources, again whenever the APK's copy is another version. */
    private fun copyGsxResources() {
        val names = assets.list("bt4gsx") ?: return
        if ("version.txt" !in names) return
        val want = assets.open("bt4gsx/version.txt").use { it.readBytes().decodeToString().trim() }
        val dst = java.io.File(GamePaths(this, Game.BT4).root, "gsx") // (before GameActivity's onCreate sets `paths`)
        val mark = java.io.File(dst, "version.txt")
        if (mark.exists() && mark.readText().trim() == want) return
        java.io.File(dst, "resources").deleteRecursively()
        fun copy(from: String, to: java.io.File) {
            val kids = assets.list(from).orEmpty()
            if (kids.isEmpty()) {
                to.parentFile?.mkdirs()
                assets.open(from).use { i -> to.outputStream().use { o -> i.copyTo(o) } }
            } else {
                to.mkdirs()
                kids.forEach { copy("$from/$it", java.io.File(to, it)) }
            }
        }
        copy("bt4gsx/resources", java.io.File(dst, "resources"))
        mark.writeText(want + "\n")
    }

    override fun onDestroy() {
        super.onDestroy()
        runCatching { image?.close() }
        image = null
    }

    override fun getLibraries(): Array<String> = arrayOf("SDL3", "bt4")

    /** The game folder, the app's native libraries, the disc image ("" when it could not be opened) and the chosen
     *  custom GPU driver's folder and library ("" for the system's; ARMSX2's renderer loads it). */
    override fun getArguments(): Array<String> {
        val driver = runCatching { com.dfdx047.dragonrage.data.GpuDrivers(this, contentResolver).selected() }.getOrNull()
        return arrayOf(
            paths.root.path, applicationInfo.nativeLibraryDir, image?.let { "/proc/self/fd/${it.fd}" } ?: "",
            driver?.dir?.path ?: "", driver?.library ?: "",
        )
    }
}
