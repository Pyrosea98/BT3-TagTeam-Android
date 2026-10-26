package com.dfdx047.dragonrage.data

import android.content.Context
import com.dfdx047.dragonrage.ui.theme.ThemeMode
import kotlinx.coroutines.flow.MutableStateFlow
import kotlinx.coroutines.flow.StateFlow
import kotlinx.coroutines.flow.asStateFlow

data class UiState(val theme: ThemeMode = ThemeMode.DARK, val dynamicColor: Boolean = false)

/** The launcher's own look, kept apart from the engine's settings. */
class UiPrefs private constructor(context: Context) {
    companion object {
        @Volatile private var one: UiPrefs? = null
        /** One for the whole app: the welcome screen and both games' pages see the same look. */
        fun get(context: Context): UiPrefs = one ?: synchronized(this) { one ?: UiPrefs(context.applicationContext).also { one = it } }
    }

    private val prefs = context.getSharedPreferences("ui", Context.MODE_PRIVATE)
    private val _state = MutableStateFlow(
        UiState(
            theme = runCatching { ThemeMode.valueOf(prefs.getString("theme", ThemeMode.DARK.name)!!) }.getOrDefault(ThemeMode.DARK),
            dynamicColor = prefs.getBoolean("dynamic", false),
        ),
    )
    val state: StateFlow<UiState> = _state.asStateFlow()

    fun setTheme(mode: ThemeMode) {
        prefs.edit().putString("theme", mode.name).apply()
        _state.value = _state.value.copy(theme = mode)
    }

    /** The game chosen last on the welcome screen (Game.id). */
    val game: Game get() = Game.of(prefs.getString("game", null))

    fun setGame(g: Game) { prefs.edit().putString("game", g.id).apply() }

    fun setDynamic(on: Boolean) {
        prefs.edit().putBoolean("dynamic", on).apply()
        _state.value = _state.value.copy(dynamicColor = on)
    }
}
