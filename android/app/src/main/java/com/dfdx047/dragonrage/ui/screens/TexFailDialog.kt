package com.dfdx047.dragonrage.ui.screens

import android.content.Context
import android.content.Intent
import android.net.Uri
import androidx.compose.foundation.layout.Arrangement
import androidx.compose.foundation.layout.Column
import androidx.compose.material.icons.Icons
import androidx.compose.material.icons.rounded.BrokenImage
import androidx.compose.material.icons.rounded.ErrorOutline
import androidx.compose.material3.AlertDialog
import androidx.compose.material3.Button
import androidx.compose.material3.Icon
import androidx.compose.material3.MaterialTheme
import androidx.compose.material3.Text
import androidx.compose.material3.TextButton
import androidx.compose.runtime.Composable
import androidx.compose.runtime.getValue
import androidx.compose.ui.platform.LocalContext
import androidx.compose.ui.res.stringResource
import androidx.compose.ui.text.font.FontWeight
import androidx.compose.ui.unit.dp
import androidx.lifecycle.compose.collectAsStateWithLifecycle
import com.dfdx047.dragonrage.AppViewModel
import com.dfdx047.dragonrage.R

/** The share sheet for a report the app saved in Downloads (to send it on Discord, by mail, ...). */
internal fun shareLog(context: Context, uri: Uri) {
    val send = Intent(Intent.ACTION_SEND).apply {
        type = "text/plain"
        putExtra(Intent.EXTRA_STREAM, uri)
        addFlags(Intent.FLAG_GRANT_READ_URI_PERMISSION)
    }
    runCatching { context.startActivity(Intent.createChooser(send, context.getString(R.string.log_share)).addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)) }
}

/** "This texture pack could not be installed" (or the ISO, a driver, a mod, an update: `title`): why, and that the report is already in Downloads, to send (with no
 *  reason: a report saved from the Textures page's button). */
@Composable
fun TexFailDialog(vm: AppViewModel) {
    val f by vm.texFail.collectAsStateWithLifecycle()
    val fail = f ?: return
    val context = LocalContext.current
    AlertDialog(
        onDismissRequest = { vm.dismissTexFail() },
        icon = { Icon(if (fail.title == R.string.texfail_title) Icons.Rounded.BrokenImage else Icons.Rounded.ErrorOutline, null, tint = MaterialTheme.colorScheme.error) },
        title = { Text(stringResource(if (fail.reason.isEmpty()) R.string.log_saved_title else fail.title)) },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                if (fail.reason.isNotEmpty()) Text(fail.reason, style = MaterialTheme.typography.bodyMedium)
                SavedLogNote(fail.log)
            }
        },
        confirmButton = {
            Button(onClick = { fail.log?.let { shareLog(context, it.uri) } ?: vm.exportLog(); vm.dismissTexFail() }) {
                Text(stringResource(R.string.log_send))
            }
        },
        dismissButton = { TextButton(onClick = { vm.dismissTexFail() }) { Text(stringResource(R.string.close)) } },
    )
}

/** "The log was saved in Downloads as …; send it to the developer", or that it could not be saved. */
@Composable
internal fun SavedLogNote(log: AppViewModel.SavedLog?) {
    if (log != null) {
        Text(stringResource(R.string.log_auto_saved, log.name), style = MaterialTheme.typography.bodyMedium, fontWeight = FontWeight.SemiBold, color = MaterialTheme.colorScheme.primary)
    } else {
        Text(stringResource(R.string.log_auto_failed), style = MaterialTheme.typography.bodySmall, color = MaterialTheme.colorScheme.onSurfaceVariant)
    }
}

/** Play found older game data and its update failed (no network, a refused file): why, and "Play anyway" on the data
 *  there is (Home keeps its update button for later). */
@Composable
fun PlayAnywayDialog(vm: AppViewModel) {
    val w by vm.playAnyway.collectAsStateWithLifecycle()
    val why = w ?: return
    AlertDialog(
        onDismissRequest = { vm.dismissPlayAnyway() },
        icon = { Icon(Icons.Rounded.ErrorOutline, null, tint = MaterialTheme.colorScheme.error) },
        title = { Text("The game data could not be updated") },
        text = {
            Column(verticalArrangement = Arrangement.spacedBy(8.dp)) {
                Text(why, style = MaterialTheme.typography.bodyMedium)
                Text(
                    "The game needs the v${com.dfdx047.dragonrage.data.TagTeamV05Update.VERSION} game data for its newest fighters, " +
                        "stages and modes. You can play now on the data you have (v${vm.tagTeamV05.dataVersion()}) and update later " +
                        "from Home, or press Play again to retry.",
                    style = MaterialTheme.typography.bodySmall,
                    color = MaterialTheme.colorScheme.onSurfaceVariant,
                )
            }
        },
        confirmButton = { Button(onClick = { vm.playAnywayNow() }) { Text("Play anyway") } },
        dismissButton = { TextButton(onClick = { vm.dismissPlayAnyway() }) { Text(stringResource(R.string.close)) } },
    )
}
