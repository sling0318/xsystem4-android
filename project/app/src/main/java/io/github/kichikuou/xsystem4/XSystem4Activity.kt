package io.github.kichikuou.xsystem4

import android.app.AlertDialog
import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.Process
import android.os.SystemClock
import android.view.Gravity
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import org.libsdl.app.SDLActivity
import java.io.File
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

// Intent for this activity must have the following extras:
// - EXTRA_GAME_ROOT (string): A path to the game installation.
// - EXTRA_SAVE_DIR (string): A path to a directory where save files are stored.
class XSystem4Activity : SDLActivity() {
    companion object {
        const val EXTRA_GAME_ROOT = "GAME_ROOT"
        const val EXTRA_SAVE_DIR = "SAVE_DIR"
        const val COMMAND_OPEN_PLAYING_MANUAL = 0x8000  // xsystem4/src/hll/SystemService.c

        private const val ACTION_OPEN_SAVE = 1
        private const val ACTION_OPEN_LOAD = 2
        private const val ACTION_RIGHT_CLICK = 3
        private const val SLOT_COUNT = 12
    }

    private var lastGameActionAt = 0L

    private external fun nativeQueueGameAction(action: Int, slot: Int)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Workaround for https://github.com/libsdl-org/SDL/issues/8995
        SDLActivity.setWindowStyle(true)
        setupSaveLoadOverlay()
    }

    private fun dp(value: Int) = (value * resources.displayMetrics.density).toInt()

    private fun saveDir(): File =
        File(intent.getStringExtra(EXTRA_SAVE_DIR) ?: filesDir.absolutePath)

    // Direct VM snapshots are not interchangeable with native game saves, so
    // they live under their own names and never appear in the game's load UI.
    private fun slotFile(slot: Int) = File(saveDir(), "Quick${'A' + slot}.asd")

    private fun setupSaveLoadOverlay() {
        val root = findViewById<ViewGroup>(android.R.id.content) ?: return

        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
        }
        panel.addView(createActionButton("存档") { showSlotPicker(save = true) })
        panel.addView(createActionButton("读档") { showSlotPicker(save = false) })
        panel.addView(createActionButton("返回") { triggerGameAction(ACTION_RIGHT_CLICK, 0) })

        val lp = FrameLayout.LayoutParams(
            dp(62), FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.END or Gravity.CENTER_VERTICAL
        ).apply { rightMargin = dp(12) }
        root.addView(panel, lp)
    }

    private fun createActionButton(text: String, onClick: () -> Unit): TextView {
        return TextView(this).apply {
            this.text = text
            gravity = Gravity.CENTER
            setTextColor(Color.WHITE)
            textSize = 14f
            background = GradientDrawable().apply {
                cornerRadius = dp(6).toFloat()
                setColor(Color.argb(150, 0, 0, 0))
                setStroke(dp(1), Color.argb(120, 255, 255, 255))
            }
            layoutParams = LinearLayout.LayoutParams(dp(62), dp(36))
            setOnClickListener { onClick() }
        }
    }

    private fun slotLabel(slot: Int): String {
        val name = "${'A' + slot} 槽"
        val file = slotFile(slot)
        if (!file.exists())
            return "$name    空"
        val date = SimpleDateFormat("MM-dd HH:mm", Locale.getDefault()).format(Date(file.lastModified()))
        return "$name    $date"
    }

    private fun showSlotPicker(save: Boolean) {
        val labels = Array(SLOT_COUNT) { slotLabel(it) }
        AlertDialog.Builder(this)
            .setTitle(if (save) "存档到" else "读取存档")
            .setItems(labels) { _, slot ->
                if (save) {
                    if (slotFile(slot).exists()) {
                        AlertDialog.Builder(this)
                            .setMessage("覆盖 ${'A' + slot} 槽的存档？")
                            .setPositiveButton("覆盖") { _, _ -> commitSlot(save = true, slot) }
                            .setNegativeButton("取消", null)
                            .show()
                    } else {
                        commitSlot(save = true, slot)
                    }
                } else {
                    if (slotFile(slot).exists())
                        commitSlot(save = false, slot)
                    else
                        Toast.makeText(this, "${'A' + slot} 槽是空的", Toast.LENGTH_SHORT).show()
                }
            }
            .setNegativeButton("取消", null)
            .show()
    }

    private fun commitSlot(save: Boolean, slot: Int) {
        triggerGameAction(if (save) ACTION_OPEN_SAVE else ACTION_OPEN_LOAD, slot)
        Toast.makeText(
            this,
            if (save) "已存档到 ${'A' + slot} 槽" else "正在读取 ${'A' + slot} 槽",
            Toast.LENGTH_SHORT
        ).show()
    }

    private fun triggerGameAction(action: Int, slot: Int) {
        val now = SystemClock.uptimeMillis()
        if (now - lastGameActionAt < 500)
            return
        lastGameActionAt = now
        nativeQueueGameAction(action, slot)
    }

    override fun getLibraries(): Array<String> {
        return arrayOf("SDL2", "xsystem4")
    }

    override fun getArguments(): Array<String> {
        val saveFolder = intent.getStringExtra(EXTRA_SAVE_DIR)!!
        val gameRoot = intent.getStringExtra(EXTRA_GAME_ROOT)!!
        val args = mutableListOf("--save-folder", saveFolder, "--save-format=rsm", gameRoot)
        // Chinese-patched game data (GBK-encoded messages): opt in via a marker
        // file in the game directory, together with a local CJK font.
        if (java.io.File(gameRoot, ".text-gbk").exists()) {
            val font = java.io.File(gameRoot, "font-cn.ttf")
            if (font.exists()) {
                args.addAll(arrayOf("--font-gothic", font.path, "--font-mincho", font.path))
            }
            args.addAll(arrayOf("--text-code", "gbk", "--no-fnl"))
        }
        return args.toTypedArray()
    }

    override fun onDestroy() {
        try {
            // Let SDL stop and join its native thread before terminating the game process.
            super.onDestroy()
        } finally {
            Process.killProcess(Process.myPid())
        }
    }

    override fun onUnhandledMessage(command: Int, param: Any): Boolean {
        when (command) {
            COMMAND_OPEN_PLAYING_MANUAL -> {
                val gameRoot = intent.getStringExtra(EXTRA_GAME_ROOT)!!
                val manualDir = java.io.File(gameRoot, "Manual")
                if (manualDir.isDirectory) {
                    val intent = android.content.Intent(this, ManualActivity::class.java).apply {
                        val url = "file://${manualDir.absolutePath}/index.html"
                        putExtra(ManualActivity.EXTRA_URL, url)
                    }
                    startActivity(intent)
                }
                return true
            }
        }
        return super.onUnhandledMessage(command, param)
    }
}
