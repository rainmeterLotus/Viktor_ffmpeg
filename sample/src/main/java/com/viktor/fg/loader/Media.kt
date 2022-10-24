package com.viktor.fg.loader

import com.viktor.fg.util.ViktorUtil.isImage
import com.viktor.fg.util.ViktorUtil.isGif
import com.viktor.fg.util.ViktorUtil.isVideo
import android.os.Parcelable
import android.provider.MediaStore
import android.content.ContentUris
import android.database.Cursor
import android.net.Uri
import android.os.Parcel

class Media : Parcelable {
    var id = 0L
    var mimeType: String? = null
    var contentUri: Uri? = null
    var size = 0L
    var duration = 0L
    var displayName: String? = null

    private constructor(id: Long, mimeType: String, size: Long, duration: Long, displayName: String) {
        this.id = id
        this.mimeType = mimeType
        val contentUri: Uri
        contentUri = when {
            isImage -> {
                MediaStore.Images.Media.EXTERNAL_CONTENT_URI
            }
            isVideo -> {
                MediaStore.Video.Media.EXTERNAL_CONTENT_URI
            }
            else -> {
                MediaStore.Files.getContentUri("external")
            }
        }
        this.contentUri = ContentUris.withAppendedId(contentUri, id)
        this.size = size
        this.duration = duration
        this.displayName = displayName
    }

    private constructor(source: Parcel) {
        id = source.readLong()
        mimeType = source.readString()
        contentUri = source.readParcelable(Uri::class.java.classLoader)
        size = source.readLong()
        duration = source.readLong()
        displayName = source.readString()
    }

    override fun describeContents(): Int {
        return 0
    }

    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.writeLong(id)
        dest.writeString(mimeType)
        dest.writeParcelable(contentUri, 0)
        dest.writeLong(size)
        dest.writeLong(duration)
        dest.writeString(displayName)
    }

    val isImage: Boolean = isImage(mimeType)
    val isGif: Boolean = isGif(mimeType)
    val isVideo: Boolean = isVideo(mimeType)

    companion object {
        @JvmField
        val CREATOR: Parcelable.Creator<Media> = object : Parcelable.Creator<Media> {
            override fun createFromParcel(source: Parcel): Media {
                return Media(source)
            }

            override fun newArray(size: Int): Array<Media?> {
                return arrayOfNulls(size)
            }
        }

        fun valueOf(cursor: Cursor): Media {
            return Media(
                cursor.getLong(cursor.getColumnIndex(MediaStore.Files.FileColumns._ID)),
                cursor.getString(cursor.getColumnIndex(MediaStore.MediaColumns.MIME_TYPE)),
                cursor.getLong(cursor.getColumnIndex(MediaStore.MediaColumns.SIZE)),
                cursor.getLong(cursor.getColumnIndex("duration")),
                cursor.getString(cursor.getColumnIndex(MediaStore.MediaColumns.DISPLAY_NAME))
            )
        }
    }
}