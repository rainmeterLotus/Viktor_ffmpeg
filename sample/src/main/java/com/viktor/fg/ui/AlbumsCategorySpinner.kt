package com.viktor.fg.ui

import android.content.Context
import android.view.View
import android.widget.TextView
import android.widget.AdapterView.OnItemSelectedListener
import com.viktor.fg.loader.AlbumCategory
import com.viktor.fg.R
import android.widget.AdapterView
import android.widget.CursorAdapter
import androidx.appcompat.widget.ListPopupWindow
import androidx.core.content.ContextCompat
import com.viktor.fg.util.ViktorConstants

class AlbumsCategorySpinner(context: Context) {
    private var mAdapter: CursorAdapter? = null
    private var selectTextView: TextView? = null
    private var listWindow: ListPopupWindow? = null
    private var mOnItemSelectedListener: OnItemSelectedListener? = null
    private var mCurrentType = ViktorConstants.TYPE_VIDEO
    var curPosition = 0
    init {
        listWindow = ListPopupWindow(context, null, R.attr.listPopupWindowStyle).apply {
            isModal = true
            width = ListPopupWindow.MATCH_PARENT
            setBackgroundDrawable(ContextCompat.getDrawable(context,R.drawable.shape_album_list_view_bg))
            setOnItemClickListener { parent: AdapterView<*>, view: View?, position: Int, id: Long ->
                onItemSelected(parent.context, position)
                mOnItemSelectedListener?.onItemSelected(parent, view, position, id)
            }

        }

    }

    fun setOnItemSelectedListener(listener: OnItemSelectedListener?) {
        mOnItemSelectedListener = listener
    }

    fun setSelection(context: Context, position: Int) {
        listWindow?.setSelection(position)
        onItemSelected(context, position)
    }

    fun setCurrentType(type: Int) {
        mCurrentType = type
    }

    private fun onItemSelected(context: Context, position: Int) {
        curPosition = position
        listWindow?.dismiss()
        val cursor = mAdapter?.cursor ?: return
        cursor.moveToPosition(curPosition)
        val album = AlbumCategory.valueOf(cursor, mCurrentType)
        val displayName = album?.getDisplayName(context)
        if (selectTextView?.visibility == View.VISIBLE) {
            selectTextView?.text = displayName
        } else {
            selectTextView?.visibility = View.VISIBLE
            selectTextView?.text = displayName
            selectTextView?.animate()?.alpha(1.0f)?.setDuration(500)?.start()
        }
    }

    fun setAdapterAndAnchorView(adapter: CursorAdapter?,view: View?) {
        listWindow?.setAdapter(adapter)
        mAdapter = adapter
        listWindow?.anchorView = view
    }

    fun onSelectedView(textView: TextView?) {
        selectTextView = textView
        selectTextView?.visibility = View.GONE
        selectTextView?.setOnClickListener { v: View ->
            val itemHeight = v.resources.getDimensionPixelSize(R.dimen.dp_60)
            val count = mAdapter?.count ?: return@setOnClickListener
            listWindow?.height = if (count > 7) itemHeight * 7 else itemHeight * count
            listWindow?.show()
        }
        selectTextView?.setOnTouchListener(listWindow?.createDragToOpenListener(selectTextView))
    }
}