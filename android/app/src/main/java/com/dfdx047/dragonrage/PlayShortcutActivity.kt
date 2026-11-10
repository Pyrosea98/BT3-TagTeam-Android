package com.dfdx047.dragonrage

import android.app.Activity
import android.content.Intent
import android.os.Bundle
import com.dfdx047.dragonrage.data.GamePaths
import com.dfdx047.dragonrage.engine.EngineBridge
import com.dfdx047.dragonrage.engine.GameActivity

/**
 * The target of the "Play" shortcuts (the icon on the home screen, and the one in the app icon's long-press menu):
 * it shows nothing and starts the game at once. If the game is not installed yet, the launcher opens instead, so
 * that the person sees what is missing.
 */
class PlayShortcutActivity : Activity() {
    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        val paths = GamePaths(this)
        val ready = paths.installedMarker.exists() && EngineBridge.isAvailable(this)
        // older game data than the app's: the app opens and its Play updates the data first (with its progress)
        val update = ready && com.dfdx047.dragonrage.data.TagTeamV05Update(paths).needed()
        val target = if (ready && !update) GameActivity::class.java else MainActivity::class.java
        startActivity(Intent(this, target).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK).putExtra(MainActivity.EXTRA_PLAY, update))
        finish()
    }
}
