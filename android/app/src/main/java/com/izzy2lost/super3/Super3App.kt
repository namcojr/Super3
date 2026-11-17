package com.izzy2lost.super3

import android.app.Application
import java.io.OutputStream
import java.io.PrintStream

/**
 * Release builds write nothing to the Android log. Android sends System.out and
 * System.err to logcat; here they are replaced with a stream that discards
 * everything. android.util.Log calls are stripped by R8 (proguard-rules.pro) and
 * native logging is compiled out (CMakeLists.txt, SUPER3_LOGGING).
 */
class Super3App : Application() {
    override fun onCreate() {
        super.onCreate()
        if (!BuildConfig.LOGGING) {
            val discard = PrintStream(object : OutputStream() {
                override fun write(b: Int) {}
                override fun write(b: ByteArray, off: Int, len: Int) {}
            })
            System.setOut(discard)
            System.setErr(discard)
        }
    }
}
