package com.dfdx047.dragonrage.data

import android.content.ContentResolver
import android.content.Context
import android.net.Uri
import android.util.JsonReader
import java.io.File
import java.io.FileInputStream
import java.nio.ByteBuffer
import java.nio.channels.FileChannel
import java.security.MessageDigest
import java.util.zip.GZIPInputStream
import java.util.zip.Inflater
import java.util.zip.ZipFile
import kotlin.coroutines.coroutineContext
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.ensureActive
import kotlinx.coroutines.launch
import kotlinx.coroutines.withContext

/**
 * The Tag Team / Real Power Scale import, the same as the PC v0.4.1 installer's (installer/v041/importer.py in
 * Pyrosea98/BT3-TagTeam-Native): the player's own Power Scale BETA 8 image (the original or Codex's R4 normal / 2x
 * image, each known by its full SHA-256) and the release's import recipe (BT3-TagTeam-v0.4.1-ImportRecipes.zip: a
 * content list and XOR deltas, no game data) give the game's loose files, every one checked by its SHA-256.
 *
 * Normal or 2x maps are a choice made here, whatever the image: the list says which files belong to which.
 * Nothing of an existing install is replaced until the new one is complete: it is written beside it and swapped in
 * at the end; the old one is kept as gamedata.previous until the player removes it (saves and settings are outside
 * gamedata and are never touched).
 */
class TagTeamImport(private val paths: GamePaths, private val resolver: ContentResolver) {

    class Stop(message: String) : Exception(message)

    enum class Maps(val id: String) { DOUBLE("2x"), NORMAL("normal") }

    private class Plan(val offset: Long, val bytes: Long, val baseSha: String?, val patchFile: String?, val patchBytes: Long, val patchSha: String?)
    private class Row(val path: String, val variants: List<String>, val bytes: Long, val sha: String, val sources: Map<String, Plan>)
    private class Source(val id: String, val sha: String, val bytes: Long)

    /** The recipe this app was made with: the v0.4.1 release's asset, by its hash, and the content list in it. */
    val recipe = File(paths.root, RECIPE_NAME)
    val previous = File(paths.root, "gamedata.previous")

    fun recipeReady(): Boolean = recipe.isFile && recipe.length() == RECIPE_BYTES

    /** Checks a recipe zip (downloaded or picked) by its SHA-256 and keeps it as [recipe]. */
    suspend fun adoptRecipe(file: File, report: (Float) -> Unit) = withContext(Dispatchers.IO) {
        if (file.length() != RECIPE_BYTES || sha256(file, report) != RECIPE_SHA256) {
            file.delete()
            throw Stop("This is not the v0.4.1 import recipe (BT3-TagTeam-v0.4.1-ImportRecipes.zip): its checksum does not match.")
        }
        if (file != recipe) {
            recipe.delete()
            if (!file.renameTo(recipe)) { file.copyTo(recipe, overwrite = true); file.delete() }
        }
    }

    suspend fun install(uri: Uri, maps: Maps, report: (InstallState.Working) -> Unit): InstallState.Installed = withContext(Dispatchers.IO) {
        if (!recipeReady()) throw Stop("The import recipe is missing.")
        val pfd = resolver.openFileDescriptor(uri, "r") ?: throw Stop("The image cannot be opened.")
        pfd.use {
            val channel = FileInputStream(pfd.fileDescriptor).channel
            ZipFile(recipe).use { zip ->
                report(InstallState.Working("Checking the import recipe", 0f, ""))
                val manifestEntry = zip.getEntry("import-data/content.json.gz") ?: throw Stop("The import recipe has no content list.")
                val manifestSha = MessageDigest.getInstance("SHA-256").let { md ->
                    zip.getInputStream(manifestEntry).use { s -> val b = ByteArray(1 shl 16); while (true) { val n = s.read(b); if (n < 0) break; md.update(b, 0, n) } }
                    hex(md.digest())
                }
                if (manifestSha != MANIFEST_SHA256) throw Stop("The import recipe's content list failed verification.")
                val (sources, rows) = zip.getInputStream(manifestEntry).use { parse(GZIPInputStream(it, 1 shl 16)) }

                report(InstallState.Working("Checking your ISO", 0f, "Reading the whole image to check it (a few minutes)"))
                val size = channel.size()
                val isoSha = sha256(channel) { f -> report(InstallState.Working("Checking your ISO", f * 0.05f, "${(f * 100).toInt()}%")) }
                val source = sources.firstOrNull { it.sha == isoSha && it.bytes == size }
                    ?: throw Stop("Unsupported ISO. Use the original Power Scale BETA 8, or the compatible R4 normal / 2x image listed in the manual.")

                val selected = rows.filter { maps.id in it.variants }
                if (selected.map { it.path.lowercase() }.toSet().size != selected.size) throw Stop("Duplicate content path")

                // The image's four archives are copied whole (DATA/PAK3US1..4.AFS, where the engine reads an entry
                // through the archive's own table: plat_fcache.c, Port_AfsSpan). A file of the list that is exactly
                // such an entry, unchanged, is then served from there and not written again on its own: about 79,000
                // of the 83,000 files. Making that many small files on a phone's shared storage took most of an
                // hour; the archives are four large files. The image itself is authenticated by its full SHA-256.
                val archives = IsoImage(channel).files
                    .filter { it.path.uppercase().let { p -> p.startsWith("DATA/PAK3US") && p.endsWith(".AFS") } }
                    .mapNotNull { e ->
                        val head = read(channel, e.offset, 8)
                        if (String(head, 0, 3, Charsets.US_ASCII) != "AFS") return@mapNotNull null
                        val count = ByteBuffer.wrap(head).order(java.nio.ByteOrder.LITTLE_ENDIAN).getInt(4)
                        if (count <= 0 || count > 0x100000) return@mapNotNull null
                        val toc = ByteBuffer.wrap(read(channel, e.offset + 8, count * 8)).order(java.nio.ByteOrder.LITTLE_ENDIAN)
                        val name = e.path.substringAfterLast('/').substringBeforeLast('.').lowercase() // pak3us1
                        name to Triple(e, count, LongArray(count * 2) { k -> toc.getInt(k * 4).toLong() and 0xFFFFFFFFL })
                    }.toMap().filterKeys { it == "pak3us2" } // (the one archive whose entries are served from inside)
                fun served(row: Row): Boolean {
                    val plan = row.sources[source.id] ?: return false
                    if (plan.patchFile != null) return false
                    // Only the big second archive (pzs3us2, 65,000 entries: that folder was what made the import
                    // slow) is served from inside: the mod's own readers of the other three read their loose
                    // files, and the ISO's appended songs (entries from 64970) are read loose too (plat_songs.c).
                    if (!row.path.startsWith("pzs3us2/")) return false
                    if ((row.path.substringAfter('/').removeSuffix(".bin").toIntOrNull() ?: 0) >= 64970) return false
                    val part = row.path.substringBefore('/').replace("pzs3us", "pak3us") // (the list keeps pak3us1/2 as pzs3us1/2)
                    val name = row.path.substringAfter('/')
                    val (e, count, toc) = archives[part] ?: return false
                    if ('/' in name || !name.endsWith(".bin")) return false
                    val index = name.removeSuffix(".bin").toIntOrNull() ?: return false
                    return index in 0 until count && plan.offset == e.offset + toc[index * 2] && plan.bytes == toc[index * 2 + 1] && plan.bytes == row.bytes
                }
                val loose = selected.filterNot { served(it) }
                val archiveBytes = archives.values.sumOf { it.first.size }
                val total = archiveBytes + loose.sumOf { it.bytes }
                if (paths.root.usableSpace < total + (1L shl 30)) {
                    throw Stop("Not enough free space: the game data takes ${total shr 20} MB and ${paths.root.usableSpace shr 20} MB are free.")
                }
                ActivityLog.info("iso", "import plan: ${archives.size} archive (${archiveBytes shr 20} MB) serve ${selected.size - loose.size} files; ${loose.size} written loose")
                val temp = File(paths.root, "gamedata.partial")
                temp.deleteRecursively() // (an earlier, interrupted import's: never the installed data)
                temp.mkdirs()
                val done = java.util.concurrent.atomic.AtomicLong()
                val checked = java.util.concurrent.ConcurrentHashMap.newKeySet<String>()
                val next = java.util.concurrent.atomic.AtomicInteger()
                val finished = java.util.concurrent.atomic.AtomicInteger()
                try {
                  for ((_, a) in archives) {
                    val e = a.first
                    val dst = File(temp, "disc/DATA/" + e.path.substringAfterLast('/').uppercase())
                    dst.parentFile?.mkdirs()
                    dst.outputStream().use { out ->
                        val buf = ByteBuffer.allocate(8 shl 20)
                        var at = e.offset
                        var left = e.size
                        while (left > 0) {
                            coroutineContext.ensureActive()
                            buf.clear()
                            buf.limit(minOf(left, buf.capacity().toLong()).toInt())
                            val n = channel.read(buf, at)
                            if (n <= 0) throw Stop("Truncated ISO")
                            out.write(buf.array(), 0, n)
                            at += n; left -= n
                            val d = done.addAndGet(n.toLong())
                            report(InstallState.Working("Copying the game archives", 0.05f + 0.94f * d / maxOf(1L, total), dst.name))
                        }
                    }
                  }
                  // four at once: the rest is a few thousand files, each made on the shared storage
                  kotlinx.coroutines.coroutineScope {
                   repeat(4) {
                    launch(Dispatchers.IO) {
                     while (true) {
                        val i = next.getAndIncrement()
                        if (i >= loose.size) break
                        val row = loose[i]
                        coroutineContext.ensureActive()
                        val plan = row.sources[source.id] ?: throw Stop("The content list has no source for ${row.path}")
                        if (plan.offset < 0 || plan.bytes < 0 || plan.offset + plan.bytes > source.bytes) throw Stop("Invalid ISO read bounds")
                        val dst = child(temp, row.path)
                        dst.parentFile?.mkdirs()
                        val md = MessageDigest.getInstance("SHA-256")
                        if (plan.patchFile != null) {
                            if (plan.bytes > 128L shl 20 || row.bytes > 128L shl 20) throw Stop("Patch exceeds bounded resource size")
                            val before = read(channel, plan.offset, plan.bytes.toInt())
                            if (hex(MessageDigest.getInstance("SHA-256").digest(before)) != plan.baseSha) throw Stop("Source resource changed: ${row.path}")
                            val entry = zip.getEntry("import-data/deltas/${plan.patchFile}") ?: throw Stop("Missing delta ${plan.patchFile}")
                            if (plan.patchBytes > 64L shl 20 || entry.size != plan.patchBytes) throw Stop("Invalid patch size")
                            val packed = zip.getInputStream(entry).use { it.readBytes() }
                            if (plan.patchFile !in checked) {
                                if (hex(MessageDigest.getInstance("SHA-256").digest(packed)) != plan.patchSha) throw Stop("Resource patch failed verification")
                                checked += plan.patchFile
                            }
                            val delta = ByteArray(row.bytes.toInt())
                            val inflater = Inflater()
                            try {
                                inflater.setInput(packed)
                                var got = 0
                                while (got < delta.size && !inflater.finished()) {
                                    val n = inflater.inflate(delta, got, delta.size - got)
                                    if (n == 0 && (inflater.needsInput() || inflater.needsDictionary())) break
                                    got += n
                                }
                                val extra = ByteArray(1)
                                if (got != delta.size || (!inflater.finished() && inflater.inflate(extra) != 0) || !inflater.finished() || inflater.remaining != 0) {
                                    throw Stop("Invalid resource delta length")
                                }
                            } finally {
                                inflater.end()
                            }
                            // (the importer XORs the delta with the start of the source; past its end the delta is the data)
                            for (k in 0 until minOf(delta.size, before.size)) delta[k] = (delta[k].toInt() xor before[k].toInt()).toByte()
                            md.update(delta)
                            dst.writeBytes(delta)
                        } else {
                            if (plan.bytes != row.bytes) throw Stop("Copy size mismatch")
                            dst.outputStream().use { out ->
                                val buf = ByteBuffer.allocate(8 shl 20)
                                var at = plan.offset
                                var left = plan.bytes
                                while (left > 0) {
                                    buf.clear()
                                    buf.limit(minOf(left, buf.capacity().toLong()).toInt())
                                    val n = channel.read(buf, at)
                                    if (n <= 0) throw Stop("Truncated ISO")
                                    md.update(buf.array(), 0, n)
                                    out.write(buf.array(), 0, n)
                                    at += n; left -= n
                                }
                            }
                        }
                        if (hex(md.digest()) != row.sha) throw Stop("Reconstructed resource failed verification: ${row.path}")
                        val bytesDone = done.addAndGet(row.bytes)
                        val n = finished.incrementAndGet()
                        if (n % 1000 == 0) {
                            ActivityLog.info("iso", "import: $n / ${loose.size} files, ${bytesDone shr 20} / ${total shr 20} MB")
                        }
                        if (n % 50 == 0 || n == loose.size) {
                            report(InstallState.Working("Preparing game data", 0.05f + 0.94f * bytesDone / maxOf(1L, total), "$n / ${loose.size}"))
                        }
                     }
                    }
                   }
                  }
                    File(temp, "import-receipt.json").writeText(
                        "{\"version\":\"0.4.1\",\"platform\":\"android\",\"source_iso_sha256\":\"$isoSha\",\"source\":\"${source.id}\"," +
                            "\"map_size\":\"${maps.id}\",\"files\":${selected.size},\"bytes\":${done.get()},\"manifest_sha256\":\"$MANIFEST_SHA256\",\"verified\":true}\n"
                    )
                    // the engine reads either archive name (pak3us1/2 <-> pzs3us1/2: plat_fcache.c, plat_file.c), so no aliases
                    File(temp, ".installed").writeText("files=${selected.size}\nbytes=${done.get()}\nlayout=folders\nmodified=1\nmaps=${maps.id}\nsource=${source.id}\n")
                    // (a first install's gamedata is only the empty folders the app made: not worth keeping)
                    if (paths.gameData.exists() && !paths.gameData.walkTopDown().any { it.isFile }) paths.gameData.deleteRecursively()
                    if (paths.gameData.exists()) {
                        previous.deleteRecursively()
                        if (!paths.gameData.renameTo(previous)) throw Stop("The installed game data could not be set aside.")
                    }
                    if (!temp.renameTo(paths.gameData)) {
                        if (previous.exists()) previous.renameTo(paths.gameData)
                        throw Stop("The new game data could not be put in place.")
                    }
                    report(InstallState.Working("Ready", 1f, ""))
                    return@withContext InstallState.Installed(selected.size, done.get(), true)
                } catch (e: Throwable) {
                    temp.deleteRecursively()
                    throw e
                }
            }
        }
    }

    private fun parse(input: java.io.InputStream): Pair<List<Source>, List<Row>> {
        val sources = ArrayList<Source>()
        val rows = ArrayList<Row>(90000)
        JsonReader(input.reader(Charsets.UTF_8).buffered(1 shl 16)).use { r ->
            r.beginObject()
            while (r.hasNext()) {
                when (r.nextName()) {
                    "schema" -> if (r.nextInt() != 1) throw Stop("Invalid content manifest")
                    "sources" -> {
                        r.beginArray()
                        while (r.hasNext()) {
                            var id = ""; var sha = ""; var bytes = 0L
                            r.beginObject()
                            while (r.hasNext()) when (r.nextName()) { "id" -> id = r.nextString(); "sha256" -> sha = r.nextString(); "bytes" -> bytes = r.nextLong(); else -> r.skipValue() }
                            r.endObject()
                            sources += Source(id, sha, bytes)
                        }
                        r.endArray()
                    }
                    "files" -> {
                        r.beginArray()
                        while (r.hasNext()) rows += row(r)
                        r.endArray()
                    }
                    else -> r.skipValue()
                }
            }
            r.endObject()
        }
        if (rows.isEmpty() || rows.size > 120000) throw Stop("Invalid content manifest")
        return sources to rows
    }

    private fun row(r: JsonReader): Row {
        var path = ""; var bytes = 0L; var sha = ""
        val variants = ArrayList<String>(2)
        val sources = HashMap<String, Plan>(4)
        r.beginObject()
        while (r.hasNext()) {
            when (r.nextName()) {
                "path" -> path = r.nextString()
                "bytes" -> bytes = r.nextLong()
                "sha256" -> sha = r.nextString()
                "variants" -> { r.beginArray(); while (r.hasNext()) variants += r.nextString(); r.endArray() }
                "sources" -> {
                    r.beginObject()
                    while (r.hasNext()) {
                        val id = r.nextName()
                        var offset = -1L; var n = -1L; var base: String? = null; var pf: String? = null; var pb = 0L; var ps: String? = null
                        r.beginObject()
                        while (r.hasNext()) {
                            when (r.nextName()) {
                                "offset" -> offset = r.nextLong()
                                "bytes" -> n = r.nextLong()
                                "base_sha256" -> base = r.nextString()
                                "patch" -> {
                                    r.beginObject()
                                    while (r.hasNext()) when (r.nextName()) { "file" -> pf = r.nextString(); "bytes" -> pb = r.nextLong(); "sha256" -> ps = r.nextString(); else -> r.skipValue() }
                                    r.endObject()
                                }
                                else -> r.skipValue()
                            }
                        }
                        r.endObject()
                        sources[id] = Plan(offset, n, base, pf, pb, ps)
                    }
                    r.endObject()
                }
                else -> r.skipValue()
            }
        }
        r.endObject()
        return Row(path, variants, bytes, sha, sources)
    }

    /** A path of the content list under [root], refused if it could leave it. */
    private fun child(root: File, name: String): File {
        val parts = name.split('/')
        if (name.startsWith("/") || parts.isEmpty() || parts.any { it.isEmpty() || it == "." || it == ".." || ':' in it || '\\' in it }) {
            throw Stop("Unsafe content path")
        }
        return File(root, name)
    }

    private fun read(channel: FileChannel, offset: Long, n: Int): ByteArray {
        val buf = ByteBuffer.allocate(n)
        var at = offset
        while (buf.hasRemaining()) {
            val got = channel.read(buf, at)
            if (got <= 0) throw Stop("Truncated ISO")
            at += got
        }
        return buf.array()
    }

    private fun sha256(channel: FileChannel, onProgress: (Float) -> Unit): String {
        val md = MessageDigest.getInstance("SHA-256")
        val buf = ByteBuffer.allocate(8 shl 20)
        val size = channel.size()
        var at = 0L
        var step = 0
        while (at < size) {
            buf.clear()
            val n = channel.read(buf, at)
            if (n <= 0) break
            md.update(buf.array(), 0, n)
            at += n
            if (++step % 16 == 0) onProgress(at.toFloat() / size)
        }
        return hex(md.digest())
    }

    private fun sha256(file: File, onProgress: (Float) -> Unit): String = FileInputStream(file).channel.use { sha256(it, onProgress) }

    private fun hex(b: ByteArray) = b.joinToString("") { "%02x".format(it) }

    companion object {
        const val RECIPE_NAME = "BT3-TagTeam-v0.4.1-ImportRecipes.zip"
        const val RECIPE_URL = "https://github.com/Pyrosea98/BT3-TagTeam-Native/releases/download/v0.4.1/$RECIPE_NAME"
        const val RECIPE_SHA256 = "1189068004347ab03a08ebd4454587df2923ad7452fc2a42096c216a4b2fd318"
        const val RECIPE_BYTES = 429_872_734L
        const val MANIFEST_SHA256 = "63802e4de080c781720642565f5c27bb6bf8f7222ddd6bbcb309bb2158b8df5e"

        /**
         * The recommended settings: the PC's tested setup (its bt3_settings.txt, the same as the PC v0.4.1 installer's
         * recommended-settings.txt) without the device-specific keys (controls, screen, volume), in
         * assets/recommended-settings.txt. [overwrite] false: only the settings the player has not set (a first
         * install, an import); true: those keys set to the PC's values (the Settings page's reset), the rest kept.
         */
        fun applyRecommended(context: Context, paths: GamePaths, overwrite: Boolean = false): Int {
            val wanted = context.assets.open("recommended-settings.txt").bufferedReader().readLines().filter { '=' in it }
            val keys = wanted.map { it.substringBefore('=') }.toSet()
            val lines = if (paths.settings.isFile) paths.settings.readLines().filter { it.isNotBlank() } else emptyList()
            val have = lines.filter { '=' in it }.map { it.substringBefore('=') }.toSet()
            val kept = if (overwrite) lines.filter { it.substringBefore('=') !in keys } else lines
            val add = if (overwrite) wanted else wanted.filter { it.substringBefore('=') !in have }
            if (add.isEmpty()) return 0
            val tmp = File(paths.settings.path + ".tmp")
            tmp.writeText((kept + add).joinToString("\n", postfix = "\n"))
            if (!tmp.renameTo(paths.settings)) { paths.settings.writeText(tmp.readText()); tmp.delete() }
            return add.size
        }

        /** The completed-progression save, only where there is no save yet. */
        fun applyCompletedSave(context: Context, paths: GamePaths): Boolean {
            val card = File(paths.saves, "card1/BASLUS-21678DBZT3")
            if (File(card, "BASLUS-21678DBZT3").exists()) return false
            card.mkdirs()
            for (name in listOf("BASLUS-21678DBZT3", "icon.sys", "dbzsm.ico")) {
                runCatching { context.assets.open("save-seed/$name").use { i -> File(card, name).outputStream().use { i.copyTo(it) } } }
            }
            return true
        }
    }
}
