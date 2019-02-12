package com.ffmpeg.demo

import io.reactivex.Observable
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.schedulers.Schedulers

class FFmpegCmd {

    init {
        System.loadLibrary("native-lib")
    }

    private external fun executeCmd(cmds: Array<String>): Int

    /**
     * 将mp4格式转换为flv格式
     */
    fun transformMp4ToFlvVideo(srcFile: String, targetFile: String): Observable<Int> {
        val cmd = "ffmpeg -i $srcFile -c copy -f flv $targetFile"
        return execute(cmd.split(" ").toTypedArray())
    }

    /**
     * 截取视频的前10s
     */
    fun cutSegmentVideo(srcFile: String, targetFile: String): Observable<Int> {
        val cmd = "ffmpeg -i $srcFile -c copy -t 10 -copyts $targetFile"
        return execute(cmd.split(" ").toTypedArray())
    }

    /**
     * 为视频第七秒截图
     */
    fun screenshotOfVideo(srcFile: String, targetFile: String): Observable<Int> {
        val cmd = "ffmpeg -i $srcFile -ss 00:00:7.435 -vframes 1 $targetFile"
        return execute(cmd.split(" ").toTypedArray())
    }

    private fun execute(commands: Array<String>): Observable<Int> {
        val observable: Observable<Int> = Observable.create {
            val result = executeCmd(commands)
            it.onNext(result)
            it.onComplete()
        }
        return observable
            .observeOn(AndroidSchedulers.mainThread())
            .subscribeOn(Schedulers.newThread())
    }

    companion object {
        private var instance: FFmpegCmd? = null

        fun getInstance(): FFmpegCmd {
            if (instance == null) {
                instance = FFmpegCmd()
            }
            return instance!!
        }
    }
}