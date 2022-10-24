package com.viktor.fg.loader

import android.database.Cursor
import androidx.recyclerview.widget.RecyclerView.ViewHolder
import androidx.recyclerview.widget.RecyclerView
import android.provider.MediaStore

abstract class CursorAdapter<VH : ViewHolder?> protected constructor(c: Cursor?) : RecyclerView.Adapter<VH>() {

    init {
        setHasStableIds(true)
        changeCursor(c)
    }

    var cursor: Cursor? = null
        private set
    private var mRowColumnId = 0
    protected abstract fun onBindViewHolder(holder: VH, cursor: Cursor)
    override fun onBindViewHolder(holder: VH, position: Int) {
        check(!(cursor?.isClosed ?: false)) { "not bind holder ,cursor is illegal" }
        check(cursor?.moveToPosition(position) ?: true) { "could not move to position $position" }
        val cr = cursor ?: return
        onBindViewHolder(holder, cr)
    }

    override fun getItemCount(): Int {
        return if (cursor?.isClosed != true) {
            cursor?.count ?: 0
        } else {
            0
        }
    }

    override fun getItemId(position: Int): Long {
        check(!(cursor?.isClosed ?: false)) { "getItemId ,cursor is illegal" }
        check(cursor?.moveToPosition(position) ?: true) { "could not move to position $position" }
        return cursor?.getLong(mRowColumnId) ?:0L
    }

    fun changeCursor(newCursor: Cursor?) {
        if (newCursor === cursor) {
            return
        }
        if (newCursor != null) {
            cursor = newCursor
            mRowColumnId = cursor?.getColumnIndexOrThrow(MediaStore.Files.FileColumns._ID) ?: 0
            notifyDataSetChanged()
        } else {
            notifyItemRangeRemoved(0, itemCount)
            cursor = null
            mRowColumnId = -1
        }
    }

}