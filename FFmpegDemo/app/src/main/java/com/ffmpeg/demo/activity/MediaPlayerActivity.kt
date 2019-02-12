package com.ffmpeg.demo.activity

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.view.SurfaceHolder
import android.view.SurfaceView
import com.ffmpeg.demo.MediaPlayer
import com.ffmpeg.demo.R
import com.ffmpeg.demo.util.FileUtils

class MediaPlayerActivity : AppCompatActivity(), SurfaceHolder.Callback {
    private lateinit var mediaPlayer: MediaPlayer

    private lateinit var surfaceHolder: SurfaceHolder

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_media_player)
        mediaPlayer = MediaPlayer()
        initView()
    }

    private fun initView() {
        val surfaceView: SurfaceView = findViewById(R.id.surface_media)
        surfaceHolder = surfaceView.holder
        surfaceHolder.addCallback(this)
    }

    override fun surfaceChanged(holder: SurfaceHolder?, format: Int, width: Int, height: Int) {

    }

    override fun surfaceDestroyed(holder: SurfaceHolder?) {
    }

    override fun onPause() {
        super.onPause()
        mediaPlayer.release()
    }

    override fun surfaceCreated(holder: SurfaceHolder?) {
        Thread(Runnable {
            val result = mediaPlayer.setup(FileUtils.filePath(FileUtils.TARGET_NAME), surfaceHolder.surface)
            if (result < 0) {
                return@Runnable
            }
            mediaPlayer.play()
        }).start()
    }
}