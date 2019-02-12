package com.ffmpeg.demo.activity

import android.os.Bundle
import android.support.v7.widget.LinearLayoutManager
import android.support.v7.widget.RecyclerView
import android.widget.Toast
import com.ffmpeg.demo.R
import com.ffmpeg.demo.activity.adapter.HandleAdapter
import com.ffmpeg.demo.model.HandleItem
import com.ffmpeg.demo.util.FileUtils
import io.reactivex.Observable
import io.reactivex.android.schedulers.AndroidSchedulers
import io.reactivex.schedulers.Schedulers

abstract class HandleActivity : BaseActivity() {
    private lateinit var rvHandleList: RecyclerView

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        setContentView(R.layout.activity_handler_list)
        initView()
    }

    private fun initView() {
        rvHandleList = findViewById(R.id.rv_handler_list)
        rvHandleList.layoutManager = LinearLayoutManager(this)

        val adapter = HandleAdapter(R.layout.item_handle_action, handleList())
        rvHandleList.adapter = adapter
        adapter.setOnItemClickListener { adapter, view, position ->
            val action = adapter.data[position] as HandleItem
            action.handleAction()
        }

    }

    abstract fun handleList(): ArrayList<HandleItem>
}
