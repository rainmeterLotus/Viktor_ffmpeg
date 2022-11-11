package com.viktor.fg.select

import android.database.Cursor
import android.graphics.Rect
import com.viktor.fg.loader.AlbumMediaManager.AlbumMediaCallbacks
import com.viktor.fg.loader.AlbumMediaManager
import com.viktor.fg.adapter.AlbumMediaAdapter
import android.view.LayoutInflater
import android.view.ViewGroup
import android.os.Bundle
import android.view.View
import androidx.fragment.app.Fragment
import com.viktor.fg.R
import com.viktor.fg.loader.AlbumCategory
import androidx.recyclerview.widget.GridLayoutManager
import androidx.recyclerview.widget.RecyclerView
import com.viktor.fg.loader.SelectMedia
import com.viktor.fg.util.ViktorConstants
import kotlinx.android.synthetic.main.fragment_media_list.*
import kotlin.collections.ArrayList

class MediaItemFragment : Fragment(), AlbumMediaCallbacks {
    private val mAlbumMediaCollection = AlbumMediaManager()
    private var mAdapter: AlbumMediaAdapter? = null

    var onActivityMediaClick:((selectMedia:SelectMedia,isAdd:Boolean) -> Unit)? = null

    override fun onCreateView(inflater: LayoutInflater, container: ViewGroup?, savedInstanceState: Bundle?): View? {
        return inflater.inflate(R.layout.fragment_media_list, container, false)
    }

    private var mCurType = ViktorConstants.TYPE_VIDEO
    override fun onActivityCreated(savedInstanceState: Bundle?) {
        super.onActivityCreated(savedInstanceState)
        mCurType = arguments?.getInt(EXTRA_MEDIA_TYPE) ?:ViktorConstants.TYPE_PHOTO
        val hasMediaSelectList = arguments?.getParcelableArrayList(EXTRA_DATA) ?: emptyList<SelectMedia>()

        val album: AlbumCategory = arguments?.getParcelable(EXTRA_ALBUM) ?: return
        mAdapter = AlbumMediaAdapter(null, getSize()).apply {
            onMediaClick = {selectMedia,isAdd ->
                onActivityMediaClick?.invoke(selectMedia,isAdd)
            }
            refreshHasMediaSelect(hasMediaSelectList)
        }

        recyclerview?.apply {
            val spanCount = 3
            val spacing = resources.getDimensionPixelSize(R.dimen.dp_5)
            layoutManager = GridLayoutManager(context, 3)
            adapter = mAdapter
            setHasFixedSize(true)
            addItemDecoration(object : RecyclerView.ItemDecoration(){
                override fun getItemOffsets(
                    outRect: Rect, view: View, parent: RecyclerView,
                    state: RecyclerView.State
                ) {
                    val position = parent.getChildAdapterPosition(view)
                    val column = position % spanCount
                    outRect.left = column * spanCount / spanCount
                    outRect.right = spacing - (column + 1) * spacing / spanCount
                    if (position >= spanCount) {
                        outRect.top = spacing
                    }
                }
            })
        }
        activity?.let {
            progress_bar?.visibility = View.VISIBLE
            mAlbumMediaCollection.init(it, this)
            mAlbumMediaCollection.load(album)
        }

    }


    private fun getSize():Int{
        var size = 0
        val context = this.context ?: return 0
        val spanCount = (recyclerview?.layoutManager as? GridLayoutManager)?.spanCount ?: 3
        val screenWidth = context.resources.displayMetrics.widthPixels
        val availableWidth = screenWidth - context.resources.getDimensionPixelSize(
            R.dimen.dp_5
        ) * (spanCount - 1)
        size = availableWidth / spanCount
        size = (size * 1.0f).toInt()
        return size
    }

    override fun onDestroyView() {
        super.onDestroyView()
        mAlbumMediaCollection.release()
    }

    override fun onAlbumMediaLoad(cursor: Cursor?) {
        progress_bar?.visibility = View.GONE
        mAdapter?.changeCursor(cursor)
        val count = mAdapter?.itemCount ?: 0
        if (count > 0) {
            tv_tips?.visibility = View.GONE
        } else {
            tv_tips?.visibility = View.VISIBLE
            if (mCurType == ViktorConstants.TYPE_PHOTO) {
                tv_tips?.setText(R.string.str_text_no_find_photo)
            } else {
                tv_tips?.setText(R.string.str_text_no_find_video)
            }
        }
    }

    override fun onAlbumMediaReset() {
        mAdapter?.changeCursor(null)
    }

    companion object {
        const val EXTRA_ALBUM = "album"
        const val EXTRA_MEDIA_TYPE = "media_type"
        const val EXTRA_DATA = "data"
        const val TAG = "media_fg"
        fun newInstance(album: AlbumCategory?, type: Int, hasMediaSelectList: ArrayList<SelectMedia>): MediaItemFragment {
            val fragment = MediaItemFragment()
            val args = Bundle()
            args.putParcelable(EXTRA_ALBUM, album)
            args.putParcelableArrayList(EXTRA_DATA, hasMediaSelectList)
            args.putInt(EXTRA_MEDIA_TYPE, type)
            fragment.arguments = args
            return fragment
        }
    }
}