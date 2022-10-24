package com.viktor.fg.loader

import android.content.ContentUris
import android.content.Context
import android.database.Cursor
import android.database.MatrixCursor
import android.database.MergeCursor
import android.net.Uri
import android.os.Build
import android.os.Bundle
import android.provider.MediaStore
import androidx.fragment.app.FragmentActivity
import androidx.loader.app.LoaderManager
import androidx.loader.content.CursorLoader
import androidx.loader.content.Loader
import com.viktor.fg.ViktorApplication
import com.viktor.fg.util.ViktorUtil
import java.util.HashMap
import java.util.HashSet

/**
 * 加载图片文件夹
 */
class AlbumCategoryManager : LoaderManager.LoaderCallbacks<Cursor> {
    private var mContext : Context? = null
    private var loaderManager: LoaderManager? = null
    private var callbacks: AlbumCategoryCallbacks? = null
    var curPos = 0
        private set
    private var isLoadFinished = false

    companion object {
        private const val LOADER_ID = 99
    }

    override fun onCreateLoader(id: Int, args: Bundle?): Loader<Cursor> {
        val context = mContext ?: ViktorApplication.context
        isLoadFinished = false
        return AlbumLoader.newInstance(context)
    }

    override fun onLoadFinished(loader: Loader<Cursor>, data: Cursor) {
        if (!isLoadFinished) {
            isLoadFinished = true
            callbacks?.onAlbumCagetoryLoad(data)
        }
    }

    override fun onLoaderReset(loader: Loader<Cursor>) {
        callbacks?.onAlbumCategoryReset()
    }

    fun init(activity: FragmentActivity, callbacks: AlbumCategoryCallbacks?) {
        mContext = activity
        loaderManager = activity.supportLoaderManager
        this.callbacks = callbacks
    }


    fun release() {
        loaderManager?.destroyLoader(LOADER_ID)
        callbacks = null
    }

    fun loadAlbums() {
        loaderManager?.initLoader(LOADER_ID, null, this)
    }

    fun setStateCurrentSelection(currentSelection: Int) {
        this.curPos = currentSelection
    }

    interface AlbumCategoryCallbacks {
        fun onAlbumCagetoryLoad(cursor: Cursor?)
        fun onAlbumCategoryReset()
    }
}


class AlbumLoader private constructor(context: Context, selection: String, selectionArgs: Array<String>) : CursorLoader(
    context,
    QUERY_URI,
    if (isBefore10) PROJECTION else PROJECTION_29,
    selection,
    selectionArgs,
    ORDER_BY
) {
    override fun loadInBackground(): Cursor? {
        val albums = super.loadInBackground()
        val allAlbum = MatrixCursor(COLUMNS)
        var totalCount = 0
        var allAlbumCoverUri: Uri? = null
        return if (isBefore10) {
            val otherAlbums = MatrixCursor(COLUMNS)
            if (albums != null) {
                while (albums.moveToNext()) {
                    val fileId = albums.getLong(albums.getColumnIndex(MediaStore.Files.FileColumns._ID))
                    val bucketId = albums.getLong(albums.getColumnIndex(BUCKET_ID))
                    val bucketDisplayName = albums.getString(albums.getColumnIndex(BUCKET_DISPLAY_NAME))
                    val mimeType = albums.getString(albums.getColumnIndex(MediaStore.MediaColumns.MIME_TYPE))
                    val uri = getUri(albums)
                    val count = albums.getInt(albums.getColumnIndex(COUNT))
                    otherAlbums.addRow(arrayOf(fileId.toString(), bucketId.toString(), bucketDisplayName, mimeType, uri.toString(), count.toString()))
                    totalCount += count
                }
                if (albums.moveToFirst()) {
                    allAlbumCoverUri = getUri(albums)
                }
            }
            allAlbum.addRow(arrayOf("-1", "-1", "All", null,
                allAlbumCoverUri?.toString(), totalCount.toString()))
            MergeCursor(arrayOf<Cursor>(allAlbum, otherAlbums))
        } else {
            val countMap: MutableMap<Long, Long> = HashMap()
            if (albums != null) {
                while (albums.moveToNext()) {
                    val bucketId = albums.getLong(albums.getColumnIndex(BUCKET_ID))
                    var count = countMap[bucketId]
                    if (count == null) {
                        count = 1L
                    } else {
                        count++
                    }
                    countMap[bucketId] = count
                }
            }
            val otherAlbums = MatrixCursor(COLUMNS)
            if (albums != null) {
                if (albums.moveToFirst()) {
                    allAlbumCoverUri = getUri(albums)
                    val done: MutableSet<Long> = HashSet()
                    do {
                        val bucketId = albums.getLong(albums.getColumnIndex(BUCKET_ID))
                        if (done.contains(bucketId)) {
                            continue
                        }
                        val fileId = albums.getLong(albums.getColumnIndex(MediaStore.Files.FileColumns._ID))
                        val bucketDisplayName = albums.getString(albums.getColumnIndex(BUCKET_DISPLAY_NAME))
                        val mimeType = albums.getString(albums.getColumnIndex(MediaStore.MediaColumns.MIME_TYPE))
                        val uri = getUri(albums)
                        val count = countMap[bucketId] ?: 0
                        otherAlbums.addRow(arrayOf(fileId.toString(), bucketId.toString(), bucketDisplayName, mimeType, uri.toString(), count.toString()))
                        done.add(bucketId)
                        totalCount += count.toInt()
                    } while (albums.moveToNext())
                }
            }
            allAlbum.addRow(arrayOf("-1", "-1", "All", null, allAlbumCoverUri?.toString(), totalCount.toString()))
            MergeCursor(arrayOf<Cursor>(allAlbum, otherAlbums))
        }
    }

    companion object {
        private const val BUCKET_ID = "bucket_id"
        private const val BUCKET_DISPLAY_NAME = "bucket_display_name"
        const val URI = "uri"
        const val COUNT = "count"
        private val QUERY_URI = MediaStore.Files.getContentUri("external")
        private val COLUMNS = arrayOf(MediaStore.Files.FileColumns._ID, BUCKET_ID, BUCKET_DISPLAY_NAME, MediaStore.MediaColumns.MIME_TYPE, "uri", "count")
        private val PROJECTION = arrayOf(MediaStore.Files.FileColumns._ID, BUCKET_ID, BUCKET_DISPLAY_NAME, MediaStore.MediaColumns.MIME_TYPE, "COUNT(*) AS " + COUNT)
        private val PROJECTION_29 = arrayOf(MediaStore.Files.FileColumns._ID, BUCKET_ID, BUCKET_DISPLAY_NAME, MediaStore.MediaColumns.MIME_TYPE)
        private const val SELECTION_NORMAL = ("(" + MediaStore.Files.FileColumns.MEDIA_TYPE + "=? OR "
                + MediaStore.Files.FileColumns.MEDIA_TYPE + "=?) AND " + MediaStore.MediaColumns.SIZE + ">0"
                + ") GROUP BY (bucket_id")
        private const val SELECTION_29 = ("(" + MediaStore.Files.FileColumns.MEDIA_TYPE + "=? OR " + MediaStore.Files.FileColumns.MEDIA_TYPE + "=?)"
                + " AND " + MediaStore.MediaColumns.SIZE + ">0")
        private val SELECTION_ARGS = arrayOf(MediaStore.Files.FileColumns.MEDIA_TYPE_IMAGE.toString(), MediaStore.Files.FileColumns.MEDIA_TYPE_VIDEO.toString())

        private const val ORDER_BY = "datetaken DESC"


        fun newInstance(context: Context): CursorLoader {
            return AlbumLoader(context, if (isBefore10) SELECTION_NORMAL else SELECTION_29, SELECTION_ARGS)
        }

        private fun getUri(cursor: Cursor): Uri {
            val id = cursor.getLong(cursor.getColumnIndex(MediaStore.Files.FileColumns._ID))
            val mimeType = cursor.getString(
                cursor.getColumnIndex(MediaStore.MediaColumns.MIME_TYPE)
            )
            val contentUri: Uri = when {
                ViktorUtil.isImage(mimeType) -> {
                    MediaStore.Images.Media.EXTERNAL_CONTENT_URI
                }
                ViktorUtil.isVideo(mimeType) -> {
                    MediaStore.Video.Media.EXTERNAL_CONTENT_URI
                }
                else -> {
                    MediaStore.Files.getContentUri("external")
                }
            }
            return ContentUris.withAppendedId(contentUri, id)
        }

        private val isBefore10: Boolean = Build.VERSION.SDK_INT < Build.VERSION_CODES.Q
    }
}