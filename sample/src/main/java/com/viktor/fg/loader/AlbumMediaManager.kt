package com.viktor.fg.loader

import android.content.Context
import android.database.Cursor
import android.os.Bundle
import android.provider.MediaStore
import androidx.fragment.app.FragmentActivity
import androidx.loader.app.LoaderManager
import androidx.loader.content.CursorLoader
import androidx.loader.content.Loader
import com.viktor.fg.ViktorApplication
import com.viktor.fg.util.ViktorConstants
import kotlin.jvm.JvmOverloads

class AlbumMediaManager : LoaderManager.LoaderCallbacks<Cursor> {
    private var mContext : Context? = null
    private var loaderManager: LoaderManager? = null
    private var callback: AlbumMediaCallbacks? = null
    companion object {
        private const val LOADER_ID = 100
        private const val ARGS_ALBUM = "args_album_category"
    }

    override fun onCreateLoader(id: Int, args: Bundle?): Loader<Cursor> {
        val context = mContext ?: ViktorApplication.context
        val album = args?.getParcelable<AlbumCategory>(ARGS_ALBUM)
        return AlbumMediaLoader.newInstance(context, album)
    }

    override fun onLoadFinished(loader: Loader<Cursor>, data: Cursor) {
        callback?.onAlbumMediaLoad(data)
    }

    override fun onLoaderReset(loader: Loader<Cursor>) {
        callback?.onAlbumMediaReset()
    }

    fun init(context: FragmentActivity, callbacks: AlbumMediaCallbacks) {
        mContext = context
        loaderManager = context.supportLoaderManager
        callback = callbacks
    }

    fun release() {
        loaderManager?.destroyLoader(LOADER_ID)
        callback = null
    }

    @JvmOverloads
    fun load(target: AlbumCategory?) {
        val args = Bundle()
        args.putParcelable(ARGS_ALBUM, target)
        loaderManager?.initLoader(LOADER_ID, args, this)
    }

    interface AlbumMediaCallbacks {
        fun onAlbumMediaLoad(cursor: Cursor?)
        fun onAlbumMediaReset()
    }
}


class AlbumMediaLoader private constructor(context: Context, selection: String, selectionArgs: Array<String?>) :
    CursorLoader(context, URI, PROJECTION, selection, selectionArgs, ORDER) {

    companion object {
        const val DURATION_MIN = 0L
        private val URI = MediaStore.Files.getContentUri("external")
        private val PROJECTION = arrayOf(
            MediaStore.Files.FileColumns._ID,
            MediaStore.MediaColumns.DISPLAY_NAME,
            MediaStore.MediaColumns.MIME_TYPE,
            MediaStore.MediaColumns.SIZE,
            "duration"
        )
        private const val ORDER = MediaStore.Images.Media.DATE_TAKEN + " DESC"
        fun newInstance(context: Context, album: AlbumCategory?): CursorLoader {
            if (album == null){
                val selection = ("((" + MediaStore.Files.FileColumns.MEDIA_TYPE + "=? AND duration > ?)  OR "
                        + MediaStore.Files.FileColumns.MEDIA_TYPE + "=?) AND " + MediaStore.MediaColumns.SIZE + ">0")
                val selectionArgs:Array<String?> = arrayOf(
                    MediaStore.Files.FileColumns.MEDIA_TYPE_VIDEO.toString(),
                    DURATION_MIN.toString(), MediaStore.Files.FileColumns.MEDIA_TYPE_IMAGE.toString()
                )
                return AlbumMediaLoader(context, selection, selectionArgs)
            }
            val selection: String
            val selectionArgs: Array<String?>
            if (album.mediaType == ViktorConstants.TYPE_PHOTO) {
                if (album.isAll) {
                    selection = MediaStore.Files.FileColumns.MEDIA_TYPE + "=? AND " + MediaStore.MediaColumns.SIZE + ">0"
                    selectionArgs = arrayOf(MediaStore.Files.FileColumns.MEDIA_TYPE_IMAGE.toString())
                } else {
                    selection = MediaStore.Files.FileColumns.MEDIA_TYPE + "=? AND bucket_id=? AND " + MediaStore.MediaColumns.SIZE + ">0"
                    selectionArgs = arrayOf(MediaStore.Files.FileColumns.MEDIA_TYPE_IMAGE.toString(), album.id)
                }
            }else if (album.mediaType == ViktorConstants.TYPE_VIDEO) {
                if (album.isAll) {
                    selection = MediaStore.Files.FileColumns.MEDIA_TYPE + "=? AND duration > ? AND " + MediaStore.MediaColumns.SIZE + ">0"
                    selectionArgs = arrayOf(
                        MediaStore.Files.FileColumns.MEDIA_TYPE_VIDEO.toString(),
                        DURATION_MIN.toString()
                    )
                } else {
                    selection =
                        MediaStore.Files.FileColumns.MEDIA_TYPE + "=? AND duration > ? AND bucket_id=? AND " + MediaStore.MediaColumns.SIZE + ">0"
                    selectionArgs = arrayOf(
                        MediaStore.Files.FileColumns.MEDIA_TYPE_VIDEO.toString(),
                        DURATION_MIN.toString(),
                        album.id
                    )
                }
            } else {
                if (album.isAll) {
                    selection = ("((" + MediaStore.Files.FileColumns.MEDIA_TYPE + "=? AND duration > ?)  OR "
                            + MediaStore.Files.FileColumns.MEDIA_TYPE + "=?) AND " + MediaStore.MediaColumns.SIZE + ">0")
                    selectionArgs = arrayOf(
                        MediaStore.Files.FileColumns.MEDIA_TYPE_VIDEO.toString(),
                        DURATION_MIN.toString(), MediaStore.Files.FileColumns.MEDIA_TYPE_IMAGE.toString()
                    )
                } else {
                    selection = ("((" + MediaStore.Files.FileColumns.MEDIA_TYPE + "=? AND duration > ?) OR "
                            + MediaStore.Files.FileColumns.MEDIA_TYPE + "=?) AND bucket_id=? AND " + MediaStore.MediaColumns.SIZE + ">0")
                    selectionArgs = arrayOf(
                        MediaStore.Files.FileColumns.MEDIA_TYPE_VIDEO.toString(),
                        DURATION_MIN.toString(), MediaStore.Files.FileColumns.MEDIA_TYPE_IMAGE.toString(),
                        album.id
                    )
                }
            }
            return AlbumMediaLoader(context, selection, selectionArgs)
        }
    }
}