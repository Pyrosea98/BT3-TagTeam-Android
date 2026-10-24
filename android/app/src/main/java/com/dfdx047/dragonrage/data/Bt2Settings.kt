package com.dfdx047.dragonrage.data

import java.io.File

/**
 * Budokai Tenkaichi 2's settings: the file its engine reads (<root>/bt2/user/settings, "key = value" lines;
 * bt2/runtime/host/settings.h), of which the app changes only the keys below and keeps every other line as it is
 * (the disc and memory card the engine chose, the controls). The engine refuses unknown keys, so the options that
 * exist only on Android are in a file of their own (<root>/bt2/user/android, read by
 * bt2/runtime/android/android_main.cpp).
 */
class Bt2Settings(paths: GamePaths) {
    private val file = File(paths.bt2Root, "user/settings")
    private val android = File(paths.bt2Root, "user/android")

    /** The engine's keys the app shows, with the engine's defaults. */
    data class Values(
        val superSampling: Int = 1,      // 1, 2, 4, 8, 16 samples per pixel (4 and more: twice the resolution)
        val widescreen: Boolean = false, // the game's own 16:9 patch
        val sixtyFps: Boolean = false,   // pictures between the fights' frames (heavy)
        val showFrameRate: Boolean = false,
        val integerScaling: Boolean = true,
        val hdTextures: Boolean = false,
        val masterVolume: Int = 100,
        val musicVolume: Int = 100,
        val effectsVolume: Int = 100,
        val blendDemote: Boolean = false, // Android: paraLLEl-GS's lighter blending (BT2_GS_HACKS)
        val noMipmaps: Boolean = false,   // Android: paraLLEl-GS without mipmaps
    )

    fun load(): Values {
        val keys = read(file)
        val hacks = read(android)["gs_hacks"].orEmpty().split(',').map { it.trim() }.toSet()
        fun flag(key: String, default: Boolean) = keys[key]?.let { it == "1" } ?: default
        fun number(key: String, default: Int) = keys[key]?.toIntOrNull() ?: default
        val d = Values()
        return Values(
            superSampling = number("super_sampling", d.superSampling),
            widescreen = flag("widescreen", d.widescreen),
            sixtyFps = flag("sixty_fps", d.sixtyFps),
            showFrameRate = flag("show_frame_rate", d.showFrameRate),
            integerScaling = flag("integer_scaling", d.integerScaling),
            hdTextures = flag("hd_textures", d.hdTextures),
            masterVolume = number("master_volume", d.masterVolume),
            musicVolume = number("music_volume", d.musicVolume),
            effectsVolume = number("effects_volume", d.effectsVolume),
            blendDemote = "blend_demote" in hacks,
            noMipmaps = "no_mipmaps" in hacks,
        )
    }

    fun save(v: Values) {
        write(file, mapOf(
            "super_sampling" to v.superSampling.toString(), "widescreen" to bit(v.widescreen), "sixty_fps" to bit(v.sixtyFps),
            "show_frame_rate" to bit(v.showFrameRate), "integer_scaling" to bit(v.integerScaling), "hd_textures" to bit(v.hdTextures),
            "master_volume" to v.masterVolume.toString(), "music_volume" to v.musicVolume.toString(), "effects_volume" to v.effectsVolume.toString(),
        ))
        val hacks = listOfNotNull("blend_demote".takeIf { v.blendDemote }, "no_mipmaps".takeIf { v.noMipmaps })
        write(android, mapOf("gs_hacks" to hacks.joinToString(",")))
    }

    private fun bit(b: Boolean) = if (b) "1" else "0"

    private fun read(f: File): Map<String, String> = runCatching {
        f.readLines().mapNotNull { line ->
            val at = line.indexOf(" = ")
            if (at < 0) null else line.substring(0, at) to line.substring(at + 3)
        }.toMap()
    }.getOrDefault(emptyMap())

    /** The file with [changes] applied: existing lines in their place, new keys added at the end. */
    private fun write(f: File, changes: Map<String, String>) {
        val lines = runCatching { f.readLines() }.getOrDefault(emptyList()).filter { it.isNotBlank() }.toMutableList()
        val left = changes.toMutableMap()
        for (i in lines.indices) {
            val key = lines[i].substringBefore(" = ", "")
            left.remove(key)?.let { lines[i] = "$key = $it" }
        }
        left.forEach { (k, v) -> lines += "$k = $v" }
        f.parentFile?.mkdirs()
        f.writeText(lines.joinToString("\n", postfix = "\n"))
    }
}
