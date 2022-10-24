package com.viktor.fg.loader

import android.content.Context
import android.database.Cursor
import android.net.Uri
import android.os.Parcelable
import android.os.Parcel
import com.viktor.fg.R

class AlbumCategory : Parcelable {
    val id: String?
    val coverUri: Uri?
    private val displayName: String?
    var count: Long
        private set
    val mediaType: Int

    constructor(id: String?, coverUri: Uri?, albumName: String?, count: Long, mediaType: Int) {
        this.id = id
        this.coverUri = coverUri
        displayName = albumName
        this.count = count
        this.mediaType = mediaType
    }

    private constructor(source: Parcel) {
        id = source.readString()
        coverUri = source.readParcelable(Uri::class.java.classLoader)
        displayName = source.readString()
        count = source.readLong()
        mediaType = source.readInt()
    }

    override fun describeContents(): Int {
        return 0
    }

    override fun writeToParcel(dest: Parcel, flags: Int) {
        dest.writeString(id)
        dest.writeParcelable(coverUri, 0)
        dest.writeString(displayName)
        dest.writeLong(count)
    }

    fun getDisplayName(context: Context): String? {
        return if (isAll) {
            context.getString(R.string.str_all_medias)
        } else displayName
    }

    val isAll: Boolean
        get() = "-1" == id

    val isEmpty: Boolean
        get() = count == 0L

    companion object {
        @JvmField
        val CREATOR: Parcelable.Creator<AlbumCategory?> = object : Parcelable.Creator<AlbumCategory?> {
            override fun createFromParcel(source: Parcel): AlbumCategory? {
                return AlbumCategory(source)
            }

            override fun newArray(size: Int): Array<AlbumCategory?> {
                return arrayOfNulls(size)
            }
        }
        fun valueOf(cursor: Cursor?, type: Int): AlbumCategory? {
            cursor ?: return null
            val column = cursor.getString(cursor.getColumnIndex(AlbumLoader.URI))
            return AlbumCategory(
                cursor.getString(cursor.getColumnIndex("bucket_id")),
                Uri.parse(column ?: ""),
                cursor.getString(cursor.getColumnIndex("bucket_display_name")),
                cursor.getLong(cursor.getColumnIndex(AlbumLoader.COUNT)),
                type
            )
        }
    }
}