package io.github.kichikuou.xsystem4

import android.graphics.Color
import android.graphics.Typeface
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.Process
import android.os.SystemClock
import android.text.InputType
import android.view.Gravity
import android.view.ViewGroup
import android.widget.EditText
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.ScrollView
import android.widget.TextView
import android.widget.Toast
import org.libsdl.app.SDLActivity

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

        // Sentinel returned by nativeCheatValue when the addressed variable
        // can't be located right now (game not running / mid-load).
        private const val CHEAT_INVALID_VALUE = Long.MIN_VALUE
    }

    private var lastGameActionAt = 0L

    private external fun nativeQueueGameAction(action: Int)
    private external fun nativeCheatCount(): Int
    private external fun nativeCheatLabel(index: Int): String?
    private external fun nativeCheatValue(index: Int): Long
    private external fun nativeCheatSet(index: Int, value: Int): Boolean

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Workaround for https://github.com/libsdl-org/SDL/issues/8995
        SDLActivity.setWindowStyle(true)
        setupSaveLoadOverlay()
    }

    private fun dp(value: Int) = (value * resources.displayMetrics.density).toInt()

    private fun setupSaveLoadOverlay() {
        val root = findViewById<ViewGroup>(android.R.id.content) ?: return

        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
        }
        panel.addView(createActionButton("存档") { triggerGameAction(ACTION_OPEN_SAVE) })
        panel.addView(createActionButton("读档") { triggerGameAction(ACTION_OPEN_LOAD) },
            LinearLayout.LayoutParams(dp(62), dp(36)).apply { topMargin = dp(8) })
        panel.addView(createActionButton("修改") { showCheatDialog() },
            LinearLayout.LayoutParams(dp(62), dp(36)).apply { topMargin = dp(8) })

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

    private fun triggerGameAction(action: Int) {
        val now = SystemClock.uptimeMillis()
        if (now - lastGameActionAt < 500)
            return
        lastGameActionAt = now
        nativeQueueGameAction(action)
    }

    // ---------------------------------------------------------------- cheat

    private class CheatRow(val index: Int, val valueView: TextView, val input: EditText)

    private val cheatRows = mutableListOf<CheatRow>()

    private fun showCheatDialog() {
        val count = nativeCheatCount()

        val content = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            val pad = dp(16)
            setPadding(pad, dp(12), pad, dp(4))
        }

        // Own title row: the platform dialog title doesn't play well with the
        // dark custom background.
        content.addView(TextView(this).apply {
            text = "修改器"
            setTextColor(Color.WHITE)
            textSize = 17f
        })
        content.addView(TextView(this).apply {
            text = "修改立即生效；存档后保留，读档后以存档内容为准。"
            setTextColor(Color.argb(190, 255, 255, 255))
            textSize = 12f
            setPadding(0, dp(4), 0, 0)
        })

        if (count == 0) {
            content.addView(TextView(this).apply {
                text = "游戏尚未加载完成，或当前游戏不支持修改器。"
                setTextColor(Color.WHITE)
                textSize = 14f
                setPadding(0, dp(20), 0, dp(12))
            })
        } else {
            val list = LinearLayout(this).apply {
                orientation = LinearLayout.VERTICAL
                setPadding(0, dp(8), 0, 0)
            }
            cheatRows.clear()
            for (i in 0 until count) {
                val row = buildCheatRow(i)
                cheatRows.add(row)
                list.addView(makeRowContainer(row))
            }
            content.addView(ScrollView(this).apply { addView(list) },
                LinearLayout.LayoutParams(ViewGroup.LayoutParams.MATCH_PARENT, dp(380)))
            content.addView(makeCheatButton("刷新当前值") { refreshCheatValues() },
                LinearLayout.LayoutParams(dp(120), dp(36)).apply { topMargin = dp(8) })
        }

        val dialog = android.app.AlertDialog.Builder(this)
            .setView(content)
            .setPositiveButton("关闭", null)
            .create()
        dialog.window?.setBackgroundDrawable(GradientDrawable().apply {
            cornerRadius = dp(10).toFloat()
            setColor(Color.argb(235, 24, 24, 28))
        })
        dialog.show()
        if (count > 0)
            refreshCheatValues()
    }

    private fun makeRowContainer(row: CheatRow): LinearLayout {
        val container = LinearLayout(this).apply {
            orientation = LinearLayout.HORIZONTAL
            gravity = Gravity.CENTER_VERTICAL
            setPadding(0, dp(4), 0, dp(4))
        }
        container.addView(TextView(this).apply {
            text = nativeCheatLabel(row.index) ?: "?"
            setTextColor(Color.WHITE)
            textSize = 14f
        }, LinearLayout.LayoutParams(0, ViewGroup.LayoutParams.WRAP_CONTENT, 1f))
        container.addView(row.valueView, LinearLayout.LayoutParams(
            ViewGroup.LayoutParams.WRAP_CONTENT, ViewGroup.LayoutParams.WRAP_CONTENT).apply {
            marginEnd = dp(8)
        })
        container.addView(row.input, LinearLayout.LayoutParams(
            dp(96), ViewGroup.LayoutParams.WRAP_CONTENT).apply { marginEnd = dp(6) })
        container.addView(makeCheatButton("设为") { onCheatSetClicked(row) },
            LinearLayout.LayoutParams(dp(56), dp(40)))
        return container
    }

    private fun buildCheatRow(index: Int): CheatRow {
        val valueView = TextView(this).apply {
            gravity = Gravity.END
            setTextColor(Color.argb(220, 160, 220, 160))
            textSize = 14f
            typeface = Typeface.MONOSPACE
            minWidth = dp(76)
        }
        val input = EditText(this).apply {
            inputType = InputType.TYPE_CLASS_NUMBER or InputType.TYPE_NUMBER_FLAG_SIGNED
            setTextColor(Color.WHITE)
            setHintTextColor(Color.argb(120, 255, 255, 255))
            hint = "新值"
            textSize = 13f
            isSingleLine = true
            background = GradientDrawable().apply {
                cornerRadius = dp(4).toFloat()
                setColor(Color.argb(80, 0, 0, 0))
                setStroke(dp(1), Color.argb(90, 255, 255, 255))
            }
        }
        return CheatRow(index, valueView, input)
    }

    private fun makeCheatButton(text: String, onClick: () -> Unit): TextView {
        return TextView(this).apply {
            this.text = text
            gravity = Gravity.CENTER
            setTextColor(Color.WHITE)
            textSize = 13f
            background = GradientDrawable().apply {
                cornerRadius = dp(6).toFloat()
                setColor(Color.argb(160, 60, 60, 70))
                setStroke(dp(1), Color.argb(120, 255, 255, 255))
            }
            setOnClickListener { onClick() }
        }
    }

    private fun onCheatSetClicked(row: CheatRow) {
        val text = row.input.text.toString().trim()
        if (text.isEmpty()) {
            Toast.makeText(this, "请输入数值", Toast.LENGTH_SHORT).show()
            return
        }
        val v = text.toLongOrNull() ?: run {
            Toast.makeText(this, "数值格式不正确", Toast.LENGTH_SHORT).show()
            return
        }
        if (v > Int.MAX_VALUE || v < Int.MIN_VALUE) {
            Toast.makeText(this, "超出可设置范围（±21 亿）", Toast.LENGTH_SHORT).show()
            return
        }
        if (nativeCheatSet(row.index, v.toInt())) {
            refreshCheatValues()
        } else {
            Toast.makeText(this, "修改失败：目标暂不可用", Toast.LENGTH_SHORT).show()
        }
    }

    private fun refreshCheatValues() {
        for (row in cheatRows) {
            val v = nativeCheatValue(row.index)
            row.valueView.text = if (v == CHEAT_INVALID_VALUE) "?" else v.toString()
        }
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
