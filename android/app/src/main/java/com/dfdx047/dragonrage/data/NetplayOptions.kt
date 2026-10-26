package com.dfdx047.dragonrage.data

import java.io.File

/** Stored in GamePaths.gameData, next to dragonnet-device.txt (the engine's Port_FileRoot). */
/** Delay: -2 automatic, -1 off, otherwise a fixed frame count. Rollback: 0 off, otherwise its frame limit. */
data class NetplayOptions(val delay: Int = AUTO, val rollback: Int = 8) {
    companion object {
        const val AUTO = -2
        const val OFF = -1
        val DEFAULT = NetplayOptions()

        fun read(root: File): NetplayOptions {
            val lines = runCatching { File(root, "netplay_options.txt").readLines() }.getOrNull() ?: return DEFAULT
            if (lines.size != 2) return DEFAULT
            val delayText = lines[0].removePrefix("delay=")
                .takeIf { lines[0].startsWith("delay=") } ?: return DEFAULT
            val rollText = lines[1].removePrefix("rollback=")
                .takeIf { lines[1].startsWith("rollback=") } ?: return DEFAULT
            val delay = when (delayText) {
                "auto" -> AUTO
                "off" -> OFF
                else -> delayText.toIntOrNull()?.takeIf { it in 0..8 } ?: return DEFAULT
            }
            val rollback = rollText.toIntOrNull()?.takeIf { it == 0 || it in 2..8 && it % 2 == 0 } ?: return DEFAULT
            return NetplayOptions(delay, rollback)
        }

        fun write(root: File, options: NetplayOptions) {
            val validDelay = options.delay == AUTO || options.delay == OFF || options.delay in 0..8
            val validRollback = options.rollback == 0 || options.rollback in 2..8 && options.rollback % 2 == 0
            val safe = if (validDelay && validRollback) options else DEFAULT
            val delayText = when (safe.delay) { AUTO -> "auto"; OFF -> "off"; else -> safe.delay.toString() }
            val file = File(root, "netplay_options.txt")
            val temp = File(root, "netplay_options.txt.tmp")
            temp.writeText("delay=$delayText\nrollback=${safe.rollback}\n")
            if (file.exists() && !file.delete()) error("Could not replace netplay options")
            if (!temp.renameTo(file)) error("Could not save netplay options")
        }
    }
}
