package com.viktor.fg.adapter

import android.database.Cursor
import android.text.TextUtils
import android.text.format.DateUtils
import android.view.LayoutInflater
import android.view.View
import android.view.ViewGroup
import android.widget.ImageView
import android.widget.TextView
import androidx.core.content.ContextCompat
import androidx.recyclerview.widget.RecyclerView
import com.bumptech.glide.Glide
import com.bumptech.glide.request.RequestOptions
import com.viktor.fg.R
import com.viktor.fg.loader.Media
import com.viktor.fg.loader.CursorAdapter
import com.viktor.fg.util.ViktorUtil
import java.io.File

/**
 * 每个视频或者照片
 */
class AlbumMediaAdapter(c: Cursor?, private val size:Int = 0) : CursorAdapter<AlbumMediaAdapter.MediaHolder>(c) {

    var onMediaClick:((path:String,isAdd:Boolean) -> Unit)? = null
    private val selectMediaList = ArrayList<String>()

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): MediaHolder {
        return MediaHolder(LayoutInflater.from(parent.context).inflate(R.layout.layout_media_item, parent, false))
    }

    override fun onBindViewHolder(holder: MediaHolder, cursor: Cursor) {
        holder.binder(Media.valueOf(cursor))
    }

    fun refreshHasMediaSelect(hasMediaList:List<String>?){
        hasMediaList?.forEach { path->
            selectMediaList.add(path)
        }
    }

    inner class MediaHolder(itemView: View):RecyclerView.ViewHolder(itemView){
        val ivThumbnail = itemView.findViewById<ImageView>(R.id.iv_thumbnail)
        val ivCheck = itemView.findViewById<ImageView>(R.id.iv_check_overlay)
        val tvName = itemView.findViewById<TextView>(R.id.tv_display_name)
        val tvDuration = itemView.findViewById<TextView>(R.id.tv_duration)

        private val placeholder = ContextCompat.getDrawable(itemView.context,R.drawable.shape_placeholder)

        fun binder(item: Media){
            val context = itemView.context

            if (item.isGif) {
                Glide.with(context)
                    .asBitmap()
                    .load(item.contentUri)
                    .apply(
                        RequestOptions()
                            .override(size, size)
                            .placeholder(placeholder)
                            .centerCrop()
                    )
                    .into(ivThumbnail)
            } else {

                Glide.with(context)
                    .asBitmap()
                    .load(item.contentUri)
                    .apply(
                        RequestOptions()
                            .override(size, size)
                            .placeholder(placeholder)
                            .centerCrop()
                    )
                    .into(ivThumbnail)
            }

            tvName.text = item.displayName

            if (item.isVideo) {
                tvDuration.visibility = View.VISIBLE
                tvDuration.text = DateUtils.formatElapsedTime(item.duration / 1000)
            } else {
                tvDuration.visibility = View.GONE
            }

            val path = ViktorUtil.obtainPathByUri(itemView.context, item.contentUri) ?: return
            ivCheck.isSelected = selectMediaList.contains(path)
            ivThumbnail.setOnClickListener {
                if (TextUtils.isEmpty(path) || !File(path).exists()) {
                    return@setOnClickListener
                }
                ivCheck.isSelected = !ivCheck.isSelected

                if (ivCheck.isSelected){
                    selectMediaList.add(path)
                } else {
                    selectMediaList.remove(path)
                }

                onMediaClick?.invoke(path,ivCheck.isSelected)
            }
        }

    }
}