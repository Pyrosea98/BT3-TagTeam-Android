package com.dfdx047.dragonrage.data

import java.io.File
import java.io.FileInputStream
import java.math.BigInteger
import java.security.MessageDigest
import java.util.zip.Inflater
import java.util.zip.ZipFile
import kotlin.coroutines.coroutineContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.withContext
import org.json.JSONObject

/**
 * The game data of the PC v0.5 releases (now 0.5.alpha.1: Dragon History, Scenarios, achievements, the new interface pictures and the
 * BETA 8 data fixes), applied over a v0.4.1 import the way the PC's own updater does it (update.py in
 * BT3-TagTeam-v0.5.alpha.1-Windows-Update.zip; it updates a v0.4.1 import or one already at 0.5.alpha): every file has the SHA-256 it must have before and after, and a blob
 * (zlib) that is either the whole new file or XORed with the old one. Only the data root (gamedata/...) and the
 * stage files are taken; the PC's programs are not. Every file is rebuilt and checked before any is replaced, so a
 * failure leaves the v0.4.1 data as it was.
 */
class TagTeamV05Update(private val paths: GamePaths) {
    val zip = File(paths.root, ZIP_NAME)
    private val receipt = File(paths.gameData, RECEIPT_NAME)

    /** The version of the game data: what this update's receipt says, else the v0.4.1 import's. */
    fun dataVersion(): String = runCatching { org.json.JSONObject(receipt.readText()).optString("version") }.getOrNull()?.takeIf { it.isNotEmpty() } ?: "0.4.1"

    /** The Tag Team import is there and its data is not this update's version yet. */
    fun needed(): Boolean = File(paths.gameData, "import-receipt.json").isFile && dataVersion() != VERSION

    fun applied(): Boolean = receipt.isFile && dataVersion() == VERSION

    fun zipReady(): Boolean = zip.isFile && zip.length() == ZIP_BYTES

    suspend fun adoptZip(file: File, report: (Float) -> Unit) = withContext(Dispatchers.IO) {
        if (file.length() != ZIP_BYTES || sha256(file, report) != ZIP_SHA256) {
            file.delete()
            throw TagTeamImport.Stop("The v$VERSION update file is not the published one (size or SHA-256 differ).")
        }
        if (!file.renameTo(zip)) throw TagTeamImport.Stop("Cannot keep the v$VERSION update file.")
    }

    /** Applies the update; returns the number of files written. */
    suspend fun apply(report: (Float, String) -> Unit): Int = withContext(Dispatchers.IO) {
        val work = File(paths.gameData, ".v05-update")
        work.deleteRecursively()
        work.mkdirs()
        try {
            ZipFile(zip).use { z ->
                val manifest = JSONObject(z.getInputStream(z.getEntry("payload/update.json") ?: throw TagTeamImport.Stop("The update has no manifest."))
                    .bufferedReader().readText())
                if (manifest.optString("version") != VERSION) throw TagTeamImport.Stop("The update is not v$VERSION.")
                val files = manifest.getJSONArray("files")
                val planned = ArrayList<Pair<File, File>>() // (new file in work, destination)
                for (i in 0 until files.length()) {
                    coroutineContext.ensureActive()
                    val r = files.getJSONObject(i)
                    val path = r.getString("path")
                    val wanted = r.getString("sha256")
                    when (r.getString("root")) {
                        "data" -> if (!path.startsWith("gamedata/")) throw TagTeamImport.Stop("Unsafe update path: $path")
                        "app" -> if (!path.startsWith("stages/")) continue // the PC's programs and launcher
                        else -> continue
                    }
                    val dest = child(paths.root, path)
                    val before = if (dest.isFile) sha256(dest) {} else null
                    if (before == wanted) continue
                    val plans = r.getJSONObject("plans")
                    val key = before ?: "None"
                    if (!plans.has(key)) {
                        throw TagTeamImport.Stop("A game file differs from the v0.4.1 import (changed by a mod or another version): $path")
                    }
                    val plan = plans.getJSONObject(key)
                    val blobEntry = z.getEntry("payload/" + plan.getString("file")) ?: throw TagTeamImport.Stop("The update is missing a part of $path.")
                    val blob = z.getInputStream(blobEntry).use { it.readBytes() }
                    if (hex(MessageDigest.getInstance("SHA-256").digest(blob)) != plan.getString("sha256")) throw TagTeamImport.Stop("A part of the update is damaged.")
                    val size = r.getLong("bytes").toInt()
                    var raw = inflate(blob, size)
                    if (plan.getString("kind") == "xor") {
                        // update.py: int.from_bytes(raw,'little') ^ int.from_bytes(old[:bytes],'little') -> bytes, little-endian
                        val old = dest.readBytes()
                        val out = ByteArray(size)
                        for (k in 0 until size) out[k] = ((if (k < raw.size) raw[k].toInt() else 0) xor (if (k < old.size) old[k].toInt() else 0)).toByte()
                        raw = out
                    } else if (plan.getString("kind") != "full") {
                        throw TagTeamImport.Stop("Unknown update kind for $path.")
                    }
                    if (raw.size != size || hex(MessageDigest.getInstance("SHA-256").digest(raw)) != wanted) {
                        throw TagTeamImport.Stop("A rebuilt file did not check out: $path")
                    }
                    val temp = File(work, "new-${planned.size}")
                    temp.writeBytes(raw)
                    planned.add(temp to dest)
                    report((i + 1).toFloat() / files.length() * 0.9f, "${i + 1} / ${files.length()} files checked")
                }
                // all checked: replace
                for ((n, p) in planned.withIndex()) {
                    val (temp, dest) = p
                    dest.parentFile?.mkdirs()
                    if (dest.exists() && !dest.delete()) throw TagTeamImport.Stop("Cannot replace ${dest.name}.")
                    if (!temp.renameTo(dest)) throw TagTeamImport.Stop("Cannot write ${dest.name}.")
                    report(0.9f + 0.1f * (n + 1) / maxOf(1, planned.size), "${n + 1} / ${planned.size} files written")
                }
                receipt.writeText("{\"version\":\"$VERSION\",\"revision\":\"${manifest.optString("revision")}\",\"platform\":\"android\",\"files\":${planned.size},\"update_sha256\":\"$ZIP_SHA256\"}\n")
                // the update files of earlier versions are of no further use
                for (old in OLD_ZIPS) File(paths.root, old).delete()
                planned.size
            }
        } finally {
            work.deleteRecursively()
        }
    }

    private fun inflate(blob: ByteArray, size: Int): ByteArray {
        val inf = Inflater()
        try {
            inf.setInput(blob)
            val out = ByteArray(size)
            var at = 0
            while (!inf.finished() && at < size) {
                val n = inf.inflate(out, at, size - at)
                if (n == 0 && (inf.needsInput() || inf.needsDictionary())) break
                at += n
            }
            return if (at == size) out else out.copyOf(at)
        } finally {
            inf.end()
        }
    }

    private fun child(root: File, name: String): File {
        val parts = name.split('/')
        if (name.startsWith("/") || parts.any { it.isEmpty() || it == "." || it == ".." || ':' in it || '\\' in it }) throw TagTeamImport.Stop("Unsafe update path: $name")
        return File(root, name)
    }

    private fun sha256(file: File, onProgress: (Float) -> Unit): String {
        val md = MessageDigest.getInstance("SHA-256")
        val total = file.length().coerceAtLeast(1)
        var done = 0L
        FileInputStream(file).use { input ->
            val buf = ByteArray(1 shl 20)
            while (true) {
                val n = input.read(buf)
                if (n <= 0) break
                md.update(buf, 0, n)
                done += n
                onProgress(done.toFloat() / total)
            }
        }
        return hex(md.digest())
    }

    private fun hex(b: ByteArray) = String.format("%064x", BigInteger(1, b))

    companion object {
        const val VERSION = "0.5.alpha.1"
        const val RECEIPT_NAME = "v05-update-receipt.json"
        const val ZIP_NAME = "BT3-TagTeam-v0.5.alpha.1-Windows-Update.zip"
        const val ZIP_URL = "https://github.com/Pyrosea98/BT3-TagTeam-Native/releases/download/v0.5.alpha.1/$ZIP_NAME"
        const val ZIP_SHA256 = "55a6dc069b80415b3303793d296580b330c012f96c3e3345000652b0848f121e"
        const val ZIP_BYTES = 243_185_421L
        const val ZIP_MB = 232
        private val OLD_ZIPS = listOf("BT3-TagTeam-v0.5.alpha-Windows-Update.zip")
    }
}
