package com.viktor.fg.adapter

import android.content.Context
import android.database.Cursor
import android.graphics.drawable.Drawable
import com.viktor.fg.R
import android.view.ViewGroup
import android.view.LayoutInflater
import android.view.View
import android.widget.CursorAdapter
import android.widget.ImageView
import com.viktor.fg.loader.AlbumCategory
import android.widget.TextView
import androidx.core.content.ContextCompat
import com.bumptech.glide.Glide
import com.bumptech.glide.request.RequestOptions
import com.viktor.fg.util.ViktorConstants

/**
 * 相册分类（即文件夹）
 */
class AlbumsAdapter : CursorAdapter {
    private var placeholder: Drawable? = null
    private var curType = ViktorConstants.TYPE_VIDEO
    fun setCurrentType(type: Int) {
        curType = type
    }

    private var size = 0

    constructor(context: Context, c: Cursor?, autoRequery: Boolean) : super(context, c, autoRequery) {
        placeholder = ContextCompat.getDrawable(context,R.drawable.shape_placeholder)
        size = context.resources.getDimensionPixelSize(R.dimen.dp_40)
    }

    override fun newView(context: Context, cursor: Cursor, parent: ViewGroup): View {
        return LayoutInflater.from(context).inflate(R.layout.album_list_item, parent, false)
    }

    override fun bindView(view: View, context: Context, cursor: Cursor) {
        val album = AlbumCategory.valueOf(cursor, curType) ?: return
        (view.findViewById<View>(R.id.tv_name) as? TextView)?.text = album.getDisplayName(context)
        (view.findViewById<View>(R.id.tv_count) as? TextView)?.text = album.count.toString()
        val imageView = view.findViewById<View>(R.id.ic_cover) as ImageView

        Glide.with(context)
            .asBitmap()
            .load(album.coverUri)
            .apply(
                RequestOptions()
                    .override(size, size)
                    .placeholder(placeholder)
                    .centerCrop()
            )
            .into(imageView)
    }
}