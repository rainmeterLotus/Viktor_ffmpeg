package com.viktor.fg.loader

import android.os.Parcelable
import kotlinx.android.parcel.Parcelize

@Parcelize
data class SelectMedia(val path:String? = null,var durationMills:Long = 0): Parcelable {
    override fun equals(other: Any?): Boolean {
        val otherMedia = other as? SelectMedia
        return path == otherMedia?.path
    }
}