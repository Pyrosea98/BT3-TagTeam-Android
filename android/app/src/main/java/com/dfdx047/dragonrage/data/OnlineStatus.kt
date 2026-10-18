package com.dfdx047.dragonrage.data

import java.io.File
import java.net.HttpURLConnection
import java.net.URL
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.withContext
import org.json.JSONObject

/**
 * The state of online play as the matchmaking service sees this build (the PC port's service: port/matchmaking): asked
 * when the launcher opens, with the two requests that change nothing there. "hello" says which PC release the
 * service lets in (MIN_RELEASE) and the newest one; "queues" says how many players search now, and is refused with
 * "build key" when the service only lets in signed (official PC) releases.
 */
sealed interface OnlineState {
    data object Checking : OnlineState
    data object Unreachable : OnlineState
    data class Outdated(val mine: Int, val min: Int) : OnlineState
    data class OfficialOnly(val mine: Int) : OnlineState
    data class Working(val mine: Int, val searching: Int, val latest: Int) : OnlineState
}

object OnlineStatus {
    private const val NET_VERSION = 4 // the online protocol's version (port/src/gs/net.c NET_VERSION)
    const val DEFAULT_URL = "https://odd-surf-0af7.bt3decomp.workers.dev"
    /** A matchmaking service run by players (fork Androidxxt01/Tenkaichi3DecompAndroidOnline): chosen in Settings,
     *  never by default; only the players who chose it meet there (no PC players of the official one). */
    const val COMMUNITY_URL = "https://mitigo-rooms.minetivacraft.workers.dev"

    /** The service in use: match_url.txt in the game folder (the engine reads it too, net_match.c), else the official. */
    fun current(paths: GamePaths): String = runCatching { File(paths.root, "match_url.txt").readText().trim() }.getOrNull()
        ?.takeIf { it.startsWith("https://") }?.trimEnd('/') ?: DEFAULT_URL

    fun choose(paths: GamePaths, url: String) {
        val f = File(paths.root, "match_url.txt")
        if (url == DEFAULT_URL) runCatching { f.delete() } else runCatching { f.writeText(url + "\n") }
    }

    private fun post(url: String, body: String): Pair<Int, String> {
        val c = (URL(url).openConnection() as HttpURLConnection).apply {
            requestMethod = "POST"
            doOutput = true
            connectTimeout = 8_000
            readTimeout = 10_000
            setRequestProperty("Content-Type", "application/json")
            setRequestProperty("User-Agent", "DragonRage-Android")
        }
        return try {
            c.outputStream.use { it.write(body.toByteArray()) }
            val code = c.responseCode
            val text = (if (code in 200..299) c.inputStream else c.errorStream)?.bufferedReader()?.use { it.readText() } ?: ""
            code to text
        } finally {
            c.disconnect()
        }
    }

    suspend fun check(paths: GamePaths, mine: Int): OnlineState = withContext(Dispatchers.IO) {
        val base = runCatching { File(paths.root, "match_url.txt").readText().trim() }.getOrNull()
            ?.takeIf { it.startsWith("https://") }?.trimEnd('/') ?: DEFAULT_URL
        val hello = runCatching { post("$base/v1/hello", """{"rel":$mine}""") }.getOrNull()
            ?: return@withContext OnlineState.Unreachable
        if (hello.first !in 200..299) return@withContext OnlineState.Unreachable
        val h = runCatching { JSONObject(hello.second) }.getOrNull() ?: return@withContext OnlineState.Unreachable
        val min = h.optInt("min", 0)
        val latest = h.optInt("latest", 0)
        if (min > mine) return@withContext OnlineState.Outdated(mine, min)
        val q = runCatching { post("$base/v1/queues", """{"v":$NET_VERSION,"rel":$mine}""") }.getOrNull()
        if (q != null && q.first == 403 && q.second.contains("build key")) return@withContext OnlineState.OfficialOnly(mine)
        var searching = 0
        if (q != null && q.first in 200..299) {
            runCatching {
                val o = JSONObject(q.second).getJSONObject("waiting")
                o.keys().forEach { k -> searching += o.optInt(k, 0).coerceAtLeast(0) }
            }
        }
        OnlineState.Working(mine, searching, latest)
    }

    fun release(n: Int): String = "0.${n / 100}.${n % 100}"
}
