package io.example.dn

import android.os.Handler
import android.os.Looper
import android.util.Log

class AnrMonitor(private val timeoutMs: Long = 5000) {
    private val mainHandler = Handler(Looper.getMainLooper())
    private val watchdogThread = Thread {
        while (!Thread.interrupted()) {
            val pingTask = PingTask()
            mainHandler.post(pingTask)
            try {
                Thread.sleep(timeoutMs)
                if (!pingTask.isPinged) {
                    // 主线程可能被阻塞
                    Log.e("AnrMonitor", "可能的ANR检测到: 主线程已阻塞超过 $timeoutMs ms")
                    // 打印主线程堆栈跟踪
                    for (thread in Thread.getAllStackTraces().keys) {
                        if (thread.name == "main") {
                            Log.e("AnrMonitor", "主线程堆栈: ${thread.stackTrace.joinToString("\n")}")
                            break
                        }
                    }
                }
            } catch (e: InterruptedException) {
                break
            }
        }
    }
    
    private class PingTask : Runnable {
        @Volatile
        var isPinged = false
        
        override fun run() {
            isPinged = true
        }
    }
    
    fun start() {
        watchdogThread.start()
    }
    
    fun stop() {
        watchdogThread.interrupt()
    }
} 