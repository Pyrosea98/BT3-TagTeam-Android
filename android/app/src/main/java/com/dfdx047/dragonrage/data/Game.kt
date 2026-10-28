package com.dfdx047.dragonrage.data

/**
 * The two games the launcher opens. Each has its own folder, disc image, saves and settings, and its own engine:
 *  - BT3: Budokai Tenkaichi 3 (USA, SLUS-21678), the Tenkaichi3Decomp port (libbt3.so), as always: its folder is the
 *    app's folder itself, so nothing of an existing install moves.
 *  - BT4: Budokai Tenkaichi 4, Team BT4's mod of BT3 (SLUS-219.78, Beta 14 Rev2), recompiled with PS2Recomp
 *    (recomp-code/dbz-bt4-recomp) and drawn by Dragon Rage's own GS (libbt4.so). In the folder bt4/.
 */
enum class Game(val id: String, val folder: String, val engine: String) {
    BT3("bt3", "", "bt3"),
    BT4("bt4", "bt4", "bt4");

    companion object {
        fun of(id: String?): Game = entries.firstOrNull { it.id == id } ?: BT3
    }
}
