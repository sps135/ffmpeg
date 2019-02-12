package com.ffmpeg.demo.activity

import android.os.Bundle
import android.widget.Toast
import com.ffmpeg.demo.FFmpegCmd
import com.ffmpeg.demo.model.HandleItem
import com.ffmpeg.demo.util.FileUtils
import io.reactivex.Observable
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.schedulers.Schedulers

class VideoHandlerActivity : HandleActivity() {

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
    }

    override fun handleList(): ArrayList<HandleItem> {
        val list = ArrayList<HandleItem>()

        // 将MP4格式转换为flv格式
        val action1 = HandleItem("剪切") {
            FFmpegCmd
                .getInstance()
                .transformMp4ToFlvVideo(
                    FileUtils.filePath(FileUtils.TARGET_NAME),
                    FileUtils.filePath(FileUtils.TRANSFORM_NAME)
                )
                .doOnSubscribe {
                    this@VideoHandlerActivity.showLoadingDialog("转换中...")
                }
                .subscribe({
                    hideLoadingDialog()
                }, {
                    hideLoadingDialog()
                }).apply {
                    disposables.add(this)
                }
        }

        val action2 = HandleItem("截取视频前10s") {
            FFmpegCmd
                .getInstance()
                .cutSegmentVideo(
                    FileUtils.filePath(FileUtils.TARGET_NAME),
                    FileUtils.filePath(FileUtils.PART_NAME)
                )
                .doOnSubscribe {
                    this@VideoHandlerActivity.showLoadingDialog("转换中...")
                }
                .subscribe({
                    hideLoadingDialog()
                }, {
                    hideLoadingDialog()
                }).apply {
                    disposables.add(this)
                }
        }

        val action3 = HandleItem("为第七秒截图") {
            FFmpegCmd
                .getInstance()
                .screenshotOfVideo(
                    FileUtils.filePath(FileUtils.TARGET_NAME),
                    FileUtils.filePath(FileUtils.SCREENSHOT_NAME)
                )
                .doOnSubscribe {
                    this@VideoHandlerActivity.showLoadingDialog("转换中...")
                }
                .subscribe({
                    hideLoadingDialog()
                }, {
                    hideLoadingDialog()
                }).apply {
                    disposables.add(this)
                }
        }


        list.add(action1)
        list.add(action2)
        list.add(action3)
        return list
    }
}