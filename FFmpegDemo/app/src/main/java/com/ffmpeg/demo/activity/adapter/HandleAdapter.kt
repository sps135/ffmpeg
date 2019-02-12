package com.ffmpeg.demo.activity.adapter

import com.chad.library.adapter.base.BaseQuickAdapter
import com.chad.library.adapter.base.BaseViewHolder
import com.ffmpeg.demo.R
import com.ffmpeg.demo.model.HandleItem

class HandleAdapter(layoutResId: Int, val data: ArrayList<HandleItem>) :
    BaseQuickAdapter<HandleItem, BaseViewHolder>(layoutResId, data) {

    override fun convert(helper: BaseViewHolder, item: HandleItem) {
        helper.setText(R.id.tv_action_name, item.action)
    }
}