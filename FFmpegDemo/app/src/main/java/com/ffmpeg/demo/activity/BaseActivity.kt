package com.ffmpeg.demo.activity

import android.os.Bundle
import android.support.v7.app.AppCompatActivity
import android.widget.Toast
import com.android.tu.loadingdialog.LoadingDailog
import com.ffmpeg.demo.util.FileUtils
import io.reactivex.Observable
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.disposables.CompositeDisposable
import io.reactivex.schedulers.Schedulers

open class BaseActivity : AppCompatActivity() {

    protected val disposables = CompositeDisposable()

    private var mLoadingDialog: LoadingDailog? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        prepareFile()
    }

    private fun prepareFile() {
        if (FileUtils.checkTargetFile(FileUtils.TARGET_NAME)) {
            Toast.makeText(this@BaseActivity, "目标文件准备就绪！", Toast.LENGTH_SHORT).show()
            return
        }
        Observable.create<Boolean> {
            FileUtils.copy(this@BaseActivity, FileUtils.TARGET_NAME, FileUtils.TARGET_NAME)
            it.onNext(true)
            it.onComplete()
        }
            .doOnSubscribe {
                this@BaseActivity.showLoadingDialog("拷贝样例文件中...")
            }
            .subscribeOn(Schedulers.io())
            .observeOn(AndroidSchedulers.mainThread())
            .subscribe({
                hideLoadingDialog()
                if (it) {
                    Toast.makeText(this@BaseActivity, "拷贝成功！", Toast.LENGTH_SHORT).show()
                }
            }, {
                hideLoadingDialog()
            }).apply {
                disposables.add(this)
            }
    }

    protected fun showLoadingDialog(message: String) {
        val loadBuilder = LoadingDailog.Builder(this)
            .setMessage(message)
            .setCancelable(false)
            .setCancelOutside(false)
        mLoadingDialog = loadBuilder.create()
        mLoadingDialog!!.show()
    }

    protected fun hideLoadingDialog() {
        if (mLoadingDialog != null && mLoadingDialog!!.isShowing) {
            mLoadingDialog!!.dismiss()
        }
    }

    override fun onDestroy() {
        super.onDestroy()
        disposables.dispose()
    }
}