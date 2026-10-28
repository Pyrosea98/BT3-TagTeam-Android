package com.dfdx047.dragonrage.data

import android.content.ContentResolver
import android.net.Uri
import android.provider.OpenableColumns
import android.util.Log
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

/**
 * The app's own diary: one line for everything the launcher does (installing the game from an ISO, texture packs,
 * mods, drivers, updates, the online check, starting the game) and the whole error when something fails. Kept in
 * app_activity.txt in the game folder (the newest 3000 lines), so it is still there after the app was closed, and
 * put in every exported log (LogExport). Also written to logcat with the tag "DragonRage".
 */
object ActivityLog {
    private const val TAG = "DragonRage"
    private const val KEEP = 3000
    private var file: File? = null
    private val stamp = SimpleDateFormat("yyyy-MM-dd HH:mm:ss", Locale.ROOT)
    private var writes = 0

    fun init(paths: GamePaths) {
        file = File(paths.root, "app_activity.txt")
    }

    fun file(): File? = file

    fun info(area: String, text: String) = write("I", area, text)

    fun warn(area: String, text: String) = write("W", area, text)

    fun fail(area: String, text: String, e: Throwable? = null) {
        write("E", area, if (e == null) text else "$text: ${e.javaClass.name}: ${e.message}")
        if (e != null) write("E", area, Log.getStackTraceString(e).trimEnd().lines().take(40).joinToString("\n    ", "    "))
    }

    /** A picked file's name and size as the system gives them ("name.iso, 4.1 GB"), for the log. */
    fun describe(resolver: ContentResolver, uri: Uri): String = runCatching {
        resolver.query(uri, arrayOf(OpenableColumns.DISPLAY_NAME, OpenableColumns.SIZE), null, null, null)?.use { c ->
            if (!c.moveToFirst()) return@use null
            val name = c.getString(0) ?: "?"
            val size = if (c.isNull(1)) -1L else c.getLong(1)
            "\"$name\", " + (if (size >= 0) "$size bytes" else "size unknown") + ", ${uri.authority}"
        }
    }.getOrNull() ?: uri.toString()

    @Synchronized
    private fun write(level: String, area: String, text: String) {
        when (level) {
            "E" -> Log.e(TAG, "[$area] $text")
            "W" -> Log.w(TAG, "[$area] $text")
            else -> Log.i(TAG, "[$area] $text")
        }
        val f = file ?: return
        runCatching {
            f.appendText("${stamp.format(Date())} $level [$area] $text\n")
            if (++writes % 200 == 0 && f.length() > 600_000) {
                val lines = f.readLines()
                if (lines.size > KEEP) f.writeText(lines.takeLast(KEEP).joinToString("\n", postfix = "\n"))
            }
        }
    }

    /** The newest lines, for an exported log. */
    fun tail(lines: Int): List<String> = runCatching { file?.readLines()?.takeLast(lines) }.getOrNull() ?: emptyList()
}
