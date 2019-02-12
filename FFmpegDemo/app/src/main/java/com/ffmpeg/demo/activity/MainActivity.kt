package com.ffmpeg.demo.activity

import android.Manifest
import android.content.Intent
import android.os.Bundle
import com.ffmpeg.demo.model.HandleItem
import com.tbruyelle.rxpermissions2.RxPermissions

class MainActivity : HandleActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)

        val rxPermissions = RxPermissions(this)
        rxPermissions.request(
            Manifest.permission.WRITE_EXTERNAL_STORAGE,
            Manifest.permission.READ_EXTERNAL_STORAGE
        )
            .subscribe {

            }.apply {
                disposables.add(this)
            }

    }

    override fun handleList(): ArrayList<HandleItem> {
        val list = ArrayList<HandleItem>()

        val action1 = HandleItem("视频处理") {
            val intent = Intent()
            intent.setClass(this@MainActivity, VideoHandlerActivity::class.java)
            this@MainActivity.startActivity(intent)
        }
        val action2 = HandleItem("视频播放") {
            val intent = Intent()
            intent.setClass(this@MainActivity, MediaPlayerActivity::class.java)
            this@MainActivity.startActivity(intent)
        }
        list.add(action1)
        list.add(action2)
        return list
    }
}
