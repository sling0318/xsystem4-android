package io.github.kichikuou.xsystem4

import android.graphics.Color
import android.graphics.drawable.GradientDrawable
import android.os.Bundle
import android.os.Process
import android.os.SystemClock
import android.view.Gravity
import android.view.View
import android.view.ViewGroup
import android.widget.FrameLayout
import android.widget.LinearLayout
import android.widget.TextView
import android.widget.Toast
import org.libsdl.app.SDLActivity
import java.io.File

// Intent for this activity must have the following extras:
// - EXTRA_GAME_ROOT (string): A path to the game installation.
// - EXTRA_SAVE_DIR (string): A path to a directory where save files are stored.
class XSystem4Activity : SDLActivity() {
    companion object {
        const val EXTRA_GAME_ROOT = "GAME_ROOT"
        const val EXTRA_SAVE_DIR = "SAVE_DIR"
        const val COMMAND_OPEN_PLAYING_MANUAL = 0x8000  // xsystem4/src/hll/SystemService.c

        private const val ACTION_QUICK_SAVE = 1
        private const val ACTION_QUICK_LOAD = 2
        private const val ACTION_OPEN_SAVE = 3
        private const val ACTION_OPEN_LOAD = 4
    }

    private var lastGameActionAt = 0L
    private var screenOverlay: View? = null

    private external fun nativeQueueGameAction(action: Int)

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Workaround for https://github.com/libsdl-org/SDL/issues/8995
        SDLActivity.setWindowStyle(true)
        setupSaveLoadOverlay()
    }

    private fun dp(value: Int) = (value * resources.displayMetrics.density).toInt()

    private fun setupSaveLoadOverlay() {
        val root = findViewById<ViewGroup>(android.R.id.content) ?: return

        // Semi-transparent black overlay that covers the entire game surface.
        // Shown when opening the native save/load UI to prevent battle/map
        // parts from bleeding through behind the save slots.
        screenOverlay = View(this).apply {
            setBackgroundColor(Color.argb(200, 0, 0, 0))
            visibility = View.GONE
        }
        root.addView(screenOverlay, FrameLayout.LayoutParams(
            FrameLayout.LayoutParams.MATCH_PARENT,
            FrameLayout.LayoutParams.MATCH_PARENT))

        val panel = LinearLayout(this).apply {
            orientation = LinearLayout.VERTICAL
            gravity = Gravity.CENTER_HORIZONTAL
        }
        panel.addView(createQuickButton("快存") { triggerGameAction(ACTION_QUICK_SAVE) })
        panel.addView(createQuickButton("快读") { triggerGameAction(ACTION_QUICK_LOAD) },
            LinearLayout.LayoutParams(dp(62), dp(36)).apply { topMargin = dp(8) })
        panel.addView(createQuickButton("存档") { triggerGameAction(ACTION_OPEN_SAVE) },
            LinearLayout.LayoutParams(dp(62), dp(36)).apply { topMargin = dp(16) })
        panel.addView(createQuickButton("读档") { triggerGameAction(ACTION_OPEN_LOAD) },
            LinearLayout.LayoutParams(dp(62), dp(36)).apply { topMargin = dp(8) })

        val lp = FrameLayout.LayoutParams(
            dp(62), FrameLayout.LayoutParams.WRAP_CONTENT,
            Gravity.END or Gravity.CENTER_VERTICAL
        ).apply { rightMargin = dp(12) }
        root.addView(panel, lp)
    }

    private fun createQuickButton(text: String, onClick: () -> Unit): TextView {
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

    private fun quickSaveFile(): File? {
        val saveDir = intent.getStringExtra(EXTRA_SAVE_DIR) ?: return null
        return File(saveDir, "QC.asd")
    }

    private fun triggerGameAction(action: Int) {
        val now = SystemClock.uptimeMillis()
        if (now - lastGameActionAt < 500)
            return
        lastGameActionAt = now

        if (action == ACTION_QUICK_LOAD) {
            val saveFile = quickSaveFile()
            if (saveFile == null || !saveFile.exists()) {
                Toast.makeText(this, "还没有快速存档", Toast.LENGTH_SHORT).show()
                return
            }
        }

        // Show the dark overlay before opening the native save/load UI so
        // that battle / map parts don't bleed through behind the slot list.
        if (action == ACTION_OPEN_SAVE || action == ACTION_OPEN_LOAD) {
            screenOverlay?.visibility = View.VISIBLE
        }

        nativeQueueGameAction(action)

        // Hide the overlay after a short delay — the save/load scene takes a
        // few frames to render its own background. 500 ms is enough for the
        // first frame of the modal to appear; if it closes quickly the overlay
        // will be hidden again by the next call.
        if (action == ACTION_OPEN_SAVE || action == ACTION_OPEN_LOAD) {
            screenOverlay?.postDelayed({ screenOverlay?.visibility = View.GONE }, 500)
        }

        when (action) {
            ACTION_QUICK_SAVE -> Toast.makeText(this, "已快速存档", Toast.LENGTH_SHORT).show()
            ACTION_QUICK_LOAD -> Toast.makeText(this, "已快速读档", Toast.LENGTH_SHORT).show()
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
