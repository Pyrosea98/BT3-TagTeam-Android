package com.dfdx047.dragonrage.data

import android.content.ContentResolver
import android.content.Context
import com.dfdx047.dragonrage.R
import android.net.Uri
import android.os.StatFs
import java.io.File
import java.io.FileInputStream
import java.io.IOException
import java.nio.ByteBuffer
import java.nio.ByteOrder
import java.security.MessageDigest
import kotlin.coroutines.coroutineContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.withContext

sealed interface InstallState {
    data object NotInstalled : InstallState
    data class Working(val step: String, val progress: Float, val detail: String) : InstallState
    data class Installed(val files: Int, val bytes: Long, val modified: Boolean = false) : InstallState
    data class Failed(val message: String) : InstallState
}

/**
 * The PC port's setup (port/setup/native.cpp, steps 1 and 2) for Android: checks that the image is the
 * unmodified USA release and unpacks the game's data in the layout of port/tools/extract_disc.py:
 *
 *   gamedata/disc/SLUS_216.78, gamedata/disc/BIN/..., gamedata/disc/DATA/... (the AFS archives whole)
 *
 * (The PC setup also splits each archive into one file per entry, gamedata/<name>/00000.bin ...; the engine reads
 * either. Loose files and mods/ still win over the archive.)
 *
 * The image is read where it is (through the document the user picked); nothing of it is kept except the
 * unpacked files.
 */
class GameDataInstaller(
    private val context: Context,
    private val paths: GamePaths,
    private val resolver: ContentResolver,
    private val checkSums: Boolean = true, // (off only in tests with a synthetic image)
) {

    class Stop(message: String) : Exception(message)

    fun currentState(): InstallState {
        if (!paths.installedMarker.exists()) return InstallState.NotInstalled
        val lines = paths.installedMarker.readText().lines()
        val files = lines.firstOrNull { it.startsWith("files=") }?.substringAfter('=')?.toIntOrNull() ?: 0
        val bytes = lines.firstOrNull { it.startsWith("bytes=") }?.substringAfter('=')?.toLongOrNull() ?: 0L
        val modified = lines.any { it == "modified=1" }
        return InstallState.Installed(files, bytes, modified)
    }

    suspend fun install(uri: Uri, report: (InstallState.Working) -> Unit): InstallState.Installed = withContext(Dispatchers.IO) {
        val pfd = resolver.openFileDescriptor(uri, "r") ?: throw Stop(context.getString(R.string.err_open_file))
        pfd.use {
            val channel = FileInputStream(pfd.fileDescriptor).channel
            val iso = try {
                IsoImage(channel)
            } catch (e: IOException) {
                throw Stop(notIsoMessage(context, channel))
            }
            iso.use {
                report(InstallState.Working(context.getString(R.string.step_checking), 0f, context.getString(R.string.detail_finding)))
                val original = verify(iso, report)
                if (!original) compatible(iso)
                extract(iso, report, modified = !original)
            }
        }
    }

    /** True for the unmodified USA release. A modified image (a translation, a mod such as "Tenkaichi 4") is
     *  installed too: its files are used as they are, but the engine runs the original program, so changes a mod made
     *  to the game's code (not to its data) do not apply. */
    private suspend fun verify(iso: IsoImage, report: (InstallState.Working) -> Unit): Boolean {
        val slus = executable(iso)
        val dbzp = iso.find("BIN/DBZP.BIN")
        if (slus == null || dbzp == null) {
            // another release: say which (SLES = Europe, SLPS / SLPM = Japan, SLKA = Korea)
            val serial = iso.files.map { it.path }.firstOrNull { Regex("^S[LC][A-Z]{2}_\\d{3}\\.\\d{2}$", RegexOption.IGNORE_CASE).matches(it) }
            throw Stop(context.getString(R.string.err_not_usa, serial?.replace('_', '-')?.replace(".", "") ?: "?"))
        }
        report(InstallState.Working(context.getString(R.string.step_checking), 0.5f, context.getString(R.string.detail_checksums)))
        coroutineContext.ensureActive()
        val elf = iso.read(slus.offset, slus.size.toInt())
        val rom = ByteArray((ROM_END - ROM_BASE).toInt())
        if (elf.size > 0x34 && elf[0] == 0x7F.toByte() && elf[1] == 'E'.code.toByte()) {
            val b = ByteBuffer.wrap(elf).order(ByteOrder.LITTLE_ENDIAN)
            val shoff = b.getInt(0x20).toLong() and 0xFFFFFFFFL
            val shentsize = b.getShort(0x2E).toInt() and 0xFFFF
            val shnum = b.getShort(0x30).toInt() and 0xFFFF
            for (i in 0 until shnum) {
                val at = shoff + i.toLong() * shentsize
                if (at + 24 > elf.size) break
                val p = at.toInt()
                val type = b.getInt(p + 4).toLong() and 0xFFFFFFFFL
                val flags = b.getInt(p + 8).toLong() and 0xFFFFFFFFL
                val addr = b.getInt(p + 12).toLong() and 0xFFFFFFFFL
                val off = b.getInt(p + 16).toLong() and 0xFFFFFFFFL
                val size = b.getInt(p + 20).toLong() and 0xFFFFFFFFL
                // allocated, not NOBITS, inside the flat image, inside the file
                if (flags and 2L != 0L && type != 8L && size != 0L && addr >= ROM_BASE && addr + size <= ROM_END && off + size <= elf.size) {
                    System.arraycopy(elf, off.toInt(), rom, (addr - ROM_BASE).toInt(), size.toInt())
                }
            }
        }
        val menu = iso.read(dbzp.offset, dbzp.size.toInt())
        return !checkSums || (sha1(rom) == ROM_SHA1 && sha1(menu) == DBZP_SHA1)
    }

    /**
     * A modified image (a translation, a mod) is only usable if the engine finds its data tables where the original has
     * them: the engine copies them from SLUS_216.78 and BIN/DBZP.BIN by the offsets in its list (libbt3.so.dat, an
     * asset). A menu program shorter than the list needs stopped the game at its start ("damaged file"): such an
     * image is turned away here, with the reason.
     */
    private fun compatible(iso: IsoImage) {
        val dbzp = iso.find("BIN/DBZP.BIN") ?: return
        val list = runCatching { context.assets.open("libbt3.so.dat").use { it.readBytes() } }.getOrNull() ?: return
        if (list.size < 16 || String(list, 0, 4, Charsets.US_ASCII) != "BT3D") return
        val b = ByteBuffer.wrap(list).order(ByteOrder.LITTLE_ENDIAN)
        val count = b.getInt(4)
        var need = 0L
        for (i in 0 until count) {
            val at = 16 + i * 16
            if (at + 16 > list.size) break
            if (b.getInt(at + 4) == 1) need = maxOf(need, (b.getInt(at + 8).toLong() and 0xFFFFFFFFL) + (b.getInt(at + 12).toLong() and 0xFFFFFFFFL))
        }
        if (dbzp.size < need) throw Stop(context.getString(R.string.err_mod_incompatible))
    }

    private suspend fun extract(iso: IsoImage, report: (InstallState.Working) -> Unit, modified: Boolean): InstallState.Installed {
        val exe = executable(iso)?.path?.uppercase()
        val take = iso.files.filter {
            val u = it.path.uppercase()
            u == exe || u.startsWith("BIN/") || u.startsWith("DATA/")
        }
        val total = take.sumOf { it.size }
        val free = StatFs(paths.root.path).availableBytes
        if (free < total + 256L * 1024 * 1024) {
            throw Stop(context.getString(R.string.err_space, formatBytes(total), formatBytes(free)))
        }
        // A fresh start: whatever an earlier, interrupted run left is replaced (mods stay).
        paths.installedMarker.delete()
        paths.gameData.listFiles()?.filter { it.name != "mods" }?.forEach { it.deleteRecursively() }
        paths.gameData.mkdirs()

        val buf = ByteBuffer.allocateDirect(8 shl 20)
        var done = 0L
        var files = 0
        var lastPercent = -1
        fun progress(n: Long) {
            done += n
            val percent = if (total > 0) (done * 100 / total).toInt() else 0
            if (percent != lastPercent) {
                lastPercent = percent
                report(InstallState.Working(context.getString(R.string.step_extracting), done.toFloat() / total, context.getString(R.string.detail_extract, percent, files)))
            }
        }

        // Every file is copied as it is, the AFS archives too: the engine reads their entries through the
        // archive's table (port/src/plat_fcache.c). Unpacking them into ~20 000 small files, as the PC setup does,
        // is what made the install take so long on Android's shared storage.
        for (f in take) {
            coroutineContext.ensureActive()
            val u = f.path.uppercase()
            // a mod's renamed executable is stored under the original name: the engine opens disc/SLUS_216.78
            val name = if (u == exe) "SLUS_216.78" else u
            val out = File(paths.gameData, "disc/$name") // upper case, as the game asks for them
            out.parentFile?.mkdirs()
            copyOut(iso, f.offset, f.size, out, buf) { progress(it) }
            // (a copy that came out short: a storage that said yes and then wrote less, or an image read through a
            // provider that ended early. The game stopped at its start with such files.)
            if (out.length() != f.size) throw Stop(context.getString(R.string.err_copy_short, f.path))
            files++
        }
        val bytes = paths.gameData.sizeDeep()
        paths.installedMarker.writeText("files=$files\nbytes=$bytes\nlayout=afs\n" + (if (modified) "modified=1\n" else ""))
        return InstallState.Installed(files, bytes, modified)
    }

    private suspend fun copyOut(iso: IsoImage, offset: Long, size: Long, out: File, buf: ByteBuffer, onBytes: (Long) -> Unit = {}) {
        out.outputStream().channel.use { oc ->
            var pos = offset
            var left = size
            while (left > 0) {
                coroutineContext.ensureActive()
                buf.clear()
                buf.limit(minOf(left, buf.capacity().toLong()).toInt())
                iso.readInto(pos, buf)
                buf.flip()
                val n = buf.limit().toLong()
                while (buf.hasRemaining()) oc.write(buf)
                pos += n
                left -= n
                onBytes(n)
            }
        }
    }

    fun uninstall() {
        paths.installedMarker.delete()
        paths.gameData.listFiles()?.filter { it.name != "mods" }?.forEach { it.deleteRecursively() }
    }

    /** The game's program: SLUS_216.78, or the renamed copy a known USA-based mod boots instead (MOD_EXECUTABLES). */
    private fun executable(iso: IsoImage) =
        iso.find("SLUS_216.78") ?: MOD_EXECUTABLES.firstNotNullOfOrNull { iso.find(it) }

    private fun sha1(data: ByteArray): String =
        MessageDigest.getInstance("SHA-1").digest(data).joinToString("") { "%02x".format(it) }

    companion object {
        /** Why a file is not a disc image, from its first bytes: a compressed archive or disc format (the players sent
         *  .7z, .zip, .rar and .chd files), a web page saved instead of the download, or a file far too small. */
        fun notIsoMessage(context: Context, channel: java.nio.channels.FileChannel): String {
            val head = ByteArray(16)
            val size = runCatching { channel.size() }.getOrDefault(-1L)
            val n = runCatching {
                val buf = ByteBuffer.wrap(head)
                channel.position(0)
                var got = 0
                while (buf.hasRemaining()) { val r = channel.read(buf); if (r <= 0) break; got += r }
                got
            }.getOrDefault(0)
            fun starts(vararg b: Int) = n >= b.size && b.indices.all { head[it] == b[it].toByte() }
            fun text(t: String) = n >= t.length && String(head, 0, t.length, Charsets.ISO_8859_1).equals(t, ignoreCase = true)
            val kind = when {
                starts(0x50, 0x4B, 0x03, 0x04) -> "ZIP"
                starts(0x37, 0x7A, 0xBC, 0xAF, 0x27, 0x1C) -> "7Z"
                starts(0x52, 0x61, 0x72, 0x21, 0x1A, 0x07) -> "RAR"
                text("MComprHD") -> "CHD"
                text("CISO") -> "CSO"
                text("ZISO") -> "ZSO"
                starts(0x1F, 0x8B) -> "GZ"
                else -> null
            }
            return when {
                kind != null -> context.getString(R.string.err_iso_packed, kind)
                n > 0 && (text("<!DOCTYPE") || text("<html") || head[0] == '<'.code.toByte()) -> context.getString(R.string.err_iso_web_page)
                size in 0 until 100L * 1024 * 1024 -> context.getString(R.string.err_iso_too_small, android.text.format.Formatter.formatShortFileSize(context, size))
                else -> context.getString(R.string.err_not_iso)
            }
        }

        // config/SLUS_216.78.yaml and config/DBZP.yaml of the decompilation
        const val ROM_SHA1 = "caee6c2269bba89fc51eea9e7beac3adbec9dc52"
        const val DBZP_SHA1 = "4f910969e05d9b25c83af7642b60da9949a4348b"
        const val ROM_BASE = 0x100000L
        const val ROM_END = 0x2FF180L
        // USA-based mods that boot their executable under another name. Power Scale (BETA 8): SLUS_219.99, same size
        // and segment layout as SLUS_216.78 (patched in place), so the engine's data list still lines up.
        val MOD_EXECUTABLES = listOf("SLUS_219.99")
    }
}
