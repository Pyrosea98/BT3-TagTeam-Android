package com.dfdx047.dragonrage.data

import org.json.JSONArray
import org.json.JSONObject

const val REPO = "dfdx047/Tenkaichi3DecompAndroid"
const val MOD_BUILD = true
/** The Tag Team / Power Scale app's own repository: its releases are the app's updates. */
const val MOD_REPO = "Pyrosea98/BT3-TagTeam-Android"
const val REPO_URL = "https://github.com/$MOD_REPO" // (Dragon Rage itself: "https://github.com/$REPO")

/** Stable: published releases only. Nightly (preview): test builds as well (GitHub pre-releases). */
enum class UpdateChannel { STABLE, NIGHTLY }

/** [sha256]: the APK's SHA-256 as GitHub gives it for the asset (hex), checked after the download; empty: not checked. */
data class UpdateInfo(
    val version: String, val notes: String, val apkUrl: String, val size: Long, val nightly: Boolean,
    val code: Int = 0, val sha256: String = "",
)

/** The player's update choices (asked once, at the first start; changed under Settings > About). */
class UpdatePrefs(context: android.content.Context) {
    private val prefs = context.getSharedPreferences("updates", android.content.Context.MODE_PRIVATE)
    /** false until the player has answered the first-start question. */
    val asked: Boolean get() = prefs.getBoolean("asked", false)
    val checkOnLaunch: Boolean get() = prefs.getBoolean("launch", false)
    val channel: UpdateChannel get() = if (prefs.getString("channel", "stable") == "nightly") UpdateChannel.NIGHTLY else UpdateChannel.STABLE

    fun set(channel: UpdateChannel = this.channel, checkOnLaunch: Boolean = this.checkOnLaunch) {
        prefs.edit().putBoolean("asked", true).putBoolean("launch", checkOnLaunch)
            .putString("channel", if (channel == UpdateChannel.NIGHTLY) "nightly" else "stable").apply()
    }
}

/** The update dialog's state. */
sealed interface UpdateState {
    object Idle : UpdateState
    /** The first start: whether to look for updates when the app starts, and on which channel. */
    object Ask : UpdateState
    object Checking : UpdateState
    data class Available(val info: UpdateInfo) : UpdateState
    data class Downloading(val info: UpdateInfo, val done: Long, val total: Long) : UpdateState
    /** Downloaded, but Android has not yet let this app install packages. */
    data class NeedPermission(val info: UpdateInfo) : UpdateState
}

/**
 * Looks at the repository's GitHub releases. A test build (0.0.0-dev.N, version code N = the CI run) follows the
 * "android-nightly" pre-release, whose text carries the run number; a normal build follows the highest
 * `android-vX.Y.Z` release.
 */
object AppUpdates {
    suspend fun check(currentName: String, currentCode: Int, channel: UpdateChannel = UpdateChannel.STABLE): UpdateInfo? {
        // The Tag Team / Power Scale build never looks at Dragon Rage's releases: they share its signing key and would
        // install over the mod (engine and all). It follows its own repository instead.
        if (MOD_BUILD) return checkMod(currentCode, channel)
        val releases = JSONArray(Net.getText("https://api.github.com/repos/$REPO/releases?per_page=30"))
        val dev = currentName.contains("-dev")
        var best: UpdateInfo? = null
        var bestV: List<Int>? = null
        for (i in 0 until releases.length()) {
            val r = releases.getJSONObject(i)
            if (r.optBoolean("draft")) continue
            val tag = r.optString("tag_name")
            val (apkUrl, size) = apkAsset(r) ?: continue
            if (dev) {
                if (tag != "android-nightly") continue
                val run = Regex("""dev\.(\d+)""").find(r.optString("body"))?.groupValues?.get(1)?.toIntOrNull() ?: return null
                return if (run > currentCode) UpdateInfo("0.0.0-dev.$run", "", apkUrl, size, true) else null
            }
            if (!tag.startsWith("android-v") || r.optBoolean("prerelease")) continue
            val v = parse(tag.removePrefix("android-v")) ?: continue
            if (bestV == null || compare(v, bestV) > 0) {
                bestV = v
                best = UpdateInfo(tag.removePrefix("android-v"), r.optString("body").trim().take(8000), apkUrl, size, false)
            }
        }
        val mine = parse(currentName) ?: listOf(0)
        val found = best?.takeIf { compare(bestV!!, mine) > 0 } ?: return null
        // The notes: the changelog in the repository as it is at that version's tag (CHANGELOG.md / .pt.md / .es.md
        // since 0.7.3, which hold the latest version's notes; CHANGELOG-vX.Y.Z.md before), else the release's own text.
        val lang = java.util.Locale.getDefault().language
        val tag = "https://raw.githubusercontent.com/$REPO/android-v${found.version}"
        suspend fun fetch(url: String) = runCatching { Net.getText(url).trim().take(8000) }.getOrDefault("")
        suspend fun notes(base: String) = (if (lang == "pt" || lang == "es") fetch("$base.$lang.md") else "").ifBlank { fetch("$base.md") }
        val file = notes("$tag/CHANGELOG").ifBlank { notes("$tag/CHANGELOG-v${found.version}") }
        if (file.isNotBlank()) return found.copy(notes = file)
        return found
    }

    private val CODE = Regex("""<!--\s*bt3-android-code:\s*(\d+)\s*-->""")
    private val MARKERS = Regex("""<!--\s*bt3-[\w-]+:[^>]*-->""")

    /**
     * The mod's releases (MOD_REPO). A release is offered when it is not a draft, its tag starts with "v", its text
     * carries `<!-- bt3-android-code: N -->` (N = the APK's versionCode) with N above this app's, and it has one .apk
     * asset with GitHub's SHA-256 digest. Pre-releases only on the Nightly channel. The highest N wins, so neither
     * channel ever offers an older build (Stable after a preview: nothing until a stable release passes it).
     */
    private suspend fun checkMod(currentCode: Int, channel: UpdateChannel): UpdateInfo? {
        val releases = JSONArray(Net.getText("https://api.github.com/repos/$MOD_REPO/releases?per_page=30"))
        var best: UpdateInfo? = null
        for (i in 0 until releases.length()) {
            val r = releases.getJSONObject(i)
            if (r.optBoolean("draft")) continue
            val pre = r.optBoolean("prerelease")
            if (pre && channel != UpdateChannel.NIGHTLY) continue
            val tag = r.optString("tag_name")
            if (!tag.startsWith("v")) continue
            val body = r.optString("body")
            val code = CODE.find(body)?.groupValues?.get(1)?.toIntOrNull() ?: continue
            if (code <= currentCode || (best != null && code <= best.code)) continue
            val apks = (0 until (r.optJSONArray("assets")?.length() ?: 0)).map { r.getJSONArray("assets").getJSONObject(it) }
                .filter { it.optString("name").endsWith(".apk") }
            if (apks.size != 1) continue
            val a = apks[0]
            val digest = a.optString("digest")
            if (!digest.startsWith("sha256:")) continue
            best = UpdateInfo(tag.removePrefix("v"), body.replace(MARKERS, "").trim().take(8000), a.getString("browser_download_url"),
                a.optLong("size", -1), pre, code, digest.removePrefix("sha256:").lowercase())
        }
        return best
    }

    private fun apkAsset(r: JSONObject): Pair<String, Long>? {
        val assets = r.optJSONArray("assets") ?: return null
        for (i in 0 until assets.length()) {
            val a = assets.getJSONObject(i)
            if (a.optString("name").endsWith(".apk")) return a.getString("browser_download_url") to a.optLong("size", -1)
        }
        return null
    }

    private fun parse(v: String): List<Int>? =
        v.substringBefore('-').split('.').map { it.toIntOrNull() ?: return null }.takeIf { it.isNotEmpty() }

    private fun compare(a: List<Int>, b: List<Int>): Int {
        for (i in 0 until maxOf(a.size, b.size)) {
            val d = a.getOrElse(i) { 0 } - b.getOrElse(i) { 0 }
            if (d != 0) return d
        }
        return 0
    }
}

data class CatalogPack(val name: String, val description: String, val url: String, val size: Long)

sealed interface CatalogState {
    object Loading : CatalogState
    object Failed : CatalogState
    data class Loaded(val packs: List<CatalogPack>) : CatalogState
}

/**
 * The list of texture packs the app offers to download: android/texture-packs.json in the repository, read from
 * `main`, so packs can be added or fixed without a new app version.
 * Format: {"packs":[{"name":"…","description":"…","url":"https://…/pack.zip","size":123456}]}
 */
object TextureCatalog {
    const val URL = "https://raw.githubusercontent.com/$REPO/main/android/texture-packs.json"

    suspend fun load(): List<CatalogPack> {
        val arr = JSONObject(Net.getText(URL)).optJSONArray("packs") ?: return emptyList()
        return (0 until arr.length()).mapNotNull {
            val o = arr.getJSONObject(it)
            val url = o.optString("url")
            if (!url.startsWith("https://")) null
            else CatalogPack(o.optString("name", "Texture pack"), o.optString("description"), url, o.optLong("size", -1))
        }
    }
}
