package com.dfdx047.dragonrage.data

import org.json.JSONArray

/** A driver package published on GitHub: one asset (.zip or .adpkg) of a release. */
data class DriverAsset(val name: String, val url: String, val size: Long)

data class DriverRelease(val title: String, val tag: String, val date: String, val assets: List<DriverAsset>)

/**
 * Where drivers come from: the GitHub repositories of the people who build them. [gpus] says which GPUs a source builds
 * for (GL_RENDERER, e.g. "Adreno (TM) 740", "Mali-G57 MC2"); the drivers page lists the sources for the device's GPU.
 * The two ARMSX2 sources (the PlayStation 2 emulator's own drivers, github.com/ARMSX2/ARMSX2): MaliSX2, a Vulkan
 * driver for Mali on Arm's kbase kernel driver (MIT; Mali-G52, G57, G68, G77, G78, G615, G715), and their Turnip
 * (Mesa with their patches, Adreno 6xx and 7xx). Both are AdrenoTools packs (meta.json and the .so in a zip), loaded
 * like every other one.
 */
enum class DriverSource(val label: String, val repo: String, val gpus: (String) -> Boolean = { true }, val experimental: Boolean = false) {
    MALISX2("ARMSX2 · MaliSX2", "bmdhacks/malisx2", ::isMaliSx2Gpu, experimental = true),
    ARMSX2_TURNIP("ARMSX2 · Turnip", "bmdhacks/armsx2-turnip", { isAdreno(it) }),
    MR_PURPLE("Mr. Purple", "MrPurple666/purple-turnip", { isAdreno(it) }),
    STEVEN("StevenMXZ", "StevenMXZ/Adreno-Tools-Drivers", { isAdreno(it) }),
    KIMCHI("K11MCH1", "K11MCH1/AdrenoToolsDrivers", { isAdreno(it) }),
    CRUETER("crueter (8 Elite)", "crueter/GameHub-8Elite-Drivers", { isAdreno(it) }),
    ;

    val page: String get() = "https://github.com/$repo/releases"

    companion object {
        /** The sources for this GPU, the GPU maker's own first ("" when unknown: all of them). */
        fun forGpu(gpu: String): List<DriverSource> = if (gpu.isBlank()) entries.toList() else entries.filter { it.gpus(gpu) }
    }
}

fun isAdreno(gpu: String) = gpu.contains("Adreno", ignoreCase = true)

/** Mali-G52 (Bifrost, MaliSX2 0.2.0 on), G57, G68, G77, G78 (Valhall v9), G615, G715 (v11): the GPUs MaliSX2 is built for.
 *  No word boundary after the number so "G78AE" matches; a longer model number ("G5710") does not. */
private val MALISX2_GPUS = Regex("""\bG(?:52|57|68|77|78|615|715)(?!\d)""", RegexOption.IGNORE_CASE)
fun isMaliSx2Gpu(gpu: String) =
    (gpu.contains("Mali", ignoreCase = true) || gpu.contains("Immortalis", ignoreCase = true)) && MALISX2_GPUS.containsMatchIn(gpu)

object DriverSources {
    /** The newest releases of a source that have a driver package to download (GitHub's API, no login). */
    suspend fun releases(source: DriverSource, count: Int = 15): List<DriverRelease> {
        val a = JSONArray(Net.getText("https://api.github.com/repos/${source.repo}/releases?per_page=$count"))
        val out = mutableListOf<DriverRelease>()
        for (i in 0 until a.length()) {
            val r = a.optJSONObject(i) ?: continue
            if (r.optBoolean("draft")) continue
            val assets = mutableListOf<DriverAsset>()
            r.optJSONArray("assets")?.let { list ->
                for (j in 0 until list.length()) {
                    val x = list.optJSONObject(j) ?: continue
                    val name = x.optString("name")
                    if (!name.endsWith(".zip", true) && !name.endsWith(".adpkg", true)) continue
                    assets += DriverAsset(name, x.optString("browser_download_url"), x.optLong("size"))
                }
            }
            if (assets.isEmpty()) continue
            out += DriverRelease(
                title = r.optString("name").ifBlank { r.optString("tag_name") },
                tag = r.optString("tag_name"),
                date = r.optString("published_at").take(10),
                assets = assets,
            )
        }
        return out
    }
}
