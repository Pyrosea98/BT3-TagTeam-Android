package com.dfdx047.dragonrage.data

import org.json.JSONArray
import org.json.JSONObject
import java.io.File
import java.net.HttpURLConnection
import java.net.Inet4Address
import java.net.NetworkInterface
import java.net.URL

/**
 * BT2's line to the Dragon Net matchmaking service (port/dragonnet/worker.js: rooms with a code, and quick match):
 * the same requests Dragon Rage's BT3 makes, with a netcode version of its own, so the service never pairs a BT2 player
 * with a BT3 one. Nothing of the game passes through it: it only tells each side the other's name and addresses. The
 * connection between the two games, and the fight, are not here.
 *
 * The service is the official one, or another in <root>/bt2/user/dragonnet_url.txt (one line, https://...; for
 * developing: bt2/tools/dev_server/serve.mjs, http://127.0.0.1:8787 through `adb reverse`).
 */
object Bt2Online {
    /** BT2's netcode version on the service (the service's ALLOWED_VERSIONS must list it). BT3's is 5001. */
    const val VERSION = 6001
    private const val QUEUE = "S" // single fights
    private const val GAME_PORT = 7000 // (the port the game will listen on, when it connects: for now only what the addresses say)

    class Failure(val why: String) : Exception(why)
    data class Peer(val name: String, val addrs: List<String>)
    /** A room this player is in: its code, this player's key in it, and the other player when already there. */
    data class Room(val code: String, val key: String, val peer: Peer?, val hosting: Boolean)

    fun url(paths: GamePaths): String =
        runCatching { File(paths.bt2Root, "user/dragonnet_url.txt").readText().trim() }.getOrNull()?.takeIf { it.startsWith("http") }?.trimEnd('/')
            ?: OnlineStatus.DRAGON_URL

    /** The name as the service takes it: printable ASCII, 16 letters at most ("Player" when nothing is left). */
    fun cleanName(name: String): String = name.filter { it.code in 0x20..0x7e }.trim().take(16).ifEmpty { "Jogador" }

    fun host(base: String, name: String): Room {
        val r = post(base, "/v1/host", JSONObject().put("v", VERSION).put("name", cleanName(name)).put("addrs", addrs()))
        return Room(r.getString("code"), r.getString("key"), null, true)
    }

    fun join(base: String, code: String, name: String): Room {
        val r = post(base, "/v1/join", JSONObject().put("v", VERSION).put("code", code.trim().uppercase()).put("name", cleanName(name)).put("addrs", addrs()))
        return Room(code.trim().uppercase(), r.getString("key"), peer(r.getJSONObject("peer")), false)
    }

    /** Quick match: put with whoever waits longest, or given a room of its own to wait in. */
    fun quick(base: String, name: String, who: String): Room {
        val r = post(base, "/v1/quick", JSONObject().put("v", VERSION).put("name", cleanName(name)).put("addrs", addrs()).put("queue", QUEUE).put("who", who))
        val join = r.optString("role") == "join"
        return Room(r.getString("code"), r.getString("key"), if (join) peer(r.getJSONObject("peer")) else null, !join)
    }

    /** The one who joined the room this player hosts, once there. */
    fun poll(base: String, room: Room): Peer? =
        post(base, "/v1/poll", JSONObject().put("code", room.code).put("key", room.key)).optJSONObject("peer")?.let(::peer)

    fun leave(base: String, room: Room) {
        runCatching { post(base, "/v1/leave", JSONObject().put("code", room.code).put("key", room.key)) }
    }

    /** What the service said, in words for the player. */
    fun explain(why: String): String = when (why) {
        "version" -> "O jogo da outra pessoa é de outra versão."
        "no room" -> "Sala não encontrada. Confira o código."
        "full" -> "A sala já está cheia."
        "busy" -> "Muitas tentativas. Espere um pouco."
        "old" -> "Este jogo está desatualizado."
        "build key" -> "O servidor só aceita versões oficiais do jogo."
        "bad request" -> "O servidor não aceitou o pedido."
        "network" -> "Sem conexão com o servidor."
        else -> "O servidor respondeu: $why"
    }

    private fun peer(o: JSONObject) = Peer(o.optString("name", "Jogador"), (0 until (o.optJSONArray("addrs")?.length() ?: 0)).map { o.getJSONArray("addrs").getString(it) })

    /** This phone's addresses as the service wants them ("ip:port", four at most). */
    private fun addrs(): JSONArray {
        val found = runCatching {
            NetworkInterface.getNetworkInterfaces().toList().filter { it.isUp && !it.isLoopback }
                .flatMap { it.inetAddresses.toList() }.filterIsInstance<Inet4Address>().mapNotNull { it.hostAddress }
        }.getOrDefault(emptyList())
        return JSONArray((found.ifEmpty { listOf("0.0.0.0") }).take(4).map { "$it:$GAME_PORT" })
    }

    private fun post(base: String, path: String, body: JSONObject): JSONObject {
        val c = (URL(base + path).openConnection() as HttpURLConnection)
        try {
            c.requestMethod = "POST"
            c.connectTimeout = 6000
            c.readTimeout = 12000
            c.doOutput = true
            c.setRequestProperty("Content-Type", "application/json")
            c.outputStream.use { it.write(body.toString().toByteArray()) }
            val stream = if (c.responseCode < 400) c.inputStream else (c.errorStream ?: throw Failure("network"))
            val json = JSONObject(stream.bufferedReader().use { it.readText() })
            json.optString("error").takeIf { it.isNotEmpty() }?.let { throw Failure(it) }
            return json
        } catch (e: Failure) {
            throw e
        } catch (e: Exception) {
            throw Failure("network")
        } finally {
            c.disconnect()
        }
    }
}
