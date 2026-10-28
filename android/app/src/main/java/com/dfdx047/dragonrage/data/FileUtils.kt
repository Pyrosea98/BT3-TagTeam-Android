package com.dfdx047.dragonrage.data

import android.content.ContentResolver
import android.net.Uri
import android.provider.OpenableColumns
import java.io.File
import java.io.IOException
import java.io.InputStream
import java.nio.channels.SeekableByteChannel
import org.apache.commons.compress.archivers.zip.ZipArchiveInputStream
import org.apache.commons.compress.archivers.zip.ZipFile
import kotlin.coroutines.coroutineContext
import kotlinx.coroutines.ensureActive

fun ContentResolver.displayName(uri: Uri): String {
    query(uri, arrayOf(OpenableColumns.DISPLAY_NAME), null, null, null)?.use { c ->
        if (c.moveToFirst()) {
            val i = c.getColumnIndex(OpenableColumns.DISPLAY_NAME)
            if (i >= 0) c.getString(i)?.let { return it }
        }
    }
    return uri.lastPathSegment?.substringAfterLast('/') ?: "file"
}

fun ContentResolver.size(uri: Uri): Long {
    query(uri, arrayOf(OpenableColumns.SIZE), null, null, null)?.use { c ->
        if (c.moveToFirst()) {
            val i = c.getColumnIndex(OpenableColumns.SIZE)
            if (i >= 0 && !c.isNull(i)) return c.getLong(i)
        }
    }
    return -1L
}

/** A name that is safe as a single folder or file name. */
fun safeName(name: String): String =
    name.replace(Regex("[\\\\/:*?\"<>|\\u0000-\\u001F]"), "_").trim().trimStart('.').ifBlank { "item" }.take(80)

fun File.sizeDeep(): Long = if (isFile) length() else walkTopDown().filter { it.isFile }.sumOf { it.length() }

fun File.countFiles(predicate: (File) -> Boolean = { true }): Int =
    walkTopDown().count { it.isFile && predicate(it) }

/** A folder name that does not exist yet in [parent]: "Name", "Name (2)", ... */
fun uniqueChild(parent: File, wanted: String): File {
    var f = File(parent, wanted)
    var n = 2
    while (f.exists()) f = File(parent, "$wanted (${n++})")
    return f
}

/**
 * Unpacks a zip into [dest]. Entries that would land outside [dest] ("zip slip") are refused. [onEntry] sees
 * every file written, relative to [dest]; [onBytes] the number of compressed-stream bytes read so far is not
 * available from a zip read as a stream, so progress is reported in uncompressed bytes written.
 *
 * Read with Commons Compress rather than java.util.zip, which refuses what 7-Zip writes with Deflate64 or BZip2
 * and the stored (uncompressed) entries of a zip written as a stream ("only DEFLATED entries can have EXT
 * descriptor"): all of them came out of the zip tools players use (checked against a real pack, 2026-10-25).
 */
suspend fun unzip(
    input: InputStream,
    dest: File,
    onBytes: (Long) -> Unit = {},
    charset: java.nio.charset.Charset = Charsets.UTF_8,
    onEntry: (String) -> Unit = {},
): Int {
    val canonicalDest = dest.canonicalFile
    var files = 0
    var written = 0L
    val buf = ByteArray(1 shl 16)
    ZipArchiveInputStream(input.buffered(1 shl 16), charset.name(), true, true).use { zip ->
        while (true) {
            coroutineContext.ensureActive()
            val e = zip.nextEntry ?: break
            val name = e.name.replace('\\', '/')
            if (name.startsWith("__MACOSX/") || name.endsWith(".DS_Store")) continue
            val out = File(dest, name).canonicalFile
            if (!out.path.startsWith(canonicalDest.path + File.separator)) throw IOException("Invalid entry in the zip: $name")
            if (e.isDirectory) {
                out.mkdirs()
                continue
            }
            out.parentFile?.mkdirs()
            out.outputStream().use { os ->
                while (true) {
                    val n = zip.read(buf)
                    if (n < 0) break
                    os.write(buf, 0, n)
                    written += n
                    onBytes(written)
                }
            }
            files++
            onEntry(name) // (the entry's own path in the zip, '/'-separated: its last part is the file's name)
        }
    }
    return files
}

/**
 * What a zip's files take unpacked, from its index at the end (sizes the entries themselves may not carry), or
 * null when the source cannot be read at random (a pipe, some cloud files) or has no readable index.
 */
fun zipUnpackedSize(open: () -> SeekableByteChannel?): Long? = runCatching {
    open()?.use { ch ->
        ZipFile.builder().setSeekableByteChannel(ch).get().use { z ->
            var sum = 0L
            for (e in z.entries) if (!e.isDirectory && e.size > 0) sum += e.size
            sum
        }
    }
}.getOrNull()

/** True for the error of a full storage (ENOSPC), as the system words it. */
fun Throwable.isNoSpace(): Boolean =
    generateSequence(this) { it.cause }.any { it.message?.let { m -> m.contains("ENOSPC") || m.contains("No space left", true) } == true }

fun formatBytes(bytes: Long): String {
    if (bytes < 1024) return "$bytes B"
    val units = listOf("KB", "MB", "GB", "TB")
    var v = bytes / 1024.0
    var i = 0
    while (v >= 1024 && i < units.lastIndex) {
        v /= 1024
        i++
    }
    return if (v >= 100) "%.0f %s".format(v, units[i]) else "%.1f %s".format(v, units[i])
}
