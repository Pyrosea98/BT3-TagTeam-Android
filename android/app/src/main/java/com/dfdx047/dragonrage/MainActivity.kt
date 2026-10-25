package com.dfdx047.dragonrage

import androidx.compose.runtime.saveable.rememberSaveable
import androidx.compose.runtime.mutableStateOf
import androidx.compose.runtime.setValue
import com.dfdx047.dragonrage.data.Game
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2HomeScreen
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2TexturesScreen
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2ModsScreen
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2ControlsScreen
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2CheatsScreen
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2DriversScreen
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2DebugScreen
import com.dfdx047.dragonrage.ui.screens.bt2.Bt2SettingsScreen
import android.os.Bundle
import android.view.InputDevice
import android.view.KeyEvent
import android.view.MotionEvent
import androidx.activity.ComponentActivity
import androidx.activity.compose.setContent
import androidx.activity.enableEdgeToEdge
import androidx.activity.viewModels
import androidx.compose.foundation.background
import androidx.compose.foundation.layout.Box
import androidx.compose.foundation.layout.Column
import androidx.compose.foundation.layout.Row
import androidx.compose.foundation.layout.WindowInsets
import androidx.compose.foundation.layout.fillMaxSize
import androidx.compose.foundation.layout.padding
import androidx.compose.foundation.layout.safeDrawing
import androidx.compose.foundation.layout.widthIn
import androidx.compose.foundation.layout.windowInsetsPadding
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.BugReport
import androidx.compose.material.icons.rounded.DeveloperBoard
import androidx.compose.material.icons.rounded.Extension
import androidx.compose.material.icons.rounded.Home
import androidx.compose.material.icons.rounded.LockOpen
import androidx.compose.material.icons.rounded.Settings
import androidx.compose.material.icons.rounded.SportsEsports
import androidx.compose.material.icons.rounded.Texture
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.NavigationBar
import androidx.compose.material3.NavigationBarItem
import androidx.compose.material3.NavigationRail
import androidx.compose.material3.NavigationRailItem
import androidx.compose.material3.Scaffold
import androidx.compose.material3.SnackbarHost
import androidx.compose.material3.SnackbarHostState
import androidx.compose.material3.Text
import androidx.compose.runtime.Composable
import androidx.compose.runtime.LaunchedEffect
import androidx.compose.runtime.getValue
import androidx.compose.runtime.remember
import androidx.compose.ui.Alignment
import androidx.compose.ui.Modifier
import androidx.compose.ui.res.stringResource
import com.dfdx047.dragonrage.R
import androidx.compose.ui.graphics.Color
import androidx.compose.ui.graphics.vector.ImageVector
import androidx.compose.ui.platform.LocalConfiguration
import androidx.compose.ui.unit.dp
import androidx.lifecycle.Lifecycle
import androidx.lifecycle.compose.LifecycleEventEffect
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import androidx.navigation.NavGraph.Companion.findStartDestination
import androidx.navigation.compose.NavHost
import androidx.navigation.compose.composable
import androidx.navigation.compose.currentBackStackEntryAsState
import androidx.navigation.compose.rememberNavController
import com.dfdx047.dragonrage.ui.components.BusyBar
import com.dfdx047.dragonrage.ui.components.kiBackground
import com.dfdx047.dragonrage.ui.screens.CheatsScreen
import com.dfdx047.dragonrage.ui.screens.ControlsScreen
import com.dfdx047.dragonrage.ui.screens.HomeScreen
import com.dfdx047.dragonrage.ui.screens.ModsScreen
import com.dfdx047.dragonrage.ui.screens.SettingsScreen
import com.dfdx047.dragonrage.ui.screens.TexturesScreen
import com.dfdx047.dragonrage.ui.screens.UpdateDialog
import com.dfdx047.dragonrage.ui.screens.DebugScreen
import com.dfdx047.dragonrage.ui.screens.DriversScreen
import com.dfdx047.dragonrage.ui.screens.GpuOfferDialog
import com.dfdx047.dragonrage.ui.screens.LastRunDialog
import com.dfdx047.dragonrage.ui.screens.TexFailDialog
import androidx.compose.foundation.clickable
import androidx.compose.foundation.horizontalScroll
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.fillMaxWidth
import androidx.compose.foundation.layout.navigationBars
import androidx.compose.foundation.layout.size
import androidx.compose.foundation.layout.width
import androidx.compose.foundation.rememberScrollState
import androidx.compose.foundation.shape.RoundedCornerShape
import androidx.compose.material3.Surface
import androidx.compose.ui.draw.clip
import androidx.compose.ui.text.style.TextOverflow
import com.dfdx047.dragonrage.ui.theme.DragonRageTheme

class MainActivity : ComponentActivity() {
    private val vm: AppViewModel by viewModels()
    private val bt2: Bt2ViewModel by viewModels()

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        enableEdgeToEdge()
        setContent {
            val ui by vm.uiState.collectAsStateWithLifecycle()
            DragonRageTheme(mode = ui.theme, dynamicColor = ui.dynamicColor) {
                DragonRageApp(vm, bt2)
            }
        }
    }

    // A controller (Bluetooth or USB) is the game's only, as a PS2 pad: the launcher ignores it (touch drives it).
    private fun fromController(source: Int): Boolean =
        source and InputDevice.SOURCE_GAMEPAD == InputDevice.SOURCE_GAMEPAD ||
            source and InputDevice.SOURCE_JOYSTICK == InputDevice.SOURCE_JOYSTICK

    override fun dispatchKeyEvent(event: KeyEvent): Boolean =
        if (KeyEvent.isGamepadButton(event.keyCode) || fromController(event.source)) true else super.dispatchKeyEvent(event)

    override fun dispatchGenericMotionEvent(ev: MotionEvent): Boolean =
        if (fromController(ev.source)) true else super.dispatchGenericMotionEvent(ev)
}

private enum class Dest(val route: String, val label: Int, val icon: ImageVector) {
    HOME("home", R.string.nav_home, Icons.Rounded.Home),
    TEXTURES("textures", R.string.nav_textures, Icons.Rounded.Texture),
    MODS("mods", R.string.nav_mods, Icons.Rounded.Extension),
    CONTROLS("controls", R.string.nav_controls, Icons.Rounded.SportsEsports),
    CHEATS("cheats", R.string.nav_cheats, Icons.Rounded.LockOpen),
    DRIVERS("drivers", R.string.nav_drivers, Icons.Rounded.DeveloperBoard),
    DEBUG("debug", R.string.nav_debug, Icons.Rounded.BugReport),
    SETTINGS("settings", R.string.nav_settings, Icons.Rounded.Settings),
}

@Composable
private fun DragonRageApp(vm: AppViewModel, bt2: Bt2ViewModel) {
    val nav = rememberNavController()
    val entry by nav.currentBackStackEntryAsState()
    val current = entry?.destination?.route
    val snackbar = remember { SnackbarHostState() }
    val busy by vm.busy.collectAsStateWithLifecycle()
    val wide = LocalConfiguration.current.screenWidthDp >= 600

    LaunchedEffect(Unit) { vm.messages.collect { snackbar.showSnackbar(it) } }
    LaunchedEffect(Unit) { bt2.messages.collect { snackbar.showSnackbar(it) } }
    // Which game's pages are shown (the switch at the top): BT3's are the app's own pages, as they always were.
    var game by rememberSaveable { mutableStateOf(Game.BT3) }
    // files may have been changed from outside (file manager, adb) while the app was in the background
    LifecycleEventEffect(Lifecycle.Event.ON_RESUME) { vm.refreshAll(); vm.resumeUpdate(); vm.checkLastRun(); bt2.refresh() }
    UpdateDialog(vm)
    GpuOfferDialog(vm)
    LastRunDialog(vm)
    TexFailDialog(vm)

    fun go(d: Dest) = nav.navigate(d.route) {
        popUpTo(nav.graph.findStartDestination().id) { saveState = true }
        launchSingleTop = true
        restoreState = true
    }

    val colors = MaterialTheme.colorScheme
    Row(Modifier.fillMaxSize().background(colors.background).kiBackground(colors.primary, colors.secondary)) {
        if (wide) {
            NavigationRail(containerColor = Color.Transparent, modifier = Modifier.windowInsetsPadding(WindowInsets.safeDrawing)) {
                Dest.entries.forEach { d ->
                    NavigationRailItem(
                        selected = current == d.route, onClick = { go(d) },
                        icon = { Icon(d.icon, null) }, label = { Text(stringResource(d.label)) },
                    )
                }
            }
        }
        Scaffold(
            containerColor = Color.Transparent,
            snackbarHost = { SnackbarHost(snackbar) },
            bottomBar = {
                Column {
                    BusyBar(busy)
                    if (!wide) {
                        // eight pages: a bar that scrolls sideways when the screen is too narrow for all of them
                        Surface(color = colors.surfaceContainer.copy(alpha = 0.94f)) {
                            Row(
                                Modifier.fillMaxWidth().horizontalScroll(rememberScrollState())
                                    .windowInsetsPadding(WindowInsets.navigationBars).padding(horizontal = 4.dp, vertical = 6.dp),
                                horizontalArrangement = Arrangement.SpaceEvenly,
                            ) {
                                Dest.entries.forEach { d -> BarItem(d, current == d.route) { go(d) } }
                            }
                        }
                    }
                }
            },
        ) { pad ->
            Column(Modifier.fillMaxSize().padding(pad), horizontalAlignment = Alignment.CenterHorizontally) {
                // always shown: without BT2's engine in the APK (the public builds) its tab is where it is imported
                GameSwitch(game) { game = it }
                NavHost(
                    nav, startDestination = Dest.HOME.route,
                    modifier = Modifier.widthIn(max = 760.dp).fillMaxSize(),
                ) {
                    // BT3: the app's pages as they were. BT2: its own (ui/screens/bt2); what is set there is BT2's alone.
                    composable(Dest.HOME.route) { if (game == Game.BT3) HomeScreen(vm) else Bt2HomeScreen(bt2) }
                    composable(Dest.TEXTURES.route) { if (game == Game.BT3) TexturesScreen(vm) else Bt2TexturesScreen(bt2) }
                    composable(Dest.MODS.route) { if (game == Game.BT3) ModsScreen(vm) else Bt2ModsScreen() }
                    composable(Dest.CONTROLS.route) { if (game == Game.BT3) ControlsScreen(vm) else Bt2ControlsScreen(bt2) }
                    composable(Dest.CHEATS.route) { if (game == Game.BT3) CheatsScreen(vm) else Bt2CheatsScreen(bt2) }
                    composable(Dest.DRIVERS.route) { if (game == Game.BT3) DriversScreen(vm) else Bt2DriversScreen(bt2) }
                    composable(Dest.DEBUG.route) { if (game == Game.BT3) DebugScreen(vm) else Bt2DebugScreen(bt2) }
                    composable(Dest.SETTINGS.route) { if (game == Game.BT3) SettingsScreen(vm) else Bt2SettingsScreen(bt2) }
                }
            }
        }
    }
}

/** The game whose pages are shown: BT3 (the app's own pages) or BT2 (ui/screens/bt2). */
@Composable
private fun GameSwitch(game: Game, onChange: (Game) -> Unit) {
    Row(
        Modifier.padding(top = 6.dp, bottom = 2.dp).clip(RoundedCornerShape(20.dp))
            .background(MaterialTheme.colorScheme.surfaceContainer.copy(alpha = 0.9f)).padding(4.dp),
        horizontalArrangement = Arrangement.spacedBy(4.dp),
    ) {
        listOf(Game.BT3 to R.string.game_tab_bt3, Game.BT2 to R.string.game_tab_bt2).forEach { (g, label) ->
            val selected = g == game
            Text(
                stringResource(label),
                style = MaterialTheme.typography.labelLarge,
                color = if (selected) MaterialTheme.colorScheme.onPrimary else MaterialTheme.colorScheme.onSurfaceVariant,
                modifier = Modifier.clip(RoundedCornerShape(16.dp))
                    .background(if (selected) MaterialTheme.colorScheme.primary else Color.Transparent)
                    .clickable { onChange(g) }.padding(horizontal = 22.dp, vertical = 8.dp),
            )
        }
    }
}

/** One page of the bottom bar: its icon in a pill that lights when it is the page shown, and its name. */
@Composable
private fun BarItem(d: Dest, selected: Boolean, onClick: () -> Unit) {
    val colors = MaterialTheme.colorScheme
    Column(
        Modifier.width(76.dp).clip(RoundedCornerShape(14.dp)).clickable(onClick = onClick).padding(vertical = 4.dp),
        horizontalAlignment = Alignment.CenterHorizontally,
    ) {
        Box(
            Modifier.clip(RoundedCornerShape(16.dp))
                .background(if (selected) colors.primary.copy(alpha = 0.22f) else Color.Transparent)
                .padding(horizontal = 16.dp, vertical = 4.dp),
        ) {
            Icon(d.icon, null, tint = if (selected) colors.primary else colors.onSurfaceVariant, modifier = Modifier.size(24.dp))
        }
        Text(
            stringResource(d.label), maxLines = 1, overflow = TextOverflow.Ellipsis,
            style = MaterialTheme.typography.labelSmall, color = if (selected) colors.primary else colors.onSurfaceVariant,
        )
    }
}
