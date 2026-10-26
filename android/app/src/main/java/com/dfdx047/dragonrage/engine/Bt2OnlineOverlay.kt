package com.dfdx047.dragonrage.engine

import android.app.Activity
import android.app.AlertDialog
import android.content.ClipData
import android.content.ClipboardManager
import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Handler
import android.os.Looper
import android.text.InputFilter
import android.text.InputType
import android.view.Gravity
import android.view.ViewGroup
import android.view.inputmethod.EditorInfo
import android.view.inputmethod.InputMethodManager
import android.widget.EditText
import android.widget.LinearLayout
import android.widget.ProgressBar
import android.widget.TextView
import android.widget.Toast
import com.dfdx047.dragonrage.data.Bt2Online
import com.dfdx047.dragonrage.data.GamePaths
import java.io.File
import java.util.UUID

/**
 * What the app puts over BT2's "Online Mode" screens, which the engine draws (bt2/runtime/game/main_menu_item.cpp).
 * Everything that is not the game's own picture is the app's: the player's online name (a box at the top), the room
 * code, the search, the opponent's name, and the network itself (Bt2Online).
 *
 * The engine says what is open in its running counts (Bt2Bridge.nativeStats, the fourth): 0 nothing, 1 Online or
 * Ranked, 2 the three ways in, 11 / 12 / 13 the app is to create a room / join one / find an opponent, 14 someone is
 * there. The app answers through files in <root>/bt2/user: online_peer (the opponent's name) when someone is there,
 * online_back when the engine should go back to the three ways in (a code that was not entered).
 */
class Bt2OnlineOverlay(private val activity: Activity, private val paths: GamePaths) {
    private val handler = Handler(Looper.getMainLooper())
    private val density = activity.resources.displayMetrics.density
    private val dir = File(paths.bt2Root, "user")
    private val file = File(dir, "online_name")
    private val who = UUID.randomUUID().toString().replace("-", "").take(16)

    private val box = EditText(activity).apply {
        hint = "Seu nome online"
        setSingleLine()
        // (what the service takes: printable ASCII, 16 letters)
        filters = arrayOf(InputFilter { s, a, b, _, _, _ -> s.subSequence(a, b).filter { it.code in 0x20..0x7e } }, InputFilter.LengthFilter(MAX_NAME))
        inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS or InputType.TYPE_TEXT_VARIATION_PERSON_NAME
        imeOptions = EditorInfo.IME_ACTION_DONE or EditorInfo.IME_FLAG_NO_EXTRACT_UI or EditorInfo.IME_FLAG_NO_FULLSCREEN
        typeface = Typeface.DEFAULT_BOLD
        textSize = 14f
        setTextColor(Color.WHITE)
        setHintTextColor(Color.argb(170, 190, 196, 232))
        val p = (10 * density).toInt()
        setPadding(p, p / 2, p, p / 2)
        background = rounded(Color.argb(215, 10, 14, 64), 18f)
        setOnEditorActionListener { _, action, _ -> (action == EditorInfo.IME_ACTION_DONE).also { if (it) commit() } }
        setOnFocusChangeListener { _, focus -> if (!focus) save() }
    }

    private val title = text(18f, Color.WHITE)
    private val code = text(40f, Color.rgb(255, 214, 64)).apply { typeface = Typeface.MONOSPACE; letterSpacing = 0.15f }
    private val detail = text(15f, Color.rgb(205, 210, 240))
    private val spinner = ProgressBar(activity).apply { isIndeterminate = true }
    private val panel = LinearLayout(activity).apply {
        orientation = LinearLayout.VERTICAL
        gravity = Gravity.CENTER_HORIZONTAL
        val p = (16 * density).toInt()
        setPadding(p, p, p, p)
        background = rounded(Color.argb(205, 14, 18, 72), 22f)
        addView(title)
        addView(code)
        addView(spinner, LinearLayout.LayoutParams((36 * density).toInt(), (36 * density).toInt()).apply { topMargin = (6 * density).toInt() })
        addView(detail)
        setOnClickListener {
            val text = code.text.toString()
            if (text.isNotEmpty()) {
                (activity.getSystemService(Activity.CLIPBOARD_SERVICE) as? ClipboardManager)?.setPrimaryClip(ClipData.newPlainText("sala", text))
                Toast.makeText(activity, "Código copiado", Toast.LENGTH_SHORT).show()
            }
        }
    }

    /** One try at a way in (create, join or find): its thread, and the room it holds. */
    private inner class Session(val kind: Int) {
        @Volatile var cancelled = false
        @Volatile var room: Bt2Online.Room? = null
        val base = Bt2Online.url(paths)
    }

    private var layout: ViewGroup? = null
    private var session: Session? = null
    private var asking: AlertDialog? = null
    private val tick = object : Runnable {
        override fun run() {
            update()
            handler.postDelayed(this, 250)
        }
    }

    /** The name the player chose, or null when none was typed yet. */
    fun name(): String? = runCatching { file.readText().trim() }.getOrNull()?.takeIf { it.isNotEmpty() }

    fun start(into: ViewGroup) {
        layout = into
        if (box.text.isEmpty()) box.setText(name().orEmpty())
        handler.removeCallbacks(tick)
        handler.post(tick)
    }

    fun stop() {
        handler.removeCallbacks(tick)
        endSession()
        hideBox()
    }

    private fun update() {
        val state = runCatching { Bt2Bridge.nativeStats().getOrElse(3) { 0L } }.getOrDefault(0L).toInt()
        when (state) {
            1, 2 -> { endSession(); showBox() }
            11, 12, 13 -> { hideBox(); if (session?.kind != state - 10) begin(state - 10) }
            14 -> hideBox() // someone is there: the session says so
            else -> { endSession(); hideBox() }
        }
    }

    // ---- the ways in

    private fun begin(kind: Int) {
        endSession()
        commit()
        val s = Session(kind).also { session = it }
        showPanel()
        when (kind) {
            1 -> say("Criando a sala…", spinner = true)
            2 -> say("Entrar em uma sala", detail = "Digite o código que seu amigo passou.")
            else -> say("Procurando um oponente…", detail = "Quem estiver procurando com o mesmo jogo aparece aqui.", spinner = true)
        }
        if (kind == 2) {
            ask(s)
        } else {
            Thread { runCatching { work(s) } }.start()
        }
    }

    /** The room code, asked on the screen: what the player types goes to the service. */
    private fun ask(s: Session) {
        val edit = EditText(activity).apply {
            hint = "Código da sala"
            setSingleLine()
            filters = arrayOf(InputFilter.AllCaps(), InputFilter.LengthFilter(8))
            inputType = InputType.TYPE_CLASS_TEXT or InputType.TYPE_TEXT_FLAG_NO_SUGGESTIONS or InputType.TYPE_TEXT_FLAG_CAP_CHARACTERS
            imeOptions = EditorInfo.IME_ACTION_DONE or EditorInfo.IME_FLAG_NO_EXTRACT_UI
            typeface = Typeface.MONOSPACE
            textSize = 24f
            gravity = Gravity.CENTER
        }
        fun go() {
            val typed = edit.text.toString().trim()
            asking = null
            if (typed.isEmpty() || s.cancelled) return back()
            say("Entrando na sala $typed…", spinner = true)
            Thread { runCatching { work(s, typed) } }.start()
        }
        asking = AlertDialog.Builder(activity).setTitle("Entrar em uma sala").setView(edit)
            .setPositiveButton("Entrar") { _, _ -> go() }
            .setNegativeButton("Cancelar") { _, _ -> asking = null; back() }
            .setOnCancelListener { asking = null; back() }
            .create().also {
                it.show()
                edit.requestFocus()
                it.window?.setSoftInputMode(android.view.WindowManager.LayoutParams.SOFT_INPUT_STATE_VISIBLE)
            }
    }

    /** On the worker thread: the request, then the wait for the other player. */
    private fun work(s: Session, typedCode: String? = null) {
        val name = name() ?: ""
        try {
            val first = when (s.kind) {
                1 -> Bt2Online.host(s.base, name)
                2 -> Bt2Online.join(s.base, typedCode ?: return, name)
                else -> Bt2Online.quick(s.base, name, who)
            }
            s.room = first
            if (s.cancelled) return
            first.peer?.let { return connected(s, it) }
            if (s.kind == 1) say("Sala criada!", code = first.code, detail = "Passe este código para um amigo (toque para copiar).\nAguardando ele entrar…", spinner = true)
            var room = first
            var since = System.currentTimeMillis()
            while (!s.cancelled) {
                Thread.sleep(1000)
                Bt2Online.poll(s.base, room)?.let { return connected(s, it) }
                // (a room the service keeps ten minutes: a search that waits longer asks again)
                if (s.kind == 3 && System.currentTimeMillis() - since > 9 * 60_000) {
                    Bt2Online.leave(s.base, room)
                    room = Bt2Online.quick(s.base, name, who).also { s.room = it }
                    since = System.currentTimeMillis()
                    room.peer?.let { return connected(s, it) }
                }
            }
        } catch (e: Bt2Online.Failure) {
            if (!s.cancelled) say(Bt2Online.explain(e.why), detail = "Aperte △ para voltar.", error = true)
        }
    }

    private fun connected(s: Session, peer: Bt2Online.Peer) {
        if (s.cancelled) return
        runCatching { dir.mkdirs(); File(dir, "online_peer").writeText(peer.name) } // the engine takes it from here
        say("Conectado!", code = "", detail = "Oponente: ${peer.name}")
    }

    private fun endSession() {
        asking?.dismiss()
        asking = null
        val s = session ?: return
        s.cancelled = true
        session = null
        s.room?.let { room -> Thread { Bt2Online.leave(s.base, room) }.start() }
        handler.post { hidePanel() }
        runCatching { File(dir, "online_peer").delete() }
    }

    /** The engine goes back to the three ways in (a code that was not entered). */
    private fun back() {
        runCatching { dir.mkdirs(); File(dir, "online_back").writeText("1") }
    }

    // ---- what is shown

    private fun say(title: String, code: String = "", detail: String = "", spinner: Boolean = false, error: Boolean = false) {
        handler.post {
            this.title.text = title
            this.title.setTextColor(if (error) Color.rgb(255, 150, 140) else Color.WHITE)
            this.code.text = code
            this.code.visibility = if (code.isEmpty()) android.view.View.GONE else android.view.View.VISIBLE
            this.detail.text = detail
            this.detail.visibility = if (detail.isEmpty()) android.view.View.GONE else android.view.View.VISIBLE
            this.spinner.visibility = if (spinner) android.view.View.VISIBLE else android.view.View.GONE
        }
    }

    private fun showPanel() {
        val into = layout ?: return
        if (panel.parent != null) return
        val m = activity.resources.displayMetrics
        into.addView(panel, ViewGroup.MarginLayoutParams((m.widthPixels * 0.46f).toInt(), ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            leftMargin = (m.widthPixels * 0.07f).toInt()
            topMargin = (m.heightPixels * 0.30f).toInt()
        })
    }

    private fun hidePanel() {
        (panel.parent as? ViewGroup)?.removeView(panel)
    }

    private fun showBox() {
        val into = layout ?: return
        if (box.parent != null) return
        val width = activity.resources.displayMetrics.widthPixels
        // (the upper right corner; the controller's R1 and R2 are not used on these screens)
        into.addView(box, ViewGroup.MarginLayoutParams((width * 0.21f).toInt(), ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            leftMargin = (width * 0.77f).toInt()
            topMargin = (6 * density).toInt()
        })
    }

    private fun hideBox() {
        if (box.parent == null) return
        commit()
        (box.parent as? ViewGroup)?.removeView(box)
    }

    /** Done: keep the name, close the keyboard and give the game its keys back. */
    private fun commit() {
        save()
        box.clearFocus()
        (activity.getSystemService(Activity.INPUT_METHOD_SERVICE) as? InputMethodManager)?.hideSoftInputFromWindow(box.windowToken, 0)
        layout?.requestFocus()
    }

    private fun save() {
        val text = box.text.toString().trim()
        if (text == name().orEmpty()) return
        runCatching {
            file.parentFile?.mkdirs()
            file.writeText(text)
        }
    }

    private fun text(sp: Float, color: Int) = TextView(activity).apply {
        textSize = sp
        setTextColor(color)
        typeface = Typeface.DEFAULT_BOLD
        gravity = Gravity.CENTER
        setShadowLayer(3f, 1f, 1f, Color.BLACK)
    }

    private fun rounded(color: Int, radius: Float) = GradientDrawable().apply {
        setColor(color)
        setStroke((2 * density).toInt(), Color.rgb(110, 150, 255))
        cornerRadius = radius * density
    }

    private companion object {
        const val MAX_NAME = 16
    }
}
