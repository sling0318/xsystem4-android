package io.github.kichikuou.xsystem4

import android.os.Bundle
import android.os.Process
import org.libsdl.app.SDLActivity

// Intent for this activity must have the following extras:
// - EXTRA_GAME_ROOT (string): A path to the game installation.
// - EXTRA_SAVE_DIR (string): A path to a directory where save files are stored.
class XSystem4Activity : SDLActivity() {
    companion object {
        const val EXTRA_GAME_ROOT = "GAME_ROOT"
        const val EXTRA_SAVE_DIR = "SAVE_DIR"
        const val COMMAND_OPEN_PLAYING_MANUAL = 0x8000  // xsystem4/src/hll/SystemService.c
    }

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        // Workaround for https://github.com/libsdl-org/SDL/issues/8995
        SDLActivity.setWindowStyle(true)
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
        // Games that import HLL functions this engine only has stubs for: keep running
        // with those calls returning zero instead of aborting (see --stub-hll).
        if (java.io.File(gameRoot, ".hll-stub").exists()) {
            args.add("--stub-hll")
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
